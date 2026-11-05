#pragma once
#include <ctime>
#include <string>

#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

// The coming week's Masses: each day's date over its feast (from
// /Tabula/calendar.tsv). Confirm opens that day's Mass book.
class TabulaOtherMassesActivity final : public Activity {
 public:
  explicit TabulaOtherMassesActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&& lock) override;

 private:
  static constexpr int DAYS = 7;

  void openSelected();

  tm days_[DAYS]{};
  std::string feasts_[DAYS];
  int count_ = 0;
  int selected_ = 0;
  const char* status_ = nullptr;
  ButtonNavigator buttonNavigator_;
  int refreshCount_ = -1;  // see tabula::nextRefreshMode
};
