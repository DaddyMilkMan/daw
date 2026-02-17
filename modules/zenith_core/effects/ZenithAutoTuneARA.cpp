/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#include "ZenithAutoTuneARA.h"

#ifdef ZENITH_HAS_ARA

namespace zenith {
namespace effects {

//==============================================================================
// ZenithAutoTuneARADocumentController
//==============================================================================
ZenithAutoTuneARADocumentController::ZenithAutoTuneARADocumentController()
    : juce::ARADocumentControllerSpecialisation()
{
}

ZenithAutoTuneARADocumentController::~ZenithAutoTuneARADocumentController()
{
}

bool ZenithAutoTuneARADocumentController::doRestoreObjectsFromState (const juce::ARA::ARAReader& reader)
{
    // Restore Auto-Tune state from ARA document
    // This includes key/scale settings, corrections, etc.

    // For now, implement basic state restoration
    // TODO: Implement full state restoration

    return juce::ARADocumentControllerSpecialisation::doRestoreObjectsFromState(reader);
}

bool ZenithAutoTuneARADocumentController::doStoreObjectsToState (const juce::ARA::ARAWriter& writer)
{
    // Store Auto-Tune state to ARA document
    // This includes key/scale settings, corrections, etc.

    // For now, implement basic state storage
    // TODO: Implement full state storage

    return juce::ARADocumentControllerSpecialisation::doStoreObjectsToState(writer);
}

juce::ARAFactory::PlugInExtensionInstance* ZenithAutoTuneARADocumentController::createARAFactory()
{
    // Create ARA factory for our plugin
    // This is called by the host during plugin initialization

    return new juce::ARAFactory(juce::ARAFactory::ARAPlugInExtensionInstance {
        &ARA::ARAModel::get()->documentController,
        &ARA::ARAModel::get()->playbackRenderer,
        &ARA::ARAModel::get()->editorRenderer,
        &ARA::ARAModel::get()->editorView,
        juce::ARA::APIVersion::current()
    });
}

//==============================================================================
// ZenithAutoTuneARAPlaybackRenderer
//==============================================================================
ZenithAutoTuneARAPlaybackRenderer::ZenithAutoTuneARAPlaybackRenderer()
    : juce::ARAPlaybackRenderer()
{
}

ZenithAutoTuneARAPlaybackRenderer::~ZenithAutoTuneARAPlaybackRenderer()
{
}

void ZenithAutoTuneARAPlaybackRenderer::prepareToPlay (double sampleRate, int maxSamplesPerBlock,
                                                        int numChannels)
{
    // Prepare for ARA playback
    // This is called by the host before processing begins

    juce::ARAPlaybackRenderer::prepareToPlay (sampleRate, maxSamplesPerBlock, numChannels);
}

void ZenithAutoTuneARAPlaybackRenderer::releaseResources()
{
    // Release resources
    juce::ARAPlaybackRenderer::releaseResources();
}

bool ZenithAutoTuneARAPlaybackRenderer::processBlock (juce::AudioBuffer<float>& buffer,
                                                      juce::AudioProcessorRealtimeAdaptor* adaptor,
                                                      juce::ARPPosition startSample,
                                                      juce::ARPPosition numSamples)
{
    // Process audio through Auto-Tune with ARA timeline sync
    // The host provides timing information for perfect synchronization

    // Get the current playback position from ARA
    double playbackTime = getPlaybackPosition();

    // Process normally - the ARA layer handles timeline sync
    return juce::ARAPlaybackRenderer::processBlock(buffer, adaptor, startSample, numSamples);
}

//==============================================================================
// ZenithAutoTuneARAEditorRenderer
//==============================================================================
ZenithAutoTuneARAEditorRenderer::ZenithAutoTuneARAEditorRenderer()
    : juce::ARAEditorRenderer()
{
}

ZenithAutoTuneARAEditorRenderer::~ZenithAutoTuneARAEditorRenderer()
{
}

juce::ARAEditResult ZenithAutoTuneARAEditorRenderer::processRegion (juce::ARAAudioSource* audioSource,
                                                                    double startInTime,
                                                                    double durationInTime)
{
    // Process audio region for editing
    // This is called when user wants to analyze/edit audio in ARA context

    // Analyze pitch for the region
    // TODO: Integrate with our pitch detection

    return juce::ARAEditorRenderer::kSuccess;
}

std::vector<std::pair<double, float>> ZenithAutoTuneARAEditorRenderer::getPitchCurve() const
{
    return pitchCurve_;
}

} // namespace effects
} // namespace zenith

#endif // ZENITH_HAS_ARA
