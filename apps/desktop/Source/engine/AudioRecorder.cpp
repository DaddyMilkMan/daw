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

  if (isRecording_.load()) {
    DBG("AudioRecorder: Already recording, ignoring start request");
    return;
  }

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

    auto writerOptions = juce::AudioFormatWriter::Options()
        .withSampleRate(deviceSampleRate)
        .withNumChannels(static_cast<int>(sessionNumChannels))
        .withBitsPerSample(constants::kRecordingBitDepth);

    std::unique_ptr<juce::AudioFormatWriter> baseWriter = 
        wavFormat.createWriterFor(outputStream, writerOptions);

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
    isRecording_.store(true);
  }
}

std::vector<RecordingResult> AudioRecorder::stopRecording() {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  std::vector<RecordingResult> results;

  if (!isRecording_.load())
    return results;

  isRecording_.store(false);

  if (writerThread_) {
    writerThread_->removeTimeSliceClient(this);
  }

  // Process results BEFORE clearing sessions
  for (auto &session : sessions_) {
    if (!session->isActive.load())
      continue;

    // Final flush
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

    if (session->ringBuffer->hasOverflowed(true)) {
      DBG("AudioRecorder: WARNING - Ring buffer overflow detected");
    }

    // Reset writer blocks until done
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

  // Clear sessions
  sessions_.clear();

  // Update snapshot to clear audio thread view
  updateSessionSnapshot();

  return results;
}

void AudioRecorder::updateSessionSnapshot() {
  std::shared_ptr<SessionSnapshot> newSnapshot = std::make_shared<SessionSnapshot>(sessions_);
  activeSessionSnapshot_.store(newSnapshot.get(), std::memory_order_release);
  RealTimeGarbageCollector::getInstance().deferDelete(currentSessionSnapshot_);
  currentSessionSnapshot_ = newSnapshot;
}

void AudioRecorder::write(const float *const *inputChannelData,
                          int numInputChannels, int numSamples,
                          const std::vector<std::shared_ptr<Track>> &tracks) {
  if (!isRecording_.load() || inputChannelData == nullptr || numSamples <= 0)
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
  if (!isRecording_.load())
    return -1;

  // RCU Lock-Free Access
  // Both Audio Thread and Background Thread share read access to the snapshot
  auto *snapshot = activeSessionSnapshot_.load(std::memory_order_acquire);
  if (!snapshot)
    return -1;

  bool anyWork = false;

  for (const auto &session : snapshot->sessions) {
    if (!session->isActive.load() || !session->ringBuffer || !session->writer)
      continue;

    const int numReady = session->ringBuffer->getNumReady();
    if (numReady >= kFlushBlockSize / 2) {
      if (tempReadBuffer_.getNumChannels() < session->numChannels) {
        tempReadBuffer_.setSize(session->numChannels, kFlushBlockSize, false,
                                false, true);
      }

      const int toRead = std::min(numReady, kFlushBlockSize);
      const int numRead = session->ringBuffer->read(tempReadBuffer_, toRead);

      if (numRead > 0) {
        const float *channels[2] = {tempReadBuffer_.getReadPointer(0),
                                    session->numChannels > 1
                                        ? tempReadBuffer_.getReadPointer(1)
                                        : tempReadBuffer_.getReadPointer(0)};
        session->writer->write(channels, numRead);
        anyWork = true;
      }
    }
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
