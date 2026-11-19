#ifndef RADIO_BROWSER_H
#define RADIO_BROWSER_H

#include <Arduino.h>
#include <WebServer.h>
#include "RadioConfig.h"

// WHAT: Registers lightweight audition routes (/rb_preview, /rb_commit, /rb_stop, /rb_status).
// WHEN: Called once inside WiFiPortal::setupRoutes() alongside BbcSearch.
// HOW:  Browser queries radio-browser.info directly via CORS; ESP32 only handles audio preview/commit.
class RadioBrowser {
public:
    static void registerRoutes(WebServer& server, RadioConfig& cfg);
};

#endif
// END OF FILE
