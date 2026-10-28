#include "TabulaArc.h"

#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <strings.h>

#include <cmath>
#include <cstdio>

#include "TabulaDay.h"
#include "fontIds.h"

namespace tabula {
namespace {
void drawQuatrefoil(const GfxRenderer& renderer, const int cx, const int cy, const int r) {
  const int o = r * 3 / 5;
  const int lobe = r * 3 / 5;
  for (const auto& d : {std::pair{0, -o}, std::pair{o, 0}, std::pair{0, o}, std::pair{-o, 0}}) {
    renderer.drawRoundedRect(cx + d.first - lobe, cy + d.second - lobe, 2 * lobe + 1, 2 * lobe + 1, 1, lobe, true);
  }
}

void drawDiamond(const GfxRenderer& renderer, const int cx, const int cy, const int r, const bool filled) {
  const int xs[] = {cx, cx + r, cx, cx - r};
  const int ys[] = {cy - r, cy, cy + r, cy};
  if (filled) {
    renderer.fillPolygon(xs, ys, 4, true);
    return;
  }
  for (int i = 0; i < 4; i++) renderer.drawLine(xs[i], ys[i], xs[(i + 1) % 4], ys[(i + 1) % 4], 2, true);
}

void drawSun(const GfxRenderer& renderer, const int cx, const int cy, const int r) {
  const int disc = r / 2;
  renderer.fillRoundedRect(cx - disc, cy - disc, 2 * disc + 1, 2 * disc + 1, disc, Color::Black);
  for (int i = 0; i < 12; i++) {
    const float a = static_cast<float>(i) * static_cast<float>(M_PI) / 6.0f;
    const float c = cosf(a);
    const float s = sinf(a);
    renderer.drawLine(cx + static_cast<int>(lroundf(r * 0.68f * c)), cy + static_cast<int>(lroundf(r * 0.68f * s)),
                      cx + static_cast<int>(lroundf(r * c)), cy + static_cast<int>(lroundf(r * s)), 2, true);
  }
}

int arcY(const int cx, const int cy, const int rx, const int ry, const int x) {
  const float u = static_cast<float>(x - cx) / static_cast<float>(rx);
  return cy - static_cast<int>(lroundf(ry * sqrtf(std::max(0.0f, 1.0f - u * u))));
}
}  // namespace

namespace {
// Outline of an equilateral pointed arch: two arcs of radius (x1 - x0) that
// meet at the apex, on straight jambs down to `yb`.
void drawLancet(const GfxRenderer& renderer, const int x0, const int x1, const int yb, const int ytop,
                const int lineWidth) {
  const int w = x1 - x0;
  const int spring = ytop + static_cast<int>(lroundf(w * 0.866f));
  constexpr int steps = 18;
  int px = x0, py = spring;
  for (int i = 1; i <= steps; i++) {
    const float a = (static_cast<float>(M_PI) / 3.0f) * i / steps;
    const int x = x1 - static_cast<int>(lroundf(w * cosf(a)));
    const int y = spring - static_cast<int>(lroundf(w * sinf(a)));
    renderer.drawLine(px, py, x, y, lineWidth, true);
    renderer.drawLine(x0 + x1 - px, py, x0 + x1 - x, y, lineWidth, true);
    px = x;
    py = y;
  }
  renderer.drawLine(x0, spring, x0, yb, lineWidth, true);
  renderer.drawLine(x1, spring, x1, yb, lineWidth, true);
}

void drawCircle(const GfxRenderer& renderer, const int cx, const int cy, const int r) {
  renderer.drawRoundedRect(cx - r, cy - r, 2 * r + 1, 2 * r + 1, 1, r, true);
}
}  // namespace

void drawRubricBar(const GfxRenderer& renderer, const int width, const char* text) {
  constexpr int BAR_H = 38;
  renderer.fillRect(0, 0, width, BAR_H, true);
  for (const int cx : {26, width - 26}) {
    renderer.fillRect(cx - 1, 10, 3, 18, false);
    renderer.fillRect(cx - 6, 14, 13, 3, false);
  }
  const int textH = renderer.getTextHeight(SMALL_FONT_ID);
  renderer.drawCenteredText(SMALL_FONT_ID, (BAR_H - textH) / 2, text, false, EpdFontFamily::BOLD);
}

void drawPosture(const GfxRenderer& renderer, const int x, const int y, const char* posture, const bool ink) {
  // Figures face right; 3 px strokes survive the 1-bit panel (previews: screens.html P1).
  const auto line = [&](int x0, int y0, int x1, int y1) { renderer.drawLine(x + x0, y + y0, x + x1, y + y1, 3, ink); };
  const auto head = [&](int cx) {
    renderer.fillRoundedRect(x + cx - 3, y + 1, 7, 7, 3, ink ? Color::Black : Color::White);
  };
  if (strcasecmp(posture, "stand") == 0) {
    head(12);
    line(12, 9, 12, 16);
    line(12, 16, 9, 23);
    line(12, 16, 15, 23);
    line(12, 11, 9, 15);
    line(12, 11, 15, 15);
  } else if (strcasecmp(posture, "kneel") == 0) {
    head(13);
    line(13, 9, 13, 20);
    line(13, 20, 5, 22);
    line(13, 12, 17, 9);
  } else if (strcasecmp(posture, "sit") == 0) {
    head(9);
    line(9, 9, 9, 16);
    line(9, 16, 16, 16);
    line(16, 16, 16, 23);
    line(9, 12, 13, 14);
    renderer.drawLine(x + 4, y + 12, x + 4, y + 23, 2, ink);
  }
}

void drawChorusHeader(const GfxRenderer& renderer, const int width, const char* barText) {
  drawRubricBar(renderer, width, barText);
  const int cx = width / 2;
  drawLancet(renderer, cx - 54, cx + 54, 178, 58, 2);
  drawLancet(renderer, cx - 44, cx + 44, 178, 72, 1);
  for (int i = 0; i < 3; i++) {
    const float a = -static_cast<float>(M_PI) / 2.0f + i * 2.0f * static_cast<float>(M_PI) / 3.0f;
    drawCircle(renderer, cx + static_cast<int>(lroundf(10 * cosf(a))), 128 + static_cast<int>(lroundf(10 * sinf(a))),
               10);
  }
  renderer.fillRect(cx - 68, 177, 136, 2, true);
}

const char* latinWeekday(const int weekday) {
  static constexpr const char* FERIA[] = {"DOMINICA", "FERIA II", "FERIA III", "FERIA IV",
                                          "FERIA V",  "FERIA VI", "SABBATO"};
  return FERIA[weekday % 7];
}

void shortDate(const tm& date, char* out, const int size) {
  static constexpr const char* MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  snprintf(out, size, "%s %d, %d", MONTHS[date.tm_mon % 12], date.tm_mday, date.tm_year + 1900);
}

void drawQuatrefoilBand(const GfxRenderer& renderer, const int width) {
  constexpr int count = 9;
  const float step = static_cast<float>(width - 60) / (count - 1);
  for (int i = 0; i < count; i++) drawQuatrefoil(renderer, 30 + static_cast<int>(lroundf(i * step)), 16, 11);
  renderer.fillRect(10, 33, width - 20, 2, true);
  renderer.fillRect(10, 39, width - 20, 1, true);
}

void drawHourArc(const GfxRenderer& renderer, const int cx, const int cy, const int rx, const int ry, const int now) {
  for (int x = cx - rx; x <= cx + rx; x += 6) renderer.drawPixel(x, arcY(cx, cy, rx, ry, x), true);
  for (int i = 0; i < HOUR_COUNT; i++) {
    const int x = cx - rx + i * 2 * rx / (HOUR_COUNT - 1);
    const int y = arcY(cx, cy, rx, ry, x);
    if (i == now) {
      drawSun(renderer, x, y, 13);
    } else {
      // Clear the dotted arc under the mark so the outline reads cleanly.
      renderer.fillRect(x - 7, y - 7, 15, 15, false);
      drawDiamond(renderer, x, y, 6, now >= 0 && i < now);
    }
  }
}

int nextRefreshMode(int& count) {
  constexpr int FAST_BETWEEN_CLEAN = 10;
  if (count < 0 || count >= FAST_BETWEEN_CLEAN) {
    count = 0;
    return HalDisplay::HALF_REFRESH;
  }
  count++;
  return HalDisplay::FAST_REFRESH;
}

}  // namespace tabula
