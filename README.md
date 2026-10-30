# ESP32
Collection of ESP32 related projects

## Projects

PlatformIO + Arduino projects for Waveshare ESP32-S3 boards with ST7789 IPS LCDs (1.69", 2.0", 2.8" and 1.47"). Pick your board when building, e.g. `pio run -e lcd2 -t upload`. Board pins and troubleshooting are in [`ESP32-S3-LCD-Boards`](ESP32-S3-LCD-Boards/README.md), which holds the shared display library.

| Project | What it does |
|---|---|
| [`ESP32-S3-LCD-Flight-Radar`](ESP32-S3-LCD-Flight-Radar) | Live ADS-B radar of aircraft around you (adsb.lol) |
| [`ESP32-S3-LCD-Name`](ESP32-S3-LCD-Name) | Colour-cycling name scaled to fill the screen |
| [`ESP32-S3-LCD-Pacman`](ESP32-S3-LCD-Pacman) | Animated Pac-Man and ghost |
| [`ESP32-S3-LCD-Sketch`](ESP32-S3-LCD-Sketch) | Phone photo of a drawing shown as a picture or line-art animation |
