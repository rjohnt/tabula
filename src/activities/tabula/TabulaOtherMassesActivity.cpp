#include "TabulaOtherMassesActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <cctype>
#include <cstdio>

#include "HalDisplay.h"
#include "TabulaArc.h"
#include "TabulaDay.h"
#include "activities/ActivityManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int LIST_TOP = 58;
constexpr int ROW_H = 92;
constexpr int SIDE = 28;
constexpr int MARK_X = SIDE + 10;
constexpr int TEXT_X = SIDE + 30;

// Dotted rule between days, as in the muse pocket day list.
void drawDots(const GfxRenderer& renderer, const int x0, const int x1, const int y) {
  for (int x = x0; x <= x1; x += 6) renderer.drawPixel(x, y, true);
}
}  // namespace

TabulaOtherMassesActivity::TabulaOtherMassesActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("TabulaOtherMasses", renderer, mappedInput) {}

void TabulaOtherMassesActivity::onEnter() {
  Activity::onEnter();
  refreshCount_ = -1;
  count_ = 0;
  status_ = nullptr;
  tm today{};
  if (tabula::localNow(today)) {
    for (int i = 0; i < DAYS; i++) {
      tm day = today;
      day.tm_mday += i + 1;
      day.tm_hour = 12;  // clear of daylight-saving edges
      mktime(&day);
      days_[count_] = day;
      feasts_[count_] = tabula::feastName(day);
      count_++;
    }
  } else {
    status_ = tr(STR_TABULA_NO_CLOCK);
  }
  selected_ = 0;
  requestUpdate(true);
}

void TabulaOtherMassesActivity::openSelected() {
  if (count_ == 0) return;
  const std::string path = tabula::massPath(days_[selected_]);
  if (path.empty()) {
    status_ = tr(STR_TABULA_NOT_ON_CARD);
    requestUpdate();
    return;
  }
  activityManager.goToReader(path, false, "introit.xhtml");
}

void TabulaOtherMassesActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    activityManager.goHome();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    openSelected();
    return;
  }
  if (count_ == 0) return;
  buttonNavigator_.onNext([this] {
    selected_ = (selected_ + 1) % count_;
    requestUpdate();
  });
  buttonNavigator_.onPrevious([this] {
    selected_ = (selected_ + count_ - 1) % count_;
    requestUpdate();
  });
}

void TabulaOtherMassesActivity::render(RenderLock&&) {
  const int width = renderer.getScreenWidth();
  renderer.clearScreen();
  char bar[48];
  snprintf(bar, sizeof(bar), "%s · MISSAE", tr(STR_TABULA_OTHER_MASSES));
  for (char* c = bar; *c; c++) *c = static_cast<char>(toupper(static_cast<unsigned char>(*c)));
  tabula::drawRubricBar(renderer, width, bar);

  for (int i = 0; i < count_; i++) {
    const int y = LIST_TOP + i * ROW_H;
    const bool isSelected = i == selected_;
    if (isSelected) renderer.fillRect(SIDE - 10, y + 4, width - 2 * (SIDE - 10), ROW_H - 10, true);
    const bool ink = !isSelected;
    char date[32];
    if (i == 0) {
      snprintf(date, sizeof(date), "%s", tr(STR_TABULA_TOMORROW));
    } else {
      static constexpr const char* WEEKDAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
      char shortDate[24];
      tabula::shortDate(days_[i], shortDate, sizeof(shortDate));
      snprintf(date, sizeof(date), "%s, %s", WEEKDAYS[days_[i].tm_wday % 7], shortDate);
    }
    renderer.fillRect(MARK_X - 4, y + 22, 9, 9, ink);  // a small square, as the rubric's
    renderer.drawText(UI_12_FONT_ID, TEXT_X, y + 12, date, ink, EpdFontFamily::BOLD);
    const char* feast = feasts_[i].empty() ? tr(STR_TABULA_NOT_ON_CARD_DAY) : feasts_[i].c_str();
    const std::string fitted =
        renderer.truncatedText(SMALL_FONT_ID, feast, width - TEXT_X - SIDE, EpdFontFamily::ITALIC);
    renderer.drawText(SMALL_FONT_ID, TEXT_X, y + 46, fitted.c_str(), ink, EpdFontFamily::ITALIC);
    if (!isSelected && i + 1 != selected_ && i + 1 < count_) drawDots(renderer, SIDE, width - SIDE, y + ROW_H - 2);
  }
  if (status_) renderer.drawCenteredText(UI_10_FONT_ID, LIST_TOP + DAYS * ROW_H + 2, status_);

  const auto labels = mappedInput.mapLabels(tr(STR_HOME), tr(STR_OPEN), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(static_cast<HalDisplay::RefreshMode>(tabula::nextRefreshMode(refreshCount_)));
}
