# DarkTube – Nintendo Switch Homebrew YouTube Client
Author: DarkTube Dev
Platform: Nintendo Switch (CFW)
Toolchain: devkitPro (libnx)

---

## 1. Project Goal

DarkTube is a lightweight YouTube client for Nintendo Switch CFW.

It uses:
- Invidious API for metadata
- Direct stream URLs (adaptive format)
- libcurl for HTTP
- mbedTLS for HTTPS
- cJSON for JSON parsing
- libnx for UI + input

---

## 2. Architecture Overview

Inspired by TsVitch:
https://github.com/giovannimirulla/TsVitch

TsVitch Architecture:
- Uses libnx
- Uses network module
- Custom video player pipeline
- Streaming over HTTP
- Rendering with deko3d

DarkTube Architecture:

    UI Layer
        ↓
    API Client (Invidious)
        ↓
    JSON Parser (cJSON)
        ↓
    Stream Resolver
        ↓
    Video Playback Engine

---

## 3. Dependencies

Install via devkitPro:

    pacman -S switch-dev
    pacman -S switch-curl
    pacman -S switch-mbedtls

Embed manually:
    cJSON.c
    cJSON.h

---

## 4. Directory Structure

DarkTube/
│
├── Makefile
├── source/
│   ├── main.c
│   ├── network.c
│   ├── network.h
│   ├── api.c
│   ├── api.h
│   ├── json_parser.c
│   ├── json_parser.h
│   ├── ui.c
│   ├── ui.h
│   ├── player.c
│   └── player.h
│
├── include/
├── data/
└── cjson/
    ├── cJSON.c
    └── cJSON.h

---

## 5. Implementation Phases

-----------------------------------
PHASE 1 – Networking Foundation
-----------------------------------

Goal:
- Establish HTTPS connection
- Fetch JSON from Invidious

Steps:
1. Initialize socket:
    socketInitializeDefault();
2. Initialize curl
3. Perform GET request:
    https://<instance>/api/v1/trending

Output:
Raw JSON printed to console

-----------------------------------
PHASE 2 – JSON Parsing
-----------------------------------

Goal:
Extract:
- title
- videoId
- author
- lengthSeconds

Steps:
1. Parse JSON array
2. Store into struct:

    typedef struct {
        char title[256];
        char videoId[32];
        char author[128];
        int length;
    } VideoItem;

3. Store in dynamic list (max 20)

-----------------------------------
PHASE 3 – UI Rendering
-----------------------------------

Simple console-based list:

> Trending Videos
1. Video Title
2. Video Title
3. Video Title

Input:
UP/DOWN → Navigate
A → Select video
+ → Exit

-----------------------------------
PHASE 4 – Stream URL Resolution
-----------------------------------

Request:
    /api/v1/videos/<videoId>

Extract:
    adaptiveFormats
Choose:
    type = video/mp4
    highest bitrate

Return:
    direct stream URL

-----------------------------------
PHASE 5 – Playback Engine
-----------------------------------

Option A:
- Basic HTTP chunk download
- Decode using hardware (future)

Option B (Advanced):
- Integrate libavcodec (heavy)
- Use deko3d texture streaming
- Hardware accelerated rendering

Reference:
TsVitch video pipeline structure

-----------------------------------
PHASE 6 – UI Upgrade
-----------------------------------

Switch from console to:
- Framebuffer UI
- deko3d rendering
- Thumbnail preview
- Progress bar

---

## 6. Networking Module Example

network.c

- init_network()
- http_get(url, buffer)
- cleanup_network()

Uses:
- curl_easy_init
- curl_easy_setopt
- curl_easy_perform

---

## 7. API Module

api.c

Functions:

    fetch_trending()
    fetch_video_info(videoId)

Returns parsed struct array.

---

## 8. Error Handling

Must handle:
- HTTPS failure
- Dead Invidious instance
- JSON parse error
- Network timeout

Recommended:
Implement instance fallback list.

---

## 9. Future Improvements

- Search API
- Channel browsing
- Login support (cookies)
- DASH adaptive streaming
- HLS support
- Hardware decoding

---

## 10. Security Notes

- Use HTTPS only
- Validate response
- Limit buffer sizes
- Avoid unsafe string operations

---

## 11. Build Instructions

make clean
make

Output:
DarkTube.nro

Copy to:
SD:/switch/DarkTube/

---

END OF DOCUMENT