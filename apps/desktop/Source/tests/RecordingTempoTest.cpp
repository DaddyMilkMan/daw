/*
  ==============================================================================

    RecordingTempoTest.cpp
    Created: 2025-12-28
    Author:  Zenith DAW

    Tests for tempo-aware clip creation in RecordingManager.

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../engine/RecordingManager.h"
#include "../engine/ProjectState.h"
#include "../engine/TempoMap.h"
#include "TestUtils.h"

namespace zenith {
namespace tests {

class RecordingTempoTest : public juce::UnitTest {
public:
  RecordingTempoTest() : juce::UnitTest("Recording Tempo", "AudioEngine") {}

  void runTest() override {
    beginTest("Audio Clip Constant Tempo");
    {
        ProjectState projectState;
        projectState.newProject();
        juce::String trackId = projectState.createTrack("Audio", "audio", "Create Audio Track");
        
        RecordingManager manager;
        manager.setProjectState(&projectState);
        const double sampleRate = 44100.0;
        manager.prepare(sampleRate);

        TempoMap tempoMap;
        tempoMap.setSingleTempo(120.0); // 2 beats per second

        // Simulate 2 seconds of recording (4 beats)
        juce::int64 startSamples = 0;
        juce::int64 lengthSamples = static_cast<juce::int64>(2.0 * sampleRate);
        
        juce::File tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("test_audio_120.wav");
        
        manager.createAudioClip(tempFile, trackId, startSamples, lengthSamples, tempoMap);
        
        // Check ProjectState for the clip
        auto trackTree = projectState.getTrack(trackId);
        auto clipsTree = trackTree.getChildWithName(ProjectState::ID_CLIPS);
        expect(clipsTree.getNumChildren() == 1);
        
        auto clip = clipsTree.getChild(0);
        double clipStart = clip[ProjectState::PROP_START];
        double clipLength = clip[ProjectState::PROP_LENGTH];
        
        // ProjectState stores internal positions as samples @ Project Tempo (default 120)
        // 120 BPM -> 1 beat = sampleRate * 0.5
        // 4 beats = 2 seconds = 2 * sampleRate samples
        expectWithinAbsoluteError(clipStart, 0.0, 0.001);
        expectWithinAbsoluteError(clipLength, 2.0 * sampleRate, 0.001);
    }

    beginTest("Audio Clip Tempo Change");
    {
        ProjectState projectState;
        projectState.newProject();
        juce::String trackId = projectState.createTrack("Audio", "audio", "Create Audio Track");
        
        RecordingManager manager;
        manager.setProjectState(&projectState);
        const double sampleRate = 44100.0;
        manager.prepare(sampleRate);

        // Create a TempoMap with a change:
        // 0s-1s: 120 BPM (2 beats)
        // 1s-2s: 60 BPM (1 beat)
        // Total 2s recording = 3 beats
        
        juce::ValueTree tempoTree(ProjectState::ID_TEMPO_MAP);
        
        juce::ValueTree p1(ProjectState::ID_TEMPO_POINT);
        p1.setProperty(ProjectState::PROP_TIME_BEATS, 0.0, nullptr);
        p1.setProperty(ProjectState::PROP_BPM, 120.0, nullptr);
        tempoTree.appendChild(p1, nullptr);
        
        juce::ValueTree p2(ProjectState::ID_TEMPO_POINT);
        p2.setProperty(ProjectState::PROP_TIME_BEATS, 2.0, nullptr); // After 2 beats (1s)
        p2.setProperty(ProjectState::PROP_BPM, 60.0, nullptr);
        tempoTree.appendChild(p2, nullptr);
        
        TempoMap tempoMap;
        tempoMap.updateFromValueTree(tempoTree);

        juce::int64 startSamples = 0;
        juce::int64 lengthSamples = static_cast<juce::int64>(2.0 * sampleRate); // 2 seconds
        
        juce::File tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("test_audio_ramp.wav");
        
        manager.createAudioClip(tempFile, trackId, startSamples, lengthSamples, tempoMap);
        
        auto trackTree = projectState.getTrack(trackId);
        auto clipsTree = trackTree.getChildWithName(ProjectState::ID_CLIPS);
        auto clip = clipsTree.getChild(0);
        
        double clipLength = clip[ProjectState::PROP_LENGTH];
        
        // In Zenith, ProjectState::PROP_LENGTH is beats * (sampleRate * 60 / projectTempo)
        // projectTempo defaults to 120. 
        // 3 beats @ 120 BPM ref = 3 * (sampleRate * 0.5) = 1.5 * sampleRate
        expectWithinAbsoluteError(clipLength, 1.5 * sampleRate, 0.001);
    }

    beginTest("MIDI Note Tempo Change Timing");
    {
        ProjectState projectState;
        projectState.newProject();
        juce::String trackId = projectState.createTrack("MIDI", "midi", "Create MIDI Track");
        
        RecordingManager manager;
        manager.setProjectState(&projectState);
        const double sampleRate = 44100.0;
        manager.prepare(sampleRate);

        // Tempo change at 1s: 120 -> 60 BPM
        juce::ValueTree tempoTree(ProjectState::ID_TEMPO_MAP);
        juce::ValueTree p1(ProjectState::ID_TEMPO_POINT);
        p1.setProperty(ProjectState::PROP_TIME_BEATS, 0.0, nullptr);
        p1.setProperty(ProjectState::PROP_BPM, 120.0, nullptr);
        tempoTree.appendChild(p1, nullptr);
        juce::ValueTree p2(ProjectState::ID_TEMPO_POINT);
        p2.setProperty(ProjectState::PROP_TIME_BEATS, 2.0, nullptr);
        p2.setProperty(ProjectState::PROP_BPM, 60.0, nullptr);
        tempoTree.appendChild(p2, nullptr);
        
        TempoMap tempoMap;
        tempoMap.updateFromValueTree(tempoTree);

        juce::MidiMessageSequence sequence;
        // Note 1: at 0.5s (Beat 1.0)
        sequence.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0.5);
        sequence.addEvent(juce::MidiMessage::noteOff(1, 60), 0.6); // 0.1s duration (0.2 beats @ 120)
        
        // Note 2: at 1.5s (Record start + 1.5s)
        // Absolute: 1.5s. 
        // 0s-1s = 2 beats.
        // 1s-1.5s = 0.5s @ 60 BPM = 0.5 beats.
        // Total = 2.5 beats.
        sequence.addEvent(juce::MidiMessage::noteOn(1, 64, 0.8f), 1.5);
        sequence.addEvent(juce::MidiMessage::noteOff(1, 64), 1.7); // 0.2s duration (0.2 beats @ 60)

        sequence.updateMatchedPairs();

        manager.createMidiClip(sequence, trackId, 0, tempoMap);

        auto trackTree = projectState.getTrack(trackId);
        auto clipsTree = trackTree.getChildWithName(ProjectState::ID_CLIPS);
        auto clip = clipsTree.getChild(0);
        auto notesTree = clip.getChildWithName(ProjectState::ID_NOTES);
        
        expect(notesTree.getNumChildren() == 2);
        
        auto n1 = notesTree.getChild(0);
        expectWithinAbsoluteError(static_cast<double>(n1[ProjectState::PROP_START_BEATS]), 1.0, 0.01);
        expectWithinAbsoluteError(static_cast<double>(n1[ProjectState::PROP_LENGTH_BEATS]), 0.2, 0.01);
        
        auto n2 = notesTree.getChild(1);
        expectWithinAbsoluteError(static_cast<double>(n2[ProjectState::PROP_START_BEATS]), 2.5, 0.01);
        expectWithinAbsoluteError(static_cast<double>(n2[ProjectState::PROP_LENGTH_BEATS]), 0.2, 0.01);
    }
  }
};

static RecordingTempoTest recordingTempoTest;

} // namespace tests
} // namespace zenith
