#include "RadioBrowser.h"
#include "RadioAudition.h"

// WHAT: Registers /rb_preview, /rb_commit, /rb_stop, and /rb_status on the portal WebServer.
// WHEN: Called once inside WiFiPortal::setupRoutes().
// HOW:  Drives RadioAudition state machine while browser fetches radio-browser.info directly.
// WITH: server (WebServer reference), cfg (RadioConfig reference).
void RadioBrowser::registerRoutes(WebServer& server, RadioConfig& cfg) {
    server.on("/rb_preview", HTTP_POST, [&server, &cfg]() {
        if (!server.hasArg("url") || server.arg("url").length() == 0) {
            server.send(400, "text/plain", "Missing stream URL");
            return;
        }
        String name = server.hasArg("name") ? server.arg("name") : "Preview";
        String url  = server.arg("url");
        String uuid = server.hasArg("uuid") ? server.arg("uuid") : "";
        RadioAudition::startPreview(cfg, name.c_str(), url.c_str(), uuid.c_str());
        server.send(200, "text/plain", "OK");
    });

    server.on("/rb_commit", HTTP_POST, [&server, &cfg]() {
        if (!RadioAudition::isActive()) {
            server.send(400, "text/plain", "No active preview");
            return;
        }
        int savedSlot = RadioAudition::commitAndResume(cfg);
        if (savedSlot >= 0) {
            server.send(200, "text/plain", String(savedSlot + 1));
        } else {
            server.send(409, "text/plain", "Favourites full");
        }
    });

    server.on("/rb_stop", HTTP_POST, [&server, &cfg]() {
        RadioAudition::stopAndResume(cfg);
        server.send(200, "text/plain", "OK");
    });

    server.on("/rb_status", HTTP_GET, [&server]() {
        const AuditionState& aud = RadioAudition::getState();
        String json = String("{\"active\":") + (aud.active ? "true" : "false") +
                      ",\"name\":\"" + String(aud.name) + "\"}";
        server.send(200, "application/json", json);
    });
}
// END OF FILE
