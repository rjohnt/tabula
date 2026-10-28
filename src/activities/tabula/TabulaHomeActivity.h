#pragma once
#include <ctime>
#include <string>

#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

// Tabula home ("Hodie"): the day's feast at the top (artwork drawn off-device,
// /Tabula/Today/<date>.head.bmp) and a few large choices under it: today's Mass,
// the Hour now, Continue, and More. Nothing here shows folders or file names.
class TabulaHomeActivity final : public Activity {
 public:
  explicit TabulaHomeActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&& lock) override;
  bool isHomeActivity() const override { return true; }

 private:
  enum class Row : uint8_t { Mass, Hour, Continue, More };
  static constexpr int MAX_ROWS = 4;

  void applyFirstRunDefaults();
  void activate(Row row);
  int labelFontId() const;

  Row rows_[MAX_ROWS]{};
  std::string labels_[MAX_ROWS];
  std::string subtitles_[MAX_ROWS];
  int rowCount_ = 0;
  int selected_ = 0;
  int hour_ = 0;
  bool haveDate_ = false;
  tm today_{};
  std::string headerArt_;
  std::string continuePath_;
  const char* status_ = nullptr;
  ButtonNavigator buttonNavigator_;
  int refreshCount_ = -1;  // see tabula::nextRefreshMode
};
