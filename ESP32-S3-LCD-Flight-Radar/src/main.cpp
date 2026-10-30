#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include <WaveshareLCD.h>
#include "secrets.h"

// Data: adsb.lol community ADS-B API (no key needed).
// GET /v2/point/{lat}/{lon}/{radius_nm}

// Active radar centre. Starts at HOME_LAT/HOME_LON; replaced by IP geolocation if USE_AUTO_LOCATION is set.
static float homeLat = HOME_LAT, homeLon = HOME_LON;

struct Plane {
  char call[9];
  float dx, dy;   // nm east / north of home
  float dist;     // nm
  int alt;        // ft
  int track;      // degrees
};

static const int MAX_PLANES = 40;
static Plane planes[MAX_PLANES];
static int planeCount = 0;
static uint32_t lastFetch = 0;
static const uint32_t FETCH_MS = 10000;
static const char *status = "starting";

LGFX lcd;
LGFX_Sprite radar(&lcd);

// Landscape layout: round radar on the left, aircraft list on the right. Sized from the screen in setup().
static const int LIST_W = 150;     // min width reserved for the list
static int D, R, CX, CY;           // radar sprite size, radius and centre (sprite coords)
static int RADAR_Y;                // top of the radar sprite on screen
static int LIST_X, MAX_ROWS;

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) delay(250);
  Serial.println(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "wifi failed");
}

// Approximate location from the public IP via ipwho.is (no key). City-level accuracy only.
bool geolocate() {
  WiFiClientSecure client;
  client.setInsecure();  // demo only: skips TLS certificate validation
  HTTPClient http;
  if (!http.begin(client, "https://ipwho.is/?fields=success,latitude,longitude,city")) return false;
  http.setTimeout(8000);
  if (http.GET() != 200) { http.end(); return false; }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err || !(doc["success"] | false)) return false;
  homeLat = doc["latitude"];
  homeLon = doc["longitude"];
  Serial.printf("Located near %s: %.4f, %.4f\n", (const char *)(doc["city"] | "?"), homeLat, homeLon);
  return true;
}

bool fetchPlanes() {
  if (WiFi.status() != WL_CONNECTED) { connectWifi(); if (WiFi.status() != WL_CONNECTED) { status = "no wifi"; return false; } }
  WiFiClientSecure client;
  client.setInsecure();  // demo only: skips TLS certificate validation
  HTTPClient http;
  char url[128];
  snprintf(url, sizeof url, "https://api.adsb.lol/v2/point/%.4f/%.4f/%d", homeLat, homeLon, RANGE_NM);
  if (!http.begin(client, url)) { status = "http err"; return false; }
  http.setTimeout(8000);
  int code = http.GET();
  if (code != 200) { Serial.printf("HTTP %d\n", code); status = "api error"; http.end(); return false; }

  JsonDocument filter;
  JsonObject f = filter["ac"].add<JsonObject>();
  f["flight"] = true; f["lat"] = true; f["lon"] = true;
  f["alt_baro"] = true; f["track"] = true;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) { Serial.println(err.c_str()); status = "json err"; return false; }

  planeCount = 0;
  const float kx = cosf(homeLat * DEG_TO_RAD) * 60.0f;
  for (JsonObject ac : doc["ac"].as<JsonArray>()) {
    if (planeCount >= MAX_PLANES) break;
    if (ac["lat"].isNull() || ac["lon"].isNull()) continue;
    Plane &p = planes[planeCount];
    p.dx = ((float)ac["lon"] - homeLon) * kx;
    p.dy = ((float)ac["lat"] - homeLat) * 60.0f;
    p.dist = sqrtf(p.dx * p.dx + p.dy * p.dy);
    p.alt = ac["alt_baro"].is<int>() ? ac["alt_baro"].as<int>() : 0;  // "ground" -> 0
    p.track = ac["track"] | 0;
    const char *cs = ac["flight"] | "";
    strlcpy(p.call, cs, sizeof p.call);
    for (int i = strlen(p.call) - 1; i >= 0 && p.call[i] == ' '; i--) p.call[i] = 0;
    if (!p.call[0]) strcpy(p.call, "------");
    planeCount++;
  }
  // nearest first
  for (int i = 1; i < planeCount; i++)
    for (int j = i; j > 0 && planes[j].dist < planes[j - 1].dist; j--) std::swap(planes[j], planes[j - 1]);
  status = "live";
  return true;
}

