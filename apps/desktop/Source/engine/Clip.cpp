/*
  ==============================================================================

    Clip.cpp
    Ported from: ZenithDAW-Native/Source/Audio/Clip.cpp (2025-11-11)
    Author:  Zenith DAW → Zenith DAW

    Audio/MIDI clip implementation

    JUCE 8 / C++20 adaptations:
    - Wrapped in namespace zenith
    - No container changes needed

  ==============================================================================
*/

#include "Clip.h"
#include "AudioFilePool.h"
#include "EngineConstants.h"
#include "Track.h"
#include <algorithm>
#include <cmath>

namespace zenith {

//==============================================================================
Clip::Clip() : midiSequence_(std::make_shared<juce::MidiMessageSequence>()), fadeInLength(0), fadeOutLength(0) {}

Clip::~Clip() { releaseResources(); }

// Move Constructor
Clip::Clip(Clip &&other) noexcept
    : clipName(std::move(other.clipName)),
      clipType(other.clipType),
      startPosition(other.startPosition.load()),
      clipLength(other.clipLength.load()),
      clipOffset(other.clipOffset.load()),
      transportPosition(other.transportPosition.load()),
      fadeInLength(other.fadeInLength.load()),
      fadeOutLength(other.fadeOutLength.load()),
      gain(other.gain.load()),
      playing(other.playing.load()),
      looping(other.looping.load()),
      mute(other.mute.load()),
      solo(other.solo.load()),
      clipColor(other.clipColor),
      audioFile(std::move(other.audioFile)),
      audioBuffer(std::move(other.audioBuffer)),
      audioSource(std::move(other.audioSource)),
      audioFileHandle_(std::move(other.audioFileHandle_)),
      midiSequence_(std::move(other.midiSequence_)),
      playbackRate_(other.playbackRate_.load()),
      preservePitch_(other.preservePitch_.load()),
      wsolaWindow_(std::move(other.wsolaWindow_)),
      wsolaOutputBuffer_(std::move(other.wsolaOutputBuffer_)) {
}

// Move Assignment Operator
Clip &Clip::operator=(Clip &&other) noexcept {
  if (this == &other)
    return *this;

  clipName = std::move(other.clipName);
  clipType = other.clipType;
  startPosition.store(other.startPosition.load());
  clipLength.store(other.clipLength.load());
  clipOffset.store(other.clipOffset.load());
  transportPosition.store(other.transportPosition.load());
  fadeInLength.store(other.fadeInLength.load());
  fadeOutLength.store(other.fadeOutLength.load());
  gain.store(other.gain.load());
  playing.store(other.playing.load());
  looping.store(other.looping.load());
  mute.store(other.mute.load());
  solo.store(other.solo.load());
  clipColor = other.clipColor;

  audioFile = std::move(other.audioFile);
  audioFileHandle_ = std::move(other.audioFileHandle_);
  midiSequence_ = std::move(other.midiSequence_);

  // Handover audio buffer (Efficient move)
  audioBuffer = std::move(other.audioBuffer);
  audioSource = std::move(other.audioSource);

  playbackRate_.store(other.playbackRate_.load());
  preservePitch_.store(other.preservePitch_.load());
  wsolaWindow_ = std::move(other.wsolaWindow_);
  wsolaOutputBuffer_ = std::move(other.wsolaOutputBuffer_);

  return *this;
}

//==============================================================================
void Clip::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
  // Roast Fix #5: Validate buffer size parameters
  jassert(samplesPerBlockExpected > 0 && samplesPerBlockExpected <= 8192);
  jassert(sampleRate > 0.0 && sampleRate <= 192000.0);

  // Log buffer size changes for debugging
  if (currentBlockSize != samplesPerBlockExpected && currentBlockSize > 0) {
    DBG("Clip::prepareToPlay - Buffer size changed from " +
        juce::String(currentBlockSize) + " to " +
        juce::String(samplesPerBlockExpected));
  }

  currentSampleRate = sampleRate;
  currentBlockSize = samplesPerBlockExpected;


}

void Clip::releaseResources() {

}

