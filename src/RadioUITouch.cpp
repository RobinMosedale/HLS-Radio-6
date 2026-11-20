#include "RadioUI.h"

#define SLOTS_PER_PAGE 10

extern uint8_t g_favPage;

UiAction RadioUI::handleTouch() {
    uint16_t touchX = 0, touchY = 0;
    if (!_tft.getTouch(&touchX, &touchY)) return ACTION_NONE;

    if (millis() - _lastTouchMs < 220) return ACTION_NONE;
    _lastTouchMs = millis();

    touchX = 320 - touchX;
    touchY = map(touchY, 0, 180, 0, 240);
    if (touchY > 240) touchY = 240;

    if (_screen == SCREEN_MAIN) return handleMainTouch(touchX, touchY);
    if (_screen == SCREEN_SETTINGS) return handleSettingsTouch(touchX, touchY);
    return handleFavPreviewTouch(touchX, touchY);
}

UiAction RadioUI::handleMainTouch(uint16_t x, uint16_t y) {
    if (x > 274) return ACTION_NONE;

    if (y >= 116 && y <= 149) {
        int clampedX = constrain(x, 54, 222);
        uint8_t newVol = map(clampedX, 54, 222, 0, 21);
        _cfg.setVolume(newVol);
        drawSliders();
        return ACTION_VOLUME_CHANGED;
    }

    if (y >= 150 && y <= 184) {
        int clampedX = constrain(x, 54, 222);
        uint8_t newBrt = map(clampedX, 54, 222, BRT_MIN_PCT, BRT_MAX_PCT);
        setBrightness(newBrt);
        drawSliders();
        return ACTION_NONE;
    }

    if (y >= 186) {
        if (x < 74) {
            int prev = _cfg.getNextValidFav(_cfg.getActiveFavIndex(), -1);
            _cfg.setActiveFavIndex(prev);
            _metaText = "Tuning...";
            drawStationCard();
            return ACTION_STATION_CHANGED;
        } else if (x < 141) {
            int next = _cfg.getNextValidFav(_cfg.getActiveFavIndex(), 1);
            _cfg.setActiveFavIndex(next);
            _metaText = "Tuning...";
            drawStationCard();
            return ACTION_STATION_CHANGED;
        } else if (x < 208) {
            g_favPage = _cfg.getActiveFavIndex() / SLOTS_PER_PAGE;
            _screen = SCREEN_FAV_PREVIEW;
            drawCurrentScreen();
            return ACTION_NONE;
        } else {
            _screen = SCREEN_SETTINGS;
            drawCurrentScreen();
            return ACTION_NONE;
        }
    }
    return ACTION_NONE;
}

UiAction RadioUI::handleSettingsTouch(uint16_t x, uint16_t y) {
    if (y >= 38 && y <= 94) {
        int nextTz = (_cfg.getTimezoneIndex() + 1) % _cfg.getTimezoneCount();
        _cfg.setTimezoneIndex(nextTz);
        drawSettingsScreen();
    } else if (y >= 104 && y <= 160) {
        return ACTION_START_WIFI_BEACON;
    } else if (y >= 174) {
        _screen = SCREEN_MAIN;
        drawCurrentScreen();
    }
    return ACTION_NONE;
}

UiAction RadioUI::handleFavPreviewTouch(uint16_t x, uint16_t y) {
    if (y >= 36 && y <= 166) {
        int col = (x < 160) ? 0 : 1;
        int row = (y - 36) / 26;
        int slotIdx = (g_favPage * SLOTS_PER_PAGE) + (col * 5) + row;
        if (slotIdx >= 0 && slotIdx < MAX_FAVOURITES && strlen(_cfg.getFavourite(slotIdx).url) > 0) {
            _cfg.setActiveFavIndex(slotIdx);
            _metaText = "Tuning...";
            _screen = SCREEN_MAIN;
            drawCurrentScreen();
            return ACTION_STATION_CHANGED;
        }
    } else if (y >= 174) {
        if (x >= 160) {
            g_favPage = (g_favPage == 0) ? 1 : 0;
            drawFavPreviewScreen();
        } else {
            _screen = SCREEN_MAIN;
            drawCurrentScreen();
        }
    }
    return ACTION_NONE;
}
// END OF FILE
