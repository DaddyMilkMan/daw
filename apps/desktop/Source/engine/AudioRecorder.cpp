/*
  ==============================================================================

    AudioRecorder.cpp
    Created: 2025-12-09
    Author:  Zenith DAW

    Implementation of RT-safe multi-track audio recording using RCU pattern.

  ==============================================================================
*/

#include "AudioRecorder.h"
#include "Track.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include "RealTimeGarbageCollector.h"

namespace zenith {

//==============================================================================
// AudioRingBuffer Implementation
//==============================================================================

AudioRingBuffer::AudioRingBuffer(int numChannels, int bufferSizeFrames)
    : fifo_(bufferSizeFrames), buffer_(numChannels, bufferSizeFrames),
      numChannels_(numChannels), bufferSizeFrames_(bufferSizeFrames) {
  buffer_.clear();
}

int AudioRingBuffer::write(const float *const *data, int numChannels,
                           int numSamples) {
  int start1, size1, start2, size2;
  fifo_.prepareToWrite(numSamples, start1, size1, start2, size2);

  const int totalAvailable = size1 + size2;
  if (totalAvailable < numSamples) {
    overflowed_.store(true);
  }

  const int channelsToCopy = std::min(numChannels, numChannels_);

  if (size1 > 0) {
    for (int ch = 0; ch < channelsToCopy; ++ch) {
      if (data[ch] != nullptr) {
        juce::FloatVectorOperations::copy(buffer_.getWritePointer(ch, start1),
                                          data[ch], size1);
      } else {
        juce::FloatVectorOperations::clear(buffer_.getWritePointer(ch, start1),
                                           size1);
      }
    }
  }

  if (size2 > 0) {
    for (int ch = 0; ch < channelsToCopy; ++ch) {
      if (data[ch] != nullptr) {
        juce::FloatVectorOperations::copy(buffer_.getWritePointer(ch, start2),
                                          data[ch] + size1, size2);
      } else {
        juce::FloatVectorOperations::clear(buffer_.getWritePointer(ch, start2),
                                           size2);
      }
    }
  }

  fifo_.finishedWrite(totalAvailable);
  return totalAvailable;
}

int AudioRingBuffer::read(juce::AudioBuffer<float> &output, int numSamples) {
  const int availableToRead = std::min(numSamples, fifo_.getNumReady());
  if (availableToRead == 0)
    return 0;

  int start1, size1, start2, size2;
  fifo_.prepareToRead(availableToRead, start1, size1, start2, size2);

  const int channelsToCopy = std::min(output.getNumChannels(), numChannels_);

  if (output.getNumSamples() < availableToRead) {
    output.setSize(output.getNumChannels(), availableToRead, false, false,
                   true);
  }

  if (size1 > 0) {
    for (int ch = 0; ch < channelsToCopy; ++ch) {
      juce::FloatVectorOperations::copy(output.getWritePointer(ch),
                                        buffer_.getReadPointer(ch, start1),
                                        size1);
    }
  }

  if (size2 > 0) {
    for (int ch = 0; ch < channelsToCopy; ++ch) {
      juce::FloatVectorOperations::copy(output.getWritePointer(ch, size1),
                                        buffer_.getReadPointer(ch, start2),
                                        size2);
    }
  }

  fifo_.finishedRead(availableToRead);
  return availableToRead;
}

bool AudioRingBuffer::hasOverflowed(bool reset) {
  if (reset)
    return overflowed_.exchange(false);
  return overflowed_.load();
}

void AudioRingBuffer::reset() {
  fifo_.reset();
  buffer_.clear();
  overflowed_.store(false);
}

//==============================================================================
// RecordingSession Move Operators
//==============================================================================

RecordingSession::RecordingSession(RecordingSession &&other) noexcept
    : ringBuffer(std::move(other.ringBuffer)), writer(std::move(other.writer)),
      file(std::move(other.file)), trackId(std::move(other.trackId)),
      trackIndex(other.trackIndex), inputChannelStart(other.inputChannelStart),
      numChannels(other.numChannels),
      startSamplePosition(other.startSamplePosition),
      samplesRecorded(other.samplesRecorded.load()),
      sampleRate(other.sampleRate), isActive(other.isActive.load()) {}

RecordingSession &
RecordingSession::operator=(RecordingSession &&other) noexcept {
  if (this != &other) {
    ringBuffer = std::move(other.ringBuffer);
    writer = std::move(other.writer);
    file = std::move(other.file);
    trackId = std::move(other.trackId);
    trackIndex = other.trackIndex;
    inputChannelStart = other.inputChannelStart;
    numChannels = other.numChannels;
    startSamplePosition = other.startSamplePosition;
    samplesRecorded.store(other.samplesRecorded.load());
    sampleRate = other.sampleRate;
    isActive.store(other.isActive.load());
  }
  return *this;
}

//==============================================================================
// AudioRecorder Implementation
//==============================================================================

AudioRecorder::AudioRecorder() {
  writerThread_ =
      std::make_unique<juce::TimeSliceThread>("Audio Recorder Thread");
  writerThread_->startThread(juce::Thread::Priority::high);

  tempReadBuffer_.setSize(2, kFlushBlockSize);

  // Initialize RCU snapshot
  updateSessionSnapshot();
}

AudioRecorder::~AudioRecorder() {
  // Owner is responsible for stopping recording on message thread before
  // destruction. We cannot safely call stopRecording (which asserts message
  // thread) here.

  if (writerThread_) {
    writerThread_->removeTimeSliceClient(this);
    writerThread_->stopThread(2000);
  }
}

void AudioRecorder::prepare(double sampleRate) { sampleRate_ = sampleRate; }

void AudioRecorder::startRecording(
    const std::vector<std::shared_ptr<Track>> &tracks,
    const juce::AudioDeviceManager &deviceManager, juce::int64 startSample,
    const juce::File &recordingsDir) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (state_.load() != RecordingState::Idle) {
    DBG("AudioRecorder: Not idle, ignoring start request");
    return;
  }

