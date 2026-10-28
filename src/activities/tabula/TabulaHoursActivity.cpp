#include "TabulaHoursActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>
#include <Memory.h>

#include <cmath>
#include <memory>

#include "HalDisplay.h"
#include "TabulaArc.h"
#include "TabulaMoreActivity.h"
#include "activities/ActivityManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int ARC_CY = 150;
constexpr int ARC_RX = 214;
constexpr int ARC_RY = 80;
constexpr int TITLE_Y = 196;
constexpr int LIST_TOP = 236;
constexpr int ROW_H = 54;
constexpr int SIDE = 28;
}  // namespace

TabulaHoursActivity::TabulaHoursActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("TabulaHours", renderer, mappedInput) {}

void TabulaHoursActivity::onEnter() {
  Activity::onEnter();
  refreshCount_ = -1;
  tm local{};
  now_ = tabula::localNow(local) ? tabula::hourIndex(local) : -1;
  selected_ = now_ < 0 ? 0 : now_;
  status_ = nullptr;
  requestUpdate(true);
}

void TabulaHoursActivity::openSelected() {
  tm local{};
  if (!tabula::localNow(local)) {
    status_ = tr(STR_TABULA_NO_CLOCK);
    requestUpdate();
    return;
  }
  const std::string path = tabula::hoursPath(local);
  if (path.empty()) {
    status_ = tr(STR_TABULA_NOT_ON_CARD);
    requestUpdate();
    return;
  }
  activityManager.goToReader(path, false, std::string(tabula::hourId(selected_)) + ".xhtml");
}

void TabulaHoursActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    if (auto more = makeUniqueNoThrow<TabulaMoreActivity>(renderer, mappedInput)) {
      activityManager.replaceActivity(std::move(more));
    } else {
      activityManager.goHome();
    }
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    openSelected();
    return;
  }
  buttonNavigator_.onNext([this] {
    selected_ = (selected_ + 1) % tabula::HOUR_COUNT;
    requestUpdate();
  });
  buttonNavigator_.onPrevious([this] {
    selected_ = (selected_ + tabula::HOUR_COUNT - 1) % tabula::HOUR_COUNT;
    requestUpdate();
  });
}

void TabulaHoursActivity::render(RenderLock&&) {
  const int width = renderer.getScreenWidth();
  renderer.clearScreen();
  tabula::drawQuatrefoilBand(renderer, width);
  tabula::drawHourArc(renderer, width / 2, ARC_CY, ARC_RX, ARC_RY, now_);
  renderer.drawCenteredText(UI_12_FONT_ID, TITLE_Y, "HORÆ CANONICÆ", true, EpdFontFamily::BOLD);

  for (int i = 0; i < tabula::HOUR_COUNT; i++) {
    const int y = LIST_TOP + i * ROW_H;
    const bool isSelected = i == selected_;
    if (isSelected) renderer.fillRect(0, y, width, ROW_H - 4, true);
    const bool ink = !isSelected;
    renderer.drawText(UI_12_FONT_ID, SIDE, y + 10, tabula::hourLatin(i), ink);
    const char* english = tabula::hourEnglish(i);
    const int w = renderer.getTextWidth(SMALL_FONT_ID, english, EpdFontFamily::ITALIC);
    renderer.drawText(SMALL_FONT_ID, width - SIDE - w, y + 16, english, ink, EpdFontFamily::ITALIC);
  }
  if (status_) {
    renderer.drawCenteredText(UI_10_FONT_ID, LIST_TOP + tabula::HOUR_COUNT * ROW_H + 4, status_);
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_OPEN), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(static_cast<HalDisplay::RefreshMode>(tabula::nextRefreshMode(refreshCount_)));
}
