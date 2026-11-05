#include "TabulaHomeActivity.h"

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>
#include <Memory.h>

#include <cstdio>
#include <memory>

#include "CrossPointSettings.h"
#include "HalDisplay.h"
#include "OrderOfMassActivity.h"
#include "RecentBooksStore.h"
#include "SdCardFontSystem.h"
#include "TabulaArc.h"
#include "TabulaDay.h"
#include "TabulaHoursActivity.h"
#include "TabulaOtherMassesActivity.h"
#include "TabulaSettingsActivity.h"
#include "activities/ActivityManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
// The header art is drawn at this size by docs/tabula/tools/render_tabula_art.py.
constexpr int HEAD_H = 300;
// Seven rows fit between the art (its blank foot overlaps the first row's top
// margin) and the button hints.
constexpr int ROWS_TOP = 288;
constexpr int ROW_H = 66;
constexpr int FILL_H = ROW_H - 6;
constexpr int SIDE = 18;
constexpr int MARK_X = SIDE + 24;
constexpr int TEXT_X = SIDE + 48;
constexpr char READING_FONT[] = "EBGaramond";
constexpr char READING_FONT_DIR[] = "/.fonts/EBGaramond";
constexpr uint8_t READING_POINT_SIZE = 16;
constexpr uint8_t READING_LAYOUT = 2;

// A small quatrefoil: four circles round a centre (the Chorus row mark).
void drawQuatrefoil(const GfxRenderer& renderer, const int cx, const int cy, const bool ink) {
  constexpr int r = 5;
  constexpr int o = 4;
  for (const auto& d : {std::pair{0, -o}, std::pair{o, 0}, std::pair{0, o}, std::pair{-o, 0}}) {
    renderer.drawRoundedRect(cx + d.first - r, cy + d.second - r, 2 * r + 1, 2 * r + 1, 1, r, ink);
  }
}
}  // namespace

TabulaHomeActivity::TabulaHomeActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("Home", renderer, mappedInput) {}

// The golden card ships EB Garamond; make it the reading face the first time
// Tabula starts, with left-aligned lines for the Latin/English pairs.
void TabulaHomeActivity::applyFirstRunDefaults() {
  if (SETTINGS.sdFontFamilyName[0] == '\0' && Storage.exists(READING_FONT_DIR)) {
    strncpy(SETTINGS.sdFontFamilyName, READING_FONT, sizeof(SETTINGS.sdFontFamilyName) - 1);
    SETTINGS.fontPointSize = READING_POINT_SIZE;
    SETTINGS.paragraphAlignment = CrossPointSettings::LEFT_ALIGN;
    SETTINGS.saveToFile();
    sdFontSystem.markRegistryDirty();
  }
  // Layout 1: Latin and English lines set close and flush left (no first-line
  // indent, which would shift only the Latin line), the gap only between pairs.
  if (SETTINGS.tabulaLayout < 1) {
    SETTINGS.lineSpacing = CrossPointSettings::LINE_SPACING_110;
    SETTINGS.paragraphIndentSpaces = 0;
    SETTINGS.extraParagraphSpacing = 1;
  }
  // Layout 2: the dark sleep screen is the default.
  if (SETTINGS.tabulaLayout < 2) SETTINGS.tabulaSleepDark = 1;
  if (SETTINGS.tabulaLayout < READING_LAYOUT) {
    SETTINGS.tabulaLayout = READING_LAYOUT;
    SETTINGS.saveToFile();
  }
}

int TabulaHomeActivity::labelFontId() const {
  const int id = SETTINGS.getReaderFontId();
  return renderer.getFontMap().count(id) ? id : UI_12_FONT_ID;
}

void TabulaHomeActivity::onEnter() {
  Activity::onEnter();
  refreshCount_ = -1;
  applyFirstRunDefaults();
  {
    // Same lock the text settings take: the render task may be drawing with
    // the resident SD font that ensureLoaded() replaces.
    RenderLock lock;
    sdFontSystem.ensureLoaded(renderer);
  }

  tm local{};
  haveDate_ = tabula::localNow(local);
  today_ = local;
  hour_ = haveDate_ ? tabula::hourIndex(local) : 0;
  headerArt_ = haveDate_ ? tabula::headerArtPath(local) : std::string{};

  continuePath_.clear();
  std::string continueTitle;
  for (const RecentBook& book : RECENT_BOOKS.getBooks()) {
    if (RecentBooksStore::isMissing(book)) continue;
    continuePath_ = book.path;
    continueTitle = book.title;
    break;
  }

  rowCount_ = 0;
  const auto add = [this](Row row, std::string label, std::string subtitle) {
    rows_[rowCount_] = row;
    labels_[rowCount_] = std::move(label);
    subtitles_[rowCount_] = std::move(subtitle);
    rowCount_++;
  };
  char hours[48];
  snprintf(hours, sizeof(hours), tr(STR_TABULA_HOURS_SUB), tabula::hourName(hour_));
  add(Row::Mass, tr(STR_TABULA_MASS_TODAY), tr(STR_TABULA_MASS_TODAY_SUB));
  add(Row::Hours, tr(STR_TABULA_HOURS), hours);
  if (!continuePath_.empty()) {
    char resume[96];
    snprintf(resume, sizeof(resume), tr(STR_TABULA_RESUME), continueTitle.c_str());
    add(Row::Continue, tr(STR_TABULA_CONTINUE), resume);
  }
  add(Row::OtherMasses, tr(STR_TABULA_OTHER_MASSES), tr(STR_TABULA_OTHER_MASSES_SUB));
  add(Row::OrderOfMass, tr(STR_TABULA_ORDER_OF_MASS), tr(STR_TABULA_ORDO_SUB));
  add(Row::Prayers, tr(STR_TABULA_PRAYERS), tr(STR_TABULA_PRAYERS_SUB));
  add(Row::Settings, tr(STR_SETTINGS_TITLE), tr(STR_TABULA_SETTINGS_SUB));

  selected_ = 0;
  status_ = nullptr;
  requestUpdate(true);
}

