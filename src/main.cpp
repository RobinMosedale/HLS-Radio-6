/*
 * ============================================================================
 * C++ PROJECT CONVENTIONS & QUICK REFERENCE
 * ============================================================================
 * 1. Leading Underscores (_volume, _tft, _cfg, _creds):
 *    By standard C++ convention, a leading underscore marks a private member
 *    variable that belongs exclusively to that class instance. This distinguishes
 *    class-level state (_volume) from temporary local variables or parameters
 *    (vol) inside a function.
 *
 * 2. const at the End of a Function (e.g., uint8_t getVolume() const):
 *    Promises the compiler that calling this function will only read data and
 *    will never alter any _ member variables inside the class.
 *
 * 3. Pointer Decay & .c_str():
 *    Functions like strncpy(), prefs.putString(), and audio.connecttohost()
 *    expect a raw C-string memory address (const char*). Calling .c_str() on
 *    an Arduino String returns a pointer to its underlying character buffer.
 * ============================================================================
 */
#include <Arduino.h>
#include <WiFi.h>
#include "Audio.h"
#include <TFT_eSPI.h>
#include "RadioConfig.h"
#include "RadioUI.h"
#include "WiFiPortal.h"

#define I2S_DOUT             9
#define I2S_BCLK             3
#define I2S_LRC              1
#define META_BUF_LEN         48
#define STREAM_TIMEOUT_MS    15000
#define RECONNECT_BACKOFF_MS 4000
#define IGNORE_CLOSE_MS      2000

SET_LOOP_TASK_STACK_SIZE(20480);

TFT_eSPI tft = TFT_eSPI();
RadioConfig config;
RadioUI ui(tft, config);
WiFiPortal wifiPortal(tft, config);
Audio audio;

volatile bool streamDropped = false;
unsigned long lastStreamActivityMs = 0;
unsigned long lastReconnectAttemptMs = 0;
unsigned long ignoreClosingUntilMs = 0;

// WHAT: Tunes ESP32-audioI2S directly to the active favourite's saved URL.
// WHEN: Called at boot, on PREV/NEXT/Preset tap, or by the stream watchdog.
void playActiveFavourite() {
    int idx = config.getActiveFavIndex();
    const FavStation& st = config.getFavourite(idx);
    if (strlen(st.url) == 0) return;

    ignoreClosingUntilMs = millis() + IGNORE_CLOSE_MS;
    audio.stopSong();
    ui.updateVuMeter(0, 0);
    streamDropped = false;
    lastStreamActivityMs = millis();
    lastReconnectAttemptMs = millis();

    Serial.printf("[PLAYER] Playing Fav #%d: %s -> %s\n", idx + 1, st.name, st.url);
    audio.connecttohost(st.url);
}

// WHAT: Asynchronous event callback invoked by ESP32-audioI2S during playback.
// WHEN: Triggered on VU frames, stream title, bitrate, EOF, or error logs.
void my_audio_info(Audio::msg_t m) {
    if (m.e == Audio::evt_vu) {
        if (m.vec1.size() >= 2) ui.updateVuMeter(m.vec1[0], m.vec1[1]);
        return;
    }
    lastStreamActivityMs = millis();
    if (m.e == Audio::evt_streamtitle) {
        ui.setStreamMetadata(m.msg);
    } else if (m.e == Audio::evt_bitrate) {
        long bps = atol(m.msg);
        char rateBuf[META_BUF_LEN];
        if (bps >= 1000) snprintf(rateBuf, sizeof(rateBuf), "%ld kbps", (bps + 500) / 1000);
        else snprintf(rateBuf, sizeof(rateBuf), "%s bps", m.msg);
        ui.setStreamMetadata(rateBuf);
    } else if (m.e == Audio::evt_eof && millis() > ignoreClosingUntilMs) {
        streamDropped = true;
    } else if (m.e == Audio::evt_info && millis() > ignoreClosingUntilMs) {
        if (strstr(m.msg, "failed") || strstr(m.msg, "timeout")) streamDropped = true;
    }
}

// WHAT: Non-blocking monitor that reconnects the active station on drop or stall.
// WHEN: Called on every pass of loop().
void checkStreamHealth() {
    if (WiFi.status() != WL_CONNECTED) return;
    bool stalled = (!audio.isRunning() && (millis() - lastStreamActivityMs > 3000)) ||
                   (millis() - lastStreamActivityMs > STREAM_TIMEOUT_MS);

    if ((streamDropped || stalled) && (millis() - lastReconnectAttemptMs > RECONNECT_BACKOFF_MS)) {
        ui.setStreamMetadata("Reconnecting...");
        playActiveFavourite();
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    pinMode(TFT_BL, OUTPUT);
    ui.setBrightness(85);
    tft.init();
    tft.setRotation(3);
    ui.showBootScreen("Initialising storage...");
    config.begin();
    ui.showBootScreen("Connecting to Wi-Fi...");
    wifiPortal.connectOrStartBeacon("BBC-Radio-Setup");
    ui.showBootScreen("Syncing NTP time...");
    config.applyTimezone();
    ui.drawCurrentScreen();
    Audio::audio_info_callback = my_audio_info;
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(config.getVolume());
    audio.settings.VU_LEVEL = true;
    playActiveFavourite();
}

void loop() {
    audio.loop();
    wifiPortal.handleClient();
    ui.updateClockAndSignal();
    checkStreamHealth();

    UiAction act = ui.handleTouch();
    if (act == ACTION_STATION_CHANGED) playActiveFavourite();
    else if (act == ACTION_VOLUME_CHANGED) audio.setVolume(config.getVolume());
    else if (act == ACTION_START_WIFI_BEACON) {
        audio.stopSong();
        wifiPortal.clearAllCredentials();
        ESP.restart();
    }
}
// END OF FILE
