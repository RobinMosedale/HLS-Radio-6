#ifndef RADIO_UI_H
#define RADIO_UI_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "RadioConfig.h"

#define COL_BG        0x0841
#define COL_CARD      0x10A2
#define COL_BORDER    0x31A6
#define COL_ACCENT    0x05DF
#define COL_BTN       0x2124
#define COL_AMBER     0xFD20

#define CARD_INNER_W  288
#define FAV_CELL_W    134
#define CLOCK_STR_LEN 6
#define GLYPH_BUF_LEN 2
#define BTN_COUNT     4
#define BRT_MIN_PCT   10
#define BRT_MAX_PCT   100

enum UiScreen {
    SCREEN_MAIN,
    SCREEN_SETTINGS,
    SCREEN_FAV_PREVIEW
};

enum UiAction {
    ACTION_NONE,
    ACTION_STATION_CHANGED,
    ACTION_VOLUME_CHANGED,
    ACTION_START_WIFI_BEACON
};

class RadioUI {
public:
    RadioUI(TFT_eSPI& tftRef, RadioConfig& cfgRef);

    void showBootScreen(const char* statusMsg);
    void drawCurrentScreen();
    void updateClockAndSignal();
    void updateVuMeter(uint8_t left, uint8_t right);
    void setStreamMetadata(const char* meta);
    void setBrightness(uint8_t pct);
    uint8_t getBrightness() const { return _brightness; }
    UiAction handleTouch();

private:
    TFT_eSPI& _tft;
    RadioConfig& _cfg;
    UiScreen _screen = SCREEN_MAIN;

    String _metaText = "Ready";
    uint8_t _brightness = 85;
    uint8_t _vuL = 0;
    uint8_t _vuR = 0;
    unsigned long _lastTouchMs = 0;
    unsigned long _lastStatusMs = 0;
    int _lastMinute = -1;
    char _prevTimeStr[CLOCK_STR_LEN] = "     ";

    String fitTextToWidth(const String& src, int maxPx);

    void drawMainScreen();
    void drawTopBar();
    void drawClockText(bool forceRedraw);
    void drawWifiBars(int x, int y, int rssi);
    void drawStationCard();
    void drawVuPanel();
    void drawVuBars();
    void drawSliders();
    void drawMainButtons();

    void drawSettingsScreen();
    void drawFavPreviewScreen();

    UiAction handleMainTouch(uint16_t x, uint16_t y);
    UiAction handleSettingsTouch(uint16_t x, uint16_t y);
    UiAction handleFavPreviewTouch(uint16_t x, uint16_t y);
};

#endif
