#include "OrderOfMassActivity.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>
#include <PersistableStore.h>
#include <TrustedTime.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "HalDisplay.h"
#include "TabulaArc.h"
#include "activities/ActivityManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr char ORDO_PATH[] = "/Tabula/Missal/ordo.json";
constexpr char ORDINARY_BOOK[] = "/Tabula/Missal/Ordo-Missae.epub";
constexpr char TODAY_DIR[] = "/Tabula/Today";

// Layout, in pixels at the X3/X4 portrait width (matches the 1-bit previews
// rendered by docs/tabula/tools/render_ordo_previews.py).
constexpr int RAIL_X = 44;
constexpr int DOT_R = 8;
constexpr int TEXT_X = RAIL_X + 24;
constexpr int RIGHT_INSET = 26;
constexpr int SECTION_H = 32;
constexpr int STEP_H = 58;
constexpr int LEGEND_H = 30;
}  // namespace

OrderOfMassActivity::OrderOfMassActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("OrderOfMass", renderer, mappedInput) {}

bool OrderOfMassActivity::loadOrdo() {
  steps.clear();
  JsonDocument doc;
  if (!PersistableStoreBase::readDocFromFile(ORDO_PATH, doc)) {
    LOG_ERR("ORDO", "Cannot read %s", ORDO_PATH);
    return false;
  }
  JsonArrayConst arr = doc["steps"].as<JsonArrayConst>();
  steps.reserve(std::min(arr.size(), MAX_STEPS));
  for (JsonObjectConst obj : arr) {
    if (steps.size() >= MAX_STEPS) break;
    Step step;
    if (!obj["section"].isNull()) {
      step.isSection = true;
      step.latin = obj["section"] | "";
      for (char& c : step.latin) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    } else {
      step.id = obj["id"] | "";
      step.latin = obj["la"] | "";
      step.english = obj["en"] | "";
      step.posture = obj["posture"] | "";
      for (char& c : step.posture) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
      step.proper = strcmp(obj["kind"] | "", "proper") == 0;
      if (step.id.empty()) continue;
    }
    steps.push_back(std::move(step));
  }
  return !steps.empty();
}

void OrderOfMassActivity::onEnter() {
  Activity::onEnter();
  refreshCount_ = -1;
  loaded = loadOrdo();
  selected = 0;
  while (loaded && selected < static_cast<int>(steps.size()) && steps[selected].isSection) selected++;
  requestUpdate(true);
}

int OrderOfMassActivity::stepHeight(const Step& step) const { return step.isSection ? SECTION_H : STEP_H; }

int OrderOfMassActivity::pageStartFor(const int index) const {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int top = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int band = renderer.getScreenHeight() - metrics.buttonHintsHeight - LEGEND_H - top;
  int start = 0;
  int used = 0;
  for (int i = 0; i < static_cast<int>(steps.size()); i++) {
    const int h = stepHeight(steps[i]);
    if (used + h > band && used > 0) {
      start = i;
      used = 0;
    }
    if (i == index) return start;
    used += h;
  }
  return start;
}

void OrderOfMassActivity::moveSelection(const int delta) {
  int next = selected;
  do {
    next += delta;
  } while (next >= 0 && next < static_cast<int>(steps.size()) && steps[next].isSection);
  if (next < 0 || next >= static_cast<int>(steps.size())) return;
  selected = next;
  requestUpdate();
}

// Propers live in today's file; the Ordinary in the fixed Ordo Missae book.
// Each part is its own chapter named <id>.xhtml in either book.
void OrderOfMassActivity::openSelected() {
  if (!loaded) return;
  const Step& step = steps[selected];
  const std::string href = step.id + ".xhtml";
  if (step.proper) {
    const time_t now = static_cast<time_t>(trustedtime::trustedNow());
    if (now != 0) {
      tm local{};
      localtime_r(&now, &local);
      char path[48];
      snprintf(path, sizeof(path), "%s/%04d-%02d-%02d.propers.epub", TODAY_DIR, local.tm_year + 1900, local.tm_mon + 1,
               local.tm_mday);
      if (Storage.exists(path)) {
        activityManager.goToReader(path, false, href);
        return;
      }
    }
    activityManager.goToFileBrowser(TODAY_DIR);
    return;
  }
  activityManager.goToReader(ORDINARY_BOOK, false, href);
}

void OrderOfMassActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    activityManager.goHome();
    return;
  }
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    openSelected();
    return;
  }
  buttonNavigator.onNext([this] { moveSelection(+1); });
  buttonNavigator.onPrevious([this] { moveSelection(-1); });
}

void OrderOfMassActivity::render(RenderLock&&) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const int width = renderer.getScreenWidth();
  const int height = renderer.getScreenHeight();
  renderer.clearScreen();
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, width, metrics.headerHeight}, tr(STR_TABULA_ORDER_OF_MASS));

  const int top = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int bottom = height - metrics.buttonHintsHeight - LEGEND_H;

  if (!loaded) {
    renderer.drawCenteredText(UI_10_FONT_ID, height / 2, tr(STR_TABULA_ORDO_MISSING));
  } else {
    renderer.drawLine(RAIL_X, top, RAIL_X, bottom - 8, 2, true);
    const int start = pageStartFor(selected);
    int y = top;
    for (int i = start; i < static_cast<int>(steps.size()); i++) {
      const Step& step = steps[i];
      if (y + stepHeight(step) > bottom) break;
      if (step.isSection) {
        renderer.fillRect(RAIL_X - 6, y + 6, 13, 13, true);
        renderer.drawText(SMALL_FONT_ID, RAIL_X + 20, y + 4, step.latin.c_str());
        y += SECTION_H;
        continue;
      }
      const bool isSelected = i == selected;
      if (isSelected) {
        renderer.fillRoundedRect(RAIL_X + 14, y - 4, width - RAIL_X - 28, STEP_H, 6, Color::Black);
      }
      // Filled dot = today's proper; hollow = Ordinary.
      renderer.fillRoundedRect(RAIL_X - DOT_R, y + 14 - DOT_R, DOT_R * 2 + 1, DOT_R * 2 + 1, DOT_R, Color::Black);
      if (!step.proper) {
        renderer.fillRoundedRect(RAIL_X - DOT_R + 2, y + 16 - DOT_R, DOT_R * 2 - 3, DOT_R * 2 - 3, DOT_R - 2,
                                 Color::White);
      }
      const bool ink = !isSelected;
      renderer.drawText(UI_12_FONT_ID, TEXT_X, y + 2, step.latin.c_str(), ink, EpdFontFamily::BOLD);
      renderer.drawText(UI_10_FONT_ID, TEXT_X, y + 28, step.english.c_str(), ink);
      const int postureW = renderer.getTextWidth(SMALL_FONT_ID, step.posture.c_str());
      renderer.drawText(SMALL_FONT_ID, width - RIGHT_INSET - postureW, y + 6, step.posture.c_str(), ink);
      tabula::drawPosture(renderer, width - RIGHT_INSET - postureW - 30, y + 2, step.posture.c_str(), ink);
      y += STEP_H;
    }

    const int legendY = bottom + 2;
    renderer.fillRoundedRect(20, legendY + 2, 13, 13, 6, Color::Black);
    renderer.drawText(SMALL_FONT_ID, 38, legendY, tr(STR_TABULA_PROPER_LEGEND));
    renderer.fillRoundedRect(170, legendY + 2, 13, 13, 6, Color::Black);
    renderer.fillRoundedRect(172, legendY + 4, 9, 9, 4, Color::White);
    renderer.drawText(SMALL_FONT_ID, 188, legendY, tr(STR_TABULA_ORDINARY_LEGEND));
  }

  const auto labels = mappedInput.mapLabels(tr(STR_HOME), loaded ? tr(STR_OPEN) : "", tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer(static_cast<HalDisplay::RefreshMode>(tabula::nextRefreshMode(refreshCount_)));
}
