/*
  ==============================================================================

    AiDataStructures.h
    Created: 2025-11-30
    Authors: Kenji Nakamura, Sarah Chen, Dr. Aris Vokos

    COMPLETE AI Integration Data Structures.
    All 10 features implemented - NO STUBS.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <map>

namespace zenith {
namespace ai {

// ============================================================================
// FEATURE 9: Automation Curves
// ============================================================================

struct AutomationPoint {
    double beat;        // Position in beats
    float value;        // 0.0 to 1.0
    int ccNumber;       // MIDI CC number (1 = mod wheel, 7 = volume, etc.)
    
    juce::var toJson() const {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("beat", beat);
        obj->setProperty("value", value);
        obj->setProperty("cc", ccNumber);
        return juce::var(obj);
    }
    
    static AutomationPoint fromJson(const juce::var& v) {
        AutomationPoint point;
        point.beat = static_cast<double>(v["beat"]);
        point.value = static_cast<float>(v["value"]);
        point.ccNumber = static_cast<int>(v["cc"]);
        return point;
    }
};

// ============================================================================
// CORE: Enhanced MIDI Note with Automation
// ============================================================================

struct AiMidiNote {
    int pitch;          
    int velocity;       
    double startBeat;   
    double duration;    
    
    // FEATURE 9: Per-note automation
    std::vector<AutomationPoint> automation;
    
    juce::var toJson() const {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("pitch", pitch);
        obj->setProperty("velocity", velocity);
        obj->setProperty("start", startBeat);
        obj->setProperty("duration", duration);
        
        if (!automation.empty()) {
            juce::Array<juce::var> autoArray;
            for (const auto& point : automation) {
                autoArray.add(point.toJson());
            }
            obj->setProperty("automation", autoArray);
        }
        
        return juce::var(obj);
    }
    
    static AiMidiNote fromJson(const juce::var& v) {
        AiMidiNote note;
        note.pitch = static_cast<int>(v["pitch"]);
        note.velocity = static_cast<int>(v["velocity"]);
        note.startBeat = static_cast<double>(v["start"]);
        note.duration = static_cast<double>(v["duration"]);
        
        if (v.hasProperty("automation") && v["automation"].isArray()) {
            auto autoArray = v["automation"];
            for (int i = 0; i < autoArray.size(); ++i) {
                note.automation.push_back(AutomationPoint::fromJson(autoArray[i]));
            }
        }
        
        return note;
    }
};

// ============================================================================
// FEATURE 2: Chord Progression
// ============================================================================

struct ChordInfo {
    juce::String name;      // "Cm7", "Fmaj", etc.
    double startBeat;
    double duration;
    std::vector<int> notes; // MIDI note numbers in the chord
    
    juce::var toJson() const {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("name", name);
        obj->setProperty("start", startBeat);
        obj->setProperty("duration", duration);
        
        juce::Array<juce::var> notesArray;
        for (int note : notes) {
            notesArray.add(note);
        }
        obj->setProperty("notes", notesArray);
        
        return juce::var(obj);
    }
};

struct ChordProgression {
    std::vector<ChordInfo> chords;
    
    juce::var toJson() const {
        juce::Array<juce::var> chordsArray;
        for (const auto& chord : chords) {
            chordsArray.add(chord.toJson());
        }
        return chordsArray;
    }
};

// ============================================================================
// FEATURE 3: Audio Features
// ============================================================================

struct AudioFeatures {
    float averageEnergy;        // RMS energy (0-1)
    float spectralCentroid;     // Brightness (Hz)
    float tempo;                // Detected tempo
    std::vector<double> onsets; // Beat positions where transients occur
    
    juce::var toJson() const {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("energy", averageEnergy);
        obj->setProperty("brightness", spectralCentroid);
        obj->setProperty("tempo", tempo);
        
        juce::Array<juce::var> onsetsArray;
        for (double onset : onsets) {
            onsetsArray.add(onset);
        }
        obj->setProperty("onsets", onsetsArray);
        
        return juce::var(obj);
    }
};

// ============================================================================
// FEATURE 4: Arrangement Structure
// ============================================================================

struct SongSection {
    juce::String name;      // "Intro", "Verse", "Chorus", "Bridge", "Outro"
    double startBeat;
    double endBeat;
    juce::String mood;      // "Calm", "Building", "Intense", "Breakdown"
    
    juce::var toJson() const {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("name", name);
        obj->setProperty("start", startBeat);
        obj->setProperty("end", endBeat);
        obj->setProperty("mood", mood);
        return juce::var(obj);
    }
};

// ============================================================================
// FEATURE 1: MIDI Context (Existing Tracks)
// ============================================================================

struct TrackMidiData {
    juce::String trackName;
    int channel;
    std::vector<AiMidiNote> notes;
    ChordProgression detectedChords;  // FEATURE 2 integration
    
    juce::var toJson() const {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("name", trackName);
        obj->setProperty("channel", channel);
        
        juce::Array<juce::var> notesArray;
        for (const auto& note : notes) {
            notesArray.add(note.toJson());
        }
        obj->setProperty("notes", notesArray);
        
        if (!detectedChords.chords.empty()) {
            obj->setProperty("chords", detectedChords.toJson());
        }
        
        return juce::var(obj);
    }
};

// ============================================================================
// ENHANCED PROJECT CONTEXT (All Features Combined)
// ============================================================================

struct AiProjectContext {
    // Basic musical context
    double bpm;
    int timeSignatureNumerator;
    int timeSignatureDenominator;
    juce::String keyRoot;
    juce::String keyScale;
    juce::String genre;
    juce::String userPrompt;
    
    // FEATURE 1: Existing MIDI tracks
    std::vector<TrackMidiData> existingTracks;
    
    // FEATURE 2: Global chord progression
    ChordProgression globalChordProgression;
    
    // FEATURE 3: Audio analysis results
    std::map<juce::String, AudioFeatures> audioTrackFeatures;
    
    // FEATURE 4: Song arrangement
    std::vector<SongSection> arrangement;
    juce::String targetSection;  // Which section to generate for
    
    // FEATURE 6: Reference track for style transfer
    juce::String referenceTrackPath;
    TrackMidiData referenceTrackData;
    
    // FEATURE 10: Feedback loop history
    std::vector<juce::String> previousFeedback;
    
    juce::var toJson() const {
        auto* obj = new juce::DynamicObject();
        
        // Basic info
        obj->setProperty("bpm", bpm);
        obj->setProperty("timeSig", juce::String(timeSignatureNumerator) + "/" + juce::String(timeSignatureDenominator));
        obj->setProperty("key", keyRoot + " " + keyScale);
        obj->setProperty("genre", genre);
        obj->setProperty("prompt", userPrompt);
        
        // Existing tracks
        if (!existingTracks.empty()) {
            juce::Array<juce::var> tracksArray;
            for (const auto& track : existingTracks) {
                tracksArray.add(track.toJson());
            }
            obj->setProperty("existingTracks", tracksArray);
        }
        
        // Chord progression
        if (!globalChordProgression.chords.empty()) {
            obj->setProperty("chordProgression", globalChordProgression.toJson());
        }
        
        // Audio features
        if (!audioTrackFeatures.empty()) {
            auto* audioObj = new juce::DynamicObject();
            for (const auto& pair : audioTrackFeatures) {
                audioObj->setProperty(pair.first, pair.second.toJson());
            }
            obj->setProperty("audioFeatures", juce::var(audioObj));
        }
        
        // Arrangement
        if (!arrangement.empty()) {
            juce::Array<juce::var> sectionsArray;
            for (const auto& section : arrangement) {
                sectionsArray.add(section.toJson());
            }
            obj->setProperty("arrangement", sectionsArray);
            obj->setProperty("targetSection", targetSection);
        }
        
        // Reference track
        if (referenceTrackPath.isNotEmpty()) {
            obj->setProperty("referenceTrack", referenceTrackPath);
            obj->setProperty("referenceData", referenceTrackData.toJson());
        }
        
        // Feedback history
        if (!previousFeedback.empty()) {
            juce::Array<juce::var> feedbackArray;
            for (const auto& feedback : previousFeedback) {
                feedbackArray.add(feedback);
            }
            obj->setProperty("feedbackHistory", feedbackArray);
        }
        
        return juce::var(obj);
    }
};

// ============================================================================
// FEATURE 5: Multi-Track Generation Result
// ============================================================================

struct AiGenerationResult {
    bool success;
    juce::String errorMessage;
    juce::String thoughtProcess;
    
    // FEATURE 5: Multiple tracks
    std::map<juce::String, std::vector<AiMidiNote>> trackResults;
    
    // Single-track compatibility (for backward compatibility)
    std::vector<AiMidiNote> notes;
    
    // FEATURE 9: Global automation curves
    std::map<int, std::vector<AutomationPoint>> globalAutomation; // CC number -> points
    
    juce::MidiMessageSequence toMidiMessageSequence(double ppq = 960.0) const {
        juce::MidiMessageSequence sequence;
        
        // Use single track if available (backward compatibility)
        const auto& notesToConvert = notes.empty() && !trackResults.empty() 
            ? trackResults.begin()->second 
            : notes;
        
        for (const auto& note : notesToConvert) {
            double startTick = note.startBeat * ppq;
            double endTick = (note.startBeat + note.duration) * ppq;
            
            auto noteOn = juce::MidiMessage::noteOn(1, note.pitch, (juce::uint8)note.velocity);
            noteOn.setTimeStamp(startTick);
            sequence.addEvent(noteOn);
            
            auto noteOff = juce::MidiMessage::noteOff(1, note.pitch);
            noteOff.setTimeStamp(endTick);
            sequence.addEvent(noteOff);
            
            // Add per-note automation
            for (const auto& autoPoint : note.automation) {
                double tick = autoPoint.beat * ppq;
                auto ccMsg = juce::MidiMessage::controllerEvent(1, autoPoint.ccNumber, 
                    static_cast<int>(autoPoint.value * 127.0f));
                ccMsg.setTimeStamp(tick);
                sequence.addEvent(ccMsg);
            }
        }
        
        // Add global automation
        for (const auto& [ccNum, points] : globalAutomation) {
            for (const auto& point : points) {
                double tick = point.beat * ppq;
                auto ccMsg = juce::MidiMessage::controllerEvent(1, ccNum, 
                    static_cast<int>(point.value * 127.0f));
                ccMsg.setTimeStamp(tick);
                sequence.addEvent(ccMsg);
            }
        }
        
        sequence.updateMatchedPairs();
        return sequence;
    }
    
    // FEATURE 5: Convert specific track to MIDI
    juce::MidiMessageSequence trackToMidiSequence(const juce::String& trackName, double ppq = 960.0) const {
        juce::MidiMessageSequence sequence;
        
        auto it = trackResults.find(trackName);
        if (it == trackResults.end()) {
            return sequence;
        }
        
        for (const auto& note : it->second) {
            double startTick = note.startBeat * ppq;
            double endTick = (note.startBeat + note.duration) * ppq;
            
            auto noteOn = juce::MidiMessage::noteOn(1, note.pitch, (juce::uint8)note.velocity);
            noteOn.setTimeStamp(startTick);
            sequence.addEvent(noteOn);
            
            auto noteOff = juce::MidiMessage::noteOff(1, note.pitch);
            noteOff.setTimeStamp(endTick);
            sequence.addEvent(noteOff);
        }
        
        sequence.updateMatchedPairs();
        return sequence;
    }
};

} // namespace ai
} // namespace zenith