void Clip::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) {
  bufferToFill.clearActiveBufferRegion();

  if (!playing.load())
    return;

  // Check if ANY part of the buffer range overlaps with the clip
  // (processAudioClip handles the partial overlap logic)
  const int64_t transport = transportPosition.load();
  const int64_t bufferEnd = transport + bufferToFill.numSamples;
  const int64_t clipStart = startPosition.load();
  const int64_t clipEnd = clipStart + clipLength.load();
  
  // Early exit only if buffer is completely before or after clip
  if (bufferEnd <= clipStart || transport >= clipEnd)
    return;

  if (clipType == Type::Audio) {
    processAudioClip(bufferToFill, transport);
  } else if (clipType == Type::MIDI) {
    // MIDI clips produce silence in audio path (handled by clearActiveBufferRegion above)
  }
}

//==============================================================================
void Clip::setStartPosition(int64_t position) {
  // Bug 38: Clamp to valid range to prevent overflow
  startPosition.store(
      juce::jlimit(int64_t(0), constants::kMaxSamplePosition, position));
}

void Clip::setLength(int64_t lengthInSamples) {
  clipLength.store(juce::jmax(int64_t(0), lengthInSamples));
}

void Clip::setOffset(int64_t offsetInSamples) {
  clipOffset.store(juce::jmax(int64_t(0), offsetInSamples));
}

//==============================================================================
void Clip::setTransportPosition(int64_t position) {
  transportPosition.store(position);
}

void Clip::setPlaying(bool shouldPlay) { playing.store(shouldPlay); }

// Phase 1.3: Check if active at given playhead position
bool Clip::isActiveAt(int64_t playheadSamples) const {
  const int64_t start = startPosition.load();
  const int64_t end = start + clipLength.load();

  return playheadSamples >= start && playheadSamples < end;
}

// Legacy: Check if active at stored transport position
bool Clip::isActive() const {
  const int64_t pos = transportPosition.load();
  const int64_t start = startPosition.load();
  const int64_t end = start + clipLength.load();

  return pos >= start && pos < end;
}

//==============================================================================
// Phase 1.2: Set audio file using AudioFilePool (preferred method)
void Clip::setAudioFileFromPool(const juce::File &file, AudioFilePool &pool) {
  // Bug 72: Ensure File I/O happens on message thread
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
  const juce::ScopedLock sl(audioLock);

  audioFile = file;

  // Load file through pool (message thread, does I/O)
  juce::String error;
  auto handle = pool.loadFile(file, error);

  if (handle) {
    // Store handle (shared_ptr is RT-safe for read)
    audioFileHandle_ = handle;

    // Set clip length to match audio file
    clipLength.store(handle->lengthInSamples);

    DBG("Clip: Loaded " + file.getFileName() + " (" +
        juce::String(handle->lengthInSamples) + " samples)");
  } else {
    DBG("Clip: Failed to load " + file.getFileName() + ": " + error);
    audioFileHandle_ = nullptr;
  }



}


juce::File Clip::getAudioFile() const { return audioFile; }




void Clip::getAudioSamples(juce::AudioBuffer<float>& destBuffer, int64_t startSampleInClip, int numSamples) const {
    if (clipType != Type::Audio || numSamples <= 0) return;

    auto handlePtr = std::atomic_load_explicit(&audioFileHandle_, std::memory_order_acquire);
    auto handle = std::static_pointer_cast<const AudioFilePool::AudioFileHandle>(handlePtr);
    const juce::AudioBuffer<float>* sourceBuffer = nullptr;

    if (handle != nullptr && handle->isValid()) {
        sourceBuffer = &handle->buffer;
    } else {
        sourceBuffer = &audioBuffer; // Fallback to empty buffer
    }

    if (sourceBuffer == nullptr || sourceBuffer->getNumSamples() == 0) return;

    const int sourceLen = sourceBuffer->getNumSamples();
    const int64_t clipOff = clipOffset.load();
    int64_t sourcePos = startSampleInClip + clipOff;

    // Handle looping if necessary, or just clamp
    if (looping.load() && sourceLen > 0) {
        sourcePos = (sourcePos % sourceLen + sourceLen) % sourceLen;
    }

    const int samplesToCopy = juce::jmin(numSamples, (int)(sourceLen - sourcePos));
    if (samplesToCopy <= 0) return;

    for (int ch = 0; ch < juce::jmin(destBuffer.getNumChannels(), sourceBuffer->getNumChannels()); ++ch) {
        destBuffer.copyFrom(ch, 0, *sourceBuffer, ch, (int)sourcePos, samplesToCopy);
    }
}

