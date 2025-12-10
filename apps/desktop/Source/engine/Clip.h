/*
  ==============================================================================

    Clip.h
    Ported from: ZenithDAW-Native/Source/Audio/Clip.h (2025-11-11)
    Author:  Zenith DAW → Zenith DAW

    Audio/MIDI clip with transport synchronization and playback control

    CANONICAL IMPLEMENTATION: This supersedes
  VexelDAW-Native/Source/Audio/Clip.* Phase 1.4 implementation (2025-11-13)

    Key Design: Playhead-driven timing (parameter-based) instead of internal
    transportPosition member. Integrates with AudioFilePool for RT-safe buffer
  access.

    JUCE 8 / C++20 adaptations:
    - Wrapped in namespace zenith
    - Kept as Track::Clip (nested class)
    - No container changes needed (uses std::unique_ptr internally)

  ==============================================================================
*/

#pragma once

#include "Track.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <atomic>


namespace zenith {

// Forward declarations
class AudioFilePool;

//==============================================================================
/**
 * @brief MIDI note specification (Phase 8)
 * Matches ProjectState::MidiNoteSpec for compatibility
 */
struct MidiNoteSpec {
  juce::String id;    // Unique note ID
  int pitch;          // MIDI note number (0-127)
  double startBeats;  // Start time in beats (relative to clip start)
  double lengthBeats; // Duration in beats
  int velocity;       // Note velocity (0-127)
  bool muted;         // Muted flag

  MidiNoteSpec()
      : pitch(60), startBeats(0.0), lengthBeats(1.0), velocity(100),
        muted(false) {}
};

//==============================================================================
/**
    Represents an audio or MIDI clip on the timeline.

    Each clip has a position on the timeline, a length, and can contain either
    audio samples or MIDI events. Clips support fade in/out, looping, and
    time offset for trimming.

    Thread-safe design allows clips to be modified from the UI thread while
    playing back on the audio thread.
*/
class Track::Clip : public juce::AudioSource {
public:
  //==============================================================================
  enum class Type { Audio, MIDI };

  //==============================================================================
  Clip();
  ~Clip() override;

  //==============================================================================
  // AudioSource interface
  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
  void releaseResources() override;
  void
  getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override;

  //==============================================================================
  // Clip properties
  Type getType() const { return clipType; }
  void setType(Type type) { clipType = type; }

  const juce::String &getName() const { return clipName; }
  void setName(const juce::String &name) { clipName = name; }

  //==============================================================================
  // Timeline position (in samples)
  void setStartPosition(int64_t position);
  int64_t getStartPosition() const { return startPosition.load(); }

  void setLength(int64_t lengthInSamples);
  int64_t getLength() const { return clipLength.load(); }

  int64_t getEndPosition() const {
    return startPosition.load() + clipLength.load();
  }

  // Offset within the source material (for trimming)
  void setOffset(int64_t offsetInSamples);
  int64_t getOffset() const { return clipOffset.load(); }

  //==============================================================================
  // Transport control
  void setTransportPosition(int64_t position);
  int64_t getTransportPosition() const { return transportPosition.load(); }

  void setPlaying(bool shouldPlay);
  bool isPlaying() const { return playing.load(); }

  // Check if clip is active at given playhead position (Phase 1.3: uses Engine
  // playhead)
  bool isActiveAt(int64_t playheadSamples) const;

  // Legacy: check if clip is active at stored transport position
  bool isActive() const;

  //==============================================================================
  // Audio clip specific

  // Phase 1.2: Use AudioFilePool for RT-safe file access
  void setAudioFileFromPool(const juce::File &file,
                            zenith::AudioFilePool &pool);

  // Legacy method (deprecated - loads file directly without pool)
  void setAudioFile(const juce::File &file);
  juce::File getAudioFile() const { return audioFile; }

  void setAudioBuffer(const juce::AudioBuffer<float> &buffer);
  const juce::AudioBuffer<float> *getAudioBuffer() const {
    return &audioBuffer;
  }

  //==============================================================================
  // MIDI clip specific
  void setMidiSequence(const juce::MidiMessageSequence &sequence);
  const juce::MidiMessageSequence *getMidiSequence() const {
    return &midiSequence;
  }

  /**
   * @brief Rebuild MIDI sequence from note specifications (Phase 8)
   * @param notes Array of note specs from ProjectState
   * @param clipStartBeats Clip start time in beats (for absolute positioning)
   * @param tempo Project tempo (for beat-to-time conversion)
   *
   * This method should be called from the message thread whenever MIDI notes
   * change. It rebuilds the cached midiSequence that the audio thread reads.
   */
  void buildMidiSequenceFromNotes(const juce::Array<MidiNoteSpec> &notes,
                                  double clipStartBeats, double tempo);

