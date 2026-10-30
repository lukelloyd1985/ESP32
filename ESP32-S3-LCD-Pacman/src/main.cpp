#include <Arduino.h>
#include <WaveshareLCD.h>

// Pac-Man chasing dots across a landscape display, with a ghost in pursuit. Sizes scale to the screen.
// Frames are drawn into an off-screen sprite and pushed at once to avoid flicker.

LGFX lcd;
LGFX_Sprite canvas(&lcd);

static int W, H;                   // screen size after rotation, set in setup()
static int LANE_Y, DOT_GAP, PAC_R; // derived from the screen height
static const int MAX_DOTS = 64;
static const uint16_t BLUE = 0x001F;
static const uint16_t PAC_YELLOW = 0xFFE0;
static const uint16_t PINK = 0xFB56;

float pacX, ghostX;
bool dots[MAX_DOTS];
int numDots;
uint32_t frame = 0;

void resetRun() {
  pacX = -PAC_R;
  ghostX = -PAC_R - 4 * PAC_R;
  for (auto &d : dots) d = true;
}

void drawMaze() {
  int hh = PAC_R * 2;  // half height of the corridor
  canvas.drawRoundRect(2, LANE_Y - hh, W - 4, 2 * hh, 10, BLUE);
  canvas.drawRoundRect(5, LANE_Y - hh + 3, W - 10, 2 * hh - 6, 8, BLUE);
}

void drawPacman(int x, int y, int r, int mouth /*0..45 degrees*/) {
  canvas.fillArc(x, y, 0, r, mouth, 360 - mouth, PAC_YELLOW);
  canvas.fillCircle(x + r / 6, y - r / 2, 2, TFT_BLACK);  // eye
}

void drawGhost(int x, int y, int r, uint16_t col, bool legsPhase) {
  canvas.fillCircle(x, y - r / 4, r, col);                 // head
  canvas.fillRect(x - r, y - r / 4, 2 * r + 1, r + r / 4, col);  // body
  int w = (2 * r) / 4;
  for (int i = 0; i < 4; i++) {                              // wavy hem
    int cx = x - r + w / 2 + i * w;
    uint16_t c = ((i + legsPhase) & 1) ? TFT_BLACK : col;
    canvas.fillTriangle(cx - w / 2, y + r, cx + w / 2, y + r, cx, y + r - r / 3, c);
  }
  canvas.fillCircle(x - r / 3, y - r / 3, r / 3, TFT_WHITE);
  canvas.fillCircle(x + r / 3, y - r / 3, r / 3, TFT_WHITE);
  canvas.fillCircle(x - r / 3 + 2, y - r / 3, r / 6, BLUE);  // pupils look right
  canvas.fillCircle(x + r / 3 + 2, y - r / 3, r / 6, BLUE);
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.setRotation(1);  // landscape; use 3 to flip 180 degrees
  lcd.setBrightness(200);
  W = lcd.width();
  H = lcd.height();
  LANE_Y = H / 2;
  PAC_R = H / 8;
  DOT_GAP = PAC_R * 10 / 11;
  numDots = min(W / DOT_GAP + 2, MAX_DOTS);
  canvas.setColorDepth(16);
  canvas.createSprite(W, H);
  resetRun();
}

void loop() {
  uint32_t t0 = millis();
  frame++;
  float step = W / 100.0f;
  pacX += step;
  ghostX += step;

  // eat dots
  for (int i = 0; i < numDots; i++)
    if (dots[i] && i * DOT_GAP + 10 < pacX) dots[i] = false;

  // loop once both have left the screen
  if (ghostX - PAC_R > W) {
    resetRun();
  }

  canvas.fillScreen(TFT_BLACK);
  drawMaze();
  for (int i = 0; i < numDots; i++)
    if (dots[i]) canvas.fillCircle(i * DOT_GAP + 10, LANE_Y, i % 8 == 0 ? 5 : 2, TFT_WHITE);

  int mouth = abs((int)(frame % 12) - 6) * 7;  // opens and closes
  drawPacman((int)pacX, LANE_Y, PAC_R, mouth);
  drawGhost((int)ghostX, LANE_Y, PAC_R - 2, PINK, (frame / 4) & 1);
  canvas.pushSprite(0, 0);

  int32_t wait = 33 - (millis() - t0);  // ~30 fps
  if (wait > 0) delay(wait);
}
