# Sketch to Screen (Waveshare ESP32-S3 LCD boards)

Photograph a drawing with your phone and see it on the board, either as a picture or as a hand-drawn style animation.

The boards have no camera, so the phone does the capturing and image processing. The board serves a web page; your phone's browser crops the photo to the screen, then sends compact raw data to the board:

- **Line-art animation:** the drawing is turned into a 1-bit ink mask (adaptive threshold, so uneven lighting and paper colour are OK). The board animates it with these modes:
  - **Boil:** pencil lines shimmer like hand-drawn animation.
  - **Rainbow:** the lines cycle through colours on black.
  - **Draw-on:** the drawing is drawn in diagonally, then held, then loops.
- **Picture:** the cropped photo as a static RGB565 image.

Whatever you sent last is saved to flash and comes back after a power cut.

## Setup
1. `cp src/secrets.h.example src/secrets.h` and set your WiFi (2.4 GHz).
2. Build and flash with the env for your board (`lcd169`, `lcd2`, `lcd28` or `lcd147`):
   ```
   pio run -e lcd2 -t upload
   pio device monitor
   ```
3. The screen shows the board's address once it is on WiFi. Open the IP shown, or `http://sketch-xxxx.local` (xxxx = last 4 characters of the board's MAC address), from a phone on the same network. If WiFi fails, the board starts its own open network named `Sketch-to-Screen-XXXX`, with the same per-board suffix so several boards in one room don't clash. The network name and IP are shown on the screen; join it and browse to that IP.

## Using it
1. Tap the file field and take a photo of your drawing (dark pen on light paper works best; fill the frame, even lighting).
2. Adjust **Sensitivity** until the preview shows clean lines. Tick *Light ink on dark paper* for chalk-style drawings.
3. Tap **Send to device**. The BOOT button on the board also cycles Picture / Boil / Rainbow / Draw-on.

## Notes
- No authentication: anyone on your network can open the page and change the display.
- Display pins and troubleshooting are in `../ESP32-S3-LCD-Boards`.
- Ideas: use the camera connector on the ESP32-S3-LCD-2 to capture directly, more animation modes, save several drawings.
