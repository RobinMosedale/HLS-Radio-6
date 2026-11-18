#include "WiFiPortal.h"
#include <WiFi.h>
#include <Preferences.h>

// WHAT: Constructor binding references (_tft, _cfg) to global instances and setting HTTP port 80.
WiFiPortal::WiFiPortal(TFT_eSPI& tftRef, RadioConfig& cfgRef)
    : _tft(tftRef), _cfg(cfgRef), _server(80) {}

// WHAT: Loads all 10 stored SSID/password pairs from NVS ("wifi_cfg") into _creds[].
void WiFiPortal::loadCredentials() {
    Preferences prefs;
    prefs.begin("wifi_cfg", true);
    for (int i = 0; i < MAX_CREDS; i++) {
        _creds[i].ssid = prefs.getString(("s_" + String(i)).c_str(), "");
        _creds[i].pass = prefs.getString(("p_" + String(i)).c_str(), "");
    }
    prefs.end();
}

// WHAT: Writes one SSID/password pair to NVS and mirrors it into _creds[slot].
void WiFiPortal::saveCredential(int slot, const String& ssid, const String& pass) {
    if (slot < 0 || slot >= MAX_CREDS) return;
    Preferences prefs;
    prefs.begin("wifi_cfg", false);
    prefs.putString(("s_" + String(slot)).c_str(), ssid);
    prefs.putString(("p_" + String(slot)).c_str(), pass);
    prefs.end();
    _creds[slot].ssid = ssid;
    _creds[slot].pass = pass;
}

// WHAT: Deletes one Wi-Fi slot from NVS and clears its RAM struct entry.
void WiFiPortal::deleteCredential(int slot) {
    if (slot < 0 || slot >= MAX_CREDS) return;
    Preferences prefs;
    prefs.begin("wifi_cfg", false);
    prefs.remove(("s_" + String(slot)).c_str());
    prefs.remove(("p_" + String(slot)).c_str());
    prefs.end();
    _creds[slot].ssid = "";
    _creds[slot].pass = "";
}

// WHAT: Wipes all 10 Wi-Fi credentials from NVS and RAM.
void WiFiPortal::clearAllCredentials() {
    Preferences prefs;
    prefs.begin("wifi_cfg", false);
    prefs.clear();
    prefs.end();
    for (int i = 0; i < MAX_CREDS; i++) {
        _creds[i].ssid = "";
        _creds[i].pass = "";
    }
}

// WHAT: Returns true if at least one slot in _creds[] contains an SSID.
bool WiFiPortal::hasAnyCredential() const {
    for (int i = 0; i < MAX_CREDS; i++) {
        if (_creds[i].ssid.length() > 0) return true;
    }
    return false;
}

// WHAT: Scans nearby 2.4 GHz networks and builds HTML <option> tags in _scannedOptionsHtml.
void WiFiPortal::scanNetworks() {
    int n = WiFi.scanNetworks();
    _scannedOptionsHtml = (n <= 0) ? "<option value=''>No networks found</option>" : "";
    for (int i = 0; i < n; i++) {
        String net = WiFi.SSID(i);
        if (net.length() > 0) {
            _scannedOptionsHtml += "<option value='" + net + "'>" + net + " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
        }
    }
    WiFi.scanDelete();
}

// WHAT: Tries saved NVRAM networks on boot; launches open SoftAP captive portal if none connect.
bool WiFiPortal::connectOrStartBeacon(const char* apName) {
    loadCredentials();
    if (hasAnyCredential()) {
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        for (int i = 0; i < MAX_CREDS; i++) {
            if (_creds[i].ssid.length() == 0) continue;
            Serial.printf("[WIFI] Trying Slot %d: %s\n", i + 1, _creds[i].ssid.c_str());
            WiFi.begin(_creds[i].ssid.c_str(), _creds[i].pass.c_str());
            WiFi.setTxPower(WIFI_POWER_15dBm);
            unsigned long start = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) {
                delay(500);
                Serial.print(".");
            }
            if (WiFi.status() == WL_CONNECTED) {
                Serial.printf("\n[WIFI] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
                startWebServer();
                return true;
            }
            WiFi.disconnect(true);
            delay(200);
        }
    }

    WiFi.mode(WIFI_AP_STA);
    scanNetworks();
    WiFi.softAP(apName, NULL);
    delay(200);
    IPAddress apIP = WiFi.softAPIP();
    displayBeaconScreen(apName, apIP);

    _dnsServer.start(53, "*", apIP);
    setupRoutes();
    _server.begin();

    while (true) {
        _dnsServer.processNextRequest();
        _server.handleClient();
        delay(5);
    }
    return false;
}