  // Store for loop recording
  recordingsDir_ = recordingsDir;
  currentTakeNumber_.store(1);
  completedTakes_.clear();

  auto *device = deviceManager.getCurrentAudioDevice();
  const int numInputChannels =
      device ? device->getActiveInputChannels().countNumberOfSetBits() : 2;
  const double deviceSampleRate =
      device ? device->getCurrentSampleRate() : sampleRate_;

  if (!recordingsDir.exists()) {
    recordingsDir.createDirectory();
  }

  sessions_.clear();

  for (size_t i = 0; i < tracks.size(); ++i) {
    auto &track = tracks[i];
    if (!track || !track->isArmed())
      continue;

    if (track->getType() != Track::Type::Audio &&
        track->getType() != Track::Type::Instrument) {
      continue;
    }

    int inputChannel = track->getInputChannel();
    int sessionNumChannels = 2;
    int inputChannelStart = 0;

    if (inputChannel >= 0 && inputChannel < numInputChannels) {
      sessionNumChannels = 1;
      inputChannelStart = inputChannel;
    } else {
      sessionNumChannels = std::min(2, numInputChannels);
      inputChannelStart = 0;
    }

    juce::File recordFile =
        createRecordingFile(recordingsDir, track->getName());
    auto fileStream = std::make_unique<juce::FileOutputStream>(recordFile);

    if (!fileStream->openedOk()) {
      DBG("AudioRecorder: Failed to create file: " +
          recordFile.getFullPathName());
      continue;
    }

    juce::WavAudioFormat wavFormat;

    // Move to generic OutputStream unique_ptr for the new API
    std::unique_ptr<juce::OutputStream> outputStream = std::move(fileStream);

<<<<<<< HEAD
    std::unique_ptr<juce::AudioFormatWriter> baseWriter(
        wavFormat.createWriterFor(outputStream.release(), deviceSampleRate,
                                  static_cast<unsigned int>(sessionNumChannels),
                                  constants::kRecordingBitDepth, {}, 0));
=======
    auto writerOptions = juce::AudioFormatWriter::Options()
        .withSampleRate(deviceSampleRate)
        .withNumChannels(static_cast<int>(sessionNumChannels))
        .withBitsPerSample(constants::kRecordingBitDepth);

    std::unique_ptr<juce::AudioFormatWriter> baseWriter = 
        wavFormat.createWriterFor(outputStream, writerOptions);
>>>>>>> origin/master

    if (!baseWriter)
      continue;

    auto threadedWriter =
        std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
            baseWriter.release(), *writerThread_,
            constants::kAudioWriterFifoSize);

    auto session = std::make_shared<RecordingSession>();
    session->ringBuffer = std::make_unique<AudioRingBuffer>(sessionNumChannels);
    session->writer = std::move(threadedWriter);
    session->file = recordFile;
    session->trackId = track->getTrackId();
    session->trackIndex = static_cast<int>(i);
    session->inputChannelStart = inputChannelStart;
    session->numChannels = sessionNumChannels;
    session->startSamplePosition = startSample;
    session->samplesRecorded.store(0);
    session->sampleRate = deviceSampleRate;
    session->isActive.store(true);

