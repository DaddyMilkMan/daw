/*
  ==============================================================================

    IAiTimelineTarget.h
    Created: 2025-11-30
    Authors: Sarah Chen

    Interface for AI to interact with the timeline.
    This allows AiUndoableAction to actually modify tracks.

  ==============================================================================
*/

#pragma once

#include "AiDataStructures.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace zenith {
namespace ai {

/**
 * Interface that the Timeline/Track system must implement
 * to allow AI to add/remove MIDI clips.
 */
class IAiTimelineTarget {
public:
    virtual ~IAiTimelineTarget() = default;
    
    /**
     * Add a MIDI clip to a specific track.
     * @param trackName Name of the track
     * @param sequence MIDI data
     * @param position Start position in beats
     * @return Clip ID for later removal
     */
    virtual juce::String addMidiClip(const juce::String& trackName,
                                     const juce::MidiMessageSequence& sequence,
                                     double position) = 0;
    
    /**
     * Remove a MIDI clip by ID.
     */
    virtual bool removeMidiClip(const juce::String& clipId) = 0;
    
    /**
     * Add automation to a track.
     */
    virtual void addAutomation(const juce::String& trackName,
                              int ccNumber,
                              double beat,
                              float value) = 0;
    
    /**
     * Remove automation points in a range.
     */
    virtual void removeAutomation(const juce::String& trackName,
                                  int ccNumber,
                                  double startBeat,
                                  double endBeat) = 0;
};

} // namespace ai
} // namespace zenith
