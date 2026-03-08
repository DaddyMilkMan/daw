#include "ExpertLayout.h"
#include "AdvancedLayout.h"

namespace zenith::industrial {

std::vector<ControlSpec> ExpertLayout::build() {
  auto controls = AdvancedLayout::build();
  controls.reserve(128);

  controls.push_back({ControlSpec::Kind::Panel, "UNISON", "UNISON", "", "", 0.0f});
  for (const auto& label : {"VOICES", "DETUNE", "SPREAD", "PHASE", "DRIFT"}) {
    controls.push_back({ControlSpec::Kind::Knob, "UNISON", label, "%", "", 0.5f});
  }

  controls.push_back({ControlSpec::Kind::Panel, "OSC DEEP", "OSC DEEP", "", "", 0.0f});
  for (const auto& label : {"PHASE", "DRIFT", "SATURATION", "FM", "RING"}) {
    controls.push_back({ControlSpec::Kind::Knob, "OSC DEEP", label, "%", "", 0.5f});
  }

  controls.push_back({ControlSpec::Kind::Panel, "LFO ADV", "LFO ADV", "", "", 0.0f});
  for (const auto& label : {"SHAPE MOD", "SHUFFLE", "SKEW", "FADE", "RETRIG"}) {
    controls.push_back({ControlSpec::Kind::Knob, "LFO ADV", label, "%", "", 0.5f});
  }

  controls.push_back({ControlSpec::Kind::Panel, "CURVES", "CURVES", "", "", 0.0f});
  for (const auto& label : {"ENV1 CURVE", "ENV2 CURVE", "ENV3 CURVE"}) {
    controls.push_back({ControlSpec::Kind::Dropdown, "CURVES", label, "", "", 0.0f});
  }

  for (int i = 0; i < 16; ++i) {
    controls.push_back({ControlSpec::Kind::Screw, "DECOR", "SCREW", "", "", 0.0f});
  }

  return controls;
}

} // namespace zenith::industrial
