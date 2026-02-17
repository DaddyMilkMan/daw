/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

#ifdef ZENITH_HAS_ARA

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_processors/format_types/juce_ARA.h>

namespace zenith {
namespace effects {

//==============================================================================
/**
    ARA 2 Document Controller for Zenith Auto-Tune.

    Provides seamless integration with ARA-compatible DAWs:
    - Timeline synchronization
    - Shared undo/redo history
    - No need to render/export audio
    - Real-time updates

    Note: ARA SDK 2.2.0 or higher required
    Download from: https://github.com/Celemony/ARA_SDK
*/
class ZenithAutoTuneARADocumentController
    : public juce::ARADocumentControllerSpecialisation
{
public:
    //==============================================================================
    ZenithAutoTuneARADocumentController();
    ~ZenithAutoTuneARADocumentController() override;

    //==============================================================================
    // ARA Document Controller implementation
    bool doRestoreObjectsFromState (const juce::ARA::ARAReader& reader) override;
    bool doStoreObjectsToState (const juce::ARA::ARAWriter& writer) override;

    //==============================================================================
    // Factory method for ARA plugin wrapper
    static juce::ARAFactory::PlugInExtensionInstance* createARAFactory();

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithAutoTuneARADocumentController)
};

//==============================================================================
/**
    ARA 2 Playback Renderer for real-time processing with timeline sync.
*/
class ZenithAutoTuneARAPlaybackRenderer
    : public juce::ARAPlaybackRenderer
{
public:
    //==============================================================================
    ZenithAutoTuneARAPlaybackRenderer();
    ~ZenithAutoTuneARAPlaybackRenderer() override;

    //==============================================================================
    // ARA Playback Renderer implementation
    void prepareToPlay (double sampleRate, int maxSamplesPerBlock,
                        int numChannels) override;
    void releaseResources() override;
    bool processBlock (juce::AudioBuffer<float>& buffer,
                      juce::AudioProcessorRealtimeAdaptor* adaptor,
                      juce::ARPPosition startSample,
                      juce::ARPPosition numSamples) override;

private:
    //==============================================================================
    // Auto-Tune processor reference (will be set by host)
    // This allows ARA to control our AutoTune parameters

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithAutoTuneARAPlaybackRenderer)
};

//==============================================================================
/**
    ARA 2 Editor Renderer for visualizing and editing pitch corrections.
*/
class ZenithAutoTuneARAEditorRenderer
    : public juce::ARAEditorRenderer
{
public:
    //==============================================================================
    ZenithAutoTuneARAEditorRenderer();
    ~ZenithAutoTuneARAEditorRenderer() override;

    //==============================================================================
    // ARA Editor Renderer implementation
    juce::ARAEditResult processRegion (juce::ARAAudioSource* audioSource,
                                      double startInTime,
                                      double durationInTime) override;

    //==============================================================================
    // Get pitch data for visualization
    std::vector<std::pair<double, float>> getPitchCurve() const;

private:
    //==============================================================================
    std::vector<std::pair<double, float>> pitchCurve_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithAutoTuneARAEditorRenderer)
};

} // namespace effects
} // namespace zenith

#endif // ZENITH_HAS_ARA
