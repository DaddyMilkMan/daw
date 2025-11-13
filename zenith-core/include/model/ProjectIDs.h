/**
 * @file ProjectIDs.h
 * @brief ValueTree identifiers for project model schema (v0.1)
 *
 * This defines the single source of truth for the project data model.
 * Audio tracks with audio clips only - no FX, tempo, or markers in v0.1.
 */

#pragma once

#include <JuceHeader.h>

namespace zenith::model::ids
{
    // Node types
    static const juce::Identifier ID_PROJECT    { "project" };
    static const juce::Identifier ID_TRACK      { "track" };
    static const juce::Identifier ID_AUDIO_CLIP { "audioClip" };

    // Project props
    static const juce::Identifier attrName       { "name" };
    static const juce::Identifier attrSampleRate { "sampleRate" }; // double

    // Track props
    static const juce::Identifier attrTrackId   { "trackId" };   // int
    static const juce::Identifier attrTrackName { "trackName" }; // String

    // Clip props
    static const juce::Identifier attrClipId         { "clipId" };         // int64
    static const juce::Identifier attrFilePath       { "filePath" };       // String (absolute or project-relative)
    static const juce::Identifier attrStartSample    { "startSample" };    // int64 (timeline)
    static const juce::Identifier attrLengthSamples  { "lengthSamples" };  // int64
    static const juce::Identifier attrSrcOffset      { "srcOffset" };      // int64 (trim inside file)
    static const juce::Identifier attrGain           { "gain" };           // float
    static const juce::Identifier attrFadeInSamples  { "fadeInSamples" };  // int
    static const juce::Identifier attrFadeOutSamples { "fadeOutSamples" }; // int
    static const juce::Identifier attrMuted          { "muted" };          // bool
}
