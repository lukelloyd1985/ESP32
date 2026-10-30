#include <Arduino.h>
#include <WaveshareLCD.h>
#include "config.h"

LGFX lcd;
LGFX_Sprite canvas(&lcd);

static int W, H;  // screen size after rotation, set in setup()
static const int MARGIN = 6;

void drawName(uint16_t color) {
  canvas.fillScreen(TFT_BLACK);
  canvas.setFont(&fonts::FreeSansBold24pt7b);
  canvas.setTextSize(1);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(color);
  // Scale the text so it fills the screen (limited by width or height).
  float sx = (float)(W - 2 * MARGIN) / canvas.textWidth(DISPLAY_NAME);
  float sy = (float)(H - 2 * MARGIN) / canvas.fontHeight();
  canvas.setTextSize(min(sx, sy));
  canvas.drawString(DISPLAY_NAME, W / 2, H / 2);
  canvas.pushSprite(0, 0);
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.setRotation(1);  // landscape; use 3 to flip 180 degrees
  lcd.setBrightness(200);
  W = lcd.width();
  H = lcd.height();
  canvas.setColorDepth(16);
  canvas.createSprite(W, H);
}

// Fully saturated colour for a hue in degrees (0-359).
uint16_t hueColor(uint16_t h) {
  uint8_t x = 255 * (60 - abs((int)(h % 120) - 60)) / 60;  // triangle wave 0..255
  uint8_t r, g, b;
  switch ((h / 60) % 6) {
    case 0: r = 255; g = x;   b = 0;   break;
    case 1: r = x;   g = 255; b = 0;   break;
    case 2: r = 0;   g = 255; b = x;   break;
    case 3: r = 0;   g = x;   b = 255; break;
    case 4: r = x;   g = 0;   b = 255; break;
    default: r = 255; g = 0;  b = x;   break;
  }
  return lcd.color565(r, g, b);
}

void loop() {
  // Slowly cycle the text through the colour wheel.
  static uint16_t hue = 0;
  hue = (hue + 2) % 360;
  drawName(hueColor(hue));
  delay(40);
}
