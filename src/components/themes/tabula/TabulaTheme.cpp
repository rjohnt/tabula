#include "TabulaTheme.h"

#include <GfxRenderer.h>
#include <HalGPIO.h>

#include <cctype>
#include <cstring>

#include "fontIds.h"

namespace {
// Under the four X3 front buttons; same columns as BaseTheme's wide layout.
constexpr int NICHE_X[] = {38, 154, 268, 384};
constexpr int NICHE_W = 106;
constexpr int NICHE_INSET = 2;
constexpr int ARCH_R = 16;
constexpr int GROUND_INSET = 20;
constexpr size_t LABEL_MAX = 24;
}  // namespace

void TabulaTheme::drawButtonHints(GfxRenderer& renderer, const char* btn1, const char* btn2, const char* btn3,
                                  const char* btn4) const {
  if (gpio.hasTouch()) {
    return;
  }

  const GfxRenderer::Orientation origOrientation = renderer.getOrientation();
  renderer.setOrientation(GfxRenderer::Orientation::Portrait);

  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  const int height = LyraMetrics::values.buttonHintsHeight;
  const int top = pageHeight - height;
  const char* labels[] = {btn1, btn2, btn3, btn4};
  const bool grayscale = renderer.getRenderMode() != GfxRenderer::BW && !renderer.grayPlanesAreAbsolute();
  // Narrower panels keep the niche widths and shift the row to fit.
  const int shift = pageWidth >= 528 ? 0 : (528 - pageWidth) / 2;

  if (!grayscale) renderer.fillRect(GROUND_INSET, pageHeight - 2, pageWidth - 2 * GROUND_INSET, 2, true);
  for (int i = 0; i < 4; i++) {
    if (labels[i] == nullptr || labels[i][0] == '\0') continue;
    const int x = NICHE_X[i] - shift + NICHE_INSET;
    const int w = NICHE_W - 2 * NICHE_INSET;
    // Zero gray-plane bits leave the monochrome hint from the base pass intact.
    renderer.fillRoundedRect(x, top, w, height, ARCH_R, true, true, false, false,
                             grayscale ? Color::Black : Color::White);
    if (grayscale) continue;
    renderer.drawRoundedRect(x, top, w, height, 2, ARCH_R, true, true, false, false, true);

    char upper[LABEL_MAX];
    size_t n = 0;
    for (const char* p = labels[i]; *p && n < LABEL_MAX - 1; p++, n++) {
      upper[n] = static_cast<char>(toupper(static_cast<unsigned char>(*p)));
    }
    upper[n] = '\0';
    drawHintLabel(renderer, UI_10_FONT_ID, upper, x, w, top + 4, height - 6);
  }

  renderer.setOrientation(origOrientation);
}
