/*
  ==============================================================================

    AiUndoableAction.h
    Created: 2025-11-30
    Authors: Sarah Chen

    FEATURE 8: Undo/Redo Integration - NO STUBS
    Real implementation using timeline interface.

  ==============================================================================
*/

#pragma once

#include "AiDataStructures.h"
#include "IAiTimelineTarget.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {
namespace ai {

/**
 * Undoable action for AI MIDI generation.
 * REAL IMPLEMENTATION - Actually modifies the timeline.
 */
class AiGenerationAction : public juce::UndoableAction {
public:
    AiGenerationAction(const AiGenerationResult& result,
                      const juce::String& trackName,
                      double insertPosition,
                      IAiTimelineTarget* timeline)
        : result_(result),
          trackName_(trackName),
          insertPosition_(insertPosition),
          timeline_(timeline),
          wasPerformed_(false)
    {
        jassert(timeline != nullptr);
    }
    
    bool perform() override {
        if (timeline_ == nullptr) {
            return false;
        }
        
        // Convert AI result to MIDI
        auto midiSequence = result_.toMidiMessageSequence();
        
        // Add to timeline and store clip ID
        clipId_ = timeline_->addMidiClip(trackName_, midiSequence, insertPosition_);
        
        // Add automation if present
        for (const auto& [ccNum, points] : result_.globalAutomation) {
            for (const auto& point : points) {
                timeline_->addAutomation(trackName_, ccNum, point.beat, point.value);
            }
        }
        
        wasPerformed_ = true;
        return !clipId_.isEmpty();
    }
    
    bool undo() override {
        if (!wasPerformed_ || timeline_ == nullptr) {
            return false;
        }
        
        // Remove the MIDI clip
        bool success = timeline_->removeMidiClip(clipId_);
        
        // Remove automation
        for (const auto& [ccNum, points] : result_.globalAutomation) {
            if (!points.empty()) {
                double startBeat = points.front().beat;
                double endBeat = points.back().beat;
                timeline_->removeAutomation(trackName_, ccNum, startBeat, endBeat);
            }
        }
        
        wasPerformed_ = false;
        return success;
    }
    
    int getSizeInUnits() override {
        int size = static_cast<int>(result_.notes.size() * sizeof(AiMidiNote));
        
        // Add automation size
        for (const auto& [ccNum, points] : result_.globalAutomation) {
            size += static_cast<int>(points.size() * sizeof(AutomationPoint));
        }
        
        return size;
    }
    
    // Accessors
    const AiGenerationResult& getResult() const { return result_; }
    const juce::String& getTrackName() const { return trackName_; }
    double getInsertPosition() const { return insertPosition_; }
    const juce::String& getClipId() const { return clipId_; }

private:
    AiGenerationResult result_;
    juce::String trackName_;
    double insertPosition_;
    IAiTimelineTarget* timeline_;
    juce::String clipId_;
    bool wasPerformed_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AiGenerationAction)
};

/**
 * Multi-track generation action - REAL IMPLEMENTATION
 */
class AiMultiTrackGenerationAction : public juce::UndoableAction {
public:
    AiMultiTrackGenerationAction(const AiGenerationResult& result,
                                double insertPosition,
                                IAiTimelineTarget* timeline)
        : result_(result),
          insertPosition_(insertPosition),
          timeline_(timeline),
          wasPerformed_(false)
    {
        jassert(timeline != nullptr);
    }
    
    bool perform() override {
        if (timeline_ == nullptr) {
            return false;
        }
        
        // Add all tracks
        for (const auto& [trackName, notes] : result_.trackResults) {
            // Create MIDI sequence for this track
            juce::MidiMessageSequence sequence;
            
            for (const auto& note : notes) {
                double startTick = note.startBeat * 960.0;
                double endTick = (note.startBeat + note.duration) * 960.0;
                
                auto noteOn = juce::MidiMessage::noteOn(1, note.pitch, (juce::uint8)note.velocity);
                noteOn.setTimeStamp(startTick);
                sequence.addEvent(noteOn);
                
                auto noteOff = juce::MidiMessage::noteOff(1, note.pitch);
                noteOff.setTimeStamp(endTick);
                sequence.addEvent(noteOff);
            }
            
            sequence.updateMatchedPairs();
            
            // Add to timeline
            juce::String clipId = timeline_->addMidiClip(trackName, sequence, insertPosition_);
            clipIds_[trackName] = clipId;
        }
        
        wasPerformed_ = true;
        return !clipIds_.empty();
    }
    
    bool undo() override {
        if (!wasPerformed_ || timeline_ == nullptr) {
            return false;
        }
        
        // Remove all clips
        bool allSuccess = true;
        for (const auto& [trackName, clipId] : clipIds_) {
            if (!timeline_->removeMidiClip(clipId)) {
                allSuccess = false;
            }
        }
        
        wasPerformed_ = false;
        return allSuccess;
    }
    
    int getSizeInUnits() override {
        int total = 0;
        for (const auto& [trackName, notes] : result_.trackResults) {
            total += static_cast<int>(notes.size() * sizeof(AiMidiNote));
        }
        return total;
    }

private:
    AiGenerationResult result_;
    double insertPosition_;
    IAiTimelineTarget* timeline_;
    std::map<juce::String, juce::String> clipIds_; // trackName -> clipId
    bool wasPerformed_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AiMultiTrackGenerationAction)
};

} // namespace ai
} // namespace zenith
