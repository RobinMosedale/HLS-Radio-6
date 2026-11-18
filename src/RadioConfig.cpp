#include "RadioConfig.h"
#include <Preferences.h> // ESP32 NVS (Non-Volatile Storage) flash library
#include <time.h>        // Standard C time library for NTP and POSIX timezones

// Lookup table of supported timezones stored in flash memory.
// Each entry pairs a screen label with an official POSIX DST rule string.
static const TimezoneOption TIMEZONES[MAX_TIMEZONES] = {
    {"UK",   "GMT0BST,M3.5.0/1,M10.5.0"},
    {"EU Central",     "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"EU Eastern",     "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"US Eastern",     "EST5EDT,M3.2.0,M11.1.0"},
    {"US Pacific",     "PST8PDT,M3.2.0,M11.1.0"},
    {"UTC",            "UTC0"}
};
// WHAT: Removes a station at idx, compacts remaining stations upward, and saves to NVS.
// WHEN: Triggered when the user clicks Del on a favourite slot in the portal.
// HOW:  Shifts occupied slots upward via setFavourite(), clears the tail, and updates _activeFav.
// WITH: idx (0..MAX_FAVOURITES-1).
bool RadioConfig::deleteFavourite(int idx) {
    if (idx < 0 || idx >= MAX_FAVOURITES || _favs[idx].url[0] == '\0') return false;

    for (int i = idx; i < MAX_FAVOURITES - 1; i++) {
        if (_favs[i + 1].url[0] != '\0') {
            setFavourite(i, _favs[i + 1].name, _favs[i + 1].url);
        } else {
            clearFavourite(i);
        }
    }
    clearFavourite(MAX_FAVOURITES - 1);

    if (_activeFav == idx) {
        if (_favs[idx].url[0] == '\0' && idx > 0) setActiveFavIndex(idx - 1);
    } else if (_activeFav > idx) {
        setActiveFavIndex(_activeFav - 1);
    }
    return true;
}

// WHAT: Moves a populated favourite slot up (-1) or down (+1) within occupied slots.
// WHEN: Triggered when the user clicks Up or Down on a favourite row in the portal.
// HOW:  Blocks moving into empty slots, swaps via setFavourite(), and updates _activeFav.
// WITH: idx (0..MAX_FAVOURITES-1) and dir (-1 or +1).
bool RadioConfig::moveFavourite(int idx, int dir) {
    int target = idx + dir;
    if (idx < 0 || idx >= MAX_FAVOURITES || target < 0 || target >= MAX_FAVOURITES) return false;
    if (_favs[idx].url[0] == '\0' || _favs[target].url[0] == '\0') return false;

    FavStation temp = _favs[idx];
    setFavourite(idx, _favs[target].name, _favs[target].url);
    setFavourite(target, temp.name, temp.url);

    if (_activeFav == idx) setActiveFavIndex(target);
    else if (_activeFav == target) setActiveFavIndex(idx);
    return true;
}


// WHAT: Initialises RAM variables from non-volatile flash (NVRAM) on boot.
// WHEN: Called once inside setup() before Wi-Fi or the UI starts.
// HOW:  Opens the "radio_sys" NVS namespace; seeds defaults on first-ever boot,
//       otherwise reads saved volume, timezone, active preset, and 10 stations.
void RadioConfig::begin() {
    Preferences prefs;
    prefs.begin("radio_sys", false); // Open NVS namespace in read/write mode

    // Load volume (default 16 if key doesn't exist yet) and clamp to safe 0..21 range
    _volume = prefs.getUChar("vol", 16);
    if (_volume > 21) _volume = 16;

    // Load timezone index (default 0 = UK) and guard against out-of-bounds values
    _tzIndex = prefs.getInt("tz_idx", 0);
    if (_tzIndex < 0 || _tzIndex >= MAX_TIMEZONES) _tzIndex = 0;

    // Load last active favourite slot (0..9)
    _activeFav = prefs.getInt("act_fav", 0);
    if (_activeFav < 0 || _activeFav >= MAX_FAVOURITES) _activeFav = 0;

    if (!prefs.isKey("fav_init")) {
        // First boot after flash erase: close NVS handle and write default BBC presets
        prefs.end();
        seedDefaultFavourites();
    } else {
        // Subsequent boots: copy each saved station name and URL from NVS into _favs[]
        for (int i = 0; i < MAX_FAVOURITES; i++) {
            String n = prefs.getString(("fn_" + String(i)).c_str(), "");
            String u = prefs.getString(("fu_" + String(i)).c_str(), "");

            // strncpy copies up to (FAV_NAME_LEN - 1) bytes to prevent buffer overflows
            strncpy(_favs[i].name, n.c_str(), FAV_NAME_LEN - 1);
            _favs[i].name[FAV_NAME_LEN - 1] = '\0'; // Guarantee null-termination

            strncpy(_favs[i].url, u.c_str(), FAV_URL_LEN - 1);
            _favs[i].url[FAV_URL_LEN - 1] = '\0';
        }
        prefs.end();
    }

    applyTimezone();
}

