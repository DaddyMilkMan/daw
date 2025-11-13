/**
 * @file ProjectIDs.h
 * @brief ValueTree identifiers for project model schema
 *
 * Defines all Identifier constants used in the Zenith project ValueTree schema.
 * Using type-based identification where the ValueTree type itself indicates
 * the kind of data contained.
 */

#pragma once

#include <JuceHeader.h>

namespace zenith::ids
{
    //==========================================================================
    // Node types
    //==========================================================================

    extern const juce::Identifier project;
    extern const juce::Identifier track;
    extern const juce::Identifier audioClip;

    //==========================================================================
    // Project properties
    //==========================================================================

    extern const juce::Identifier projName;
    extern const juce::Identifier projSampleRate;

    //==========================================================================
    // Track properties
    //==========================================================================

    extern const juce::Identifier trackId;
    extern const juce::Identifier trackName;
    extern const juce::Identifier trackGain;
    extern const juce::Identifier trackPan;
    extern const juce::Identifier trackMuted;

    //==========================================================================
    // Clip properties
    //==========================================================================

    extern const juce::Identifier clipId;
    extern const juce::Identifier clipName;
    extern const juce::Identifier clipFilePath;
    extern const juce::Identifier clipStartSample;
    extern const juce::Identifier clipLengthSamples;
    extern const juce::Identifier clipSrcOffset;
    extern const juce::Identifier clipGain;
    extern const juce::Identifier clipFadeInSamples;
    extern const juce::Identifier clipFadeOutSamples;
    extern const juce::Identifier clipMuted;
}
