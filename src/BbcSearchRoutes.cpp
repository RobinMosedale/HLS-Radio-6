#include "BbcSearch.h"

#define JSON_RESERVE_BYTES 8192

// WHAT: Escapes quotes and backslashes for safe JSON string emission.
static void appendJsonEscaped(String& dst, const char* src) {
    for (size_t i = 0; src[i] != '\0'; i++) {
        if (src[i] == '"' || src[i] == '\\') dst += '\\';
        dst += src[i];
    }
}

// WHAT: Registers /fav_list, /bbc_search, /fav_save, /fav_clear, /fav_move, and /bbc redirect.
void BbcSearch::registerRoutes(WebServer& server, RadioConfig& cfg) {
    server.on("/bbc", HTTP_GET, [&server]() {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

    server.on("/fav_list", HTTP_GET, [&server, &cfg]() {
        String out = "[";
        for (int i = 0; i < MAX_FAVOURITES; i++) {
            if (i > 0) out += ",";
            out += "{\"name\":\"";
            appendJsonEscaped(out, cfg.getFavourite(i).name);
            out += "\"}";
        }
        out += "]";
        server.send(200, "application/json", out);
    });

    server.on("/bbc_search", HTTP_GET, [&server]() {
        String q = server.hasArg("q") ? server.arg("q") : "";
        BbcStationItem* items = (BbcStationItem*)ps_malloc(sizeof(BbcStationItem) * BBC_MAX_RESULTS);
        if (!items) { server.send(500, "application/json", "[]"); return; }
        int found = fetchMatches(q.c_str(), items, BBC_MAX_RESULTS);
        String out;
        out.reserve(JSON_RESERVE_BYTES);
        out = "[";
        for (int i = 0; i < found; i++) {
            if (i > 0) out += ",";
            out += "{\"name\":\"";
            appendJsonEscaped(out, items[i].name);
            out += "\",\"url\":\"";
            appendJsonEscaped(out, items[i].url);
            out += "\"}";
        }
        out += "]";
        free(items);
        server.send(200, "application/json", out);
    });

    server.on("/fav_save", HTTP_POST, [&server, &cfg]() {
        int slot = server.arg("slot").toInt();
        if (slot >= 0 && slot < MAX_FAVOURITES && server.hasArg("name") && server.hasArg("url")) {
            cfg.setFavourite(slot, server.arg("name").c_str(), server.arg("url").c_str());
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "Bad Request");
        }
    });

    server.on("/fav_clear", HTTP_GET, [&server, &cfg]() {
        if (!server.hasArg("slot")) { server.send(400, "text/plain", "Missing slot"); return; }
        int slot = server.arg("slot").toInt();
        if (cfg.deleteFavourite(slot)) {
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "Invalid slot");
        }
    });

    server.on("/fav_move", HTTP_GET, [&server, &cfg]() {
        if (!server.hasArg("slot") || !server.hasArg("dir")) {
            server.send(400, "text/plain", "Missing params");
            return;
        }
        int slot = server.arg("slot").toInt();
        int dir  = server.arg("dir").toInt();
        if ((dir == -1 || dir == 1) && cfg.moveFavourite(slot, dir)) {
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "Cannot move");
        }
    });
}
// END OF FILE
