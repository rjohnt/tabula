#include "TabulaDay.h"

#include <HalStorage.h>
#include <TrustedTime.h>

#include <cstdio>
#include <cstring>

#include "CrossPointSettings.h"
#include "I18nKeys.h"

namespace tabula {
namespace {
constexpr char TODAY_DIR[] = "/Tabula/Today";
constexpr char HOURS_DIR[] = "/Tabula/Hours";
constexpr char SLEEP_DIR[] = "/Tabula/Sleep";
constexpr char PRAYERS_BOOK[] = "/Tabula/Prayers/Orationes";
constexpr char CALENDAR[] = "/Tabula/calendar.tsv";

constexpr const char* HOUR_IDS[HOUR_COUNT] = {"matins", "lauds", "prime",   "terce",
                                              "sext",   "none",  "vespers", "compline"};
constexpr const char* HOUR_LA[HOUR_COUNT] = {"Matutinum", "Laudes", "Prima",   "Tertia",
                                             "Sexta",     "Nona",   "Vesperæ", "Completorium"};
constexpr const char* HOUR_EN[HOUR_COUNT] = {"Matins", "Lauds", "Prime",   "Terce",
                                             "Sext",   "None",  "Vespers", "Compline"};
constexpr const char* HOUR_ES[HOUR_COUNT] = {"Maitines", "Laudes", "Prima",    "Tercia",
                                             "Sexta",    "Nona",   "Vísperas", "Completas"};
// First local hour of each Hour after Matins (Roman 1962 reminder times).
constexpr int HOUR_STARTS[HOUR_COUNT - 1] = {6, 7, 9, 12, 15, 17, 20};

// Edition suffixes written by docs/tabula/tools/build_sd_content.py: "" is
// Latin with English, ".laes" Latin with Spanish.
const char* textSuffix() {
  const bool es = spanish();
  switch (SETTINGS.tabulaText) {
    case CrossPointSettings::TABULA_TEXT_LATIN:
      return ".la";
    case CrossPointSettings::TABULA_TEXT_ENGLISH:
      return es ? ".es" : ".en";
    default:
      return es ? ".laes" : "";
  }
}

// <base><suffix>.epub; for Spanish, the English edition of the same layout when the
// card has no Spanish one; else the Latin and English <base>.epub. Empty when none is on the card.
std::string editionOf(const std::string& base) {
  const char* suffix = textSuffix();
  std::string path = base + suffix + ".epub";
  if (Storage.exists(path.c_str())) return path;
  if (strcmp(suffix, ".es") == 0) {
    path = base + ".en.epub";
    if (Storage.exists(path.c_str())) return path;
  }
  path = base + ".epub";
  return Storage.exists(path.c_str()) ? path : std::string{};
}

std::string editionPath(const char* dir, const tm& local, const char* kind) {
  return editionOf(std::string(dir) + "/" + dateStem(local) + "." + kind);
}
}  // namespace

bool spanish() { return SETTINGS.language == static_cast<uint8_t>(Language::ES); }

bool localNow(tm& out) {
  const time_t now = static_cast<time_t>(trustedtime::trustedNow());
  if (now == 0) return false;
  localtime_r(&now, &out);
  return true;
}

int hourIndex(const tm& local) {
  int index = 0;
  for (int i = 0; i < HOUR_COUNT - 1; i++) {
    if (local.tm_hour >= HOUR_STARTS[i]) index = i + 1;
  }
  return index;
}

const char* hourId(const int index) { return HOUR_IDS[index]; }
const char* hourLatin(const int index) { return HOUR_LA[index]; }
const char* hourEnglish(const int index) { return HOUR_EN[index]; }
const char* hourName(const int index) { return spanish() ? HOUR_ES[index] : HOUR_EN[index]; }

std::string dateStem(const tm& local) {
  char stem[12];
  snprintf(stem, sizeof(stem), "%04d-%02d-%02d", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
  return stem;
}

std::string massPath(const tm& local) {
  const bool full = SETTINGS.tabulaMass == CrossPointSettings::TABULA_MASS_FULL;
  return editionPath(TODAY_DIR, local, full ? "full" : "propers");
}

std::string propersPath(const tm& local) { return editionPath(TODAY_DIR, local, "propers"); }

std::string prayersPath() { return editionOf(PRAYERS_BOOK); }

// calendar.tsv: one "YYYY-MM-DD<TAB>feast" line per day, in date order.
std::string feastName(const tm& local) {
  HalFile file;
  if (!Storage.openFileForRead("TAB", CALENDAR, file)) return {};
  const std::string stem = dateStem(local);
  char buf[256];
  std::string line;
  std::string feast;
  bool found = false;
  int n;
  while (!found && (n = file.read(buf, sizeof(buf))) > 0) {
    for (int i = 0; i < n && !found; i++) {
      if (buf[i] != '\n') {
        if (line.size() < 160) line += buf[i];
        continue;
      }
      if (line.compare(0, stem.size(), stem) == 0 && line.size() > stem.size() && line[stem.size()] == '\t') {
        feast = line.substr(stem.size() + 1);
        if (!feast.empty() && feast.back() == '\r') feast.pop_back();
        found = true;
      }
      line.clear();
    }
  }
  file.close();
  return feast;
}

std::string hoursPath(const tm& local) { return editionPath(HOURS_DIR, local, "hours"); }

std::string headerArtPath(const tm& local) { return std::string(TODAY_DIR) + "/" + dateStem(local) + ".head.bmp"; }

std::string sleepArtPath(const tm& local) { return std::string(SLEEP_DIR) + "/" + dateStem(local) + ".bmp"; }

bool isStaleDailyBook(const std::string& path) {
  const char* stem = nullptr;
  for (const char* dir : {TODAY_DIR, HOURS_DIR}) {
    const size_t len = strlen(dir);
    if (path.compare(0, len, dir) == 0 && path.size() > len + 10 && path[len] == '/') stem = path.c_str() + len + 1;
  }
  if (!stem) return false;
  tm local{};
  if (!localNow(local)) return false;
  return strncmp(stem, dateStem(local).c_str(), 10) != 0;
}

}  // namespace tabula
