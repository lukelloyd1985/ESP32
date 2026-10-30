# Pac-Man demo (Waveshare ESP32-S3 LCD boards)

Animated Pac-Man chomping along a corridor eating dots while a ghost chases, on a landscape display. Uses LovyanGFX with an off-screen sprite so there is no flicker.

```
pio run -e lcd2 -t upload  # lcd169, lcd2, lcd28 or lcd147
pio device monitor
```

Display pins and troubleshooting (inverted colours, shifted image, upside down -> `setRotation(3)`) are the same as `../ESP32-S3-LCD-Boards`.