    sessions_.push_back(session);

    DBG("AudioRecorder: Started recording for track '" + track->getName() +
        "'");
  }

  // Update snapshot for audio thread
  updateSessionSnapshot();

  if (!sessions_.empty()) {
    writerThread_->addTimeSliceClient(this);
    state_.store(RecordingState::Recording);
  }
}

void AudioRecorder::stopRecording(std::function<void(std::vector<RecordingResult>)> completionCallback) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (state_.load() != RecordingState::Recording) {
    if (completionCallback)
      completionCallback({});
    return;
  }

  completionCallback_ = std::move(completionCallback);
  state_.store(RecordingState::Finalizing);

  // Notify writer thread to finalize
  if (writerThread_) {
    writerThread_->notify();
  }
}

void AudioRecorder::onLoopCycle() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (!isRecording_.load() || !loopRecordingEnabled_.load())
    return;

  DBG("AudioRecorder: Loop cycle detected, creating new takes (take " +
      juce::String(currentTakeNumber_.load() + 1) + ")");

  // Finalize current sessions and save results as completed takes
  for (auto &session : sessions_) {
    if (!session->isActive.load())
      continue;

    // Flush remaining data
    while (session->ringBuffer->getNumReady() > 0) {
      const int numRead =
          session->ringBuffer->read(tempReadBuffer_, kFlushBlockSize);
      if (numRead > 0) {
        const float *channels[2] = {tempReadBuffer_.getReadPointer(0),
                                    session->numChannels > 1
                                        ? tempReadBuffer_.getReadPointer(1)
                                        : tempReadBuffer_.getReadPointer(0)};
        session->writer->write(channels, numRead);
      }
    }

    // Finalize writer
    session->writer.reset();

    // Store as completed take
    RecordingResult result;
    result.file = session->file;
    result.trackIndex = session->trackIndex;
    result.trackId = session->trackId;
    result.startSamplePosition = loopStart_.load();
    result.samplesRecorded = session->samplesRecorded.load();
    result.sampleRate = session->sampleRate;
    completedTakes_.push_back(result);
  }

<<<<<<< HEAD
  // Add any previously completed takes from loop recording
  for (const auto &take : completedTakes_) {
    results.push_back(take);
  }
  completedTakes_.clear();

  // Clear sessions
  sessions_.clear();
=======
  // Increment take number
  currentTakeNumber_.fetch_add(1);
>>>>>>> origin/master

  // Create new sessions for next take
  std::vector<std::shared_ptr<RecordingSession>> newSessions;

  for (auto &oldSession : sessions_) {
    // Create new file for this take
    juce::String trackName = "Track_" + juce::String(oldSession->trackIndex);
    juce::File recordFile = createRecordingFile(
        recordingsDir_,
        trackName + "_Take" + juce::String(currentTakeNumber_.load()));
    auto fileStream = std::make_unique<juce::FileOutputStream>(recordFile);

    if (!fileStream->openedOk()) {
      DBG("AudioRecorder: Failed to create file for new take: " +
          recordFile.getFullPathName());
      continue;
    }

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> baseWriter(
        wavFormat.createWriterFor(
            fileStream.release(), oldSession->sampleRate,
            static_cast<unsigned int>(oldSession->numChannels),
            constants::kRecordingBitDepth, {}, 0));

    if (!baseWriter)
      continue;

    auto threadedWriter =
        std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
            baseWriter.release(), *writerThread_,
            constants::kAudioWriterFifoSize);

    auto session = std::make_shared<RecordingSession>();
    session->ringBuffer =
        std::make_unique<AudioRingBuffer>(oldSession->numChannels);
    session->writer = std::move(threadedWriter);
    session->file = recordFile;
    session->trackId = oldSession->trackId;
    session->trackIndex = oldSession->trackIndex;
    session->inputChannelStart = oldSession->inputChannelStart;
    session->numChannels = oldSession->numChannels;
    session->startSamplePosition = loopStart_.load();
    session->samplesRecorded.store(0);
    session->sampleRate = oldSession->sampleRate;
    session->isActive.store(true);

    newSessions.push_back(session);
  }

  // Replace sessions
  sessions_ = std::move(newSessions);
  updateSessionSnapshot();
}

