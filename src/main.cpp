#include <Arduino.h>
#include <WiFi.h>
#include "Audio.h"
#include <TFT_eSPI.h>
#include "RadioConfig.h"
#include "RadioUI.h"
#include "WiFiPortal.h"
#include "RadioAudition.h"

#define I2S_DOUT                9
#define I2S_BCLK                3
#define I2S_LRC                 1
#define VU_CH_LEFT              0
#define VU_CH_RIGHT             1
#define META_BUF_LEN            48
#define STREAM_CONNECT_GRACE_MS 18000
#define STREAM_TIMEOUT_MS       15000
#define RECONNECT_BACKOFF_MS    6000
#define IGNORE_CLOSE_MS         4000

SET_LOOP_TASK_STACK_SIZE(20480);

TFT_eSPI tft = TFT_eSPI();
RadioConfig config;
RadioUI ui(tft, config);
WiFiPortal wifiPortal(tft, config);
Audio audio;

volatile bool tuneRequested = false;
volatile bool streamDropped = false;
unsigned long lastStreamActivityMs = 0;
unsigned long lastReconnectAttemptMs = 0;
unsigned long ignoreClosingUntilMs = 0;

void playActiveFavourite() {
    tuneRequested = true;
}

void executePendingTune() {
    if (!tuneRequested) return;
    tuneRequested = false;

    const char* targetName = nullptr;
    const char* targetUrl  = nullptr;
    if (RadioAudition::isActive()) {
        const AuditionState& aud = RadioAudition::getState();
        targetName = aud.name;
        targetUrl  = aud.url;
    } else {
        const FavStation& st = config.getFavourite(config.getActiveFavIndex());
        targetName = st.name;
        targetUrl  = st.url;
    }
    if (!targetUrl || strlen(targetUrl) == 0) return;

    ignoreClosingUntilMs = millis() + IGNORE_CLOSE_MS;
    audio.stopSong();
    ui.updateVuMeter(0, 0);
    streamDropped = false;
    lastStreamActivityMs = millis();
    lastReconnectAttemptMs = millis();

    ui.setStreamMetadata(RadioAudition::isActive() ? "Previewing..." : "Connecting...");
    ui.drawCurrentScreen();
    Serial.printf("[PLAYER] Tuning: %s -> %s\n", targetName, targetUrl);
    audio.connecttohost(targetUrl);
    lastStreamActivityMs = millis();
}

void my_audio_info(Audio::msg_t m) {
    if (m.e == Audio::evt_vu) {
        if (m.vec1.size() >= 2) ui.updateVuMeter(m.vec1[VU_CH_LEFT], m.vec1[VU_CH_RIGHT]);
        return;
    }
    lastStreamActivityMs = millis();
    if (m.e == Audio::evt_streamtitle) ui.setStreamMetadata(m.msg);
    else if (m.e == Audio::evt_bitrate) {
        long bps = atol(m.msg);
        char rateBuf[META_BUF_LEN];
        if (bps >= 1000) snprintf(rateBuf, sizeof(rateBuf), "%ld kbps", (bps + 500) / 1000);
        else snprintf(rateBuf, sizeof(rateBuf), "%s bps", m.msg);
        ui.setStreamMetadata(rateBuf);
    } else if (m.e == Audio::evt_eof && millis() > ignoreClosingUntilMs) streamDropped = true;
    else if (m.e == Audio::evt_info && millis() > ignoreClosingUntilMs) {
        if (strstr(m.msg, "failed") || strstr(m.msg, "timeout")) streamDropped = true;
    }
}

void checkStreamHealth() {
    if (WiFi.status() != WL_CONNECTED || tuneRequested) return;
    unsigned long idleMs = millis() - lastStreamActivityMs;
    unsigned long limitMs = audio.isRunning() ? STREAM_TIMEOUT_MS : STREAM_CONNECT_GRACE_MS;
    bool stalled = (idleMs > limitMs);

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
    RadioAudition::setPlaybackCallback(playActiveFavourite);
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
    executePendingTune();
    audio.loop();
    wifiPortal.handleClient();
    ui.updateClockAndSignal();
    checkStreamHealth();

    UiAction act = ui.handleTouch();
    if (act == ACTION_STATION_CHANGED) {
        if (RadioAudition::isActive()) RadioAudition::stopAndResume(config);
        else playActiveFavourite();
    } else if (act == ACTION_VOLUME_CHANGED) audio.setVolume(config.getVolume());
    else if (act == ACTION_START_WIFI_BEACON) {
        audio.stopSong();
        wifiPortal.clearAllCredentials();
        ESP.restart();
    }
}
// END OF FILE
