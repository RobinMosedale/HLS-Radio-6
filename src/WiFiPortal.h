#ifndef WIFI_PORTAL_H
#define WIFI_PORTAL_H

#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <TFT_eSPI.h>
#include "RadioConfig.h"

// DATA STRUCTURE: WiFiCred
// Purpose: Pairs a dynamic Arduino String SSID and password for one NVRAM slot.
//          Using String here accommodates variable-length WPA2 passphrases cleanly.
struct WiFiCred {
    String ssid; // Wi-Fi network name (empty "" if slot is unused)
    String pass; // Wi-Fi password (empty "" for open networks)
};

class WiFiPortal {
public:
    // Binds references (&) to global display and config instances
    WiFiPortal(TFT_eSPI& tftRef, RadioConfig& cfgRef);

    // Tries saved NVRAM networks; enters blocking open SoftAP beacon if none connect
    bool connectOrStartBeacon(const char* apName = "BBC-Radio-Setup");
    void startWebServer();
    void handleClient();
    void clearAllCredentials();

private:
    TFT_eSPI& _tft;        // Reference to global TFT display driver
    RadioConfig& _cfg;     // Reference to global NVRAM configuration manager
    WebServer _server;     // Embedded HTTP server instance on TCP port 80
    DNSServer _dnsServer;  // Wildcard DNS server used for captive-portal redirects

    static const int MAX_CREDS = 10; // Total NVRAM Wi-Fi slots (0..9)
    WiFiCred _creds[MAX_CREDS];      // RAM cache of all 10 WiFiCred structs
    String _scannedOptionsHtml;      // Cached HTML <option> tags from last Wi-Fi scan
    String _uploadBuffer;            // Accumulates incoming JSON chunks during /import

    // NVS & network helpers (WiFiPortal.cpp)
    void loadCredentials();
    void saveCredential(int slot, const String& ssid, const String& pass);
    void deleteCredential(int slot);
    bool hasAnyCredential() const;
    void scanNetworks();

    // UI & HTTP route handlers (WiFiRoutes.cpp)
    void displayBeaconScreen(const char* apName, const IPAddress& ip);
    void setupRoutes();
    String buildHtmlPage();

    // JSON Backup & Restore (WiFiBackup.cpp)
    String generateBackupJson();
    bool applyBackupJson(const String& jsonText);
};

#endif
