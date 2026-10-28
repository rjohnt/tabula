#pragma once
#include <ctime>
#include <string>

// Today's date and Hour, and the paths of the day's books on the golden card
// (docs/tabula/DESIGN.md §2). The liturgical calendar is resolved off-device;
// the firmware only maps a date to file names.
namespace tabula {

constexpr int HOUR_COUNT = 8;

// Local date from the trusted clock. False when the clock has never been set.
bool localNow(tm& out);

// Index 0..7 (Matins..Compline) of the Hour for a local time.
int hourIndex(const tm& date);
const char* hourId(int index);       // chapter file stem in the day's Hours book
const char* hourLatin(int index);    // "Vesperæ"
const char* hourEnglish(int index);  // "Vespers"

// "YYYY-MM-DD" for a local date.
std::string dateStem(const tm& date);

// The day's Mass and Hours books in the reader's chosen edition. Falls back to
// the bilingual edition when the chosen one is not on the card. Empty when the
// day is missing entirely.
std::string massPath(const tm& date);
std::string hoursPath(const tm& date);
// Day artwork drawn off-device: Today's header and the sleep screen.
std::string headerArtPath(const tm& date);
std::string sleepArtPath(const tm& date);

// True for a dated Mass or Hours book from a day other than today, which boot
// should not reopen (the reader would wake into yesterday's texts).
bool isStaleDailyBook(const std::string& path);

}  // namespace tabula
