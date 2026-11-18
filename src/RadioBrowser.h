#ifndef RADIO_BROWSER_H
#define RADIO_BROWSER_H

#include <Arduino.h>
#include <WebServer.h>
#include "RadioConfig.h"

#define RB_META_BUF_SIZE   24576
#define RB_SEARCH_BUF_SIZE 32768
#define RB_API_HOST        "de1.api.radio-browser.info"

// WHAT: Provides Community Radio Browser metadata and station search backed by PSRAM.
// WHEN: Registered once inside WiFiPortal::setupRoutes() alongside BbcSearch.
// HOW:  Lazily allocates PSRAM buffers on first query and caches dropdown lists.
class RadioBrowser {
public:
    static void registerRoutes(WebServer& server, RadioConfig& cfg);
    static const char* getMetaJson(const String& type);
    static const char* searchStationsJson(const String& country, const String& tag,
                                          const String& lang, const String& query);

private:
    static bool ensureBuffers();
    static bool fetchUrlToBuf(const String& url, char* dst, size_t maxLen);
    static String urlEncode(const String& s);
};

#endif
// END OF FILE