  /**
   * Extract MIDI events for the current playback position into a MIDI buffer.
   * Used for routing MIDI to instrument plugins.
   *
   * @param midiBuffer The MIDI buffer to add events to
   * @param numSamples The number of samples in this block
   */
  void getMidiEvents(juce::MidiBuffer &midiBuffer, int numSamples);

  //==============================================================================
  // Fades (in samples)
  void setFadeIn(int64_t fadeInSamples);
  int64_t getFadeIn() const { return fadeInLength.load(); }

  void setFadeOut(int64_t fadeOutSamples);
  int64_t getFadeOut() const { return fadeOutLength.load(); }
  
  void setFadeCurve(float curve) { fadeCurve.store(curve); }
  float getFadeCurve() const { return fadeCurve.load(); }

  //==============================================================================
  // Gain control
  void setGain(float newGain);
  float getGain() const { return gain.load(); }

  //==============================================================================
  // Looping
  void setLooping(bool shouldLoop);
  bool isLooping() const { return looping.load(); }

  //==============================================================================
  // Color for visual representation
  void setColor(juce::Colour color);
  juce::Colour getColor() const { return clipColor; }

  //==============================================================================
  // Time Stretching
  void setPlaybackRate(double rate);
  double getPlaybackRate() const;
  
  void setPreservePitch(bool preserve);
  bool isPreservingPitch() const;

  //==============================================================================
  // State management
  juce::ValueTree getState() const;
  void loadState(const juce::ValueTree &state);

  //==============================================================================
  // Allow Track to access processing methods
  //==============================================================================
  friend class Track;

private:
  //==============================================================================
  // Clip properties
  Type clipType = Type::Audio;
  juce::String clipName{"Clip"};
  juce::Colour clipColor{juce::Colours::blue};

  //==============================================================================
  // Timeline position (atomic for lock-free access)
  std::atomic<int64_t> startPosition{0};
  std::atomic<int64_t> clipLength{0};
  std::atomic<int64_t> clipOffset{0};
  std::atomic<int64_t> transportPosition{0};

  //==============================================================================
  // Playback state (atomic for lock-free access)
  std::atomic<bool> playing{false};
  std::atomic<int64_t> fadeInLength{0};
  std::atomic<int64_t> fadeOutLength{0};
  std::atomic<float> fadeCurve{0.5f}; // 0.5 = Linear
  std::atomic<float> gain{1.0f};
  std::atomic<bool> looping{false};

  //==============================================================================
  // Audio data
  juce::File audioFile;
  juce::AudioBuffer<float> audioBuffer; // Legacy: for setAudioBuffer()
  juce::CriticalSection audioLock;

  // Legacy audio source (required for setAudioFile)
  std::unique_ptr<juce::AudioFormatReaderSource> audioSource;

  // Phase 1.2: AudioFilePool handle (RT-safe shared ownership)
  std::shared_ptr<const void>
      audioFileHandle_; // Type-erased to avoid forward decl issues

  //==============================================================================
  // MIDI data
  juce::MidiMessageSequence midiSequence;
  juce::CriticalSection midiLock;

  //==============================================================================
  // Time Stretching State
  std::atomic<double> playbackRate_{1.0};
  std::atomic<bool> preservePitch_{false};
  
  // WSOLA State
  static constexpr int kWsolaWindowSize = 1024;
  std::vector<float> wsolaWindow_;
  std::vector<float> wsolaOutputBuffer_;
  double readPosition_ = 0.0; // Fractional read position for interpolation

  //==============================================================================
  // Processing state
  double currentSampleRate = 44100.0;
  int currentBlockSize = 512;

  //==============================================================================
  // Helper methods
  // Phase 1.3: Process audio clip with explicit playhead position
  void processAudioClip(const juce::AudioSourceChannelInfo &bufferToFill,
                        int64_t playheadSamples);

  // Phase 2A: Process MIDI clip with explicit playhead position
  void processMidiClip(juce::MidiBuffer &midiBuffer, int64_t playheadSamples,
                       int numSamples);

  // Legacy overloads (use internal transportPosition)
  void processAudioClip(const juce::AudioSourceChannelInfo &bufferToFill);
  void processMidiClip(const juce::AudioSourceChannelInfo &bufferToFill,
                       int64_t playheadSamples);
  void processMidiClip(const juce::AudioSourceChannelInfo &bufferToFill);

  float calculateFadeMultiplier(int64_t positionInClip) const;

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Clip)
};

} // namespace zenith
