#include "BeginnerLayout.h"

namespace zenith::industrial {

std::vector<ControlSpec> BeginnerLayout::build() {
  std::vector<ControlSpec> controls;
  controls.reserve(18);

  // OSCILLATORS section
  controls.push_back({ControlSpec::Kind::Panel, "OSCILLATORS", "OSCILLATORS", "", "", 0.0f});
  controls.push_back({ControlSpec::Kind::Knob, "OSCILLATORS", "WAVE", "", "osc1_wave", 0.2f});
  controls.push_back({ControlSpec::Kind::Knob, "OSCILLATORS", "PITCH", "SEMI", "osc1_pitch", 0.5f});
  controls.push_back({ControlSpec::Kind::Knob, "OSCILLATORS", "DETUNE", "CENTS", "osc1_detune", 0.5f});
  controls.push_back({ControlSpec::Kind::Dropdown, "OSCILLATORS", "WAVEFORM", "", "osc1_wave", 0.0f});

  // FILTER section
  controls.push_back({ControlSpec::Kind::Panel, "FILTER", "FILTER", "", "", 0.0f});
  controls.push_back({ControlSpec::Kind::Knob, "FILTER", "CUTOFF", "HZ", "filter_cutoff", 0.65f});
  controls.push_back({ControlSpec::Kind::Knob, "FILTER", "RESONANCE", "%", "filter_resonance", 0.28f});
  controls.push_back({ControlSpec::Kind::Dropdown, "FILTER", "TYPE", "", "filter_type", 0.0f});

  // AMP ENVELOPE section
  controls.push_back({ControlSpec::Kind::Panel, "AMP ENVELOPE", "AMP ENVELOPE", "", "", 0.0f});
  controls.push_back({ControlSpec::Kind::Knob, "AMP ENVELOPE", "A", "MS", "env1_attack", 0.08f});
  controls.push_back({ControlSpec::Kind::Knob, "AMP ENVELOPE", "D", "MS", "env1_decay", 0.35f});
  controls.push_back({ControlSpec::Kind::Knob, "AMP ENVELOPE", "S", "%", "env1_sustain", 0.72f});
  controls.push_back({ControlSpec::Kind::Knob, "AMP ENVELOPE", "R", "MS", "env1_release", 0.25f});

  // MASTER section
  controls.push_back({ControlSpec::Kind::Panel, "MASTER", "MASTER", "", "", 0.0f});
  controls.push_back({ControlSpec::Kind::Knob, "MASTER", "VOLUME", "DB", "master_gain", 0.75f});

  // PRESETS section
  controls.push_back({ControlSpec::Kind::Panel, "PRESETS", "PRESETS", "", "", 0.0f});
  controls.push_back({ControlSpec::Kind::Dropdown, "PRESETS", "INIT", "", "", 0.0f});

  return controls;
}

} // namespace zenith::industrial
