#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <WaveshareLCD.h>
#include "secrets.h"
#include "webpage.h"

// Sketch to Screen: photograph a drawing on your phone, the page (served by this board) converts it in
// the browser, and the board shows it as a picture or animates it as line art.
//   /image   POST  RGB565 frame (w*h*2 bytes), shown as a static picture
//   /lineart POST  1-bit ink mask (w*h bits, MSB first), animated on the device
//   /mode    POST  ?m=0 picture, 1 boil, 2 rainbow, 3 draw-on   (BOOT button cycles too)

enum Mode { PICTURE, BOIL, RAINBOW, DRAWON, NUM_MODES };

LGFX lcd;
WebServer server(80);
int W, H;
uint8_t *mask = nullptr;  // 1 = ink
bool haveImage = false;
Mode mode = BOIL;
uint16_t hueLut[360];
String ipText;
String apName;   // set only when running as our own access point
String hostName;
static const char *IMAGE_PATH = "/image.bin";
static const char *MASK_PATH = "/lineart.bin";
static const char *MODE_PATH = "/mode.txt";

size_t imageBytes() { return (size_t)W * H * 2; }
size_t maskBytes() { return ((size_t)W * H + 7) / 8; }

inline bool ink(int x, int y) {
  if (x < 0) x = 0; else if (x >= W) x = W - 1;
  size_t i = (size_t)y * W + x;
  return mask[i >> 3] & (0x80 >> (i & 7));
}

void saveMode() {
  File f = LittleFS.open(MODE_PATH, "w");
  if (f) { f.write((uint8_t)mode); f.close(); }
}

bool loadMask() {
  File f = LittleFS.open(MASK_PATH, "r");
  if (!f || f.size() != maskBytes()) return false;
  if (!mask) mask = (uint8_t *)malloc(maskBytes());
  if (!mask) return false;
  f.read(mask, maskBytes());
  f.close();
  return true;
}

void showMessage(const char *l1, const char *l2 = "", const char *l3 = "") {
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextDatum(middle_center);
  lcd.setTextSize(2);
  lcd.drawString(l1, W / 2, H / 2 - 30);
  lcd.drawString(l2, W / 2, H / 2);
  lcd.drawString(l3, W / 2, H / 2 + 30);
}

void showPicture() {
  File f = LittleFS.open(IMAGE_PATH, "r");
  if (!f || f.size() != imageBytes()) { showMessage("No picture yet", apName.c_str(), ipText.c_str()); return; }
  static uint16_t row[640];
  lcd.startWrite();
  for (int y = 0; y < H; y++) {
    f.read((uint8_t *)row, W * 2);
    lcd.pushImage(0, y, W, 1, row);
  }
  lcd.endWrite();
  f.close();
}

// One frame of a line-art animation. ink pixels get lineColor(x), the rest bg.
void drawArt(Mode m, uint32_t frame) {
  static uint16_t row[640];
  const uint16_t paper = 0xFFFF, pencil = 0x1082;
  int reveal = 0;
  if (m == DRAWON) {
    int total = W + H, drawFrames = 90, holdFrames = 60;
    int t = frame % (drawFrames + holdFrames);
    reveal = t >= drawFrames ? total : total * t / drawFrames;
  }
  lcd.startWrite();
  for (int y = 0; y < H; y++) {
    // "Line boil": the whole drawing jitters by a pixel or two every other frame, with a slow ripple.
    int jit = (int)(((frame / 2) * 7919u + (y / 6) * 104729u) % 3) - 1;
    int dx = jit + (int)(1.5f * sinf(y * 0.12f + frame * 0.25f));
    for (int x = 0; x < W; x++) {
      bool on = ink(x - dx, y);
      if (m == DRAWON && x + y > reveal) on = false;
      if (m == RAINBOW) row[x] = on ? hueLut[(x * 2 + frame * 6) % 360] : TFT_BLACK;
      else row[x] = on ? pencil : paper;
    }
    lcd.pushImage(0, y, W, 1, row);
  }
  lcd.endWrite();
}

void enterMode(Mode m) {
  mode = m;
  saveMode();
  if (m == PICTURE) showPicture();
  else if (!mask) showMessage("No drawing yet", apName.c_str(), ipText.c_str());
}

