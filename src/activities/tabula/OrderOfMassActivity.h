#pragma once
#include <string>
#include <vector>

#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

// The Mass as a vertical sequence (docs/tabula/DESIGN.md §5a): a rail with a
// filled dot for each proper and a hollow dot for each part of the Ordinary,
// the posture at right. Steps come from /Tabula/Missal/ordo.json so the parish
// can fix or reorder them without firmware. Confirm opens the step's text:
// propers in today's file, the Ordinary in Ordo-Missae.epub, both at <id>.xhtml.
class OrderOfMassActivity final : public Activity {
 public:
  explicit OrderOfMassActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void loop() override;
  void render(RenderLock&& lock) override;

 private:
  struct Step {
    std::string id;     // also the chapter file name in the EPUB
    std::string latin;  // section title when isSection
    std::string english;
    std::string posture;  // upper-cased for display
    bool proper = false;
    bool isSection = false;
  };
  // ordo.json is a few KB; the cap bounds the heap if a card carries a bad file.
  static constexpr size_t MAX_STEPS = 48;

  bool loadOrdo();
  void moveSelection(int delta);
  void openSelected();
  // First step index on the page that holds `index`, given the screen band.
  int pageStartFor(int index) const;
  int stepHeight(const Step& step) const;

  std::vector<Step> steps;
  int selected = 0;
  bool loaded = false;
  ButtonNavigator buttonNavigator;
  int refreshCount_ = -1;  // see tabula::nextRefreshMode
};