<<<<<<< HEAD
void AudioRecorder::onLoopCycle() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (!isRecording_.load() || !loopRecordingEnabled_.load())
    return;

  DBG("AudioRecorder: Loop cycle detected, creating new takes (take " +
      juce::String(currentTakeNumber_.load() + 1) + ")");

  // Finalize current sessions and save results as completed takes
  for (auto &session : sessions_) {
    if (!session->isActive.load())
      continue;

    // Flush remaining data
    while (session->ringBuffer->getNumReady() > 0) {
      const int numRead =
          session->ringBuffer->read(tempReadBuffer_, kFlushBlockSize);
      if (numRead > 0) {
        const float *channels[2] = {tempReadBuffer_.getReadPointer(0),
                                    session->numChannels > 1
                                        ? tempReadBuffer_.getReadPointer(1)
                                        : tempReadBuffer_.getReadPointer(0)};
        session->writer->write(channels, numRead);
      }
    }

    // Finalize writer
    session->writer.reset();

    // Store as completed take
    RecordingResult result;
    result.file = session->file;
    result.trackIndex = session->trackIndex;
    result.trackId = session->trackId;
    result.startSamplePosition = loopStart_.load();
    result.samplesRecorded = session->samplesRecorded.load();
    result.sampleRate = session->sampleRate;
    completedTakes_.push_back(result);
  }

  // Increment take number
  currentTakeNumber_.fetch_add(1);

  // Create new sessions for next take
  std::vector<std::shared_ptr<RecordingSession>> newSessions;

  for (auto &oldSession : sessions_) {
    // Create new file for this take
    juce::String trackName = "Track_" + juce::String(oldSession->trackIndex);
    juce::File recordFile = createRecordingFile(
        recordingsDir_,
        trackName + "_Take" + juce::String(currentTakeNumber_.load()));
    auto fileStream = std::make_unique<juce::FileOutputStream>(recordFile);

    if (!fileStream->openedOk()) {
      DBG("AudioRecorder: Failed to create file for new take: " +
          recordFile.getFullPathName());
      continue;
    }

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> baseWriter(
        wavFormat.createWriterFor(
            fileStream.release(), oldSession->sampleRate,
            static_cast<unsigned int>(oldSession->numChannels),
            constants::kRecordingBitDepth, {}, 0));

    if (!baseWriter)
      continue;

    auto threadedWriter =
        std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
            baseWriter.release(), *writerThread_,
            constants::kAudioWriterFifoSize);

    auto session = std::make_shared<RecordingSession>();
    session->ringBuffer =
        std::make_unique<AudioRingBuffer>(oldSession->numChannels);
    session->writer = std::move(threadedWriter);
    session->file = recordFile;
    session->trackId = oldSession->trackId;
    session->trackIndex = oldSession->trackIndex;
    session->inputChannelStart = oldSession->inputChannelStart;
    session->numChannels = oldSession->numChannels;
    session->startSamplePosition = loopStart_.load();
    session->samplesRecorded.store(0);
    session->sampleRate = oldSession->sampleRate;
    session->isActive.store(true);

    newSessions.push_back(session);
  }

  // Replace sessions
  sessions_ = std::move(newSessions);
  updateSessionSnapshot();
}

=======
>>>>>>> origin/master
void AudioRecorder::updateSessionSnapshot() {
  std::shared_ptr<SessionSnapshot> newSnapshot = std::make_shared<SessionSnapshot>(sessions_);
  activeSessionSnapshot_.store(newSnapshot.get(), std::memory_order_release);
  RealTimeGarbageCollector::getInstance().deferDelete(currentSessionSnapshot_);
  currentSessionSnapshot_ = newSnapshot;
}

void AudioRecorder::write(const float *const *inputChannelData,
                          int numInputChannels, int numSamples,
                          const std::vector<std::shared_ptr<Track>> &tracks) {
  if (state_.load() != RecordingState::Recording || inputChannelData == nullptr || numSamples <= 0)
    return;

  // RCU Lock-Free Access
  auto *snapshot = activeSessionSnapshot_.load(std::memory_order_acquire);
  if (!snapshot)
    return;

  for (const auto &session : snapshot->sessions) {
    if (!session->isActive.load() || !session->ringBuffer)
      continue;

    // Verify track index validity (optional safety)
    if (session->trackIndex < 0 ||
        session->trackIndex >= static_cast<int>(tracks.size()))
      continue;
    if (!tracks[session->trackIndex]->isArmed())
      continue;

    const float *channels[2] = {nullptr, nullptr};

    if (session->numChannels == 1) {
      if (session->inputChannelStart < numInputChannels) {
        channels[0] = inputChannelData[session->inputChannelStart];
      }
      channels[1] = channels[0];
    } else {
      if (session->inputChannelStart < numInputChannels)
        channels[0] = inputChannelData[session->inputChannelStart];
      if (session->inputChannelStart + 1 < numInputChannels)
        channels[1] = inputChannelData[session->inputChannelStart + 1];
      else
        channels[1] = channels[0];
    }

    if (channels[0] != nullptr) {
      const int written = session->ringBuffer->write(
          channels, session->numChannels, numSamples);
      session->samplesRecorded.fetch_add(written);
    }
  }
}

