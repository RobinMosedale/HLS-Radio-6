#include "RadioUI.h"
#include <examples/Smooth Fonts/FLASH_Array/Font_Demo_1_Array/NotoSansBold15.h>
#include <examples/Smooth Fonts/FLASH_Array/Font_Demo_1_Array/NotoSansBold36.h>
#include <WiFi.h>
#include <time.h>

#define BADGE_BUF_LEN 24
#define VAL_BUF_LEN   8
#define CLOCK_SLOTS   5
#define CLOCK_START_Y 1
#define VU_COL_W      8
#define VU_COL_H      88
#define VU_BASE_Y     212

// WHAT: Paints the complete Main Screen layout from scratch.
// WHEN: Called on boot and whenever returning from Settings or Favourites submenus.
void RadioUI::drawMainScreen() {
    _tft.fillScreen(COL_BG);
    _lastMinute = -1;
    strcpy(_prevTimeStr, "     ");
    drawTopBar();
    drawStationCard();
    drawSliders();
    drawMainButtons();
    drawVuPanel();
}

// WHAT: Draws a 4-bar mobile-style Wi-Fi signal strength indicator.
// WHEN: Called by drawTopBar() and updateClockAndSignal().
void RadioUI::drawWifiBars(int x, int y, int rssi) {
    int bars = 0;
    if (WiFi.status() == WL_CONNECTED) {
        if (rssi > -55) bars = 4;
        else if (rssi > -67) bars = 3;
        else if (rssi > -78) bars = 2;
        else bars = 1;
    }
    for (int i = 0; i < 4; i++) {
        int barH = 4 + (i * 4);
        int barY = y + (16 - barH);
        _tft.fillRect(x + (i * 6), barY, 4, barH, (i < bars) ? TFT_GREEN : COL_BORDER);
    }
}

// WHAT: Draws HH:MM clock digits with exact per-glyph spacing so digits never clip.
// WHEN: Called on full screen paint and incremental minute ticks.
void RadioUI::drawClockText(bool forceRedraw) {
    const int slotX[CLOCK_SLOTS] = { 8, 29, 50, 60, 81 };
    const int slotW[CLOCK_SLOTS] = { 21, 21, 10, 21, 21 };
    struct tm timeinfo;
    char timeBuf[CLOCK_STR_LEN] = "--:--";
    if (getLocalTime(&timeinfo, 10)) strftime(timeBuf, sizeof(timeBuf), "%H:%M", &timeinfo);

    _tft.loadFont(NotoSansBold36);
    _tft.setTextColor(TFT_CYAN, TFT_BLACK, true);
    for (int i = 0; i < CLOCK_SLOTS; i++) {
        if (forceRedraw || timeBuf[i] != _prevTimeStr[i]) {
            _tft.fillRect(slotX[i], 0, slotW[i], 35, TFT_BLACK);
            char glyph[GLYPH_BUF_LEN] = { timeBuf[i], '\0' };
            _tft.drawString(glyph, slotX[i], CLOCK_START_Y);
            _prevTimeStr[i] = timeBuf[i];
        }
    }
    _tft.unloadFont();
}

// WHAT: Renders the 36px top status bar with HH:MM clock, IP, and Wi-Fi bars.
// WHEN: Called once each time drawMainScreen() executes.
void RadioUI::drawTopBar() {
    _tft.fillRect(0, 0, 320, 36, TFT_BLACK);
    _tft.drawFastHLine(0, 36, 320, COL_BORDER);
    drawClockText(true);

    _tft.loadFont(NotoSansBold15);
    _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK, true);
    String ipStr = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "Offline";
    _tft.drawRightString(ipStr, 282, 12, 1);
    _tft.unloadFont();
    drawWifiBars(290, 10, WiFi.RSSI());
}

// WHAT: Draws the bottom-RHS stereo VU meter frame and L/R channel labels.
// WHEN: Called once each time drawMainScreen() executes.
void RadioUI::drawVuPanel() {
    _tft.fillRoundRect(278, 118, 34, 116, 6, COL_CARD);
    _tft.drawRoundRect(278, 118, 34, 116, 6, COL_BORDER);
    _tft.loadFont(NotoSansBold15);
    _tft.setTextColor(TFT_LIGHTGREY, COL_CARD, true);
    _tft.drawCentreString("L", 288, 215, 1);
    _tft.drawCentreString("R", 302, 215, 1);
    _tft.unloadFont();
    drawVuBars();
}

// WHAT: Renders vertical L/R stereo VU bars rising from bottom inside RHS panel.
// WHEN: Called by drawVuPanel() and updateVuMeter() on new audio VU frames.
void RadioUI::drawVuBars() {
    for (int ch = 0; ch < 2; ch++) {
        uint8_t lvl = (ch == 0) ? _vuL : _vuR;
        int bx = (ch == 0) ? 284 : 298;
        int fillH = map(lvl, 0, 255, 0, VU_COL_H);
        int gH = min(fillH, 60);
        int yH = (fillH > 60) ? min(fillH - 60, 18) : 0;
        int rH = (fillH > 78) ? (fillH - 78) : 0;

        if (fillH < VU_COL_H) {
            _tft.fillRect(bx, VU_BASE_Y - VU_COL_H, VU_COL_W, VU_COL_H - fillH, TFT_BLACK);
        }
        if (gH > 0) _tft.fillRect(bx, VU_BASE_Y - gH, VU_COL_W, gH, TFT_GREEN);
        if (yH > 0) _tft.fillRect(bx, VU_BASE_Y - 60 - yH, VU_COL_W, yH, TFT_YELLOW);
        if (rH > 0) _tft.fillRect(bx, VU_BASE_Y - 78 - rH, VU_COL_W, rH, TFT_RED);
    }
}

