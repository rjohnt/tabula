#pragma once
#include <ctime>

class GfxRenderer;

// Shared Chorus ornaments drawn on the device.
namespace tabula {

// A band of small quatrefoils over a double rule along the top edge (y 0..40).
void drawQuatrefoilBand(const GfxRenderer& renderer, int width);

// The rubric bar (y 0..38): a black band, `text` in white capitals, a cross at
// each end.
void drawRubricBar(const GfxRenderer& renderer, int width, const char* text);

// The Today header drawn on the device when the day's artwork is not on the
// card: rubric bar, a lancet window with a trefoil, and a sill (y 0..180).
void drawChorusHeader(const GfxRenderer& renderer, int width, const char* barText);

// A 24 px figure for a Mass posture ("KNEEL", "STAND", "SIT"; case-insensitive)
// with its top-left at (x, y). Unknown postures draw nothing.
void drawPosture(const GfxRenderer& renderer, int x, int y, const char* posture, bool ink);

// "FERIA IV" for tm_wday.
const char* latinWeekday(int weekday);
// "Oct 28, 2026" for a local date.
void shortDate(const tm& date, char* out, int size);

// The eight Hours on a dotted arc, evenly spaced across 2*rx so no mark touches
// its neighbour: filled diamonds before `now`, hollow after, a sun at `now`.
// `now` < 0 leaves every mark hollow (clock unset).
void drawHourArc(const GfxRenderer& renderer, int cx, int cy, int rx, int ry, int now);

// Refresh mode for a full-screen Tabula page: a clean (half) refresh when the
// page is first drawn and after every few quick ones, so fast-refresh ghosting
// never builds into stripes. Reset `count` to -1 in onEnter().
int nextRefreshMode(int& count);

}  // namespace tabula
