/**
 * Track.h
 * Base class for all track types in the DAW
 */

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include "Clip.h"

namespace zenith {

enum class TrackType {
    Audio,
    MIDI,
    Instrument,
    Bus,
    Return
};

/**
 * Abstract base class for tracks
 */
class Track
{
public:
    Track(int id, const juce::String& name, TrackType type)
        : m_id(id), m_name(name), m_type(type)
    {}

    virtual ~Track() = default;

    // Identification
    int getId() const { return m_id; }
    const juce::String& getName() const { return m_name; }
    void setName(const juce::String& name) { m_name = name; }
    TrackType getType() const { return m_type; }

    // Mixing parameters
    void setVolume(float volumeDb) { m_volumeDb = volumeDb; }
    float getVolume() const { return m_volumeDb; }

    void setPan(float pan) { m_pan = juce::jlimit(-1.0f, 1.0f, pan); }
    float getPan() const { return m_pan; }

    void setMute(bool muted) { m_muted = muted; }
    bool isMuted() const { return m_muted; }

    void setSolo(bool solo) { m_solo = solo; }
    bool isSoloed() const { return m_solo; }

    // Processing
    virtual void prepare(double sampleRate, int maximumBlockSize) {
        m_sampleRate = sampleRate;
        m_maximumBlockSize = maximumBlockSize;
    }

    virtual void process(const float* const* input, float* const* output,
                        int numInputs, int numOutputs, int numSamples,
                        double currentBeat) = 0;

protected:
    int m_id;
    juce::String m_name;
    TrackType m_type;

    float m_volumeDb = 0.0f;
    float m_pan = 0.0f;
    bool m_muted = false;
    bool m_solo = false;

    double m_sampleRate = 44100.0;
    int m_maximumBlockSize = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};

/**
 * Audio track - handles audio clips and audio plugin chains
 */
class AudioTrack : public Track
{
public:
    AudioTrack(int id, const juce::String& name)
        : Track(id, name, TrackType::Audio)
    {}

    void process(const float* const* input, float* const* output,
                int numInputs, int numOutputs, int numSamples,
                double currentBeat) override;
};

/**
 * MIDI track - handles MIDI clips and instrument plugins
 */
class MidiTrack : public Track
{
public:
    MidiTrack(int id, const juce::String& name)
        : Track(id, name, TrackType::MIDI)
    {}

    void process(const float* const* input, float* const* output,
                int numInputs, int numOutputs, int numSamples,
                double currentBeat) override;
};

} // namespace zenith