// WHAT: Draws central station card with 36px/26px auto-sized title and metadata.
// WHEN: Called on screen load, station change, or new stream metadata arrival.
void RadioUI::drawStationCard() {
    int favIdx = _cfg.getActiveFavIndex();
    const FavStation& st = _cfg.getFavourite(favIdx);

    _tft.fillRoundRect(8, 40, 304, 74, 8, COL_CARD);
    _tft.drawRoundRect(8, 40, 304, 74, 8, COL_ACCENT);

    String title = (strlen(st.name) > 0) ? String(st.name) : "Empty Preset";
    _tft.loadFont(NotoSansBold36);
    _tft.setTextColor(TFT_WHITE, COL_CARD, true);
    if (_tft.textWidth(title) <= CARD_INNER_W) {
        _tft.drawString(title, 16, 58);
        _tft.unloadFont();
    } else {
        _tft.unloadFont();
        _tft.setTextFont(4);
        _tft.setTextSize(1);
        _tft.setTextColor(TFT_WHITE, COL_CARD, true);
        _tft.drawString(fitTextToWidth(title, CARD_INNER_W), 16, 65);
    }

    _tft.loadFont(NotoSansBold15);
    _tft.setTextColor(TFT_YELLOW, COL_CARD, true);
    char badge[BADGE_BUF_LEN];
    snprintf(badge, sizeof(badge), "FAV %d / %d", favIdx + 1, MAX_FAVOURITES);
    _tft.drawString(badge, 16, 45);

    _tft.setTextColor(TFT_LIGHTGREY, COL_CARD, true);
    _tft.drawRightString(_cfg.getTimezoneLabel(), 302, 45, 1);

    _tft.setTextColor(COL_ACCENT, COL_CARD, true);
    _tft.drawString(fitTextToWidth(_metaText, CARD_INNER_W), 16, 95);
    _tft.unloadFont();
}

// WHAT: Draws both horizontal touch sliders (x=8..272) beside the vertical VU panel.
// WHEN: Called on Main Screen load and whenever either slider is touched.
void RadioUI::drawSliders() {
    _tft.loadFont(NotoSansBold15);
    uint8_t vol = _cfg.getVolume();
    _tft.fillRoundRect(8, 118, 264, 30, 6, COL_CARD);
    _tft.drawRoundRect(8, 118, 264, 30, 6, COL_BORDER);
    _tft.setTextColor(TFT_WHITE, COL_CARD, true);
    _tft.drawString("VOL", 16, 126);

    int volW = map(vol, 0, 21, 0, 168);
    _tft.fillRoundRect(54, 128, 168, 10, 4, TFT_BLACK);
    if (volW > 0) _tft.fillRoundRect(54, 128, volW, 10, 4, TFT_GREEN);
    _tft.fillCircle(54 + volW, 133, 7, TFT_WHITE);

    _tft.setTextColor(TFT_YELLOW, COL_CARD, true);
    char valBuf[VAL_BUF_LEN];
    snprintf(valBuf, sizeof(valBuf), "%2d", vol);
    _tft.drawRightString(valBuf, 264, 126, 1);

    _tft.fillRoundRect(8, 152, 264, 30, 6, COL_CARD);
    _tft.drawRoundRect(8, 152, 264, 30, 6, COL_BORDER);
    _tft.setTextColor(TFT_WHITE, COL_CARD, true);
    _tft.drawString("BRT", 16, 160);

    int brtW = map(_brightness, BRT_MIN_PCT, BRT_MAX_PCT, 0, 168);
    _tft.fillRoundRect(54, 162, 168, 10, 4, TFT_BLACK);
    if (brtW > 0) _tft.fillRoundRect(54, 162, brtW, 10, 4, COL_AMBER);
    _tft.fillCircle(54 + brtW, 167, 7, TFT_WHITE);

    _tft.setTextColor(COL_AMBER, COL_CARD, true);
    snprintf(valBuf, sizeof(valBuf), "%d%%", _brightness);
    _tft.drawRightString(valBuf, 264, 160, 1);
    _tft.unloadFont();
}

// WHAT: Renders the 4 bottom navigation buttons (x=8..272) beside the VU panel.
// WHEN: Called once each time drawMainScreen() executes.
void RadioUI::drawMainButtons() {
    const char* labels[BTN_COUNT] = {"PREV", "NEXT", "FAVS", "SETUP"};
    uint16_t borders[BTN_COUNT] = {COL_ACCENT, COL_ACCENT, TFT_ORANGE, TFT_GREEN};

    _tft.loadFont(NotoSansBold15);
    for (int i = 0; i < BTN_COUNT; i++) {
        int bx = 8 + (i * 67);
        _tft.fillRoundRect(bx, 188, 63, 46, 6, COL_BTN);
        _tft.drawRoundRect(bx, 188, 63, 46, 6, borders[i]);
        _tft.setTextColor(TFT_WHITE, COL_BTN, true);
        _tft.drawCentreString(labels[i], bx + 31, 204, 1);
    }
    _tft.unloadFont();
}
// END OF FILE
