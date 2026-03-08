#pragma once

#include "IndustrialComponent.h"
#include "../rendering/IndustrialTheme.h"
#include <string>
#include <vector>

namespace zenith::industrial {

class ModMatrixTable : public IndustrialComponent {
public:
  struct Row {
    std::string source;
    std::string dest;
    float depth = 0.5f;
  };

  explicit ModMatrixTable(IndustrialTheme& theme);

  void render(SkCanvas* canvas) override;
  void handleMouseDown(const MouseEvent& e) override;
  void handleMouseDrag(const MouseEvent& e) override;
  void handleMouseUp(const MouseEvent& e) override;

private:
  std::vector<Row> rows_;
  int dragRow_ = -1;
  IndustrialTheme& theme_;
};

} // namespace zenith::industrial
