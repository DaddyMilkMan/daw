/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

// MixerChannel.h - Mixer channel strip with EQ, dynamics, and send/return processing

#include <array>
#include <span>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../effects/ConsoleEmulation.h"
#include "AudioConstants.h"
#include "MeteringSystem.h"

namespace zenith {

struct FilterCoefficients : public juce::ReferenceCountedObject {
  using Ptr = juce::ReferenceCountedObjectPtr<FilterCoefficients>;

  std::array<double, 6> hpf = {1.0, 0.0, 0.0,
                               1.0, 0.0, 0.0}; // b0,b1,b2,a0,a1,a2
  std::array<std::array<double, 6>, 4> eq;     // 4 EQ bands

  FilterCoefficients() {
    for (auto &band : eq) {
      band = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
    }
  }
};

//==============================================================================
/**
    Represents a mixer channel strip with professional signal processing.

    Each mixer channel provides:
    - Input gain
    - High-pass filter
    - 4-band parametric EQ
    - Professional Compressor with RMS/Lookahead
    - Send effects (up to 4 sends)
    - Pan and volume
    - Metering (input, output, gain reduction)

    All processing is lock-free and real-time safe.
*/

} // namespace
