#include "RadioAudition.h"

#define INVALID_SLOT -1
#define ZERO_IDX     0

AuditionState RadioAudition::s_state = {false, 0, {0}, {0}, {0}};
RadioAudition::PlaybackCallback RadioAudition::s_onPlaybackChanged = nullptr;

// WHAT: Registers the main-loop playback trigger function (playActiveFavourite).
// WHEN: Called once inside setup() in main.cpp.
// HOW:  Stores function pointer invoked whenever audition starts, commits, or stops.
// WITH: cb - void function pointer that tunes ESP32-audioI2S.
void RadioAudition::setPlaybackCallback(PlaybackCallback cb) {
    s_onPlaybackChanged = cb;
}

// WHAT: Snapshots the current favourite slot and begins playing a preview stream.
// WHEN: Triggered by POST /rb_preview from the Community Radio Browser UI.
// HOW:  Preserves savedFavIndex only on initial entry so sequential previews still
//       remember the original interrupted favourite station.
// WITH: cfg (RadioConfig), name (station title), url (resolved URL), uuid (station UUID).
void RadioAudition::startPreview(RadioConfig& cfg, const char* name, const char* url, const char* uuid) {
    if (!url || url[ZERO_IDX] == '\0') return;
    if (!s_state.active) {
        s_state.savedFavIndex = cfg.getActiveFavIndex();
    }
    s_state.active = true;
    strncpy(s_state.name, name ? name : "Preview", FAV_NAME_LEN - 1);
    s_state.name[FAV_NAME_LEN - 1] = '\0';
    strncpy(s_state.url, url, FAV_URL_LEN - 1);
    s_state.url[FAV_URL_LEN - 1] = '\0';
    strncpy(s_state.uuid, uuid ? uuid : "", RB_UUID_LEN - 1);
    s_state.uuid[RB_UUID_LEN - 1] = '\0';

    if (s_onPlaybackChanged) s_onPlaybackChanged();
}

// WHAT: Appends the auditioned station to the first empty NVRAM slot and resumes audio.
// WHEN: Triggered by POST /rb_commit when the user clicks "Append to Favourites".
// HOW:  Scans slots 0..MAX_FAVOURITES-1 for an empty slot; returns INVALID_SLOT (-1)
//       without overwriting anything if all 10 slots are already occupied.
// WITH: cfg (RadioConfig).
int RadioAudition::commitAndResume(RadioConfig& cfg) {
    if (!s_state.active) return INVALID_SLOT;
    int targetSlot = INVALID_SLOT;
    for (int i = 0; i < MAX_FAVOURITES; i++) {
        if (cfg.getFavourite(i).url[ZERO_IDX] == '\0') {
            targetSlot = i;
            break;
        }
    }
    if (targetSlot == INVALID_SLOT) return INVALID_SLOT;
    cfg.setFavourite(targetSlot, s_state.name, s_state.url);
    stopAndResume(cfg);
    return targetSlot;
}

// WHAT: Cancels the audition stream and immediately resumes the interrupted favourite.
// WHEN: Triggered by POST /rb_stop or after commitAndResume() finishes saving.
// HOW:  Resets s_state.active, restores cfg.setActiveFavIndex(), and invokes playback.
// WITH: cfg (RadioConfig).
void RadioAudition::stopAndResume(RadioConfig& cfg) {
    if (!s_state.active) return;
    int restoreIdx = s_state.savedFavIndex;
    s_state.active = false;
    s_state.name[ZERO_IDX] = '\0';
    s_state.url[ZERO_IDX] = '\0';
    s_state.uuid[ZERO_IDX] = '\0';
    cfg.setActiveFavIndex(restoreIdx);
    if (s_onPlaybackChanged) s_onPlaybackChanged();
}

bool RadioAudition::isActive() { return s_state.active; }
const AuditionState& RadioAudition::getState() { return s_state; }
// END OF FILE
