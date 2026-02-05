/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    PresetGenerator.cpp
    Created: 2025-11-29


    AI Preset Generation Implementation

  ==============================================================================

*/

#include "PresetGenerator.h"

namespace zenith {

//==============================================================================
// Parameter Validation
//==============================================================================

bool PresetGenerator::isValidWaveform(const juce::String &waveform) {
  static const juce::StringArray validWaveforms = {
      "sine", "saw", "square", "triangle", "noise", "supersaw"};
  return validWaveforms.contains(waveform.toLowerCase());
}

bool PresetGenerator::isValidFilterType(const juce::String &filterType) {
  static const juce::StringArray validTypes = {"lowpass", "bandpass",
                                               "highpass"};
  return validTypes.contains(filterType.toLowerCase());
}

bool PresetGenerator::isValidLFOTarget(const juce::String &target) {
  static const juce::StringArray validTargets = {
      "filter_cutoff", "osc1_pitch", "osc2_pitch", "osc1_mix", "osc2_mix"};
  return validTargets.contains(target.toLowerCase());
}

bool PresetGenerator::isValidModSource(const juce::String &source) {
  static const juce::StringArray validSources = {
      "none", "lfo1",     "lfo2",     "env1",
      "env2", "velocity", "modwheel", "aftertouch"};
  return validSources.contains(source.toLowerCase());
}

bool PresetGenerator::isValidModDestination(const juce::String &destination) {
  static const juce::StringArray validDestinations = {
      "none",       "filter_cutoff", "filter_resonance", "osc1_pitch",
      "osc2_pitch", "osc3_pitch",    "wavetable_pos",    "pan",
      "volume",     "osc1_mix",      "osc2_mix",         "osc3_mix",
      "osc_shape"};
  return validDestinations.contains(destination.toLowerCase());
}

float PresetGenerator::clampFloat(float value, float min, float max) {
  return juce::jlimit(min, max, value);
}

int PresetGenerator::clampInt(int value, int min, int max) {
  return juce::jlimit(min, max, value);
}

//==============================================================================
// Main Validation Function
//==============================================================================

bool PresetGenerator::validatePresetParameters(const juce::String &instrumentId,
                                               const juce::var &parameters) {
  if (!parameters.isObject())
    return false;

  // For now, only validate ZenithPolySynth
  if (instrumentId != "zenith_poly_synth")
    return true; // Unknown instrument, skip validation

  auto *obj = parameters.getDynamicObject();
  if (obj == nullptr)
    return false;

  // Validate oscillator parameters
  for (int i = 1; i <= 3; ++i) {
    juce::String prefix = "osc" + juce::String(i) + "_";

    if (obj->hasProperty(prefix + "waveform")) {
      auto waveform = obj->getProperty(prefix + "waveform").toString();
      if (!isValidWaveform(waveform))
        return false;
    }

    if (obj->hasProperty(prefix + "detune")) {
      float detune = obj->getProperty(prefix + "detune");
      if (detune < -100.0f || detune > 100.0f)
        return false;
    }

    if (obj->hasProperty(prefix + "mix")) {
      float mix = obj->getProperty(prefix + "mix");
      if (mix < 0.0f || mix > 1.0f)
        return false;
    }
  }

  // Validate filter parameters
  if (obj->hasProperty("filter_type")) {
    auto filterType = obj->getProperty("filter_type").toString();
    if (!isValidFilterType(filterType))
      return false;
  }

  if (obj->hasProperty("filter_cutoff")) {
    float cutoff = obj->getProperty("filter_cutoff");
    if (cutoff < 20.0f || cutoff > 20000.0f)
      return false;
  }

  if (obj->hasProperty("filter_resonance")) {
    float resonance = obj->getProperty("filter_resonance");
    if (resonance < 0.0f || resonance > 1.0f)
      return false;
  }

  // Validate envelope parameters
  const juce::StringArray envPrefixes = {"amp_", "mod_"};
  for (const auto &prefix : envPrefixes) {
    if (obj->hasProperty(prefix + "attack")) {
      float attack = obj->getProperty(prefix + "attack");
      if (attack < 0.001f || attack > 5.0f)
        return false;
    }

    if (obj->hasProperty(prefix + "decay")) {
      float decay = obj->getProperty(prefix + "decay");
      if (decay < 0.001f || decay > 5.0f)
        return false;
    }

    if (obj->hasProperty(prefix + "sustain")) {
      float sustain = obj->getProperty(prefix + "sustain");
      if (sustain < 0.0f || sustain > 1.0f)
        return false;
    }

    if (obj->hasProperty(prefix + "release")) {
      float release = obj->getProperty(prefix + "release");
      if (release < 0.001f || release > 10.0f)
        return false;
    }
  }

  // All validations passed
  return true;
}

//==============================================================================
// Parameter Schema
//==============================================================================

juce::var
PresetGenerator::getParameterSchema(const juce::String &instrumentId) {
  if (instrumentId != "zenith_poly_synth")
    return juce::var(); // Unknown instrument

  // Return comprehensive schema (simplified version)
  auto *schema = new juce::DynamicObject();

  // Oscillators
  auto *osc = new juce::DynamicObject();
  osc->setProperty("waveform", "sine|saw|square|triangle|noise|supersaw");
  osc->setProperty("detune", "-100.0 to 100.0 cents");
  osc->setProperty("mix", "0.0 to 1.0");
  schema->setProperty("oscillator_template", juce::var(osc));

  // Filter
  auto *filter = new juce::DynamicObject();
  filter->setProperty("type", "lowpass|bandpass|highpass");
  filter->setProperty("cutoff", "20.0 to 20000.0 Hz");
  filter->setProperty("resonance", "0.0 to 1.0");
  filter->setProperty("drive", "0.0 to 10.0");
  schema->setProperty("filter", juce::var(filter));

  // Envelope
  auto *env = new juce::DynamicObject();
  env->setProperty("attack", "0.001 to 5.0 seconds");
  env->setProperty("decay", "0.001 to 5.0 seconds");
  env->setProperty("sustain", "0.0 to 1.0");
  env->setProperty("release", "0.001 to 10.0 seconds");
  schema->setProperty("envelope_template", juce::var(env));

  return juce::var(schema);
}

//==============================================================================
// Sound Type Templates
//==============================================================================

juce::var PresetGenerator::generateTemplate(const juce::String &soundType) {
  auto *params = new juce::DynamicObject();

  juce::String type = soundType.toLowerCase();

  if (type.contains("pad")) {
    // Warm pad template
    params->setProperty("osc1_waveform", "saw");
    params->setProperty("osc1_mix", 0.7);
    params->setProperty("osc2_waveform", "sine");
    params->setProperty("osc2_detune", 7.0);
    params->setProperty("osc2_mix", 0.3);
    params->setProperty("filter_type", "lowpass");
    params->setProperty("filter_cutoff", 800.0);
    params->setProperty("filter_resonance", 0.3);
    params->setProperty("amp_attack", 1.5);
    params->setProperty("amp_decay", 0.5);
    params->setProperty("amp_sustain", 0.8);
    params->setProperty("amp_release", 2.0);
    params->setProperty("lfo1_rate", 0.3);
    params->setProperty("lfo1_amount", 0.2);
    params->setProperty("lfo1_target", "filter_cutoff");
  } else if (type.contains("bass")) {
    // Bass template
    params->setProperty("osc1_waveform", "saw");
    params->setProperty("osc1_mix", 1.0);
    params->setProperty("filter_type", "lowpass");
    params->setProperty("filter_cutoff", 400.0);
    params->setProperty("filter_resonance", 0.5);
    params->setProperty("filter_drive", 2.0);
    params->setProperty("amp_attack", 0.001);
    params->setProperty("amp_decay", 0.2);
    params->setProperty("amp_sustain", 0.7);
    params->setProperty("amp_release", 0.3);
  } else if (type.contains("lead")) {
    // Lead template
    params->setProperty("osc1_waveform", "saw");
    params->setProperty("osc1_mix", 0.8);
    params->setProperty("osc2_waveform", "square");
    params->setProperty("osc2_detune", 12.0);
    params->setProperty("osc2_mix", 0.2);
    params->setProperty("filter_type", "lowpass");
    params->setProperty("filter_cutoff", 2000.0);
    params->setProperty("filter_resonance", 0.6);
    params->setProperty("amp_attack", 0.01);
    params->setProperty("amp_decay", 0.3);
    params->setProperty("amp_sustain", 0.6);
    params->setProperty("amp_release", 0.5);
    params->setProperty("lfo1_rate", 5.0);
    params->setProperty("lfo1_amount", 0.3);
    params->setProperty("lfo1_target", "filter_cutoff");
  } else if (type.contains("pluck")) {
    // Pluck template
    params->setProperty("osc1_waveform", "triangle");
    params->setProperty("osc1_mix", 1.0);
    params->setProperty("filter_type", "highpass");
    params->setProperty("filter_cutoff", 500.0);
    params->setProperty("filter_resonance", 0.2);
    params->setProperty("amp_attack", 0.001);
    params->setProperty("amp_decay", 0.1);
    params->setProperty("amp_sustain", 0.1);
    params->setProperty("amp_release", 0.1);
  } else {
    // Default template
    params->setProperty("osc1_waveform", "saw");
    params->setProperty("osc1_mix", 1.0);
    params->setProperty("filter_type", "lowpass");
    params->setProperty("filter_cutoff", 1000.0);
    params->setProperty("filter_resonance", 0.3);
    params->setProperty("amp_attack", 0.01);
    params->setProperty("amp_decay", 0.3);
    params->setProperty("amp_sustain", 0.7);
    params->setProperty("amp_release", 0.5);
  }

  return juce::var(params);
}

//==============================================================================
// Sound Type Detection
//==============================================================================

juce::String PresetGenerator::detectSoundType(const juce::String &description) {
  juce::String lower = description.toLowerCase();

  if (lower.contains("pad") || lower.contains("ambient") ||
      lower.contains("atmosphere"))
    return "pad";
  else if (lower.contains("bass") || lower.contains("sub"))
    return "bass";
  else if (lower.contains("lead") || lower.contains("solo"))
    return "lead";
  else if (lower.contains("pluck") || lower.contains("stab"))
    return "pluck";
  else if (lower.contains("arp") || lower.contains("sequence"))
    return "arp";
  else
    return "general";
}

//==============================================================================
// Parameter Clamping
//==============================================================================

juce::var PresetGenerator::clampParameter(const juce::String &paramName,
                                          const juce::var &value,
                                          const juce::String &instrumentId) {
  if (instrumentId != "zenith_poly_synth")
    return value; // Unknown instrument, don't clamp

  // Oscillator parameters
  if (paramName.endsWith("_detune"))
    return clampFloat(value, -100.0f, 100.0f);
  else if (paramName.endsWith("_mix"))
    return clampFloat(value, 0.0f, 1.0f);

  // Filter parameters
  else if (paramName == "filter_cutoff" || paramName == "filter2_cutoff")
    return clampFloat(value, 20.0f, 20000.0f);
  else if (paramName.contains("resonance"))
    return clampFloat(value, 0.0f, 1.0f);
  else if (paramName.contains("drive"))
    return clampFloat(value, 0.0f, 10.0f);

  // Envelope parameters
  else if (paramName.endsWith("_attack") || paramName.endsWith("_decay"))
    return clampFloat(value, 0.001f, 5.0f);
  else if (paramName.endsWith("_sustain"))
    return clampFloat(value, 0.0f, 1.0f);
  else if (paramName.endsWith("_release"))
    return clampFloat(value, 0.001f, 10.0f);

  // LFO parameters
  else if (paramName.contains("lfo") && paramName.endsWith("_rate"))
    return clampFloat(value, 0.1f, 20.0f);
  else if (paramName.contains("lfo") && paramName.endsWith("_amount"))
    return clampFloat(value, 0.0f, 1.0f);

  // Unison
  else if (paramName == "unison_voices")
    return clampInt(value, 1, 7);
  else if (paramName == "unison_detune")
    return clampFloat(value, 0.0f, 100.0f);

  // Effects
  else if (paramName == "distortion" || paramName == "chorus")
    return clampFloat(value, 0.0f, 1.0f);

  // Global
  else if (paramName == "glide_time")
    return clampFloat(value, 0.0f, 2.0f);
  else if (paramName == "master_gain")
    return clampFloat(value, 0.0f, 2.0f);

  // Default: return as-is
  return value;
}

//==============================================================================
// Complete Preset Creation
//==============================================================================

ZenithInstrumentPreset PresetGenerator::createPresetFromParameters(
    const juce::String &instrumentId, const juce::String &presetName,
    const juce::String &description, const juce::var &parameters,
    const juce::String &genre) {
  auto preset = ZenithInstrumentPreset::createNew(
      presetName.isEmpty() ? "AI Generated" : presetName.toStdString(),
      instrumentId.toStdString(),
      "Grok AI");

  preset.category = "AI Generated";
  preset.description = description.toStdString();
  // preset.tags = ... ZenithInstrumentPreset uses vector<string> for tags
  if (genre.isNotEmpty())
    preset.tags.push_back(genre.toStdString());
  else
    preset.tags.push_back(detectSoundType(description).toStdString());

  // Clamp all parameters to valid ranges
  // ZenithInstrumentPreset uses std::map<std::string, float> for parameters

  if (parameters.isObject()) {
    auto *sourceObj = parameters.getDynamicObject();
    if (sourceObj != nullptr) {
      auto &properties = sourceObj->getProperties();
      for (int i = 0; i < properties.size(); ++i) {
        auto paramName = properties.getName(i).toString();
        auto paramValue = properties.getValueAt(i);

        // Clamp to valid range
        auto clampedValue = clampParameter(paramName, paramValue, instrumentId);

        // Convert to float and store
        preset.parameters[paramName.toStdString()] = (float)clampedValue;
      }
    }
  }

  return preset;
}

//==============================================================================
// Mutation Logic
//==============================================================================

static bool isDiscreteParameter(const juce::String &name) {
  return name.contains("waveform") || name.contains("type") ||
         name.contains("target") || name.contains("source") ||
         name.contains("destination") ||
         // Shape often continuous, but sometimes treated as distinct if it
         // switches modes. In Zenith, osc_shape is pulse width usually
         // (continuous).
         name.contains("quality") ||
         name.contains("voices"); // handle unison_voices
}

static float getParameterRange(const juce::String &name) {
  if (name.contains("cutoff"))
    return 19980.0f; // 20-20000
  if (name.contains("detune"))
    return 200.0f; // +/- 100
  if (name.contains("rate"))
    return 19.9f; // 0.1 - 20
  if (name.contains("release"))
    return 10.0f;
  if (name.contains("attack") || name.contains("decay"))
    return 5.0f;
  if (name.contains("drive"))
    return 10.0f;
  if (name.contains("glide"))
    return 2.0f;
  if (name.contains("gain"))
    return 2.0f; // Master gain
  return 1.0f;   // Default 0-1
}

void PresetGenerator::mutatePreset(ZenithInstrumentPreset &preset,
                                   float mutationAmount) {
  juce::Random random;

  // Iterate over parameters
  for (auto &[nameStr, value] : preset.parameters) {
    juce::String name(nameStr);

    // Skip some parameters occasionally to preserve character
    if (random.nextFloat() > 0.7f + (mutationAmount * 0.2f))
      continue;

    if (isDiscreteParameter(name)) {
      // Discrete Parameter Logic
      // Only mutate if mutationAmount is high enough to warrant a mode change
      if (random.nextFloat() < mutationAmount) {
        int currentVal = static_cast<int>(std::round(value));
        int maxVal = 1; // Default

        // Determine ranges for specific discrete params
        if (name.contains("waveform"))
          maxVal = 5; // 0-5 (Sine..Supersaw)
        else if (name.contains("filter_type"))
          maxVal = 2; // 0-2
        else if (name.contains("target"))
          maxVal = 5; // LFOTarget
        else if (name.contains("source"))
          maxVal = 7; // ModSource
        else if (name.contains("destination"))
          maxVal = 10; // ModDest

        // Random change: +/- 1 or random jump
        if (random.nextBool()) {
          currentVal = currentVal + (random.nextBool() ? 1 : -1);
        } else {
          currentVal = random.nextInt(maxVal + 1);
        }

        // Wrap/Clamp
        if (currentVal < 0)
          currentVal = maxVal;
        if (currentVal > maxVal)
          currentVal = 0;

        value = static_cast<float>(currentVal);
      }
    } else {
      // Continuous Parameter Logic
      float range = getParameterRange(name);

      // Scaled mutation
      // Use cubic distribution for finer control (more small changes)
      float r = random.nextFloat() * 2.0f - 1.0f;
      float drift = r * r * r * mutationAmount * range * 0.5f;

      value += drift;

      // Clamp using existing helper logic via temporary var
      // We reuse clampParameter by passing the instrumentID explicitly
      // Assuming "zenith_poly_synth" since that's what we support

      // But we need juce::var handling for clampParameter.
      // Let's do a direct clamp here for speed and simplicity or map back.
      // We can use the logic from clampParameter essentially:

      juce::var clamped = clampParameter(name, value, "zenith_poly_synth");
      value = static_cast<float>(clamped);
    }
  }

  // Mutate Name slightly to indicate change?
  // preset.name += " (M)";
}

void PresetGenerator::mutatePreset(Preset &preset, float mutationAmount) {
  juce::Random random;

  // Iterate over parameters
  for (auto &[name, value] : preset.parameters) {
    // Skip some parameters occasionally to preserve character
    if (random.nextFloat() > 0.7f + (mutationAmount * 0.2f))
      continue;

    if (isDiscreteParameter(name)) {
      if (random.nextFloat() < mutationAmount) {
        int currentVal = static_cast<int>(std::round(value));
        int maxVal = 1;

        if (name.contains("waveform"))
          maxVal = 5;
        else if (name.contains("filter_type"))
          maxVal = 2;
        else if (name.contains("target"))
          maxVal = 5;
        else if (name.contains("source"))
          maxVal = 7;
        else if (name.contains("destination"))
          maxVal = 10;

        if (random.nextBool()) {
          currentVal = currentVal + (random.nextBool() ? 1 : -1);
        } else {
          currentVal = random.nextInt(maxVal + 1);
        }

        if (currentVal < 0)
          currentVal = maxVal;
        if (currentVal > maxVal)
          currentVal = 0;

        value = static_cast<float>(currentVal);
      }
    } else {
      float range = getParameterRange(name);
      float r = random.nextFloat() * 2.0f - 1.0f;
      float drift = r * r * r * mutationAmount * range * 0.5f;

      value += drift;

      juce::var clamped = clampParameter(name, value, "zenith_poly_synth");
      value = static_cast<float>(clamped);
    }
  }
}

} // namespace zenith