//==============================================================================
void Clip::setMidiSequence(const juce::MidiMessageSequence &sequence) {
  auto newSequence = std::make_shared<juce::MidiMessageSequence>(sequence);

  // Bug 18: Fix memory ordering mismatch - lock not needed for atomic store
  // with release const juce::ScopedLock sl(midiLock); // Redundant with atomic
  // store
  midiSequence_.store(newSequence, std::memory_order_release);

  if (newSequence->getNumEvents() > 0) {
    const double lastEventTime = newSequence->getEndTime();
    const int64_t lengthInSamples =
        static_cast<int64_t>(lastEventTime * currentSampleRate);
    clipLength.store(lengthInSamples);
  }
}

void Clip::buildMidiSequenceFromNotes(const juce::Array<MidiNoteSpec> &notes,
                                      double clipStartBeats, double tempo) {
  // This method must be called from the message thread only
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  // Create a new MIDI sequence
  juce::MidiMessageSequence newSequence;

  // Convert tempo to seconds per beat
  const double secondsPerBeat = 60.0 / std::max(0.1, tempo);

  // Build note-on and note-off events for each note
  for (const auto &note : notes) {
    // Skip muted notes
    if (note.muted)
      continue;

    // Convert beat times to seconds (relative to clip start)
    const double noteStartSeconds = note.startBeats * secondsPerBeat;
    const double noteEndSeconds =
        (note.startBeats + note.lengthBeats) * secondsPerBeat;

    // Validate pitch and velocity
    const int pitch = juce::jlimit(0, 127, (int)note.pitch);
    const int velocity = juce::jlimit(0, 127, (int)note.velocity);

    // Create note-on message
    juce::MidiMessage noteOn =
        juce::MidiMessage::noteOn(1, pitch, static_cast<juce::uint8>(velocity));
    noteOn.setTimeStamp(noteStartSeconds);
    newSequence.addEvent(noteOn);

    // Create note-off message
    juce::MidiMessage noteOff =
        juce::MidiMessage::noteOff(1, pitch, static_cast<juce::uint8>(0));
    noteOff.setTimeStamp(noteEndSeconds);
    newSequence.addEvent(noteOff);
  }

  // Sort events by timestamp
  newSequence.updateMatchedPairs();

  // Replace the current sequence (thread-safe via atomic store)
  auto sharedSeq = std::make_shared<juce::MidiMessageSequence>(newSequence);

  // RT-safe: Atomic store with release semantics is sufficient
  midiSequence_.store(sharedSeq, std::memory_order_release);

  // Update clip length based on the last MIDI event
  if (sharedSeq->getNumEvents() > 0) {
    const double lastEventTime = sharedSeq->getEndTime();
    // MIDI events are stored in beats, but length is in samples
    // We'll use a conservative default tempo if one isn't available
    const double rate = currentSampleRate > 0
                            ? currentSampleRate
                            : 44100.0; // Assuming 44100 as default sample rate
    const int64_t lengthInSamples = static_cast<int64_t>(lastEventTime * rate);
    clipLength.store(lengthInSamples);
  }

  DBG("Clip: Rebuilt MIDI sequence with " + juce::String(notes.size()) +
      " notes");
}

void Clip::getMidiEvents(juce::MidiBuffer &midiBuffer, int numSamples) {
  if (clipType != Type::MIDI)
    return;

  auto sequence = midiSequence_.load(std::memory_order_acquire);
  if (!playing.load() || sequence == nullptr || sequence->getNumEvents() == 0)
    return;

  // NO ScopedLock on audio thread
  // Calculate time range for this block
  const int64_t currentPos = transportPosition.load();
  const int64_t clipOffsetSamples = clipOffset.load();

  // Position within the clip's MIDI sequence (accounting for offset)
  const int64_t posInClip = currentPos - clipOffsetSamples;

  if (posInClip < 0)
    return; // Clip hasn't started yet

  const double safeRate = std::max(1.0, currentSampleRate);
  const double startTime = static_cast<double>(posInClip) / safeRate;
  const double endTime =
      static_cast<double>(posInClip + numSamples) / safeRate;

  // Find and add all MIDI events in this time range
  for (int i = 0; i < sequence->getNumEvents(); ++i) {
    auto *event = sequence->getEventPointer(i);
    const double eventTime = event->message.getTimeStamp();

    if (eventTime >= startTime && eventTime < endTime) {
      // Calculate sample offset within this block
      const int sampleOffset =
          static_cast<int>((eventTime - startTime) * currentSampleRate);

      // Add the MIDI message to the buffer
      midiBuffer.addEvent(event->message, sampleOffset);
    }
  }
}

