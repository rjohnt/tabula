#include "TabulaDay.h"

#include <HalStorage.h>
#include <TrustedTime.h>

#include <cstdio>
#include <cstring>

#include "CrossPointSettings.h"

namespace tabula {
namespace {
constexpr char TODAY_DIR[] = "/Tabula/Today";
constexpr char HOURS_DIR[] = "/Tabula/Hours";
constexpr char SLEEP_DIR[] = "/Tabula/Sleep";

constexpr const char* HOUR_IDS[HOUR_COUNT] = {"matins", "lauds", "prime",   "terce",
                                              "sext",   "none",  "vespers", "compline"};
constexpr const char* HOUR_LA[HOUR_COUNT] = {"Matutinum", "Laudes", "Prima",   "Tertia",
                                             "Sexta",     "Nona",   "Vesperæ", "Completorium"};
constexpr const char* HOUR_EN[HOUR_COUNT] = {"Matins", "Lauds", "Prime",   "Terce",
                                             "Sext",   "None",  "Vespers", "Compline"};
// First local hour of each Hour after Matins (Roman 1962 reminder times).
constexpr int HOUR_STARTS[HOUR_COUNT - 1] = {6, 7, 9, 12, 15, 17, 20};

const char* textSuffix() {
  switch (SETTINGS.tabulaText) {
    case CrossPointSettings::TABULA_TEXT_LATIN:
      return ".la";
    case CrossPointSettings::TABULA_TEXT_ENGLISH:
      return ".en";
    default:
      return "";
  }
}

// <dir>/<date>.<kind><suffix>.epub, else the bilingual <dir>/<date>.<kind>.epub.
std::string editionPath(const char* dir, const tm& local, const char* kind) {
  char path[64];
  const std::string stem = dateStem(local);
  snprintf(path, sizeof(path), "%s/%s.%s%s.epub", dir, stem.c_str(), kind, textSuffix());
  if (Storage.exists(path)) return path;
  snprintf(path, sizeof(path), "%s/%s.%s.epub", dir, stem.c_str(), kind);
  if (Storage.exists(path)) return path;
  return {};
}
}  // namespace

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

std::string dateStem(const tm& local) {
  char stem[12];
  snprintf(stem, sizeof(stem), "%04d-%02d-%02d", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
  return stem;
}

std::string massPath(const tm& local) {
  const bool full = SETTINGS.tabulaMass == CrossPointSettings::TABULA_MASS_FULL;
  return editionPath(TODAY_DIR, local, full ? "full" : "propers");
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
