#pragma once

#include "../rendering/IndustrialTheme.h"
#include "IndustrialComponent.h"
#include <string>
#include <vector>

namespace zenith::industrial {

class IndustrialDropdown : public IndustrialComponent {
public:
  IndustrialDropdown(std::string label, std::vector<std::string> items,
                     IndustrialTheme &theme);

  void render(SkCanvas *canvas) override;
  void handleMouseMove(const MouseEvent &e) override;
  void handleMouseUp(const MouseEvent &e) override;
  bool hitTest(float x, float y) const override;

private:
  SkRect listBounds() const;

  std::string label_;
  std::vector<std::string> items_;
  int selectedIndex_ = 0;
  int hoveredIndex_ = -1;
  bool open_ = false;
  IndustrialTheme &theme_;
};

} // namespace zenith::industrial
