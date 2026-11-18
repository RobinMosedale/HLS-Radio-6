#include "RadioBrowser.h"

// WHAT: Registers /rb_meta and /rb_search HTTP endpoints on the portal WebServer.
// WHEN: Called once inside WiFiPortal::setupRoutes().
void RadioBrowser::registerRoutes(WebServer& server, RadioConfig& cfg) {
    (void)cfg;

    server.on("/rb_meta", HTTP_GET, [&server]() {
        String type = server.hasArg("type") ? server.arg("type") : "countries";
        server.send(200, "application/json", RadioBrowser::getMetaJson(type));
    });

    server.on("/rb_search", HTTP_GET, [&server]() {
        String country = server.hasArg("country") ? server.arg("country") : "";
        String tag     = server.hasArg("tag")     ? server.arg("tag")     : "";
        String lang    = server.hasArg("lang")    ? server.arg("lang")    : "";
        String q       = server.hasArg("q")       ? server.arg("q")       : "";
        server.send(200, "application/json", RadioBrowser::searchStationsJson(country, tag, lang, q));
    });
}
// END OF FILE