uint16_t altColor(int alt) {
  if (alt < 3000) return TFT_GREEN;
  if (alt < 15000) return TFT_YELLOW;
  if (alt < 30000) return TFT_ORANGE;
  return TFT_CYAN;
}

void drawRadar() {
  radar.fillScreen(TFT_BLACK);
  uint16_t ring = 0x0320;  // dark green
  for (int i = 1; i <= 3; i++) radar.drawCircle(CX, CY, R * i / 3, ring);
  radar.drawFastHLine(CX - R, CY, 2 * R + 1, ring);
  radar.drawFastVLine(CX, CY - R, 2 * R + 1, ring);
  radar.setTextColor(0x07E0);
  radar.setTextSize(1);
  radar.drawString("N", CX + 3, CY - R);
  radar.fillCircle(CX, CY, 2, TFT_WHITE);

  float scale = (float)R / RANGE_NM;
  for (int i = 0; i < planeCount; i++) {
    Plane &p = planes[i];
    if (p.dist > RANGE_NM) continue;
    int x = CX + (int)(p.dx * scale), y = CY - (int)(p.dy * scale);
    uint16_t c = altColor(p.alt);
    float a = p.track * DEG_TO_RAD;
    radar.drawLine(x, y, x + (int)(sinf(a) * 8), y - (int)(cosf(a) * 8), c);
    radar.fillCircle(x, y, 2, c);
    if (i < 4) { radar.setTextColor(TFT_WHITE); radar.drawString(p.call, x + 4, y + 2); }
  }
  radar.pushSprite(0, RADAR_Y);
}

void drawList() {
  lcd.fillRect(LIST_X - 2, 0, lcd.width() - LIST_X + 2, lcd.height(), TFT_BLACK);
  lcd.setTextSize(1);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setCursor(LIST_X, 2);
  lcd.printf("%d ac  %dnm  [%s]", planeCount, RANGE_NM, status);
  for (int i = 0; i < planeCount && i < MAX_ROWS; i++) {
    lcd.setTextColor(altColor(planes[i].alt), TFT_BLACK);
    lcd.setCursor(LIST_X, 18 + i * 12);
    lcd.printf("%-8s %5dft %3.0fnm", planes[i].call, planes[i].alt, planes[i].dist);
  }
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.setRotation(1);  // landscape; use 3 to flip 180 degrees
  lcd.setBrightness(200);
  lcd.fillScreen(TFT_BLACK);
  D = min((int)lcd.height(), (int)lcd.width() - LIST_W);
  R = D / 2 - 2;
  CX = CY = D / 2;
  RADAR_Y = (lcd.height() - D) / 2;
  LIST_X = D + 4;
  MAX_ROWS = (lcd.height() - 18) / 12;
  radar.setColorDepth(16);
  radar.createSprite(D, D);
  radar.setTextDatum(top_left);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.drawString("Connecting WiFi...", 4, 4);
  connectWifi();
#ifdef USE_AUTO_LOCATION
  if (!geolocate()) Serial.println("Geolocation failed, using HOME_LAT/HOME_LON");
#endif
  lcd.fillScreen(TFT_BLACK);
}

void loop() {
  if (lastFetch == 0 || millis() - lastFetch > FETCH_MS) {
    lastFetch = millis();
    fetchPlanes();
    drawRadar();
    drawList();
  }
  delay(50);
}
