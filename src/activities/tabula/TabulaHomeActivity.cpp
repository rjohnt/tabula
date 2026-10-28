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
#include "RecentBooksStore.h"
#include "SdCardFontSystem.h"
#include "TabulaArc.h"
#include "TabulaDay.h"
#include "TabulaMoreActivity.h"
#include "activities/ActivityManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
// The header art is drawn at this size by docs/tabula/tools/render_tabula_art.py.
constexpr int HEAD_H = 300;
constexpr int ROWS_TOP = 316;
constexpr int ROW_H = 74;
constexpr int SIDE = 18;
constexpr int MARK_X = SIDE + 24;
constexpr int TEXT_X = SIDE + 48;
constexpr char READING_FONT[] = "EBGaramond";
constexpr char READING_FONT_DIR[] = "/.fonts/EBGaramond";
constexpr uint8_t READING_POINT_SIZE = 16;
constexpr uint8_t READING_LAYOUT = 1;

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
  if (SETTINGS.tabulaLayout < READING_LAYOUT) {
    SETTINGS.lineSpacing = CrossPointSettings::TIGHT;
    SETTINGS.paragraphIndentSpaces = 0;
    SETTINGS.extraParagraphSpacing = 1;
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
  add(Row::Mass, tr(STR_TABULA_MASS_TODAY), tr(STR_TABULA_MASS_TODAY_SUB));
  add(Row::Hour, tabula::hourLatin(hour_), std::string(tabula::hourEnglish(hour_)) + " · " + tr(STR_TABULA_HOUR_NOW));
  if (!continuePath_.empty()) add(Row::Continue, tr(STR_TABULA_CONTINUE), continueTitle);
  add(Row::More, tr(STR_TABULA_MORE), tr(STR_TABULA_MORE_SUB));

  selected_ = 0;
  status_ = nullptr;
  requestUpdate(true);
}

void TabulaHomeActivity::activate(const Row row) {
  tm local{};
  const bool dated = tabula::localNow(local);
  switch (row) {
    case Row::Mass:
    case Row::Hour: {
      if (!dated) {
        status_ = tr(STR_TABULA_NO_CLOCK);
        requestUpdate();
        return;
      }
      const bool mass = row == Row::Mass;
      const std::string path = mass ? tabula::massPath(local) : tabula::hoursPath(local);
      if (path.empty()) {
        status_ = tr(STR_TABULA_NOT_ON_CARD);
        requestUpdate();
        return;
      }
      const std::string href =
          mass ? "introit.xhtml" : std::string(tabula::hourId(tabula::hourIndex(local))) + ".xhtml";
      activityManager.goToReader(path, false, href);
      return;
    }
    case Row::Continue:
      activityManager.goToReader(continuePath_);
      return;
    case Row::More:
      if (auto more = makeUniqueNoThrow<TabulaMoreActivity>(renderer, mappedInput)) {
        activityManager.replaceActivity(std::move(more));
      } else {
        LOG_ERR("TABULA", "OOM: TabulaMoreActivity");
      }
      return;
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
    if (isSelected) renderer.fillRect(SIDE, y, width - 2 * SIDE, ROW_H - 6, true);
    const bool ink = !isSelected;
    drawQuatrefoil(renderer, MARK_X, y + (ROW_H - 6) / 2, ink);
    renderer.drawText(labelFont, TEXT_X, y + 4, labels_[i].c_str(), ink);
    const std::string sub =
        renderer.truncatedText(SMALL_FONT_ID, subtitles_[i].c_str(), width - TEXT_X - SIDE - 8, EpdFontFamily::ITALIC);
    renderer.drawText(SMALL_FONT_ID, TEXT_X, y + ROW_H - 30, sub.c_str(), ink, EpdFontFamily::ITALIC);
  }

  if (status_) {
    renderer.drawCenteredText(UI_10_FONT_ID, ROWS_TOP + MAX_ROWS * ROW_H + 10, status_);
  }

  const auto labels = mappedInput.mapLabels("", tr(STR_OPEN), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(static_cast<HalDisplay::RefreshMode>(tabula::nextRefreshMode(refreshCount_)));
}