// Multipart upload -> file on flash, validated for size in the completion handler.
File upFile;
void handleUpload(const char *path) {
  HTTPUpload &u = server.upload();
  if (u.status == UPLOAD_FILE_START) upFile = LittleFS.open(path, "w");
  else if (u.status == UPLOAD_FILE_WRITE && upFile) upFile.write(u.buf, u.currentSize);
  else if (u.status == UPLOAD_FILE_END && upFile) upFile.close();
}

void finishUpload(const char *path, size_t expected, Mode next) {
  File f = LittleFS.open(path, "r");
  size_t got = f ? f.size() : 0;
  if (f) f.close();
  if (got != expected) {
    LittleFS.remove(path);
    server.send(400, "text/plain", "Wrong size: got " + String(got) + ", expected " + String(expected));
    return;
  }
  if (next != PICTURE && !loadMask()) { server.send(500, "text/plain", "Out of memory"); return; }
  server.send(200, "text/plain", "ok");
  enterMode(next);
}

void startNetwork() {
  // Per-board suffix (last two bytes of the MAC) so several boards in one room don't clash.
  uint64_t mac = ESP.getEfuseMac();
  char suffix[5];
  snprintf(suffix, sizeof suffix, "%02X%02X", (uint8_t)(mac >> 32), (uint8_t)(mac >> 40));
  hostName = String("sketch-") + suffix;
  hostName.toLowerCase();
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) delay(250);
  if (WiFi.status() == WL_CONNECTED) {
    ipText = "http://" + WiFi.localIP().toString();
    if (MDNS.begin(hostName.c_str())) MDNS.addService("http", "tcp", 80);
  } else {  // fall back to our own network so the board is still usable
    WiFi.mode(WIFI_AP);
    apName = String("Sketch-to-Screen-") + suffix;
    WiFi.softAP(apName.c_str());
    ipText = "http://" + WiFi.softAPIP().toString();
  }
  Serial.println(ipText);
}

void setup() {
  Serial.begin(115200);
  pinMode(0, INPUT_PULLUP);  // BOOT button
  lcd.init();
  lcd.setRotation(1);  // landscape; use 3 to flip 180 degrees
  lcd.setBrightness(200);
  W = lcd.width();
  H = lcd.height();
  for (int i = 0; i < 360; i++) {
    float h = i / 60.0f, x = 1 - fabsf(fmodf(h, 2) - 1);
    float r = 0, g = 0, b = 0;
    switch ((int)h) { case 0: r = 1; g = x; break; case 1: r = x; g = 1; break; case 2: g = 1; b = x; break;
                      case 3: g = x; b = 1; break; case 4: r = x; b = 1; break; default: r = 1; b = x; }
    hueLut[i] = lcd.color565(r * 255, g * 255, b * 255);
  }
  showMessage("Connecting WiFi...");
  if (!LittleFS.begin(true)) Serial.println("LittleFS mount failed");
  startNetwork();

  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/info", HTTP_GET, [] { server.send(200, "application/json", "{\"w\":" + String(W) + ",\"h\":" + String(H) + "}"); });
  server.on("/image", HTTP_POST, [] { finishUpload(IMAGE_PATH, imageBytes(), PICTURE); }, [] { handleUpload(IMAGE_PATH); });
  server.on("/lineart", HTTP_POST, [] { finishUpload(MASK_PATH, maskBytes(), BOIL); }, [] { handleUpload(MASK_PATH); });
  server.on("/mode", HTTP_POST, [] {
    int m = server.arg("m").toInt();
    if (m < 0 || m >= NUM_MODES) { server.send(400, "text/plain", "bad mode"); return; }
    server.send(200, "text/plain", "ok");
    enterMode((Mode)m);
  });
  server.begin();

  // Restore whatever was shown before the power went off.
  haveImage = LittleFS.exists(IMAGE_PATH);
  bool haveMask = loadMask();
  File f = LittleFS.open(MODE_PATH, "r");
  if (f) { int m = f.read(); f.close(); if (m >= 0 && m < NUM_MODES) mode = (Mode)m; }
  if (mode == PICTURE && !haveImage) mode = BOIL;
  if (mode != PICTURE && !haveMask) { mode = haveImage ? PICTURE : BOIL; }
  enterMode(mode);
}

void loop() {
  server.handleClient();
  static bool wasDown = false;
  bool down = digitalRead(0) == LOW;
  if (down && !wasDown) enterMode((Mode)((mode + 1) % NUM_MODES));
  wasDown = down;

  static uint32_t frame = 0, last = 0;
  if (mode != PICTURE && mask && millis() - last >= 50) {
    last = millis();
    drawArt(mode, frame++);
  }
}
