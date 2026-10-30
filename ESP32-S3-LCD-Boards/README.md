# Supported boards

Shared display setup (`lib/WaveshareLCD/WaveshareLCD.h`, LovyanGFX) used by the flight radar, name and Pac-Man projects. Each project builds one environment per board; pick it with `-e`.

| Env | Board | SKU | Screen | LCD pins (SCLK / MOSI / DC / CS / RST / BL) |
|---|---|---|---|---|
| `lcd169` | ESP32-S3-Touch-LCD-1.69 | WAV-27932 | 240x280 ST7789V2 | 6 / 7 / 4 / 5 / 8 / 15 |
| `lcd2` | ESP32-S3-LCD-2 | WAV-30306 | 240x320 ST7789 | 39 / 38 / 42 / 45 / none / 1 |
| `lcd28` | ESP32-S3-Touch-LCD-2.8 | WAV-30223 | 240x320 ST7789 | 40 / 45 / 41 / 42 / 39 / 5 |
| `lcd147` | ESP32-S3-LCD-1.47 | - | 172x320 ST7789 | 40 / 45 / 41 / 42 / 39 / 46 |

Pins for the first three come from Waveshare's wiki and demo code (the 1.69" also needs a 20 px row offset; all use inverted colour). The 1.47" pins are from earlier documentation and were not re-checked.

```
pio run -e lcd2 -t upload
```

Troubleshooting (edit `WaveshareLCD.h`): inverted colours -> `invert`; red/blue swapped -> `rgb_order`; shifted image -> `offset_x`/`offset_y`; upside down -> `setRotation(3)` in the project's `main.cpp`.

The 1.69" and 2.8" boards have a power-hold circuit for battery use; on USB power they start normally.