//==============================================================================
void Clip::setFadeIn(int64_t fadeInSamples) {
  fadeInLength.store(juce::jmax(int64_t(0), fadeInSamples));
}

void Clip::setFadeOut(int64_t fadeOutSamples) {
  fadeOutLength.store(juce::jmax(int64_t(0), fadeOutSamples));
}

//==============================================================================
void Clip::setGain(float newGain) {
  gain.store(juce::jlimit(0.0f, 2.0f, newGain));
}

void Clip::setPlaybackRate(double newRate) {
  // Clamp rate between 0.25x and 4.0x
  playbackRate_.store(juce::jlimit(0.25, 4.0, newRate));
}

double Clip::getPlaybackRate() const { return playbackRate_.load(); }

void Clip::setPreservePitch(bool shouldPreserve) {
  preservePitch_.store(shouldPreserve);

  // Initialize WSOLA buffers if needed (on message thread)
  // Note: We use static constants defined in header or locally if needed.
  // Assuming kWsolaWindowSize=2048 is defined in class.
  if (shouldPreserve) {
    // RT-safe: Message thread only. Audio thread reads preservePitch_
    // atomically.
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Pre-allocate window and buffer
    if (wsolaWindow_.empty()) {
      wsolaWindow_.resize(kWsolaWindowSize);
      // Create Hann window
      for (int i = 0; i < kWsolaWindowSize; ++i) {
        wsolaWindow_[i] =
            0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i /
                                    (kWsolaWindowSize - 1)));
      }
      wsolaOutputBuffer_.resize(kWsolaWindowSize * 2,
                                0.0f); // Double size for safety
    }
  }
}

bool Clip::isPreservingPitch() const { return preservePitch_.load(); }

//==============================================================================
void Clip::setLooping(bool shouldLoop) { looping.store(shouldLoop); }

//==============================================================================
void Clip::setColor(juce::Colour color) { clipColor = color; }

//==============================================================================
juce::ValueTree Clip::getState() const {
  juce::ValueTree state("Clip");

  state.setProperty("name", clipName, nullptr);
  state.setProperty("type", static_cast<int>(clipType), nullptr);
  // Use juce::int64 to avoid overflow for large sample positions
  state.setProperty("startPosition",
                    static_cast<juce::int64>(startPosition.load()), nullptr);
  state.setProperty("length", static_cast<juce::int64>(clipLength.load()),
                    nullptr);
  state.setProperty("offset", static_cast<juce::int64>(clipOffset.load()),
                    nullptr);
  state.setProperty("fadeIn", static_cast<juce::int64>(fadeInLength.load()),
                    nullptr);
  state.setProperty("fadeOut", static_cast<juce::int64>(fadeOutLength.load()),
                    nullptr);
  state.setProperty("gain", gain.load(), nullptr);
  state.setProperty("looping", looping.load(), nullptr);
  state.setProperty("color", clipColor.toString(), nullptr);

  if (clipType == Type::Audio && audioFile.existsAsFile()) {
    state.setProperty("audioFile", audioFile.getFullPathName(), nullptr);
  } else if (clipType == Type::MIDI) {
    // RT-safe: Message thread only, atomic load
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    auto sequence = midiSequence_.load(std::memory_order_acquire);

    // Save MIDI sequence as base64
    juce::MidiFile midiFile;
    if (sequence)
      midiFile.addTrack(*sequence);
    juce::MemoryOutputStream stream;
    midiFile.writeTo(stream);
    state.setProperty("midiData", stream.getMemoryBlock().toBase64Encoding(),
                      nullptr);
  }

  return state;
}