void TabulaHomeActivity::activate(const Row row) {
  std::unique_ptr<Activity> next;
  switch (row) {
    case Row::Mass: {
      tm local{};
      if (!tabula::localNow(local)) {
        status_ = tr(STR_TABULA_NO_CLOCK);
        requestUpdate();
        return;
      }
      const std::string path = tabula::massPath(local);
      if (path.empty()) {
        status_ = tr(STR_TABULA_NOT_ON_CARD);
        requestUpdate();
        return;
      }
      activityManager.goToReader(path, false, "introit.xhtml");
      return;
    }
    case Row::Continue:
      activityManager.goToReader(continuePath_);
      return;
    case Row::Prayers: {
      const std::string path = tabula::prayersPath();
      if (path.empty()) {
        status_ = tr(STR_TABULA_NOT_ON_CARD);
        requestUpdate();
        return;
      }
      activityManager.goToReader(path);
      return;
    }
    case Row::Hours:
      next = makeUniqueNoThrow<TabulaHoursActivity>(renderer, mappedInput);
      break;
    case Row::OtherMasses:
      next = makeUniqueNoThrow<TabulaOtherMassesActivity>(renderer, mappedInput);
      break;
    case Row::OrderOfMass:
      next = makeUniqueNoThrow<OrderOfMassActivity>(renderer, mappedInput);
      break;
    case Row::Settings:
      next = makeUniqueNoThrow<TabulaSettingsActivity>(renderer, mappedInput);
      break;
  }
  if (next) {
    activityManager.replaceActivity(std::move(next));
  } else {
    LOG_ERR("TABULA", "OOM: Tabula screen");
  }
}

void TabulaHomeActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    activate(rows_[selected_]);
    return;
  }
  buttonNavigator_.onNext([this] {
    selected_ = (selected_ + 1) % rowCount_;
    requestUpdate();
  });
  buttonNavigator_.onPrevious([this] {
    selected_ = (selected_ + rowCount_ - 1) % rowCount_;
    requestUpdate();
  });
}

void TabulaHomeActivity::render(RenderLock&&) {
  const int width = renderer.getScreenWidth();
  const int height = renderer.getScreenHeight();
  renderer.clearScreen();

  bool drewArt = false;
  HalFile file;
  if (!headerArt_.empty() && Storage.openFileForRead("TAB", headerArt_, file)) {
    Bitmap art(file, false);
    if (art.parseHeaders() == BmpReaderError::Ok) drewArt = renderer.drawBitmap(art, 0, 0, width, HEAD_H);
    file.close();
  }
  if (!drewArt) {
    // Same Chorus header as the artwork, drawn here so Today is never plain.
    char date[24] = "";
    char bar[48] = "HODIE";
    if (haveDate_) {
      tabula::shortDate(today_, date, sizeof(date));
      snprintf(bar, sizeof(bar), "HODIE · %s", tabula::latinWeekday(today_.tm_wday));
    }
    tabula::drawChorusHeader(renderer, width, bar);
    if (haveDate_) renderer.drawCenteredText(SMALL_FONT_ID, 192, date, true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(UI_12_FONT_ID, 222, "TABULA", true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(SMALL_FONT_ID, 258, "MISSALE · BREVIARIVM");
  }

  const int labelFont = labelFontId();
  for (int i = 0; i < rowCount_; i++) {
    const int y = ROWS_TOP + i * ROW_H;
    const bool isSelected = i == selected_;
    if (isSelected) renderer.fillRect(SIDE, y, width - 2 * SIDE, FILL_H, true);
    const bool ink = !isSelected;
    drawQuatrefoil(renderer, MARK_X, y + FILL_H / 2, ink);
    // Label sits high so its descenders clear the subtitle below.
    renderer.drawText(labelFont, TEXT_X, y - 7, labels_[i].c_str(), ink);
    const std::string sub =
        renderer.truncatedText(SMALL_FONT_ID, subtitles_[i].c_str(), width - TEXT_X - SIDE - 8, EpdFontFamily::ITALIC);
    renderer.drawText(SMALL_FONT_ID, TEXT_X, y + FILL_H - 17, sub.c_str(), ink, EpdFontFamily::ITALIC);
  }

  if (status_) {
    renderer.drawCenteredText(UI_10_FONT_ID, ROWS_TOP - 22, status_);
  }

  const auto labels = mappedInput.mapLabels("", tr(STR_OPEN), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(static_cast<HalDisplay::RefreshMode>(tabula::nextRefreshMode(refreshCount_)));
}
