#include "RadioUI.h"
#include <examples/Smooth Fonts/FLASH_Array/Font_Demo_1_Array/NotoSansBold15.h>
#include <examples/Smooth Fonts/FLASH_Array/Font_Demo_1_Array/NotoSansBold36.h>

#define FAV_LABEL_BUF_LEN 40

// WHAT: Renders the 3-line boot splash card and updates the live progress pill.
// WHEN: Called in main.cpp setup() during storage init, Wi-Fi connect, and NTP sync.
// HOW:  Draws the frame and centred title stack once, then overwrites only the status pill.
// WITH: statusMsg - null-terminated C-string describing the active boot step.
void RadioUI::showBootScreen(const char* statusMsg) {
    static bool splashDrawn = false;
    if (_screen != SCREEN_MAIN) splashDrawn = false;

    if (!splashDrawn) {
        _tft.fillScreen(COL_BG);
        _tft.fillRoundRect(16, 14, 288, 212, 10, COL_CARD);
        _tft.drawRoundRect(16, 14, 288, 212, 10, COL_ACCENT);

        _tft.loadFont(NotoSansBold36);
        _tft.setTextColor(TFT_WHITE, COL_CARD, true);
        _tft.drawCentreString("Streaming", 160, 26, 1);
        _tft.drawCentreString("Radio", 160, 66, 1);
        _tft.unloadFont();

        _tft.loadFont(NotoSansBold15);
        _tft.setTextColor(TFT_YELLOW, COL_CARD, true);
        _tft.drawCentreString("HLS Enabled", 160, 110, 1);
        _tft.unloadFont();

        _tft.drawFastHLine(40, 138, 240, COL_BORDER);
        splashDrawn = true;
    }

    _tft.fillRoundRect(28, 152, 264, 34, 6, COL_BTN);
    _tft.loadFont(NotoSansBold15);
    _tft.setTextColor(COL_ACCENT, COL_BTN, true);
    _tft.drawCentreString(fitTextToWidth(String(statusMsg), 248), 160, 161, 1);
    _tft.unloadFont();
}

// WHAT: Draws the System Settings submenu (timezone selector & Wi-Fi portal reset).
// WHEN: Called when the user taps SETUP on the Main Screen or cycles the timezone.
void RadioUI::drawSettingsScreen() {
    _tft.fillScreen(COL_BG);
    _tft.fillRect(0, 0, 320, 28, TFT_NAVY);

    _tft.loadFont(NotoSansBold15);
    _tft.setTextColor(TFT_WHITE, TFT_NAVY, true);
    _tft.drawString("SYSTEM SETTINGS", 10, 7);

    _tft.fillRoundRect(10, 38, 300, 56, 6, COL_CARD);
    _tft.drawRoundRect(10, 38, 300, 56, 6, COL_ACCENT);
    _tft.setTextColor(TFT_LIGHTGREY, COL_CARD, true);
    _tft.drawString("TIMEZONE (TAP TO CYCLE):", 20, 46);
    _tft.setTextColor(TFT_YELLOW, COL_CARD, true);
    _tft.drawString(_cfg.getTimezoneLabel(), 20, 68);

    _tft.fillRoundRect(10, 104, 300, 56, 6, COL_CARD);
    _tft.drawRoundRect(10, 104, 300, 56, 6, TFT_ORANGE);
    _tft.setTextColor(TFT_ORANGE, COL_CARD, true);
    _tft.drawCentreString("LAUNCH WIFI PORTAL", 160, 125, 1);

    _tft.fillRoundRect(10, 174, 300, 54, 6, TFT_DARKGREEN);
    _tft.setTextColor(TFT_WHITE, TFT_DARKGREEN, true);
    _tft.drawCentreString("< BACK", 160, 194, 1);
    _tft.unloadFont();
}

// WHAT: Draws a 2-column grid of all 10 saved favourite presets for direct tuning.
// WHEN: Called when the user taps FAVS on the Main Screen.
void RadioUI::drawFavPreviewScreen() {
    _tft.fillScreen(COL_BG);
    _tft.fillRect(0, 0, 320, 28, TFT_DARKGREEN);

    _tft.loadFont(NotoSansBold15);
    _tft.setTextColor(TFT_WHITE, TFT_DARKGREEN, true);
    _tft.drawString("FAVOURITE PRESETS", 10, 7);

    for (int i = 0; i < MAX_FAVOURITES; i++) {
        int x = 10 + ((i / 5) * 152);
        int y = 36 + ((i % 5) * 26);
        bool active = (i == _cfg.getActiveFavIndex());
        uint16_t bg = active ? COL_ACCENT : COL_CARD;
        uint16_t fg = active ? TFT_BLACK  : TFT_WHITE;

        _tft.fillRoundRect(x, y, 146, 22, 4, bg);
        _tft.setTextColor(fg, bg, true);

        const FavStation& st = _cfg.getFavourite(i);
        char label[FAV_LABEL_BUF_LEN];
        snprintf(label, sizeof(label), "%d. %s", i + 1, (strlen(st.name) > 0) ? st.name : "---");
        _tft.drawString(fitTextToWidth(String(label), FAV_CELL_W), x + 6, y + 4);
    }

    _tft.fillRoundRect(10, 174, 300, 54, 6, COL_BTN);
    _tft.drawRoundRect(10, 174, 300, 54, 6, COL_ACCENT);
    _tft.setTextColor(TFT_WHITE, COL_BTN, true);
    _tft.drawCentreString("< BACK", 160, 194, 1);
    _tft.unloadFont();
}
// END OF FILE
