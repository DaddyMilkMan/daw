/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

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
#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>
#include "MidiNote.h"

namespace zenith {

// Forward declarations
class AudioFilePool;

//==============================================================================
using MidiNoteSpec = zenith::MidiNote;

//==============================================================================
/**
    Represents an audio or MIDI clip on the timeline.

    Each clip has a position on the timeline, a length, and can contain either
    audio samples or MIDI events. Clips support fade in/out, looping, and
    time offset for trimming.

    Thread-safe design allows clips to be modified from the UI thread while
    playing back on the audio thread.

    ## Ownership Model (to prevent shared_ptr cycles):

    **ClipTrack -> Clip:** ClipTrack owns Clip via std::unique_ptr
    **Clip -> Track:** No back-reference stored (track passed by parameter when
   needed)
    **Clip -> AudioFilePool:** Uses shared_ptr<const void> for RT-safe handle
   (no cycle)

    @note Clip should NEVER hold std::shared_ptr<Track> or
   std::shared_ptr<ClipTrack>
*/
class Clip : public juce::AudioSource {
public:
  //==============================================================================
  /**
   * @brief Enumeration of clip types.
   */
  enum class Type {
    Audio, /**< Audio clip containing waveform data */
    MIDI   /**< MIDI clip containing note data */
  };

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
  /**
   * @brief Sets the start position of the clip on the timeline.
   * @param position Position in samples.
   */
  void setStartPosition(int64_t position);

  /**
   * @brief Gets the start position of the clip.
   * @return Position in samples.
   */
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
  /**
   * @brief Sets the audio file for this clip using the AudioFilePool.
   *
   * This is the preferred method for loading audio files as it ensures
   * thread-safe access to the file handle.
   *
   * @param file The audio file to load.
   * @param pool Reference to the AudioFilePool to use for loading.
   */
  void setAudioFileFromPool(const juce::File &file,
                            zenith::AudioFilePool &pool);

  // Legacy method (deprecated - loads file directly without pool)
  void setAudioFile(const juce::File &file);
  juce::File getAudioFile() const { return audioFile; }

  void setAudioBuffer(const juce::AudioBuffer<float> &buffer);
  const juce::AudioBuffer<float> *getAudioBuffer() const {
    return &audioBuffer;
  }

  /**
   * @brief Extract a range of audio samples from the clip.
   * @param destBuffer Buffer to fill.
   * @param startSampleInClip Start position relative to clip start (0 = clip start).
   * @param numSamples Number of samples to extract.
   */
  void getAudioSamples(juce::AudioBuffer<float>& destBuffer, int64_t startSampleInClip, int numSamples) const;

  //==============================================================================
  // MIDI clip specific
  void setMidiSequence(const juce::MidiMessageSequence &sequence);
  const juce::MidiMessageSequence *getMidiSequence() const {
    auto seq = midiSequence_.load(std::memory_order_acquire);
    return seq.get();
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
  friend class AudioTrack;
  friend class InstrumentTrack;

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
  // MESSAGE THREAD ONLY - Protects file/buffer swapping on message thread.
  // Audio thread access is through audioFileHandle_ (atomic).
  juce::CriticalSection audioLock;

  // Legacy audio source (required for setAudioFile)
  std::unique_ptr<juce::AudioFormatReaderSource> audioSource;

  // Phase 1.2: AudioFilePool handle (RT-safe shared ownership)
  std::shared_ptr<const void>
      audioFileHandle_; // Type-erased to avoid forward decl issues

  //==============================================================================
  // MIDI data
  std::atomic<std::shared_ptr<const juce::MidiMessageSequence>> midiSequence_;
  // MESSAGE THREAD ONLY - Protects MIDI sequence updates.
  // Audio thread access is through midiSequence_ (atomic).
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

  void applyFadesSIMD(const juce::AudioSourceChannelInfo &bufferToFill,
                      int destOffset,
                      int64_t startPositionInClip, int numSamples);

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Clip)
};

} // namespace zenith
