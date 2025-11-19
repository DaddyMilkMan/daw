/**
 * MidiEvent.h
 * MIDI event representation and utilities
 */

#pragma once

#include <JuceHeader.h>

namespace zenith {

/**
 * Extended MIDI event with timing and metadata
 */
struct MidiEvent {
    juce::MidiMessage message;
    int samplePosition = 0;      // Position within audio buffer
    double beatPosition = 0.0;   // Position in musical time
    int trackId = 0;             // Which track this event belongs to
    juce::String sourceName;     // Input device name

    MidiEvent() = default;

    MidiEvent(const juce::MidiMessage& msg, int sample, double beat)
        : message(msg), samplePosition(sample), beatPosition(beat)
    {}

    // Comparison for sorting by time
    bool operator<(const MidiEvent& other) const {
        return beatPosition < other.beatPosition;
    }
};

/**
 * MIDI event buffer optimized for musical timeline
 */
class MidiEventSequence
{
public:
    void addEvent(const MidiEvent& event) {
        m_events.push_back(event);
    }

    void addEvent(const juce::MidiMessage& msg, double beatPosition) {
        m_events.emplace_back(msg, 0, beatPosition);
    }

    void clear() {
        m_events.clear();
    }

    void sort() {
        std::sort(m_events.begin(), m_events.end());
    }

    const std::vector<MidiEvent>& getEvents() const {
        return m_events;
    }

    std::vector<MidiEvent>& getEvents() {
        return m_events;
    }

    size_t size() const {
        return m_events.size();
    }

    bool empty() const {
        return m_events.empty();
    }

    /**
     * Get all events in a beat range
     */
    std::vector<MidiEvent> getEventsInRange(double startBeat, double endBeat) const {
        std::vector<MidiEvent> result;

        for (const auto& event : m_events) {
            if (event.beatPosition >= startBeat && event.beatPosition < endBeat) {
                result.push_back(event);
            }
        }

        return result;
    }

private:
    std::vector<MidiEvent> m_events;
};

} // namespace zenith