// WHAT: Populates slots 1..5 with default BBC HLS streams on first-ever boot.
// WHEN: Called exclusively by begin() when "fav_init" is missing from NVS.
// HOW:  Clears all 10 structs in RAM, calls setFavourite() for 5 BBC channels,
//       and writes the "fav_init" flag so user edits are never overwritten later.
void RadioConfig::seedDefaultFavourites() {
    for (int i = 0; i < MAX_FAVOURITES; i++) {
        _favs[i].name[0] = '\0';
        _favs[i].url[0] = '\0';
    }

    setFavourite(0, "BBC Radio 4",
        "http://as-hls-ww-live"
        ".akamaized.net/pool_55057080/live/ww/bbc_radio_fourfm/"
        "bbc_radio_fourfm.isml/bbc_radio_fourfm-audio%3d96000.norewind.m3u8");

    setFavourite(1, "BBC Radio 2",
        "http://as-hls-ww-live"
        ".akamaized.net/pool_74208725/live/ww/bbc_radio_two/"
        "bbc_radio_two.isml/bbc_radio_two-audio%3d96000.norewind.m3u8");

    setFavourite(2, "BBC Radio 3",
        "http://as-hls-ww-live"
        ".akamaized.net/pool_23461179/live/ww/bbc_radio_three/"
        "bbc_radio_three.isml/bbc_radio_three-audio%3d96000.norewind.m3u8");

    setFavourite(3, "BBC 6 Music",
        "http://as-hls-ww-live"
        ".akamaized.net/pool_81827798/live/ww/bbc_6music/"
        "bbc_6music.isml/bbc_6music-audio%3d96000.norewind.m3u8");

    setFavourite(4, "BBC World Service",
        "http://as-hls-ww-live"
        ".akamaized.net/pool_87948813/live/ww/bbc_world_service/"
        "bbc_world_service.isml/bbc_world_service-audio%3d96000.norewind.m3u8");

    Preferences prefs;
    prefs.begin("radio_sys", false);
    prefs.putBool("fav_init", true);
    prefs.end();
}

// WHAT: Returns the current volume (0..21) stored in private member _volume.
uint8_t RadioConfig::getVolume() const { return _volume; }

// WHAT: Updates volume in RAM and persists it to NVS flash.
// WHEN: Called when the user drags/taps the volume slider or restores a JSON backup.
// WITH: vol - target volume level (clamped to 21 max).
void RadioConfig::setVolume(uint8_t vol) {
    if (vol > 21) vol = 21;
    if (_volume == vol) return; // Skip flash write if value did not change
    _volume = vol;
    Preferences prefs;
    prefs.begin("radio_sys", false);
    prefs.putUChar("vol", _volume);
    prefs.end();
}

// WHAT: Returns the active index (0..5) into the TIMEZONES lookup array.
int RadioConfig::getTimezoneIndex() const { return _tzIndex; }

