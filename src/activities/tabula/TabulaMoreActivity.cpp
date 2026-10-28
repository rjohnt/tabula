#include "TabulaMoreActivity.h"

#include <I18n.h>
#include <Logging.h>
#include <Memory.h>

#include <memory>

#include "OrderOfMassActivity.h"
#include "TabulaHoursActivity.h"
#include "TabulaSettingsActivity.h"
#include "activities/ActivityManager.h"
#include "components/UITheme.h"

namespace fui = freeink::ui;

TabulaMoreActivity::TabulaMoreActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : UiListActivity("TabulaMore", renderer, mappedInput) {}

const char* TabulaMoreActivity::headerTitle() const { return tr(STR_TABULA_MORE); }

void TabulaMoreActivity::onBackButton() { activityManager.goHome(); }

void TabulaMoreActivity::onEnter() {
  rowCount_ = 0;
  const auto add = [this](Row row, const char* label) {
    rows_[rowCount_] = row;
    rowItems_[rowCount_] = fui::ListItem{};
    rowItems_[rowCount_].label = label;
    rowCount_++;
  };
  add(Row::OrderOfMass, tr(STR_TABULA_ORDER_OF_MASS));
  add(Row::Hours, tr(STR_TABULA_HOURS));
  add(Row::Settings, tr(STR_SETTINGS_TITLE));
#if !TABULA_PARISH
  add(Row::Library, tr(STR_LIBRARY));
#endif
  UiListActivity::onEnter();
}

void TabulaMoreActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  screen.setContentMarginFromScreen(fui::Insets{static_cast<int16_t>(metrics.topPadding + metrics.headerHeight), 0,
                                                static_cast<int16_t>(metrics.buttonHintsHeight), 0});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));
  fui::ListProps props;
  props.items = rowItems_;
  props.count = rowCount_;
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;  // physical buttons stay in loop()
  syncListViewport(screen, props);
  screen.list(props);
}

void TabulaMoreActivity::activateIndex(const int index) {
  if (index < 0 || index >= rowCount_) return;
  app.clearTapFlash();
  std::unique_ptr<Activity> next;
  switch (rows_[index]) {
    case Row::OrderOfMass:
      next = makeUniqueNoThrow<OrderOfMassActivity>(renderer, mappedInput);
      break;
    case Row::Hours:
      next = makeUniqueNoThrow<TabulaHoursActivity>(renderer, mappedInput);
      break;
    case Row::Settings:
      next = makeUniqueNoThrow<TabulaSettingsActivity>(renderer, mappedInput);
      break;
    case Row::Library:
      activityManager.goToFileBrowser("/");
      return;
  }
  if (next) {
    activityManager.replaceActivity(std::move(next));
  } else {
    LOG_ERR("TABULA", "OOM: More row %d", index);
  }
}
