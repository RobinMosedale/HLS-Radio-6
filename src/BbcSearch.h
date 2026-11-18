#ifndef BBC_SEARCH_H
#define BBC_SEARCH_H

#include <Arduino.h>
#include <WebServer.h>
#include "RadioConfig.h"

#define BBC_MAX_RESULTS  64
#define BBC_LINE_BUF_LEN 320
#define BBC_GIST_HOST    "https://githubusercontent.com"
#define BBC_GIST_PATH    "/bpsib/67089b959e4fa898af69fea59ad74bc3/raw/BBC-Radio-HLS.m3u"
#define BBC_M3U_URL      BBC_GIST_HOST BBC_GIST_PATH

// DATA STRUCTURE: BbcStationItem
// Purpose: Holds a single station title and stream URL parsed from the live BBC .m3u Gist.
struct BbcStationItem {
    char name[FAV_NAME_LEN];
    char url[FAV_URL_LEN];
};

class BbcSearch {
public:
    /**
     * WHAT: Parses cached live BBC .m3u Gist and collects entries matching query.
     * WHEN: Called by /bbc_search web endpoint when populating or filtering the BBC dropdown.
     * HOW:  Reads cached PSRAM playlist line-by-line; empty query returns all stations.
     * WITH: query (filter text), outList (result array), maxItems (capacity).
     */
    static int fetchMatches(const char* query, BbcStationItem* outList, int maxItems);

    /**
     * WHAT: Registers /bbc, /bbc_search, /fav_save, and /fav_clear routes on WebServer.
     * WHEN: Called once inside WiFiPortal::setupRoutes().
     * WITH: server (WebServer reference), cfg (RadioConfig reference).
     */
    static void registerRoutes(WebServer& server, RadioConfig& cfg);
};

#endif
// END OF FILE
