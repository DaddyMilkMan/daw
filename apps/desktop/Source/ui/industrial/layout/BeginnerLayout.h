#pragma once

#include <string>
#include <vector>

namespace zenith::industrial {

struct ControlSpec {
  enum class Kind {
    Knob,
    Slider,
    Button,
    Toggle,
    LED,
    Dropdown,
    ModMatrix,
    Visualizer,
    Panel,
    Screw
  };

  Kind kind = Kind::Knob;
  std::string section;
  std::string label;
  std::string unit;
  std::string paramID;  // Audio parameter ID for binding
  float value = 0.5f;
};

class BeginnerLayout {
public:
  static std::vector<ControlSpec> build();
};

} // namespace zenith::industrial
