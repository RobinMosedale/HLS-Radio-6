#include "RadioBrowser.h"
#include <HTTPClient.h>
#include <WiFiClient.h>

#define ZERO_IDX 0

static char* s_countriesBuf = nullptr;
static char* s_tagsBuf      = nullptr;
static char* s_langsBuf     = nullptr;
static char* s_searchBuf    = nullptr;

// WHAT: Lazily allocates PSRAM buffers for cached dropdown lists and search results.
// WHEN: Called automatically before any metadata or station search lookup.
bool RadioBrowser::ensureBuffers() {
    if (!s_countriesBuf) {
        s_countriesBuf = (char*)ps_malloc(RB_META_BUF_SIZE);
        if (s_countriesBuf) s_countriesBuf[ZERO_IDX] = '\0';
    }
    if (!s_tagsBuf) {
        s_tagsBuf = (char*)ps_malloc(RB_META_BUF_SIZE);
        if (s_tagsBuf) s_tagsBuf[ZERO_IDX] = '\0';
    }
    if (!s_langsBuf) {
        s_langsBuf = (char*)ps_malloc(RB_META_BUF_SIZE);
        if (s_langsBuf) s_langsBuf[ZERO_IDX] = '\0';
    }
    if (!s_searchBuf) {
        s_searchBuf = (char*)ps_malloc(RB_SEARCH_BUF_SIZE);
        if (s_searchBuf) s_searchBuf[ZERO_IDX] = '\0';
    }
    return (s_countriesBuf && s_tagsBuf && s_langsBuf && s_searchBuf);
}

// WHAT: Returns cached JSON array for countries, tags, or languages (fetches once on demand).
// WHEN: Called by GET /rb_meta?type=countries|tags|langs.
const char* RadioBrowser::getMetaJson(const String& type) {
    if (!ensureBuffers()) return "[]";
    char* target = nullptr;
    String endpoint;
    if (type == "countries") { target = s_countriesBuf; endpoint = "/json/countries"; }
    else if (type == "tags") { target = s_tagsBuf;      endpoint = "/json/tags"; }
    else if (type == "langs"){ target = s_langsBuf;     endpoint = "/json/languages"; }
    else return "[]";

    if (target[ZERO_IDX] != '\0') return target;
    String url = String("http://") + RB_API_HOST + endpoint +
                 "?order=stationcount&reverse=true&hidebroken=true&limit=80";
    if (!fetchUrlToBuf(url, target, RB_META_BUF_SIZE)) return "[]";
    return target;
}

// WHAT: Queries Community Radio Browser stations matching Country, Tag, Language, and Name.
// WHEN: Called by GET /rb_search?country=...&tag=...&lang=...&q=...
const char* RadioBrowser::searchStationsJson(const String& country, const String& tag,
                                             const String& lang, const String& query) {
    if (!ensureBuffers()) return "[]";
    String url = String("http://") + RB_API_HOST +
                 "/json/stations/search?hidebroken=true&order=votes&reverse=true&limit=40";
    if (country.length() > 0) url += "&countrycode=" + urlEncode(country);
    if (tag.length() > 0)     url += "&tag="         + urlEncode(tag);
    if (lang.length() > 0)    url += "&language="    + urlEncode(lang);
    if (query.length() > 0)   url += "&name="        + urlEncode(query);
    if (!fetchUrlToBuf(url, s_searchBuf, RB_SEARCH_BUF_SIZE)) return "[]";
    return s_searchBuf;
}

// WHAT: Performs HTTP GET with required User-Agent header and streams body into PSRAM buffer.
bool RadioBrowser::fetchUrlToBuf(const String& url, char* dst, size_t maxLen) {
    WiFiClient client;
    HTTPClient http;
    http.setUserAgent("ESP32-S3-Radio/1.0");
    http.setTimeout(8000);
    if (!http.begin(client, url)) return false;
    int code = http.GET();
    if (code != 200) { http.end(); return false; }

    WiFiClient* stream = http.getStreamPtr();
    size_t pos = 0;
    while (http.connected() && pos < maxLen - 1) {
        size_t avail = stream->available();
        if (avail) {
            size_t toRead = (avail > (maxLen - 1 - pos)) ? (maxLen - 1 - pos) : avail;
            pos += stream->readBytes(dst + pos, toRead);
        } else {
            delay(5);
            if (!stream->available()) break;
        }
    }
    dst[pos] = '\0';
    http.end();
    return pos > 2;
}

// WHAT: Percent-encodes spaces and special characters for URL query parameters.
String RadioBrowser::urlEncode(const String& s) {
    String out;
    const char* hex = "0123456789ABCDEF";
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += c;
        else if (c == ' ') out += "%20";
        else { out += '%'; out += hex[(c >> 4) & 0xF]; out += hex[c & 0xF]; }
    }
    return out;
}
// END OF FILE