void Clip::loadState(const juce::ValueTree &state, AudioFilePool* pool) {
  if (!state.hasType("Clip"))
    return;

  clipName = state.getProperty("name", "Clip");
  clipType = static_cast<Type>(static_cast<int>(state.getProperty("type", 0)));
  startPosition.store(
      static_cast<juce::int64>(state.getProperty("startPosition", 0)));
  clipLength.store(static_cast<juce::int64>(state.getProperty("length", 0)));
  clipOffset.store(static_cast<juce::int64>(state.getProperty("offset", 0)));
  fadeInLength.store(static_cast<juce::int64>(state.getProperty("fadeIn", 0)));
  fadeOutLength.store(
      static_cast<juce::int64>(state.getProperty("fadeOut", 0)));
  gain.store(static_cast<float>(state.getProperty("gain", 1.0f)));
  looping.store(static_cast<bool>(state.getProperty("looping", false)));

  // Convert var to String before passing to Colour::fromString
  juce::var colorVar =
      state.getProperty("color", juce::Colours::blue.toString());
  clipColor = juce::Colour::fromString(colorVar.toString());

  if (clipType == Type::Audio) {
    juce::String audioFilePath = state.getProperty("audioFile", "");
    if (audioFilePath.isNotEmpty()) {
      juce::File file(audioFilePath);
      if (pool != nullptr) {
          setAudioFileFromPool(file, *pool);
      } else {
          // Warning: Cannot load audio file without pool!
          // We store the path but cannot load the data safely.
          // This might happen during early initialization or tests without pool.
          // For now, we just rely on the path being there.
          DBG("Clip::loadState - Warning: No AudioFilePool provided, cannot load audio file: " + audioFilePath);
      }
    }
  } else if (clipType == Type::MIDI) {
    juce::String midiDataBase64 = state.getProperty("midiData", "");
    if (midiDataBase64.isNotEmpty()) {
      juce::MemoryBlock midiData;
      midiData.fromBase64Encoding(midiDataBase64);

      juce::MidiFile midiFile;
      juce::MemoryInputStream inputStream(midiData, false);
      midiFile.readFrom(inputStream);

      if (midiFile.getNumTracks() > 0) {
        setMidiSequence(*midiFile.getTrack(0));
      }
    }
  }
}

