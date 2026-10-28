#pragma once

#include "components/themes/lyra/LyraTheme.h"

// Tabula "Chorus" styling on Lyra's metrics. Button hints are small arched
// niches on a ground line, labelled in capitals (docs/tabula/previews/screens.html).
class TabulaTheme final : public LyraTheme {
 public:
  void drawButtonHints(GfxRenderer& renderer, const char* btn1, const char* btn2, const char* btn3,
                       const char* btn4) const override;
};
