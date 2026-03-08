#include "AdvancedLayout.h"

namespace zenith::industrial {

std::vector<ControlSpec> AdvancedLayout::build() {
  auto controls = BeginnerLayout::build();
  controls.reserve(64);

  controls.push_back({ControlSpec::Kind::Panel, "OSCILLATORS ADV", "OSCILLATORS ADV", "", "", 0.0f});
  for (const auto& label : {"PWM", "SYNC", "RING", "PHASE", "DRIFT", "SAT"}) {
    controls.push_back({ControlSpec::Kind::Knob, "OSCILLATORS ADV", label, "%", "", 0.5f});
  }

  controls.push_back({ControlSpec::Kind::Panel, "FILTER ADV", "FILTER ADV", "", "", 0.0f});
  for (const auto& label : {"DRIVE", "KEY TRACK", "ENV AMT", "MODEL"}) {
    controls.push_back({ControlSpec::Kind::Knob, "FILTER ADV", label, "%", "", 0.5f});
  }

  controls.push_back({ControlSpec::Kind::Panel, "ENV 2", "ENV 2", "", "", 0.0f});
  controls.push_back({ControlSpec::Kind::Panel, "ENV 3", "ENV 3", "", "", 0.0f});
  for (const auto& envSection : {"ENV 2", "ENV 3"}) {
    for (const auto& label : {"A", "D", "S", "R"}) {
      controls.push_back({ControlSpec::Kind::Knob, envSection, label, "MS", "", 0.5f});
    }
  }

  controls.push_back({ControlSpec::Kind::Panel, "LFOS", "LFOS", "", "", 0.0f});
  for (int lfo = 1; lfo <= 3; ++lfo) {
    controls.push_back({ControlSpec::Kind::Knob, "LFOS", "LFO" + std::to_string(lfo) + " RATE", "HZ", "", 0.4f});
    controls.push_back({ControlSpec::Kind::Knob, "LFOS", "LFO" + std::to_string(lfo) + " AMOUNT", "%", "", 0.5f});
    controls.push_back({ControlSpec::Kind::Toggle, "LFOS", "LFO" + std::to_string(lfo) + " SYNC", "", "", 0.0f});
    controls.push_back({ControlSpec::Kind::Dropdown, "LFOS", "LFO" + std::to_string(lfo) + " SHAPE", "", "", 0.0f});
    controls.push_back({ControlSpec::Kind::LED, "LFOS", "LFO" + std::to_string(lfo) + " LED", "", "", 1.0f});
  }

  controls.push_back({ControlSpec::Kind::Panel, "MACROS", "MACROS", "", "", 0.0f});
  for (int m = 1; m <= 4; ++m) {
    controls.push_back({ControlSpec::Kind::Knob, "MACROS", "MACRO " + std::to_string(m), "%", "", 0.5f});
  }

  controls.push_back({ControlSpec::Kind::ModMatrix, "MODULATION MATRIX", "MODULATION MATRIX", "", "", 0.0f});
  controls.push_back({ControlSpec::Kind::Visualizer, "VISUALIZER", "VISUALIZER", "", "", 0.0f});

  return controls;
}

} // namespace zenith::industrial
