#include "RadioUI.h"
#include <WiFi.h>
#include <time.h>

// WHAT: Class constructor binding display and configuration references.
// WHEN: Runs once at global initialization before setup().
RadioUI::RadioUI(TFT_eSPI& tftRef, RadioConfig& cfgRef)
    : _tft(tftRef), _cfg(cfgRef) {}

// WHAT: Sets ILI9341 backlight PWM duty cycle (10..100%) on GPIO 10.
// WHEN: Called on boot in main.cpp and whenever the brightness slider moves.
void RadioUI::setBrightness(uint8_t pct) {
    _brightness = constrain(pct, BRT_MIN_PCT, BRT_MAX_PCT);
    uint8_t pwm = map(_brightness, 0, 100, 0, 255);
    analogWrite(TFT_BL, pwm);
}

// WHAT: Trims proportional smooth-font text to fit inside a target pixel width.
// WHEN: Called before drawing station titles, metadata, and preset labels.
String RadioUI::fitTextToWidth(const String& src, int maxPx) {
    if (_tft.textWidth(src) <= maxPx) return src;
    String trimmed = src;
    while (trimmed.length() > 1 && _tft.textWidth(trimmed + "...") > maxPx) {
        trimmed.remove(trimmed.length() - 1);
    }
    return trimmed + "...";
}

// WHAT: Updates the live stream metadata string on the station card.
// WHEN: Called by my_audio_info() in main.cpp when stream info arrives.
void RadioUI::setStreamMetadata(const char* meta) {
    _metaText = meta;
    if (_screen == SCREEN_MAIN) drawStationCard();
}

// WHAT: Routes rendering to whichever UI view (_screen) is active.
// WHEN: Called at end of setup() and on screen navigation.
void RadioUI::drawCurrentScreen() {
    if (_screen == SCREEN_MAIN) drawMainScreen();
    else if (_screen == SCREEN_SETTINGS) drawSettingsScreen();
    else drawFavPreviewScreen();
}

// WHAT: Refreshes prominent HH:MM clock on minute rollover and updates Wi-Fi bars.
// WHEN: Called continuously inside loop(), throttled to 1,000 ms intervals.
void RadioUI::updateClockAndSignal() {
    if (_screen != SCREEN_MAIN) return;
    if (millis() - _lastStatusMs < 1000) return;
    _lastStatusMs = millis();

    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5)) {
        if (timeinfo.tm_min != _lastMinute) {
            _lastMinute = timeinfo.tm_min;
            drawClockText(false);
        }
    }
    drawWifiBars(290, 10, WiFi.RSSI());
}

// WHAT: Updates and renders stereo L/R VU levels inside the station card.
// WHEN: Triggered by Audio::evt_vu callback (~50 ms cadence, range 0..255).
void RadioUI::updateVuMeter(uint8_t left, uint8_t right) {
    if (_screen != SCREEN_MAIN) return;
    if (left == _vuL && right == _vuR) return;
    _vuL = left;
    _vuR = right;
    drawVuBars();
}
