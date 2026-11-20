#ifndef RADIO_CONFIG_H
#define RADIO_CONFIG_H

#include <Arduino.h>

// --- Array Capacity & Fixed Buffer Lengths ---
#define MAX_FAVOURITES   15   // Total preset slots (0..9) saved in NVRAM
#define MAX_TIMEZONES    6    // Number of selectable POSIX timezone rules
#define FAV_NAME_LEN     32   // Max station title length (31 chars + null terminator)
#define FAV_URL_LEN      256  // Max stream URL length (255 chars + null terminator)
#define TZ_LABEL_LEN     20   // Max UI display label length for timezone
#define TZ_POSIX_LEN     48   // Max POSIX timezone rule string length

// DATA STRUCTURE: FavStation
struct FavStation {
    char name[FAV_NAME_LEN]; // Null-terminated station title
    char url[FAV_URL_LEN];   // Null-terminated stream URL
};

// DATA STRUCTURE: TimezoneOption
struct TimezoneOption {
    char label[TZ_LABEL_LEN]; // Shown on screen (e.g. "UK")
    char posix[TZ_POSIX_LEN]; // Passed to configTzTime()
};

class RadioConfig {
public:
    // Initialises RAM state from NVS ("radio_sys") or seeds defaults on first boot
    void begin();

    // Volume control (0..21 range matching ESP32-audioI2S)
    uint8_t getVolume() const;
    void setVolume(uint8_t vol);

    // Timezone selection & SNTP clock configuration
    int getTimezoneIndex() const;
    void setTimezoneIndex(int idx);
    const char* getTimezoneLabel() const;
    const char* getTimezonePosix() const;
    int getTimezoneCount() const;
    const TimezoneOption& getTimezoneOption(int idx) const;
    void applyTimezone();

    // 10-slot Favourite station management
    int getActiveFavIndex() const;
    void setActiveFavIndex(int idx);
    const FavStation& getFavourite(int idx) const;
    void setFavourite(int idx, const char* name, const char* url);
    void clearFavourite(int idx);
    bool deleteFavourite(int idx);
    bool moveFavourite(int idx, int dir);
    int getValidFavouriteCount() const;
    int getNextValidFav(int currentIdx, int direction) const;

private:
    uint8_t _volume = 16;             // Private RAM mirror of saved NVRAM volume
    int _tzIndex = 0;                 // Private RAM mirror of active timezone index (0..5)
    int _activeFav = 0;               // Private RAM mirror of active favourite slot (0..9)
    FavStation _favs[MAX_FAVOURITES]; // Fixed array of 10 FavStation structs

    void seedDefaultFavourites();
};

#endif
// END OF FILE
