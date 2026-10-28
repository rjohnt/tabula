#pragma once

#include "TabulaDay.h"
#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

// The eight Hours of today's office under an arc that marks the Hour now
// (filled before, hollow after, a sun on the current one). Confirm opens the
// day's Hours book at that Hour's chapter.
class TabulaHoursActivity final : public Activity {
 public:
  explicit TabulaHoursActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&& lock) override;

 private:
  void openSelected();

  int selected_ = 0;
  int now_ = -1;  // -1 when the clock is unset
  const char* status_ = nullptr;
  ButtonNavigator buttonNavigator_;
  int refreshCount_ = -1;  // see tabula::nextRefreshMode
};
