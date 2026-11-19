#ifndef RADIO_AUDITION_H
#define RADIO_AUDITION_H

#include <Arduino.h>
#include "RadioConfig.h"

#define RB_UUID_LEN 40

// DATA STRUCTURE: AuditionState
// Holds the temporary preview stream and the interrupted favourite slot index
// so the radio can audition a Community Radio Browser stream and resume cleanly.
struct AuditionState {
    bool active;                 // True while temporarily auditioning a discovered stream
    int savedFavIndex;           // Index (0..MAX_FAVOURITES-1) of interrupted favourite
    char name[FAV_NAME_LEN];     // Truncated station title for display and NVRAM append
    char url[FAV_URL_LEN];       // Resolved stream URL currently being auditioned
    char uuid[RB_UUID_LEN];      // Station UUID for Radio Browser click registration
};

// WHAT: Manages temporary stream auditioning and restoration of interrupted favourites.
// WHEN: Driven by /rb_preview, /rb_commit, and /rb_stop web portal actions and main.cpp.
// HOW:  Maintains a single RAM AuditionState struct and coordinates playback callbacks.
class RadioAudition {
public:
    typedef void (*PlaybackCallback)();

    static void setPlaybackCallback(PlaybackCallback cb);
    static void startPreview(RadioConfig& cfg, const char* name, const char* url, const char* uuid);
    static int commitAndResume(RadioConfig& cfg);
    static void stopAndResume(RadioConfig& cfg);
    static bool isActive();
    static const AuditionState& getState();

private:
    static AuditionState s_state;
    static PlaybackCallback s_onPlaybackChanged;
};

#endif
// END OF FILE
