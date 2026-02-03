/*
  ==============================================================================

    MidiCaptureBuffer.cpp
    Created: 2025-12-28
    Author:  Zenith DAW

    Implementation of lock-free rolling MIDI buffer for retroactive recording.

  ==============================================================================
*/

#include "MidiCaptureBuffer.h"
#include "ProjectState.h"
#include "TempoMap.h"
#include "MidiNote.h"

namespace zenith {

//==============================================================================
MidiCaptureBuffer::MidiCaptureBuffer() {
    buffer_.resize(kDefaultBufferSize);
}

MidiCaptureBuffer::~MidiCaptureBuffer() = default;

//==============================================================================
// Configuration
//==============================================================================

void MidiCaptureBuffer::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
}

void MidiCaptureBuffer::setBufferDuration(float durationSeconds) {
    bufferDurationSeconds_.store(juce::jlimit(1.0f, 300.0f, durationSeconds));
}

//==============================================================================
// MIDI Capture (RT-Safe)
//==============================================================================

void MidiCaptureBuffer::capture(const juce::MidiMessage& message, 
                                 juce::int64 samplePosition,
                                 int trackIndex) {
    if (!enabled_.load(std::memory_order_relaxed))
        return;

    // Only capture note on/off and relevant control messages
    if (!message.isNoteOnOrOff() && 
        !message.isController() && 
        !message.isPitchWheel() &&
        !message.isAftertouch() &&
        !message.isChannelPressure()) {
        return;
    }

    int start1, size1, start2, size2;
    fifo_.prepareToWrite(1, start1, size1, start2, size2);

    if (size1 > 0) {
        buffer_[static_cast<size_t>(start1)] = MidiCaptureEntry(message, samplePosition, trackIndex);
        fifo_.finishedWrite(1);
    } else {
        // Buffer full - track dropped messages
        droppedMessages_.fetch_add(1, std::memory_order_relaxed);
    }
}

void MidiCaptureBuffer::captureBuffer(const juce::MidiBuffer& buffer,
                                       juce::int64 bufferStartSamplePosition,
                                       int trackIndex) {
    if (!enabled_.load(std::memory_order_relaxed))
        return;

    for (const auto metadata : buffer) {
        capture(metadata.getMessage(), 
                bufferStartSamplePosition + metadata.samplePosition,
                trackIndex);
    }
}

//==============================================================================
// Capture to Clip (Message Thread)
//==============================================================================

bool MidiCaptureBuffer::captureToClip(ProjectState& projectState,
                                       const juce::String& trackId,
                                       const TempoMap& tempoMap,
                                       juce::int64 fromSamplePosition,
                                       juce::int64 toSamplePosition,
                                       int filterTrackIndex) {
    const juce::ScopedLock lock(messageLock_);

    // Get captured sequence
    auto sequence = getCapturedSequence(fromSamplePosition, toSamplePosition, filterTrackIndex);
    
    if (sequence.getNumEvents() == 0)
        return false;

    // Determine clip start position
    juce::int64 clipStartSamples = fromSamplePosition >= 0 
        ? fromSamplePosition 
        : lastStopPosition_.load();
    
    // Convert to beats using tempo map
    double clipStartBeats = tempoMap.samplesToBeats(clipStartSamples);
    
    // Find the range of events
    double firstEventTime = sequence.getEventTime(0);
    double lastEventTime = sequence.getEndTime();
    double clipLengthSeconds = lastEventTime - firstEventTime;
    
    // Convert length to beats (approximate using tempo at clip start)
    double tempo = tempoMap.getTempoAtBeat(clipStartBeats);
    double clipLengthBeats = (clipLengthSeconds / 60.0) * tempo;
    clipLengthBeats = juce::jmax(1.0, clipLengthBeats); // Minimum 1 beat
    
    // Create the clip in ProjectState
    juce::ValueTree clipTree(ProjectState::ID_CLIP);
    clipTree.setProperty(ProjectState::PROP_ID, juce::Uuid().toString(), nullptr);
    clipTree.setProperty(ProjectState::PROP_NAME, "Captured MIDI", nullptr);
    clipTree.setProperty(ProjectState::PROP_TYPE, "MIDI", nullptr);
    clipTree.setProperty(ProjectState::PROP_START_BEATS, clipStartBeats, nullptr);
    clipTree.setProperty(ProjectState::PROP_LENGTH_BEATS, clipLengthBeats, nullptr);
    
    // Add MIDI notes
    juce::ValueTree notesTree(ProjectState::ID_NOTES);
    
    // Track note-on events to pair with note-offs
    std::map<int, std::pair<double, float>> activeNotes; // pitch -> (startTime, velocity)
    
    for (int i = 0; i < sequence.getNumEvents(); ++i) {
        auto* event = sequence.getEventPointer(i);
        const auto& msg = event->message;
        
        if (msg.isNoteOn()) {
            // Store note-on info
            activeNotes[msg.getNoteNumber()] = {
                event->message.getTimeStamp(),
                MidiNote::fromMidiVelocity(msg.getVelocity())
            };
        }
        else if (msg.isNoteOff()) {
            auto it = activeNotes.find(msg.getNoteNumber());
            if (it != activeNotes.end()) {
                double startTime = it->second.first;
                float velocity = it->second.second;
                double endTime = event->message.getTimeStamp();
                double lengthSeconds = endTime - startTime;
                
                // Convert to beats
                double noteStartBeats = (startTime / 60.0) * tempo;
                double noteLengthBeats = juce::jmax(0.0625, (lengthSeconds / 60.0) * tempo);
                
                juce::ValueTree noteTree(ProjectState::ID_NOTE);
                noteTree.setProperty(ProjectState::PROP_ID, juce::Uuid().toString(), nullptr);
                noteTree.setProperty(ProjectState::PROP_PITCH, msg.getNoteNumber(), nullptr);
                noteTree.setProperty(ProjectState::PROP_START_BEATS, noteStartBeats, nullptr);
                noteTree.setProperty(ProjectState::PROP_LENGTH_BEATS, noteLengthBeats, nullptr);
                noteTree.setProperty(ProjectState::PROP_VELOCITY, velocity, nullptr);
                
                notesTree.appendChild(noteTree, nullptr);
                activeNotes.erase(it);
            }
        }
    }
    
    clipTree.appendChild(notesTree, nullptr);
    
    // Add clip to track
    juce::ValueTree trackTree = projectState.getTrackById(trackId);
    if (!trackTree.isValid())
        return false;
    
    juce::ValueTree clipsTree = trackTree.getChildWithName(ProjectState::ID_CLIPS);
    if (!clipsTree.isValid()) {
        clipsTree = juce::ValueTree(ProjectState::ID_CLIPS);
        trackTree.appendChild(clipsTree, nullptr);
    }
    
    clipsTree.appendChild(clipTree, nullptr);
    
    return true;
}