int AudioRecorder::useTimeSlice() {
  const auto currentState = state_.load();
  if (currentState == RecordingState::Idle)
    return -1;

  // RCU Lock-Free Access
  auto *snapshot = activeSessionSnapshot_.load(std::memory_order_acquire);
  if (!snapshot)
    return -1;

  bool anyWork = false;
  bool finalizing = (currentState == RecordingState::Finalizing);

  for (const auto &session : snapshot->sessions) {
    if (!session->isActive.load() || !session->ringBuffer || !session->writer)
      continue;

    const int numReady = session->ringBuffer->getNumReady();
    
    // In finalizing state, we flush EVERYTHING. 
    // In recording state, we only flush if we have enough data (hysteresis).
    const int threshold = finalizing ? 1 : (kFlushBlockSize / 2);

    if (numReady >= threshold) {
      if (tempReadBuffer_.getNumChannels() < session->numChannels) {
        tempReadBuffer_.setSize(session->numChannels, kFlushBlockSize, false,
                                false, true);
      }

      const int toRead = std::min(numReady, kFlushBlockSize);
      const int numRead = session->ringBuffer->read(tempReadBuffer_, toRead);

      if (numRead > 0) {
        // Correcting channel mapping logic for safety
        const float* writerChannels[2];
        writerChannels[0] = tempReadBuffer_.getReadPointer(0);
        writerChannels[1] = session->numChannels > 1 ? tempReadBuffer_.getReadPointer(1) : writerChannels[0];

        session->writer->write(writerChannels, numRead);
        anyWork = true;
      }
    }
  }

  if (finalizing && !anyWork) {
    // Everything flushed from ring buffers, now finalize writers and results
    std::vector<RecordingResult> results;
    
    for (auto& session : sessions_) {
      if (session->ringBuffer->hasOverflowed(true)) {
        DBG("AudioRecorder: WARNING - Ring buffer overflow detected");
      }

      // Resetting ThreadedWriter flushes its internal fifo to disk (BLOCKING on this thread)
      session->writer.reset();

      RecordingResult result;
      result.file = session->file;
      result.trackIndex = session->trackIndex;
      result.trackId = session->trackId;
      result.startSamplePosition = session->startSamplePosition;
      result.samplesRecorded = session->samplesRecorded.load();
      result.sampleRate = session->sampleRate;
      results.push_back(result);
    }

    // Trigger callback on Message Thread
    auto callback = std::move(completionCallback_);
    juce::MessageManager::callAsync([this, callback, results]() {
      // Clear sessions on message thread
      sessions_.clear();
      updateSessionSnapshot();
      writerThread_->removeTimeSliceClient(this);
      
      state_.store(RecordingState::Idle);

      if (callback)
        callback(results);
    });

    return -1;
  }

  return anyWork ? 0 : 5;
}

juce::File AudioRecorder::createRecordingFile(const juce::File &dir,
                                              const juce::String &trackName) {
  juce::String safeName = trackName.replaceCharacter(' ', '_')
                              .replaceCharacter('/', '_')
                              .replaceCharacter('\\', '_')
                              .replaceCharacter(':', '_')
                              .replaceCharacter('*', '_')
                              .replaceCharacter('?', '_')
                              .replaceCharacter('"', '_')
                              .replaceCharacter('<', '_')
                              .replaceCharacter('>', '_')
                              .replaceCharacter('|', '_');

  juce::String timestamp =
      juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
  juce::String filename = safeName + "_" + timestamp + ".wav";
  juce::File file = dir.getChildFile(filename);

  int counter = 1;
  while (file.exists()) {
    filename =
        safeName + "_" + timestamp + "_" + juce::String(counter++) + ".wav";
    file = dir.getChildFile(filename);
  }
  return file;
}

} // namespace zenith
