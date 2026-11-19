# ESP32-S3 Internet Radio (HLS + Global Community Radio)
![Front Touchscreen UI](front_ui.jpg)
![ESP32-S3 and PCM5102A Hardware](hardware_rear.jpg)


A self-contained, SD-card-free Internet Radio built on the **ESP32-S3 N16R8** (16MB Flash + 8MB OPI PSRAM), pairing a **PCM5102A** I2S stereo DAC with a 2.8" **ILI9341** (320×240) **XPT2046** resistive touchscreen display.

---

## Core Acknowledgements & Upstream Libraries
* **Audio & HLS Engine:** Built upon Wolle's (`schreibfaul1`) **[ESP32-MiniWebRadio](https://github.com)** architecture and **[ESP32-audioI2S](https://github.com)** library for native BBC HLS `.m3u8` playlist parsing, circular PSRAM segment buffering, and I2S audio decoding.
* **Display & Touch Graphics:** Powered by Bodmer's **[TFT_eSPI](https://github.com)** driving the ILI9341 SPI display with smooth anti-aliased fonts and native XPT2046 touch handling.
* **Live Stream Sources:**
  * **BBC HLS Streams:** Dynamically fetched and cached in PSRAM from **[bpsib's maintained BBC Radio HLS Gist](https://github.com)** so URLs stay current when BBC CDN paths change.
  * **Global Stations (Version 2+):** Powered by **[radio-browser.info](https://radio-browser.info)** with multi-mirror failover.

---

## Version 1 (`master` / `version-1`) — BBC HLS Internet Radio
> **Focused on rock-solid BBC HLS live streaming, zero-SD-card reliability, blind first-boot setup, and full standalone + web control.**

* **Hardware Platform:** ESP32-S3 N16R8 (16MB DIO Flash, 8MB OPI PSRAM), PCM5102A I2S DAC, and 320×240 ILI9341 + XPT2046 SPI touchscreen.
* **SD-Card-Free NVRAM Storage:** Eliminates mechanical and filesystem failures from temperamental SD cards by storing up to 10 favourite presets, 10 Wi-Fi network credentials, volume level, and timezone settings directly in ESP32 NVS flash.
* **Dropout & Network Resilience:** Continuous stream-health watchdog detects stalled HLS chunks or EOF drops and automatically reconnects with controlled backoff, backed by multi-slot Wi-Fi failover across 10 saved networks.
* **Blind First-Boot Setup Beacon:** On first boot with empty NVRAM, automatically starts an open SoftAP hotspot (`BBC-Radio-Setup`) and displays setup instructions and its IP address directly on the TFT screen for phone/PC configuration.
* **Standalone TFT Touchscreen Interface:**
  * **NTP Clock & POSIX Timezones:** Variable-width `HH:MM` digital clock synced via SNTP with automatic Daylight Saving Time (DST) rules selectable on-screen, alongside live IP address and 4-bar Wi-Fi RSSI indicator.
  * **Station Card & Live Metadata:** Displays active preset badge (`FAV X / 10`), auto-sized station title, and real-time bitrate/stream metadata.
  * **Direct Touch Controls & Stereo VU Meter:** Smooth horizontal touch sliders for Volume (`VOL`) and Screen Brightness (`BRT`), `PREV` / `NEXT` / `FAVS` / `SETUP` navigation buttons, a 10-slot visual preset grid, and a real-time stereo `L`/`R` vertical VU meter.
* **Responsive Web Management Portal:**
  * **Live BBC Gist Picker & Favourites Manager:** Search/filter all live BBC national and regional streams, assign to slots 1–10, reorder (`↑` / `↓`), or delete with automatic slot compaction.
  * **Wi-Fi Manager & JSON Backup/Restore:** Scan nearby networks, manage 10 Wi-Fi slots, and download or upload a complete `.json` configuration backup.

---

## Version 2 (`version-2` / `dev`) — Full BBC HLS + Global Community Radio Browser
> **Retains all Version 1 capabilities while adding global station discovery via [radio-browser.info](https://radio-browser.info) and a non-destructive Audition, Append & Resume workflow.**

* **Global Community Radio Browser:** Inspired by Wolle's progressive category browsing in **[ESP32-MiniWebRadio](https://github.com)**, the web portal queries `radio-browser.info` directly with automatic mirror failover (`de2` / `fi1` / `de1`), showing codec and bitrate tags (e.g., `[MP3 128k]`, `[AAC 96k]`) across:
  * **By Country** (alphabetical A–Z with ISO country codes and station counts)
  * **By Genre / Tag**, **By Language**, and **By Codec** (MP3, AAC, AAC+, OGG, FLAC)
  * **Top Voted**, **Most Clicked**, and **Free-Text Station Name Search**
* **Temporary Audition & Resume State Machine:**
  * **Play Preview:** Temporarily tunes the radio to any discovered station without modifying saved NVRAM presets, switching the TFT station card to a highlighted **`PREVIEW`** badge with the auditioned station's title and live metadata.
  * **Append to Favourites & Resume:** Appends the auditioned station into the next empty NVRAM slot and immediately resumes the interrupted station. Includes full-slot protection that disables appending and displays a warning banner when all 10 slots are occupied.
  * **Stop & Resume:** Cancels the preview from either the web portal or TFT `PREV`/`NEXT` buttons and seamlessly returns to the interrupted favourite station.
* **Thread-Safe Deferred Stream Tuning:** Decouples HTTP web-portal tune requests from the real-time `ESP32-audioI2S` loop and extends initial connection grace windows for slow-starting HTTPS streams.
