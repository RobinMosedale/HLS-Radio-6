#include "WiFiPortal.h"
#include <ArduinoJson.h> // bblanchon/ArduinoJson v7 serialization library

// WHAT: Serialises all user configuration data (volume, timezone, active preset,
//       10 Wi-Fi credentials, and 10 favourite stations) into a formatted JSON string.
// WHEN: Called when the user clicks "Download Full Backup (.json)" (/export).
// HOW:  Constructs an ArduinoJson v7 JsonDocument (allocated dynamically on heap/PSRAM),
//       populates nested JsonArray and JsonObject structures from _creds[] and _cfg,
//       and formats the result into a human-readable String.
String WiFiPortal::generateBackupJson() {
    JsonDocument doc;

    // Top-level scalar settings read from RadioConfig via reference _cfg
    doc["volume"] = _cfg.getVolume();
    doc["timezone"] = _cfg.getTimezoneIndex();
    doc["active_fav"] = _cfg.getActiveFavIndex();

    // Nested JSON array of 10 Wi-Fi credential objects: [{"ssid":"...","pass":"..."}, ...]
    JsonArray wifiArr = doc["wifi"].to<JsonArray>();
    for (int i = 0; i < MAX_CREDS; i++) {
        JsonObject obj = wifiArr.add<JsonObject>();
        obj["ssid"] = _creds[i].ssid;
        obj["pass"] = _creds[i].pass;
    }

    // Nested JSON array of 10 favourite station objects: [{"name":"...","url":"..."}, ...]
    JsonArray favArr = doc["favourites"].to<JsonArray>();
    for (int i = 0; i < MAX_FAVOURITES; i++) {
        // Read each FavStation struct via const reference (&) to avoid copying 288 bytes
        const FavStation& st = _cfg.getFavourite(i);
        JsonObject obj = favArr.add<JsonObject>();
        obj["name"] = st.name;
        obj["url"] = st.url;
    }

    String output;
    serializeJsonPretty(doc, output);
    return output;
}

// WHAT: Parses an uploaded JSON backup string and restores every setting to NVRAM.
// WHEN: Called by the HTTP POST "/import" handler after a .json file finishes uploading.
// WITH: jsonText - const String& reference containing the raw uploaded JSON text.
// RETURNS: true if JSON was valid and applied; false if deserialization failed.
bool WiFiPortal::applyBackupJson(const String& jsonText) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, jsonText);
    if (err) {
        Serial.printf("[BACKUP] JSON parse error: %s\n", err.c_str());
        return false;
    }

    // Restore volume and timezone if present in the JSON document
    if (doc["volume"].is<int>()) {
        _cfg.setVolume(doc["volume"].as<uint8_t>());
    }
    if (doc["timezone"].is<int>()) {
        _cfg.setTimezoneIndex(doc["timezone"].as<int>());
    }

    // Restore all 10 Wi-Fi credential slots to NVS ("wifi_cfg" namespace)
    if (doc["wifi"].is<JsonArray>()) {
        JsonArray wifiArr = doc["wifi"].as<JsonArray>();
        int slot = 0;
        for (JsonObject obj : wifiArr) {
            if (slot >= MAX_CREDS) break;
            String s = obj["ssid"] | ""; // Default to "" if key is missing/null
            String p = obj["pass"] | "";
            if (s.length() > 0) saveCredential(slot, s, p);
            else deleteCredential(slot);
            slot++;
        }
    }

    // Restore all 10 favourite stations to NVS ("radio_sys" namespace)
    if (doc["favourites"].is<JsonArray>()) {
        JsonArray favArr = doc["favourites"].as<JsonArray>();
        int idx = 0;
        for (JsonObject obj : favArr) {
            if (idx >= MAX_FAVOURITES) break;
            String n = obj["name"] | "";
            String u = obj["url"] | "";
            // Pass C-string pointers (.c_str()) into RadioConfig::setFavourite()
            _cfg.setFavourite(idx, n.c_str(), u.c_str());
            idx++;
        }
    }

    // Restore last active preset index so the radio boots straight into that station
    if (doc["active_fav"].is<int>()) {
        _cfg.setActiveFavIndex(doc["active_fav"].as<int>());
    }

    Serial.println("[BACKUP] Full configuration restored to NVRAM.");
    return true;
}