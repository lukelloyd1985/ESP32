# Name display (Waveshare ESP32-S3 LCD boards)

Shows a name scaled to fill the landscape screen, slowly cycling through the colours.

Change the text in `src/config.h` (`DISPLAY_NAME`), then:

```
pio run -e lcd2 -t upload  # lcd169, lcd2, lcd28 or lcd147
pio device monitor
```

Short names scale up until they hit the screen height; long names shrink to fit the width. Display pins and troubleshooting are the same as `../ESP32-S3-LCD-Boards`.
