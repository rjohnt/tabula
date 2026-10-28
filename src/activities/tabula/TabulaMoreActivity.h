#pragma once

#include "activities/UiListActivity.h"

// "Plura": the second level under Today. Order of Mass, the Hours, Settings,
// and (development builds only) the CrossPoint library.
class TabulaMoreActivity final : public UiListActivity {
 public:
  explicit TabulaMoreActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;

 private:
  enum class Row : uint8_t { OrderOfMass, Hours, Settings, Library };
  static constexpr int MAX_ROWS = 4;

  int listCount() const override { return rowCount_; }
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  const char* headerTitle() const override;
  void onBackButton() override;

  Row rows_[MAX_ROWS]{};
  freeink::ui::ListItem rowItems_[MAX_ROWS]{};
  int rowCount_ = 0;
};