//==============================================================================
// Phase 1.3: Process audio clip with explicit playhead position and
// Time-Stretching
void Clip::processAudioClip(const juce::AudioSourceChannelInfo &bufferToFill,
                            int64_t playheadSamples) {
  // [DSP Optimization] Use RT-safe handle
  // Load once to ensure consistency and avoid multiple atomic operations
  auto handlePtr =
      std::atomic_load_explicit(&audioFileHandle_, std::memory_order_acquire);
  auto handle =
      std::static_pointer_cast<const AudioFilePool::AudioFileHandle>(handlePtr);
  const juce::AudioBuffer<float> *sourceBuffer = nullptr;

  if (handle != nullptr && handle->isValid()) {
    sourceBuffer = &handle->buffer;
  } else {
    // Fallback to legacy audioBuffer (RT-unsafe if modified, but we remove the
    // lock)
    sourceBuffer = &audioBuffer; 
  }

  if (sourceBuffer == nullptr || sourceBuffer->getNumSamples() == 0)
    return;

  const double rate = playbackRate_.load();
  const int64_t clipStart = startPosition.load();
  const int64_t clipLen = clipLength.load();
  const int64_t clipOff = clipOffset.load();
  const float clipGain = gain.load();
  const int bufferLen = bufferToFill.numSamples;

  // Calculate the range of timeline samples this buffer covers
  const int64_t bufferStart = playheadSamples;
  const int64_t bufferEnd = playheadSamples + bufferLen;
  const int64_t clipEnd = clipStart + clipLen;

  // Check if buffer range overlaps with clip range at all
  if (bufferEnd <= clipStart || bufferStart >= clipEnd)
    return;

  // Calculate where in the buffer we should start/stop writing
  // and where in the clip (source audio) we should start reading
  int destOffset = 0;  // Where to start writing in output buffer
  int64_t sourceStartInClip = 0;  // Position within clip to start reading
  int numSamplesToRender = bufferLen;

  if (bufferStart < clipStart) {
    // Buffer starts before clip: skip leading silence
    destOffset = static_cast<int>(clipStart - bufferStart);
    sourceStartInClip = 0;
    numSamplesToRender = bufferLen - destOffset;
  } else {
    // Buffer starts within or after clip start
    destOffset = 0;
    sourceStartInClip = bufferStart - clipStart;
  }

  // Don't render past clip end (unless looping)
  const bool isLooping = looping.load();
  if (!isLooping && bufferStart + destOffset + numSamplesToRender > clipEnd) {
    numSamplesToRender = static_cast<int>(clipEnd - (bufferStart + destOffset));
  }

  if (numSamplesToRender <= 0)
    return;

  const int sourceLen = sourceBuffer->getNumSamples();
  if (sourceLen <= 0)
    return;

  // Fast path: rate is 1.0 (Standard playback)
  if (std::abs(rate - 1.0) < 0.001) {
    int64_t currentClipPos = sourceStartInClip;
    int currentDestOffset = destOffset;
    int samplesPending = numSamplesToRender;

    while (samplesPending > 0) {
      int64_t sourcePos = currentClipPos + clipOff;
      // sourceLen already declared and checked in outer scope

      if (looping.load()) {
          sourcePos = sourcePos % sourceLen;
          if (sourcePos < 0) sourcePos += sourceLen;
      } else if (sourcePos >= sourceLen) {
           break; 
      }

      int maxReadable = (int)(sourceLen - sourcePos);
      if (looping.load() && maxReadable <= 0) maxReadable = sourceLen;

      int chunk = juce::jmin(samplesPending, maxReadable);
      
      for (int ch = 0; ch < juce::jmin(bufferToFill.buffer->getNumChannels(),
                                       sourceBuffer->getNumChannels());
           ++ch) {
        bufferToFill.buffer->copyFrom(ch, bufferToFill.startSample + currentDestOffset, *sourceBuffer,
                                      ch, (int)sourcePos, chunk);
        if (clipGain != 1.0f)
          juce::FloatVectorOperations::multiply(
              bufferToFill.buffer->getWritePointer(ch, bufferToFill.startSample + currentDestOffset),
              clipGain, chunk);
      }

      samplesPending -= chunk;
      currentDestOffset += chunk;
      currentClipPos += chunk;
    }

    applyFadesSIMD(bufferToFill, destOffset, sourceStartInClip, numSamplesToRender);
    return;
  }

  // Slow path: Linear Interpolation (SIMD candidate for alpha ramp)
  // Also used for looping to handle wrap-around correctly
  // For now, let's keep it simple but remove the fade calculations from the
  // inner loop
  const int numSamples = numSamplesToRender;
  // sourceLen already declared above

  for (int ch = 0; ch < juce::jmin(bufferToFill.buffer->getNumChannels(),
                                   sourceBuffer->getNumChannels());
       ++ch) {
    auto *outData =
        bufferToFill.buffer->getWritePointer(ch, bufferToFill.startSample + destOffset);
    const auto *inData = sourceBuffer->getReadPointer(ch);
    double currentReadPos = (double)(sourceStartInClip + clipOff) * rate;

    for (int i = 0; i < numSamples; ++i) {
      int idx0 = static_cast<int>(currentReadPos);
      int idx1 = idx0 + 1;
      float alpha = static_cast<float>(currentReadPos - idx0);

      // Bug 39 & 45: Guard modulo and boundary conditions
      if (looping.load() && sourceLen > 0) {
        idx0 = (idx0 % sourceLen + sourceLen) % sourceLen;
        idx1 = (idx1 % sourceLen + sourceLen) % sourceLen;
      } else {
        idx0 = juce::jlimit(0, sourceLen - 1, idx0);
        idx1 = juce::jlimit(0, sourceLen - 1, idx1);
      }

      outData[i] = (1.0f - alpha) * inData[idx0] + alpha * inData[idx1];
      currentReadPos += rate;
    }

    if (clipGain != 1.0f)
      juce::FloatVectorOperations::multiply(outData, clipGain, numSamples);
  }

  applyFadesSIMD(bufferToFill, destOffset, sourceStartInClip, numSamples);
}