juce::MidiMessageSequence MidiCaptureBuffer::getCapturedSequence(
    juce::int64 fromSamplePosition,
    juce::int64 toSamplePosition,
    int filterTrackIndex) const {
    
    const juce::ScopedLock lock(messageLock_);
    
    juce::MidiMessageSequence sequence;
    
    // Use last stop position if fromSamplePosition not specified
    juce::int64 startPos = fromSamplePosition >= 0 
        ? fromSamplePosition 
        : lastStopPosition_.load();
    
    // Read all events from the ring buffer
    int numReady = fifo_.getNumReady();
    if (numReady == 0)
        return sequence;
    
    int start1, size1, start2, size2;
    // Note: We need a non-const FIFO for prepareToRead, so we work with a copy
    // of the relevant data. This is message-thread only, so it's safe.
    auto& nonConstFifo = const_cast<juce::AbstractFifo&>(fifo_);
    nonConstFifo.prepareToRead(numReady, start1, size1, start2, size2);
    
    double referenceTime = static_cast<double>(startPos) / sampleRate_;
    
    auto processEntries = [&](int start, int count) {
        for (int i = 0; i < count; ++i) {
            const auto& entry = buffer_[static_cast<size_t>(start + i)];
            
            // Filter by position
            if (entry.samplePosition < startPos)
                continue;
            if (toSamplePosition >= 0 && entry.samplePosition > toSamplePosition)
                continue;
            
            // Filter by track
            if (filterTrackIndex >= 0 && entry.trackIndex != filterTrackIndex)
                continue;
            
            // Convert sample position to time (relative to start)
            double eventTime = static_cast<double>(entry.samplePosition) / sampleRate_ - referenceTime;
            
            auto msgWithTime = entry.message;
            msgWithTime.setTimeStamp(eventTime);
            sequence.addEvent(msgWithTime);
        }
    };
    
    if (size1 > 0) processEntries(start1, size1);
    if (size2 > 0) processEntries(start2, size2);
    
    // Don't actually consume the data - just peek
    nonConstFifo.finishedRead(0);
    
    sequence.sort();
    sequence.updateMatchedPairs();
    
    return sequence;
}

//==============================================================================
// Transport Integration
//==============================================================================

void MidiCaptureBuffer::markStopPosition(juce::int64 samplePosition) {
    lastStopPosition_.store(samplePosition);
}

//==============================================================================
// Buffer Management
//==============================================================================

void MidiCaptureBuffer::clear() {
    const juce::ScopedLock lock(messageLock_);
    
    // Read and discard all entries
    int numReady = fifo_.getNumReady();
    if (numReady > 0) {
        int start1, size1, start2, size2;
        fifo_.prepareToRead(numReady, start1, size1, start2, size2);
        fifo_.finishedRead(numReady);
    }
    
    lastStopPosition_.store(0);
    droppedMessages_.store(0);
}

//==============================================================================
// Internal Methods
//==============================================================================

void MidiCaptureBuffer::pruneOldEvents(juce::int64 currentSamplePosition) {
    // Calculate the oldest sample position to keep
    juce::int64 bufferDurationSamples = static_cast<juce::int64>(
        bufferDurationSeconds_.load() * sampleRate_);
    juce::int64 oldestToKeep = currentSamplePosition - bufferDurationSamples;
    
    if (oldestToKeep <= 0)
        return;
    
    // Count how many old events to remove from the front
    int numReady = fifo_.getNumReady();
    if (numReady == 0)
        return;
    
    int start1, size1, start2, size2;
    fifo_.prepareToRead(numReady, start1, size1, start2, size2);
    
    int numToRemove = 0;
    
    // Check events in first segment
    for (int i = 0; i < size1; ++i) {
        if (buffer_[static_cast<size_t>(start1 + i)].samplePosition < oldestToKeep) {
            ++numToRemove;
        } else {
            break; // Events are time-ordered, so stop at first valid one
        }
    }
    
    // If we removed all of size1, check size2
    if (numToRemove == size1) {
        for (int i = 0; i < size2; ++i) {
            if (buffer_[static_cast<size_t>(start2 + i)].samplePosition < oldestToKeep) {
                ++numToRemove;
            } else {
                break;
            }
        }
    }
    
    // Actually remove the old events
    fifo_.finishedRead(numToRemove);
}

} // namespace zenith
