#include "TabulaSettingsActivity.h"

#include <HalClock.h>
#include <I18n.h>
#include <Logging.h>
#include <Memory.h>
#include <WiFi.h>

#include <memory>

#include "CrossPointSettings.h"
#include "I18nKeys.h"
#include "TabulaDay.h"
#include "activities/ActivityManager.h"
#include "activities/network/WifiSelectionActivity.h"
#include "activities/settings/AboutActivity.h"
#include "activities/settings/ClockSettingsActivity.h"
#include "components/UITheme.h"

namespace fui = freeink::ui;

namespace {
// Point sizes shipped in /.fonts/EBGaramond (docs/tabula/tools/tabula-fonts.yaml).
constexpr uint8_t TEXT_SIZES[] = {14, 16, 18, 20, 22};
constexpr const char* TEXT_SIZE_NAMES[] = {"Small", "Medium", "Large", "Larger", "Largest"};

int textSizeIndex() {
  int best = 0;
  for (int i = 0; i < static_cast<int>(std::size(TEXT_SIZES)); i++) {
    if (TEXT_SIZES[i] <= SETTINGS.fontPointSize) best = i;
  }
  return best;
}

const char* readingTextName() {
  switch (SETTINGS.tabulaText) {
    case CrossPointSettings::TABULA_TEXT_LATIN:
      return tr(STR_TABULA_TEXT_LATIN);
    case CrossPointSettings::TABULA_TEXT_ENGLISH:
      return tr(STR_TABULA_TEXT_ENGLISH);
    default:
      return tr(STR_TABULA_TEXT_BOTH);
  }
}
}  // namespace

TabulaSettingsActivity::TabulaSettingsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : UiListActivity("TabulaSettings", renderer, mappedInput) {}

const char* TabulaSettingsActivity::headerTitle() const { return tr(STR_SETTINGS_TITLE); }

void TabulaSettingsActivity::onBackButton() { activityManager.goHome(); }

void TabulaSettingsActivity::refreshSubtitles() {
  textSize_ = std::string(TEXT_SIZE_NAMES[textSizeIndex()]) + " · " + std::to_string(SETTINGS.fontPointSize) + " pt";
  for (int i = 0; i < rowCount_; i++) {
    switch (rows_[i]) {
      case Row::TextSize:
        rowItems_[i].subtitle = textSize_.c_str();
        break;
      case Row::Language:
        // Each language is named in itself.
        rowItems_[i].subtitle = tabula::spanish() ? "Español" : "English";
        break;
      case Row::ReadingText:
        rowItems_[i].subtitle = readingTextName();
        break;
      case Row::MassText:
        rowItems_[i].subtitle = SETTINGS.tabulaMass == CrossPointSettings::TABULA_MASS_FULL
                                    ? tr(STR_TABULA_MASS_FULL)
                                    : tr(STR_TABULA_MASS_PROPERS);
        break;
      case Row::Wifi:
        rowItems_[i].subtitle = tr(STR_TABULA_WIFI_SUB);
        break;
      case Row::SleepScreen:
        rowItems_[i].subtitle = SETTINGS.tabulaSleepDark ? tr(STR_TABULA_DARK) : tr(STR_TABULA_LIGHT);
        break;
      default:
        break;
    }
  }
}

void TabulaSettingsActivity::buildRows() {
  rowCount_ = 0;
  const auto add = [this](Row row, const char* label) {
    rows_[rowCount_] = row;
    rowItems_[rowCount_] = fui::ListItem{};
    rowItems_[rowCount_].label = label;
    rowCount_++;
  };
  add(Row::TextSize, tr(STR_TABULA_TEXT_SIZE));
  add(Row::Language, tr(STR_TABULA_LANGUAGE));
  add(Row::ReadingText, tr(STR_TABULA_READING_TEXT));
  add(Row::MassText, tr(STR_TABULA_MASS_TEXT));
  add(Row::SleepScreen, tr(STR_TABULA_SLEEP_SCREEN));
  add(Row::Wifi, tr(STR_TABULA_WIFI));
  if (halClock.isAvailable()) add(Row::DateTime, tr(STR_TABULA_DATE_TIME));
  add(Row::About, tr(STR_TABULA_ABOUT));
#if !TABULA_PARISH
  add(Row::Library, tr(STR_TABULA_LIBRARY));
  add(Row::AllSettings, tr(STR_TABULA_ALL_SETTINGS));
#endif
  refreshSubtitles();
}

void TabulaSettingsActivity::onEnter() {
  buildRows();
  UiListActivity::onEnter();
}

void TabulaSettingsActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  screen.setContentMarginFromScreen(fui::Insets{static_cast<int16_t>(metrics.topPadding + metrics.headerHeight), 0,
                                                static_cast<int16_t>(metrics.buttonHintsHeight), 0});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));
  fui::ListProps props;
  props.items = rowItems_;
  props.count = rowCount_;
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;
  syncListViewport(screen, props);
  screen.list(props);
}

void TabulaSettingsActivity::activateIndex(const int index) {
  if (index < 0 || index >= rowCount_) return;
  app.clearTapFlash();
  switch (rows_[index]) {
    case Row::TextSize:
      // The reader reloads the SD font at the new size when it next opens.
      SETTINGS.fontPointSize = TEXT_SIZES[(textSizeIndex() + 1) % std::size(TEXT_SIZES)];
      break;
    case Row::Language: {
      // English or Spanish, each with Latin; the labels change with it.
      const Language next = tabula::spanish() ? Language::EN : Language::ES;
      SETTINGS.language = static_cast<uint8_t>(next);
      I18N.setLanguage(next);
      SETTINGS.saveToFile();
      buildRows();
      requestUpdate();
      return;
    }
    case Row::ReadingText:
      SETTINGS.tabulaText = (SETTINGS.tabulaText + 1) % CrossPointSettings::TABULA_TEXT_COUNT;
      break;
    case Row::MassText:
      SETTINGS.tabulaMass = (SETTINGS.tabulaMass + 1) % CrossPointSettings::TABULA_MASS_COUNT;
      break;
    case Row::SleepScreen:
      SETTINGS.tabulaSleepDark = SETTINGS.tabulaSleepDark ? 0 : 1;
      break;
    case Row::Wifi: {
      auto activity = makeUniqueNoThrow<WifiSelectionActivity>(renderer, mappedInput, false);
      if (!activity) {
        LOG_ERR("TABULA", "OOM: WifiSelectionActivity");
        return;
      }
      startActivityForResult(std::move(activity), [](const ActivityResult&) {
        SETTINGS.saveToFile();
        // Credentials only; release the radio the scan brought up.
        if (WiFi.getMode() != WIFI_MODE_NULL) WiFi.disconnect(true);
      });
      return;
    }
    case Row::DateTime:
      if (auto activity = makeUniqueNoThrow<ClockSettingsActivity>(renderer, mappedInput)) {
        startActivityForResult(std::move(activity), nullptr);
      }
      return;
    case Row::About:
      if (auto activity = makeUniqueNoThrow<AboutActivity>(renderer, mappedInput)) {
        startActivityForResult(std::move(activity), nullptr);
      }
      return;
    case Row::Library:
      activityManager.goToFileBrowser("/");
      return;
    case Row::AllSettings:
      activityManager.goToSettings();
      return;
  }
  SETTINGS.saveToFile();
  refreshSubtitles();
  requestUpdate();
}