// Phase 2A: Process MIDI clip - schedule MIDI events into MidiBuffer
void Clip::processMidiClip(juce::MidiBuffer &midiBuffer,
                           int64_t playheadSamples, int numSamples) {
  // Get clip parameters (atomic loads)
  const int64_t clipStart = startPosition.load();
  const int64_t clipLen = clipLength.load();
  const int64_t clipOff = clipOffset.load();

  // Snapshot MIDI sequence
  auto sequence = midiSequence_.load(std::memory_order_acquire);
  if (sequence == nullptr)
    return;

  // Calculate position within clip
  const int64_t positionInClip = playheadSamples - clipStart;

  // Check if playhead is within clip bounds
  if (positionInClip < 0 || positionInClip >= clipLen)
    return;

  // Calculate the range of samples we're rendering: [blockStart, blockEnd)
  const int64_t blockStart = positionInClip;
  const int64_t blockEnd = positionInClip + numSamples;

  // NO ScopedLock on audio thread

  // Iterate through MIDI events and schedule those that fall within this block
  for (int i = 0; i < sequence->getNumEvents(); ++i) {
    auto *event = sequence->getEventPointer(i);
    if (event == nullptr)
      continue;

    // Get event time in clip-relative samples
    const double eventTimeSeconds = event->message.getTimeStamp();
    const int64_t eventSamples =
        static_cast<int64_t>(eventTimeSeconds * currentSampleRate);

    // Apply clip offset (trimming)
    const int64_t eventInClip = eventSamples - clipOff;

    // Handle looping
    int64_t adjustedEventSamples = eventInClip;
    if (looping.load() && clipLen > 0) {
      // Wrap event position into clip length
      if (adjustedEventSamples < 0)
        adjustedEventSamples = clipLen - ((-adjustedEventSamples) % clipLen);
      else if (adjustedEventSamples >= clipLen)
        adjustedEventSamples = adjustedEventSamples % clipLen;
    }

    // Check if event falls within current block
    if (adjustedEventSamples >= blockStart && adjustedEventSamples < blockEnd) {
      // Calculate sample offset within the buffer (0 to numSamples-1)
      const int sampleOffset =
          static_cast<int>(adjustedEventSamples - blockStart);

      // Add event to MIDI buffer
      midiBuffer.addEvent(event->message, sampleOffset);
    }
  }
}




float Clip::calculateFadeMultiplier(int64_t positionInClip) const {
  const int64_t fadeIn = fadeInLength.load();
  const int64_t fadeOut = fadeOutLength.load();
  const int64_t clipLen = clipLength.load();

  float multiplier = 1.0f;

  // Fade in
  if (fadeIn > 0 && positionInClip < fadeIn) {
    multiplier *=
        static_cast<float>(positionInClip) / static_cast<float>(fadeIn);
  }

  // Fade out
  if (fadeOut > 0 && positionInClip > clipLen - fadeOut) {
    const int64_t positionInFadeOut = clipLen - positionInClip;
    multiplier *=
        static_cast<float>(positionInFadeOut) / static_cast<float>(fadeOut);
  }

  return multiplier;
}
void Clip::applyFadesSIMD(const juce::AudioSourceChannelInfo &bufferToFill,
                          int destOffset,
                          int64_t startPositionInClip, int numSamples) {
  const int64_t fadeIn = fadeInLength.load();
  const int64_t fadeOut = fadeOutLength.load();
  const int64_t clipLen = clipLength.load();

  if (fadeIn <= 0 && fadeOut <= 0)
    return;

  // Optimization: Cache pointers to avoid getWritePointer() in loop
  const int numChannels = bufferToFill.buffer->getNumChannels();
  
  // Use stack buffer for common case, heap for edge cases (huge amounts of channels)
  float** channelPtrs = nullptr;
  float* stackPtrs[64];
  std::vector<float*> heapPtrs;

  if (numChannels <= 64) {
      channelPtrs = stackPtrs;
  } else {
      heapPtrs.resize(numChannels);
      channelPtrs = heapPtrs.data();
  }

  for (int ch = 0; ch < numChannels; ++ch) {
      // Use destOffset to target correct buffer region
      channelPtrs[ch] = bufferToFill.buffer->getWritePointer(ch, bufferToFill.startSample + destOffset);
  }

  // Iterate samples
  for (int i = 0; i < numSamples; ++i) {
    const int64_t pos = startPositionInClip + i;
    float multiplier = 1.0f;

    if (fadeIn > 0 && pos < fadeIn)
      multiplier *= static_cast<float>(pos) / static_cast<float>(fadeIn);

    if (fadeOut > 0 && pos > clipLen - fadeOut) {
      const int64_t posInFadeOut = clipLen - pos;
      multiplier *=
          static_cast<float>(posInFadeOut) / static_cast<float>(fadeOut);
    }

    if (multiplier != 1.0f) {
      for (int ch = 0; ch < numChannels; ++ch)
        channelPtrs[ch][i] *= multiplier;
    }
  }
}

} // namespace zenith
