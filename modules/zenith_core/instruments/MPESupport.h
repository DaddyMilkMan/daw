/*
    MPE (MIDI Polyphonic Expression) Support for Zenith Ultra Synth
    Features: Per-note pitch bend, timbre, pressure, velocity curves, release velocity
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>
#include <unordered_map>

namespace zenith {

//==============================================================================
// MPE Note State
//==============================================================================

struct MPENote {
    int noteNumber = -1;
    int midiChannel = 0;
    float velocity = 0.0f;
    float pressure = 0.0f;        // Channel pressure (0-1)
    float timbre = 0.5f;          // Slide/bend LSB (0-1)
    float pitchBend = 0.0f;       // -1 to +1 (normalized)

    // Per-note smoothing
    float smoothedPitch = 0.0f;
    float smoothedTimbre = 0.5f;
    float smoothedPressure = 0.0f;

    // Release velocity
    float releaseVelocity = 0.0f;

    // Note state
    bool isActive = false;
    bool isSustained = false;

    // Timing
    juce::uint32 startTime = 0;
    juce::uint32 lastUpdateTime = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MPENote)
};

//==============================================================================
// Velocity Curve
//==============================================================================

class VelocityCurve {
public:
    enum class Type {
        Linear,
        Soft,
        Hard,
        Exponential,
        Logarithmic,
        Custom
    };

    VelocityCurve() = default;

    void setType(Type type) {
        type_ = type;
    }

    void setAmount(float amount) {
        amount_ = juce::jlimit(0.0f, 1.0f, amount);
    }

    void setCustomCurve(const std::vector<float>& curve) {
        customCurve_ = curve;
    }

    float process(float velocity) const {
        // Input is 0-127 (MIDI velocity)
        float normalized = velocity / 127.0f;

        switch (type_) {
            case Type::Linear:
                return normalized;

            case Type::Soft:
                // Compress low velocities, expand high
                return std::pow(normalized, 0.5f);

            case Type::Hard:
                // Expand low velocities, compress high
                return std::pow(normalized, 2.0f);

            case Type::Exponential:
                return (std::exp(normalized * 3.0f) - 1.0f) / (std::exp(3.0f) - 1.0f);

            case Type::Logarithmic:
                return std::log1p(normalized * 9.0f) / std::log1p(9.0f);

            case Type::Custom:
                if (!customCurve_.empty()) {
                    int index = juce::jmin(static_cast<int>(normalized * (customCurve_.size() - 1)),
                                        static_cast<int>(customCurve_.size() - 1));
                    return customCurve_[index];
                }
                return normalized;

            default:
                return normalized;
        }
    }

    juce::String getTypeName() const {
        switch (type_) {
            case Type::Linear: return "Linear";
            case Type::Soft: return "Soft";
            case Type::Hard: return "Hard";
            case Type::Exponential: return "Exponential";
            case Type::Logarithmic: return "Logarithmic";
            case Type::Custom: return "Custom";
            default: return "Unknown";
        }
    }

private:
    Type type_ = Type::Linear;
    float amount_ = 0.5f;
    std::vector<float> customCurve_;
};

//==============================================================================
// Release Velocity Curve
//==============================================================================

class ReleaseVelocityCurve {
public:
    ReleaseVelocityCurve() = default;

    void setSensitivity(float sensitivity) {
        sensitivity_ = juce::jlimit(0.0f, 1.0f, sensitivity);
    }

    void setInfluence(float influence) {
        influence_ = juce::jlimit(0.0f, 1.0f, influence);
    }

    float process(float releaseVelocity) const {
        float normalized = releaseVelocity / 127.0f;

        // Higher release velocity = shorter release time
        float baseInfluence = 1.0f - influence_;
        float velocityFactor = normalized * sensitivity_ * influence_;

        return baseInfluence + velocityFactor;
    }

private:
    float sensitivity_ = 0.5f;
    float influence_ = 0.5f;
};

//==============================================================================
// MPE Zone Configuration
//==============================================================================

class MPEZone {
public:
    enum class Zone {
        Lower,      // Channels 1-7 (typically)
        Upper,      // Channels 9-15 (typically)
        Omni,       // All channels
        Legacy      // Single channel mode
    };

    MPEZone() = default;

    void setZone(Zone zone) {
        zone_ = zone;
        updateChannelRange();
    }

    void setMasterChannel(int channel) {
        masterChannel_ = juce::jlimit(0, 15, channel);
    }

    void setLowerZoneChannels(int numChannels) {
        lowerZoneChannels_ = juce::jlimit(1, 15, numChannels);
        updateChannelRange();
    }

    void setUpperZoneChannels(int numChannels) {
        upperZoneChannels_ = juce::jlimit(1, 15, numChannels);
        updateChannelRange();
    }

    bool isNoteInZone(int noteNumber, int midiChannel) const {
        switch (zone_) {
            case Zone::Lower:
                return midiChannel >= 1 && midiChannel < 1 + lowerZoneChannels_;

            case Zone::Upper:
                return midiChannel >= (16 - upperZoneChannels_) && midiChannel < 16;

            case Zone::Omni:
                return true;

            case Zone::Legacy:
                return midiChannel == masterChannel_;

            default:
                return true;
        }
    }

    int getLowerZoneStart() const { return 1; }
    int getLowerZoneEnd() const { return 1 + lowerZoneChannels_; }
    int getUpperZoneStart() const { return 16 - upperZoneChannels_; }
    int getUpperZoneEnd() const { return 16; }

    int getMasterChannel() const { return masterChannel_; }

private:
    void updateChannelRange() {
        // Zone configuration is handled in isNoteInZone
    }

    Zone zone_ = Zone::Omni;
    int masterChannel_ = 0;
    int lowerZoneChannels_ = 7;
    int upperZoneChannels_ = 7;
};

//==============================================================================
// Per-Note MPE Processor
//==============================================================================

class MPENoteProcessor {
public:
    MPENoteProcessor() {
        setSampleRate(44100.0f);
    }

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        pitchSmoothing_.setSampleRate(sampleRate);
        timbreSmoothing_.setSampleRate(sampleRate);
        pressureSmoothing_.setSampleRate(sampleRate);
    }

    void setPitchBendRange(float semitones) {
        pitchBendRange_ = semitones;
    }

    void setSlideSmoothing(float timeMs) {
        slideSmoothingTime_ = timeMs;
        timbreSmoothing_.setSmoothingTime(timeMs);
    }

    void setPressureSmoothing(float timeMs) {
        pressureSmoothingTime_ = timeMs;
        pressureSmoothing_.setSmoothingTime(timeMs);
    }

    void setPitchBendSmoothing(float timeMs) {
        pitchBendSmoothingTime_ = timeMs;
        pitchSmoothing_.setSmoothingTime(timeMs);
    }

    // Process a single note's MPE data
    void processNote(MPENote& note) {
        // Smooth pitch bend
        pitchSmoothing_.setTargetValue(note.pitchBend);
        note.smoothedPitch = pitchSmoothing_.getNextValue();

        // Smooth timbre (slide)
        timbreSmoothing_.setTargetValue(note.timbre);
        note.smoothedTimbre = timbreSmoothing_.getNextValue();

        // Smooth pressure
        pressureSmoothing_.setTargetValue(note.pressure);
        note.smoothedPressure = pressureSmoothing_.getNextValue();
    }

    // Get modulated values for synthesis
    float getPitchModulation(const MPENote& note) const {
        return note.smoothedPitch * pitchBendRange_;
    }

    float getTimbreModulation(const MPENote& note) const {
        return note.smoothedTimbre * 2.0f - 1.0f;  // Convert to -1 to 1
    }

    float getPressureModulation(const MPENote& note) const {
        return note.smoothedPressure;
    }

private:
    struct SmoothedValue {
        float setSampleRate(float sr) { sampleRate_ = sr; return 0.0f; }
        void setSmoothingTime(float timeMs) {
            float timeSeconds = timeMs * 0.001f;
            coef_ = 1.0f - std::exp(-1.0f / (timeSeconds * sampleRate_));
        }
        void setTargetValue(float target) { target_ = target; }
        float getNextValue() {
            current_ += coef_ * (target_ - current_);
            return current_;
        }

        float sampleRate_ = 44100.0f;
        float current_ = 0.0f;
        float target_ = 0.0f;
        float coef_ = 1.0f;
    };

    float sampleRate_ = 44100.0f;
    float pitchBendRange_ = 48.0f;  // ±4 octaves default for MPE
    float slideSmoothingTime_ = 20.0f;
    float pressureSmoothingTime_ = 10.0f;
    float pitchBendSmoothingTime_ = 30.0f;

    SmoothedValue pitchSmoothing_;
    SmoothedValue timbreSmoothing_;
    SmoothedValue pressureSmoothing_;
};

//==============================================================================
// MPE Manager
//==============================================================================

class MPEManager {
public:
    MPEManager() {
        setSampleRate(44100.0f);
    }

    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        for (auto& processor : noteProcessors_) {
            processor.setSampleRate(sampleRate);
        }
    }

    // Zone configuration
    void setZone(const MPEZone& zone) {
        zone_ = zone;
    }

    const MPEZone& getZone() const { return zone_; }

    // Velocity curves
    void setVelocityCurveType(VelocityCurve::Type type) {
        velocityCurve_.setType(type);
    }

    void setReleaseVelocitySensitivity(float sensitivity) {
        releaseVelocityCurve_.setSensitivity(sensitivity);
    }

    void setReleaseVelocityInfluence(float influence) {
        releaseVelocityCurve_.setInfluence(influence);
    }

    // Note management
    MPENote* noteOn(int noteNumber, int midiChannel, float velocity) {
        // Find free slot
        for (auto& note : notes_) {
            if (!note.isActive) {
                note.noteNumber = noteNumber;
                note.midiChannel = midiChannel;
                note.velocity = velocityCurve_.process(velocity);
                note.pressure = 0.0f;
                note.timbre = 0.5f;
                note.pitchBend = 0.0f;
                note.smoothedPitch = 0.0f;
                note.smoothedTimbre = 0.5f;
                note.smoothedPressure = 0.0f;
                note.isActive = true;
                note.isSustained = false;
                note.startTime = juce::Time::getMillisecondCounter();
                note.lastUpdateTime = note.startTime;

                activeNotes_[noteNumber] = &note;
                return &note;
            }
        }
        return nullptr;
    }

    void noteOff(int noteNumber, float releaseVelocity) {
        if (auto* note = activeNotes_[noteNumber]) {
            note->releaseVelocity = releaseVelocityCurve_.process(releaseVelocity);
            note->isActive = false;

            // If sustain is active, mark as sustained
            if (sustainPedal_ > 0.5f) {
                note->isSustained = true;
            } else {
                activeNotes_.erase(noteNumber);
            }
        }
    }

    void allNotesOff() {
        for (auto& note : notes_) {
            note.isActive = false;
            note.isSustained = false;
        }
        activeNotes_.clear();
    }

    // MPE Messages
    void setPitchBend(int midiChannel, float value) {
        // value is -1 to 1
        for (auto& note : notes_) {
            if (note.isActive && note.midiChannel == midiChannel) {
                note.pitchBend = value;
                note.lastUpdateTime = juce::Time::getMillisecondCounter();
            }
        }
    }

    void setTimbre(int midiChannel, float value) {
        // value is 0 to 1
        for (auto& note : notes_) {
            if (note.isActive && note.midiChannel == midiChannel) {
                note.timbre = value;
                note.lastUpdateTime = juce::Time::getMillisecondCounter();
            }
        }
    }

    void setPressure(int midiChannel, float value) {
        // value is 0 to 1
        for (auto& note : notes_) {
            if (note.isActive && note.midiChannel == midiChannel) {
                note.pressure = value;
                note.lastUpdateTime = juce::Time::getMillisecondCounter();
            }
        }
    }

    // Sustain pedal
    void setSustainPedal(float value) {
        sustainPedal_ = value;

        // If pedal released, kill sustained notes
        if (value < 0.5f) {
            for (auto& note : notes_) {
                if (note.isSustained) {
                    note.isSustained = false;
                    note.isActive = false;
                    activeNotes_.erase(note.noteNumber);
                }
            }
        }
    }

    // Sostenuto pedal
    void setSostenutoPedal(float value) {
        sostenutoPedal_ = value;
    }

    // Process all active notes
    void process() {
        for (auto& note : notes_) {
            if (note.isActive) {
                int processorIndex = note.noteNumber % maxProcessors_;
                noteProcessors_[processorIndex].processNote(note);
            }
        }
    }

    // Get note by MIDI note number
    MPENote* getNote(int noteNumber) {
        return activeNotes_[noteNumber];
    }

    // Get all active notes
    std::vector<MPENote*> getActiveNotes() {
        std::vector<MPENote*> active;
        for (auto& note : notes_) {
            if (note.isActive) {
                active.push_back(&note);
            }
        }
        return active;
    }

    // Get note count
    int getActiveNoteCount() const {
        return static_cast<int>(activeNotes_.size());
    }

    // Visualization data
    struct MPEVisualizationData {
        struct NoteData {
            int noteNumber;
            float pitch;
            float timbre;
            float pressure;
            juce::Colour color;
        };

        std::vector<NoteData> notes;
        float masterPitchBend = 0.0f;
        float masterPressure = 0.0f;
    };

    MPEVisualizationData getVisualizationData() const {
        MPEVisualizationData data;

        for (const auto& pair : activeNotes_) {
            const MPENote* note = pair.second;
            MPEVisualizationData::NoteData noteData;
            noteData.noteNumber = note->noteNumber;
            noteData.pitch = note->smoothedPitch;
            noteData.timbre = note->smoothedTimbre;
            noteData.pressure = note->smoothedPressure;
            noteData.color = getNoteColor(*note);
            data.notes.push_back(noteData);
        }

        return data;
    }

    // Reset
    void reset() {
        for (auto& note : notes_) {
            note.isActive = false;
            note.isSustained = false;
        }
        activeNotes_.clear();
        sustainPedal_ = 0.0f;
        sostenutoPedal_ = 0.0f;
    }

private:
    juce::Colour getNoteColor(const MPENote& note) const {
        // Color based on MPE data
        float hue = note.smoothedTimbre;
        float saturation = note.smoothedPressure;
        float brightness = 0.5f + note.smoothedPitch * 0.5f;

        return juce::Colour::fromHSV(hue, saturation, brightness, 1.0f);
    }

    float sampleRate_ = 44100.0f;
    MPEZone zone_;
    VelocityCurve velocityCurve_;
    ReleaseVelocityCurve releaseVelocityCurve_;

    static constexpr int maxNotes_ = 256;
    static constexpr int maxProcessors_ = 16;

    std::array<MPENote, maxNotes_> notes_;
    std::unordered_map<int, MPENote*> activeNotes_;
    std::array<MPENoteProcessor, maxProcessors_> noteProcessors_;

    float sustainPedal_ = 0.0f;
    float sostenutoPedal_ = 0.0f;
};

//==============================================================================
// MPE Presets
//==============================================================================

class MPEPresets {
public:
    struct Preset {
        juce::String name;
        VelocityCurve::Type velocityCurve;
        float pitchBendRange;
        float slideSmoothing;
        float pressureSmoothing;
        juce::String description;
    };

    static std::vector<Preset> getPresets() {
        return {
            {"Default", VelocityCurve::Type::Linear, 48.0f, 20.0f, 10.0f,
             "Standard MPE configuration"},

            {"Expressive", VelocityCurve::Type::Soft, 96.0f, 30.0f, 5.0f,
             "Highly expressive with large pitch bend range"},

            {"Subtle", VelocityCurve::Type::Hard, 24.0f, 10.0f, 20.0f,
             "Subtle expression, hard velocity curve"},

            {"Violin", VelocityCurve::Type::Exponential, 12.0f, 50.0f, 15.0f,
             "Emulates violin-style expression"},

            {"Guitar", VelocityCurve::Type::Linear, 2.0f, 5.0f, 30.0f,
             "Guitar-style bend and vibrato"},

            {"Brass", VelocityCurve::Type::Soft, 12.0f, 25.0f, 20.0f,
             "Brass-style swells and expression"}
        };
    }

    static juce::StringArray getPresetNames() {
        auto presets = getPresets();
        juce::StringArray names;
        for (const auto& preset : presets) {
            names.add(preset.name);
        }
        return names;
    }

    static Preset getPreset(const juce::String& name) {
        auto presets = getPresets();
        for (const auto& preset : presets) {
            if (preset.name == name) {
                return preset;
            }
        }
        return getPresets()[0];  // Return default
    }
};

} // namespace zenith