// WHAT: Saves a new timezone index to NVS and re-syncs the ESP32 system clock.
// WHEN: Called when tapping the Timezone box in Settings or restoring JSON.
void RadioConfig::setTimezoneIndex(int idx) {
    if (idx < 0 || idx >= MAX_TIMEZONES) return;
    _tzIndex = idx;
    Preferences prefs;
    prefs.begin("radio_sys", false);
    prefs.putInt("tz_idx", _tzIndex);
    prefs.end();
    applyTimezone();
}

// WHAT: Accessors returning pointers/references into the active TIMEZONES entry.
const char* RadioConfig::getTimezoneLabel() const { return TIMEZONES[_tzIndex].label; }
const char* RadioConfig::getTimezonePosix() const { return TIMEZONES[_tzIndex].posix; }
int RadioConfig::getTimezoneCount() const { return MAX_TIMEZONES; }
const TimezoneOption& RadioConfig::getTimezoneOption(int idx) const { return TIMEZONES[idx]; }

// WHAT: Configures the ESP32 SNTP client with the active POSIX timezone and NTP pools.
// WHEN: Called on boot (before and after Wi-Fi connects) and whenever timezone changes.
void RadioConfig::applyTimezone() {
    configTzTime(TIMEZONES[_tzIndex].posix, "pool.ntp.org", "time.nist.gov");
}

// WHAT: Returns the currently selected favourite slot index (0..9).
int RadioConfig::getActiveFavIndex() const { return _activeFav; }

// WHAT: Persists the active favourite slot index so the radio resumes it after reboot.
void RadioConfig::setActiveFavIndex(int idx) {
    if (idx < 0 || idx >= MAX_FAVOURITES) return;
    _activeFav = idx;
    Preferences prefs;
    prefs.begin("radio_sys", false);
    prefs.putInt("act_fav", _activeFav);
    prefs.end();
}

// WHAT: Returns a read-only reference (&) to the FavStation struct at slot idx.
// HOW:  Returning by const reference avoids copying 288 bytes of char arrays on each call.
const FavStation& RadioConfig::getFavourite(int idx) const {
    return _favs[idx];
}

// WHAT: Writes a station name and URL into _favs[idx] in RAM and saves both to NVS.
// WITH: idx (0..9), name (C-string pointer), url (C-string pointer).
void RadioConfig::setFavourite(int idx, const char* name, const char* url) {
    if (idx < 0 || idx >= MAX_FAVOURITES) return;
    strncpy(_favs[idx].name, name, FAV_NAME_LEN - 1);
    _favs[idx].name[FAV_NAME_LEN - 1] = '\0';
    strncpy(_favs[idx].url, url, FAV_URL_LEN - 1);
    _favs[idx].url[FAV_URL_LEN - 1] = '\0';

    Preferences prefs;
    prefs.begin("radio_sys", false);
    prefs.putString(("fn_" + String(idx)).c_str(), _favs[idx].name);
    prefs.putString(("fu_" + String(idx)).c_str(), _favs[idx].url);
    prefs.end();
}

// WHAT: Empties a favourite slot by writing empty strings to RAM and NVS.
void RadioConfig::clearFavourite(int idx) {
    setFavourite(idx, "", "");
}

// WHAT: Counts how many of the 10 slots currently hold a non-empty stream URL.
int RadioConfig::getValidFavouriteCount() const {
    int count = 0;
    for (int i = 0; i < MAX_FAVOURITES; i++) {
        if (strlen(_favs[i].url) > 0) count++;
    }
    return count;
}

// WHAT: Finds the next or previous populated favourite slot, skipping empty slots.
// WHEN: Called when the user taps PREV (direction = -1) or NEXT (direction = +1).
// HOW:  Steps circularly through 0..9 using modulo arithmetic until a slot with
//       a non-empty URL is found, or returns currentIdx if all slots are empty.
int RadioConfig::getNextValidFav(int currentIdx, int direction) const {
    for (int step = 1; step <= MAX_FAVOURITES; step++) {
        int candidate = (currentIdx + (step * direction) + MAX_FAVOURITES * 2) % MAX_FAVOURITES;
        if (strlen(_favs[candidate].url) > 0) {
            return candidate;
        }
    }
    return currentIdx;
}
