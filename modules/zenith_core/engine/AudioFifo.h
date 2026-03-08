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

class AudioFifo; // Forward declaration

//==============================================================================
/**
    Professional-grade compressor with RMS detection, lookahead, and soft knee.

    Features:
    - RMS envelope detection (more musical than peak)
    - Lookahead for transparent limiting
    - Soft knee option
    - Auto makeup gain
*/

} // namespace
