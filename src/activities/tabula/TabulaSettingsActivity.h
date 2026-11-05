#pragma once
#include <string>

#include "activities/UiListActivity.h"

// Tabula's whole settings screen: text size, which language the day's texts
// open in, propers or the full Mass, light or dark sleep screen, Wi-Fi, About. Development builds add
// a row into the full CrossPoint settings for service work.
class TabulaSettingsActivity final : public UiListActivity {
 public:
  explicit TabulaSettingsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;

 private:
  enum class Row : uint8_t {
    TextSize,
    Language,
    ReadingText,
    MassText,
    SleepScreen,
    Wifi,
    DateTime,
    About,
    Library,
    AllSettings
  };
  static constexpr int MAX_ROWS = 10;

  int listCount() const override { return rowCount_; }
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  const char* headerTitle() const override;
  void onBackButton() override;

  void refreshSubtitles();
  void buildRows();

  Row rows_[MAX_ROWS]{};
  freeink::ui::ListItem rowItems_[MAX_ROWS]{};
  std::string textSize_;
  int rowCount_ = 0;
};
