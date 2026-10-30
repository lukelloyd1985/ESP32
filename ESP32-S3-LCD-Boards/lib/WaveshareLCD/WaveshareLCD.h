#pragma once
// LovyanGFX setup for Waveshare ESP32-S3 boards with ST7789 IPS LCDs.
// Select the board with one build flag (see each project's platformio.ini):
//   -DBOARD_LCD_147   ESP32-S3-LCD-1.47            172x320
//   -DBOARD_LCD_169   ESP32-S3-Touch-LCD-1.69      240x280  (SKU WAV-27932)
//   -DBOARD_LCD_2     ESP32-S3-LCD-2               240x320  (SKU WAV-30306)
//   -DBOARD_LCD_28    ESP32-S3-Touch-LCD-2.8       240x320  (SKU WAV-30223)
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

struct BoardConfig {
  const char *name;
  int panel_w, panel_h;      // native (portrait) panel size
  int offset_x, offset_y;    // panel position inside the ST7789 240x320 RAM
  bool invert;
  int sclk, mosi, miso, dc, cs, rst, bl;
};

#if defined(BOARD_LCD_147)
// Pins from Waveshare's ESP32-S3-LCD-1.47 documentation (not re-verified against the demo code).
static constexpr BoardConfig BOARD = {"ESP32-S3-LCD-1.47", 172, 320, 34, 0, true, 40, 45, -1, 41, 42, 39, 46};
#elif defined(BOARD_LCD_169)
// Waveshare ESP32-S3-Touch-LCD-1.69 wiki + Arduino demo (pin_config.h; gap 0,20; inverted).
static constexpr BoardConfig BOARD = {"ESP32-S3-Touch-LCD-1.69", 240, 280, 0, 20, true, 6, 7, -1, 4, 5, 8, 15};
#elif defined(BOARD_LCD_2)
// Waveshare ESP32-S3-LCD-2 Arduino demo 02_gfx_helloworld (IPS = inverted, RST not connected).
static constexpr BoardConfig BOARD = {"ESP32-S3-LCD-2", 240, 320, 0, 0, true, 39, 38, 40, 42, 45, -1, 1};
#elif defined(BOARD_LCD_28)
// Waveshare ESP32-S3-Touch-LCD-2.8 wiki + Arduino demo (Display_ST7789.h; INVON).
static constexpr BoardConfig BOARD = {"ESP32-S3-Touch-LCD-2.8", 240, 320, 0, 0, true, 40, 45, -1, 41, 42, 39, 5};
#else
#error "Define a board, e.g. -DBOARD_LCD_169 (see WaveshareLCD.h)"
#endif

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI _bus;
  lgfx::Light_PWM _light;

 public:
  LGFX() {
    {
      auto c = _bus.config();
      c.spi_host = SPI3_HOST;
      c.spi_mode = 0;
      c.freq_write = 40000000;
      c.pin_sclk = BOARD.sclk;
      c.pin_mosi = BOARD.mosi;
      c.pin_miso = -1;  // write-only; avoids clashing with the SD card on shared pins
      c.pin_dc = BOARD.dc;
      _bus.config(c);
      _panel.setBus(&_bus);
    }
    {
      auto c = _panel.config();
      c.pin_cs = BOARD.cs;
      c.pin_rst = BOARD.rst;
      c.panel_width = BOARD.panel_w;
      c.panel_height = BOARD.panel_h;
      c.offset_x = BOARD.offset_x;
      c.offset_y = BOARD.offset_y;
      c.invert = BOARD.invert;
      c.rgb_order = false;
      _panel.config(c);
    }
    {
      auto c = _light.config();
      c.pin_bl = BOARD.bl;
      c.invert = false;
      c.freq = 44100;
      c.pwm_channel = 7;
      _light.config(c);
      _panel.setLight(&_light);
    }
    setPanel(&_panel);
  }
};
