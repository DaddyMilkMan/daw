/*
  ==============================================================================

    SampleEditorRecordingTests.cpp
    Created: 2025-12-15
    Author:  Zenith DAW

    Tests for SampleEditorComponent recording functionality.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <array>
#include "../ui/sample-editor/SampleEditorComponent.h"
#include "../engine/Engine.h"
#include "../engine/ProjectState.h"

// Expose protected/private members for testing if needed, or use public API
// SampleEditorComponent doesn't declare friend tests. 
// We will rely on public API and side effects (file saving).

class SampleEditorRecordingTests : public juce::UnitTest {
public:
  SampleEditorRecordingTests() : juce::UnitTest("SampleEditorRecordingTests") {}

  void runTest() override {
    beginTest("Recording Flow");
    {
        // 1. Setup
        // ProjectState must outlive SampleEditorComponent because editor registers as ValueTree::Listener
        auto projectState = std::make_unique<zenith::ProjectState>();
        zenith::Engine engine;
        engine.setProjectState(projectState.get());
        
        // We need a MessageManager for Timer
        // UnitTests usually run without a message loop block, but MessageManager exists.
        
        {
            zenith::SampleEditorComponent editor(engine, *projectState);
            
            // 2. Start Recording
            editor.startRecording();
            
            // 3. Simulate Audio Input
            // We can't easily call private audioDeviceIOCallbackWithContext without being a friend.
            // But SampleEditorComponent inherits from AudioIODeviceCallback.
            // So we can cast it.
            juce::AudioIODeviceCallback* cb = static_cast<juce::AudioIODeviceCallback*>(&editor);
            
            int numSamples = 512;
            int numChannels = 1;
            juce::AudioBuffer<float> inBuffer(numChannels, numSamples);
            // Fill with some signal (e.g. sine wave) to verify data?
            // For now, just non-zero.
            inBuffer.clear();
            auto* w = inBuffer.getWritePointer(0);
            for(int i=0; i<numSamples; ++i) w[i] = 0.5f;
            
            std::array<const float*, 1> inputData = { inBuffer.getReadPointer(0) };
            std::array<float*, 1> outputData = { nullptr }; // We don't care about output
            
            // Context
            juce::AudioIODeviceCallbackContext ctx; // Dummy
            
            // Call callback 10 times -> ~5000 samples
            for(int i=0; i<10; ++i) {
                cb->audioDeviceIOCallbackWithContext(inputData.data(), numChannels, outputData.data(), 0, numSamples, ctx);
            }
            
            // 4. Trigger Timer to drain FIFO
            // We can't call private timerCallback().
            // But stopRecording() calls it!
            // So let's just stop.
            
            // 5. Stop Recording
            editor.stopRecording();
            
            // 6. Verify
            // We expect edits to be present.
            // saveAsNewFile requires a target.
            juce::File tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                      .getChildFile("test_recording.wav");
            if (tempFile.exists()) tempFile.deleteFile();
            
            editor.saveAsNewFile(tempFile);
            
            expect(tempFile.existsAsFile(), "Recording should be saved to file");
            
            if (tempFile.existsAsFile()) {
                expect(tempFile.getSize() > 0, "Created file should not be empty");
                
                // Optional: Read back and check duration?
                juce::WavAudioFormat wav;
                std::unique_ptr<juce::AudioFormatReader> reader(wav.createReaderFor(new juce::FileInputStream(tempFile), true));
                if (reader) {
                    // We pushed 10 * 512 samples = 5120 samples.
                    // The buffer might be slightly different due to FIFO/Timer timing if we were running live,
                    // but here we called callback 10 times (pushing to FIFO) and then stopRecording called timerCallback (drain FIFO).
                    // So all samples should be there.
                    expectEquals((int)reader->lengthInSamples, 5120, "Recorded length should match input");
                } else {
                    expect(false, "Could not read generated WAV file");
                }
                
                tempFile.deleteFile();
            }
        } // editor destroyed here, before projectState
        
        // Clear engine reference to projectState before projectState is destroyed
        engine.setProjectState(nullptr);
    }
  }
};

static SampleEditorRecordingTests sampleEditorRecordingTests;
