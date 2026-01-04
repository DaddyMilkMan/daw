/*
  ==============================================================================

    RecordingTests.cpp
    Created: 2025-12-28
    Author:  Zenith DAW

    Tests for asynchronous recording finalization.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "../engine/AudioRecorder.h"
#include "../engine/RealTimeGarbageCollector.h"
#include "../engine/RecordingManager.h"
#include "../engine/ProjectState.h"
#include "../engine/Track.h"
#include "../engine/TempoMap.h"
#include "TestUtils.h"

namespace zenith {
namespace tests {

class RecordingTests : public juce::UnitTest {
public:
  RecordingTests() : juce::UnitTest("Recording", "AudioEngine") {}

  void runTest() override {
    beginTest("AudioRecorder Asynchronous Stop");
    {
        auto recorder = std::make_unique<AudioRecorder>();
        recorder->prepare(44100.0);

        std::shared_ptr<Track> track = Track::create("TestTrack", Track::Type::Audio);
        track->setArmed(true);
        std::vector<std::shared_ptr<Track>> tracks;
        tracks.push_back(track);
        printf("RecordingTests: Track created\n"); fflush(stdout);

        juce::AudioDeviceManager deviceManager;
        // Mock device or just default
        
        juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                  .getChildFile("zenith_record_test_" + juce::Uuid().toString());
        tempDir.createDirectory();
        printf("RecordingTests: Temp dir created: %s\n", tempDir.getFullPathName().toRawUTF8()); fflush(stdout);

        recorder->startRecording(tracks, deviceManager, 0, tempDir);
        printf("RecordingTests: startRecording called\n"); fflush(stdout);
        expect(recorder->isRecording());

        // Push some audio
        const int numSamples = 1024;
        float silence[numSamples] = { 0.0f };
        const float* input[] = { silence, silence };
        printf("RecordingTests: writing audio...\n"); fflush(stdout);
        recorder->write(input, 2, numSamples, tracks);
        printf("RecordingTests: write finished\n"); fflush(stdout);

        bool callbackTriggered = false;
        std::vector<RecordingResult> finalResults;

        recorder->stopRecording([&](std::vector<RecordingResult> results) {
            printf("RecordingTests: stopRecording callback START\n"); fflush(stdout);
            callbackTriggered = true;
            finalResults = results;
            juce::MessageManager::getInstance()->stopDispatchLoop();
            printf("RecordingTests: stopRecording callback FINISH\n"); fflush(stdout);
        });

        expect(recorder->isFinalizing());
        expect(!recorder->isRecording());

        // Wait for finalization (pumping message loop)
        juce::MessageManager::getInstance()->runDispatchLoop();

        expect(callbackTriggered, "Completion callback should be triggered");
        expect(recorder->getState() == AudioRecorder::RecordingState::Idle);
        expect(finalResults.size() == 1, "Should have one recording result");

        if (finalResults.size() > 0) {
            expect(finalResults[0].file.existsAsFile(), "Recording file should exist");
            expect(finalResults[0].samplesRecorded == numSamples, "Record length mismatch");
        }

        tempDir.deleteRecursively();
    }

    beginTest("RecordingManager Asynchronous Finalization");
    {
        ProjectState projectState;
        projectState.newProject();
        
        RecordingManager manager;
        manager.setProjectState(&projectState);
        manager.prepare(44100.0);
        
        juce::AudioDeviceManager deviceManager;
        manager.setDeviceManager(&deviceManager);
        
        juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                  .getChildFile("zenith_mgr_test_" + juce::Uuid().toString());
        tempDir.createDirectory();
        manager.setRecordingDirectory(tempDir);

        std::shared_ptr<Track> track = Track::create("MgrTrack", Track::Type::Audio);
        track->setArmed(true);
        std::vector<std::shared_ptr<Track>> tracks;
        tracks.push_back(track);
        
        manager.startRecording(0, tracks);
        expect(manager.isRecording());

        // Push audio
        float dummy[512] = { 0.0f };
        float* dummyPtrs[] = { dummy, dummy };
        manager.captureAudio(dummyPtrs, 2, 512, tracks);

        manager.stopRecording(tracks, TempoMap());
        
        // Wait for finalization (using a small timer to periodically check and then stop loop)
        struct FinalizeWaiter : juce::Timer {
            FinalizeWaiter(ProjectState& ps) : projectState(ps) {}
            void timerCallback() override {
                bool clipCreated = false;
                for (int i = 0; i < projectState.getNumTracks(); ++i) {
                    auto trackTree = projectState.getTrackByIndex(i);
                    if (trackTree.getChildWithName(ProjectState::ID_CLIPS).getNumChildren() > 0) {
                        clipCreated = true;
                        break;
                    }
                }
                if (clipCreated || ++ticks > 100) {
                    stopTimer();
                    juce::MessageManager::getInstance()->stopDispatchLoop();
                }
            }
            ProjectState& projectState;
            int ticks = 0;
        };

        FinalizeWaiter waiter(projectState);
        waiter.startTimer(10);
        juce::MessageManager::getInstance()->runDispatchLoop();

        bool finalClipCreated = false;
        for (int i = 0; i < projectState.getNumTracks(); ++i) {
            auto trackTree = projectState.getTrackByIndex(i);
            if (trackTree.getChildWithName(ProjectState::ID_CLIPS).getNumChildren() > 0) {
                finalClipCreated = true;
                break;
            }
        }
        expect(finalClipCreated, "Audio clip should be created in ProjectState after async stop");
        
        tempDir.deleteRecursively();
    }
  }
};

static RecordingTests recordingTests;

} // namespace tests
} // namespace zenith
