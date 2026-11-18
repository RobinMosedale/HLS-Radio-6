#include "BbcSearch.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ctype.h>

#define BBC_CACHE_MAX_BYTES 32768
#define GIST_SUBDOMAIN      "gist."
#define GIST_DOMAIN         "githubusercontent.com"
#define LIVE_GIST_PATH      "/bpsib/67089b959e4fa898af69fea59ad74bc3/raw/BBC-Radio-HLS.m3u"

static char* s_cachedM3u = nullptr;

// WHAT: Case-insensitive substring check without heap allocations.
// WHEN: Filtering #EXTINF station names against the user's search query.
// HOW: Compares lowercased characters across title and query pointers.
// WITH: title (station name) and query (filter text).
static bool matchesFilter(const char* title, const char* query) {
    if (!query || strlen(query) == 0) return true;
    for (size_t i = 0; title[i] != '\0'; i++) {
        size_t j = 0;
        while (query[j] != '\0' && title[i + j] != '\0' &&
               tolower((unsigned char)title[i + j]) == tolower((unsigned char)query[j])) {
            j++;
        }
        if (query[j] == '\0') return true;
    }
    return false;
}

// WHAT: Downloads the live BBC .m3u Gist once into a 32KB PSRAM buffer.
// WHEN: Called lazily on the first /bbc_search request so subsequent queries run in 0ms.
// HOW: Connects over TLS to gist.githubusercontent.com and streams bytes into ps_malloc().
// WITH: WiFiClientSecure, HTTPClient, and s_cachedM3u PSRAM buffer.
static bool ensurePlaylistCached() {
    if (s_cachedM3u != nullptr) return true;

    const char* gistUrl = "https://" GIST_SUBDOMAIN GIST_DOMAIN LIVE_GIST_PATH;
    Serial.printf("[BBC] Fetching live Gist: %s\n", gistUrl);

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(12000);

    if (!http.begin(client, gistUrl)) return false;
    int code = http.GET();
    Serial.printf("[BBC] HTTP status: %d\n", code);
    if (code != HTTP_CODE_OK) {
        http.end();
        return false;
    }

    char* buf = (char*)ps_malloc(BBC_CACHE_MAX_BYTES);
    if (!buf) {
        http.end();
        return false;
    }

    Stream* stream = http.getStreamPtr();
    size_t total = 0;
    unsigned long lastRead = millis();

    while ((http.connected() || stream->available()) && total + 1 < BBC_CACHE_MAX_BYTES) {
        if (stream->available()) {
            size_t n = stream->readBytes(buf + total, BBC_CACHE_MAX_BYTES - 1 - total);
            total += n;
            lastRead = millis();
        } else if (millis() - lastRead > 2500) {
            break;
        } else {
            delay(5);
        }
    }
    buf[total] = '\0';
    http.end();

    if (total == 0) {
        free(buf);
        return false;
    }
    Serial.printf("[BBC] Cached %u bytes in PSRAM\n", (unsigned)total);
    s_cachedM3u = buf;
    return true;
}

// WHAT: Parses cached live BBC .m3u Gist and collects entries matching query.
// WHEN: Called by /bbc_search web endpoint when populating or filtering the BBC dropdown.
// HOW: Scans #EXTINF titles and pairs each with its following http stream URL.
// WITH: query (filter text), outList (PSRAM result array), maxItems (capacity).
int BbcSearch::fetchMatches(const char* query, BbcStationItem* outList, int maxItems) {
    if (!outList || maxItems <= 0) return 0;
    if (!ensurePlaylistCached()) return 0;

    char pendingName[FAV_NAME_LEN] = {0};
    int count = 0;
    const char* cursor = s_cachedM3u;

    while (*cursor != '\0' && count < maxItems) {
        const char* eol = strchr(cursor, '\n');
        size_t len = eol ? (size_t)(eol - cursor) : strlen(cursor);
        if (len > 0 && cursor[len - 1] == '\r') len--;

        char line[BBC_LINE_BUF_LEN];
        size_t copyLen = (len < sizeof(line) - 1) ? len : (sizeof(line) - 1);
        memcpy(line, cursor, copyLen);
        line[copyLen] = '\0';

        if (strncmp(line, "#EXTINF:", 8) == 0) {
            const char* comma = strchr(line, ',');
            const char* rawTitle = comma ? (comma + 1) : "BBC Stream";
            while (*rawTitle == ' ') rawTitle++;
            if (strncmp(rawTitle, "BBC - ", 6) == 0) rawTitle += 6;
            strlcpy(pendingName, rawTitle, FAV_NAME_LEN);
        } else if (strncmp(line, "http", 4) == 0 && strlen(pendingName) > 0) {
            if (matchesFilter(pendingName, query)) {
                strlcpy(outList[count].name, pendingName, FAV_NAME_LEN);
                strlcpy(outList[count].url, line, FAV_URL_LEN);
                count++;
            }
            *pendingName = '\0';
        }
        cursor = eol ? (eol + 1) : (cursor + len);
    }
    return count;
}
// END OF FILE
