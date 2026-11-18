#include "WiFiPortal.h"
#include "PortalHtml.h"
#include <WiFi.h>
#include "BbcSearch.h"
#include "RadioBrowser.h"



// WHAT: Renders the SoftAP hotspot name and IP address on the ILI9341 during setup mode.
void WiFiPortal::displayBeaconScreen(const char* apName, const IPAddress& ip) {
    _tft.fillScreen(TFT_BLACK);
    _tft.fillRect(0, 0, 320, 32, TFT_MAROON);
    _tft.setTextColor(TFT_WHITE, TFT_MAROON);
    _tft.setTextSize(2);
    _tft.setCursor(10, 8);
    _tft.print("WIFI SETUP BEACON");

    _tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft.setCursor(10, 48);
    _tft.print("Connect phone/PC to:");

    _tft.setTextColor(TFT_CYAN, TFT_BLACK);
    _tft.setCursor(10, 76);
    _tft.printf("SSID: %s", apName);

    _tft.setTextColor(TFT_GREEN, TFT_BLACK);
    _tft.setCursor(10, 104);
    _tft.print("Pass: None (Open)");

    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.setCursor(10, 142);
    _tft.print("Then open in browser:");

    _tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    _tft.setCursor(10, 170);
    _tft.printf("http://%s", ip.toString().c_str());
}

// WHAT: Assembles the full responsive 3-tab HTML management dashboard.
String WiFiPortal::buildHtmlPage() {
    String html = HTML_HEAD;
    html += PORTAL_CSS;
    html += HTML_MID_1;
    int firstEmpty = 0;
    for (int i = 0; i < MAX_CREDS; i++) {
        if (_creds[i].ssid.length() == 0) { firstEmpty = i; break; }
    }
    for (int i = 0; i < MAX_CREDS; i++) {
        html += "<option value='" + String(i) + "'" + (i == firstEmpty ? " selected" : "") + ">Slot " + String(i + 1) + "</option>";
    }
    html += HTML_MID_2;
    html += _scannedOptionsHtml;
    html += HTML_MID_3;
    for (int i = 0; i < MAX_CREDS; i++) {
        html += "<tr><td><strong>#" + String(i + 1) + "</strong></td><td>";
        if (_creds[i].ssid.length() > 0) {
            html += _creds[i].ssid + "</td><td style='text-align:right;'><a class='del' href='/delete?slot=" + String(i) + "'>Delete</a>";
        } else {
            html += "<em style='color:#777;'>Empty</em></td><td>";
        }
        html += "</td></tr>";
    }
    html += HTML_FOOT;
    html += PORTAL_JS;
    html += "</body></html>";
    return html;
}


// WHAT: Binds all HTTP GET/POST endpoints and file-upload callbacks on _server.
void WiFiPortal::setupRoutes() {
    BbcSearch::registerRoutes(_server, _cfg);
    RadioBrowser::registerRoutes(_server, _cfg);
    _server.on("/", HTTP_GET, [this]() { _server.send(200, "text/html", buildHtmlPage()); });
    _server.on("/rescan", HTTP_GET, [this]() {
        scanNetworks();
        _server.sendHeader("Location", "/", true);
        _server.send(302, "text/plain", "");
    });
    _server.on("/delete", HTTP_GET, [this]() {
        if (_server.hasArg("slot")) deleteCredential(_server.arg("slot").toInt());
        _server.sendHeader("Location", "/", true);
        _server.send(302, "text/plain", "");
    });
    _server.on("/save", HTTP_POST, [this]() {
        String ssid = _server.arg("ssid");
        ssid.trim();
        if (ssid.length() > 0) {
            saveCredential(_server.arg("slot").toInt(), ssid, _server.arg("pass"));
            _server.sendHeader("Location", "/", true);
            _server.send(302, "text/plain", "");
        } else {
            _server.send(400, "text/plain", "SSID cannot be empty.");
        }
    });
    _server.on("/reboot", HTTP_POST, [this]() {
        _server.send(200, "text/html", "<html><body style='background:#121212;color:#fff;text-align:center;padding-top:40px;'><h2>Rebooting...</h2></body></html>");
        delay(1200);
        ESP.restart();
    });
    _server.on("/export", HTTP_GET, [this]() {
        String json = generateBackupJson();
        _server.sendHeader("Content-Disposition", "attachment; filename=\"bbc_radio_backup.json\"");
        _server.send(200, "application/json", json);
    });
    _server.on("/import", HTTP_POST,
        [this]() {
            if (applyBackupJson(_uploadBuffer)) {
                _server.send(200, "text/html", "<html><body style='background:#121212;color:#fff;text-align:center;padding-top:40px;'><h2>Restored!</h2></body></html>");
                delay(1500);
                ESP.restart();
            } else {
                _server.send(400, "text/plain", "Invalid JSON backup file.");
            }
            _uploadBuffer = "";
        },
        [this]() {
            HTTPUpload& upload = _server.upload();
            if (upload.status == UPLOAD_FILE_START) _uploadBuffer = "";
            else if (upload.status == UPLOAD_FILE_WRITE) {
                for (size_t i = 0; i < upload.currentSize; i++) _uploadBuffer += (char)upload.buf[i];
            }
        }
    );
    _server.onNotFound([this]() {
        _server.sendHeader("Location", "/", true);
        _server.send(302, "text/plain", "");
    });
}

// WHAT: Starts the HTTP portal on your local network once STA Wi-Fi connects.
void WiFiPortal::startWebServer() {
    scanNetworks();
    setupRoutes();
    _server.begin();
    Serial.printf("[PORTAL] Web UI active at http://%s\n", WiFi.localIP().toString().c_str());
}

// WHAT: Services incoming HTTP web requests inside loop().
void WiFiPortal::handleClient() {
    _server.handleClient();
}
