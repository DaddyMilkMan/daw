/*
  ==============================================================================
    AudioStreamingManager.cpp
    Lock-free implementation for real-time audio streaming
  ==============================================================================
*/

#include "AudioStreamingManager.h"

#if __has_include(<opus/opus.h>)
  #include <opus/opus.h>
  #define HAVE_OPUS 1
#else
  #define HAVE_OPUS 0
  #warning "Opus not available - audio streaming will be disabled"
#endif

namespace zenith {
namespace network {

//==============================================================================
AudioStreamingManager::AudioStreamingManager() {
#if HAVE_OPUS
    inputInterleaved.resize(OPUS_FRAME_SIZE * 2);
    encodedBuffer.resize(4000);
#endif
}

AudioStreamingManager::~AudioStreamingManager() {
    release();
}

//==============================================================================
void AudioStreamingManager::RemoteStream::prepare() {
    fifo = std::make_unique<juce::AbstractFifo>(RING_BUFFER_SAMPLES);
    audioBuffer.resize(RING_BUFFER_SAMPLES);
    std::fill(audioBuffer.begin(), audioBuffer.end(), 0.0f);
}

void AudioStreamingManager::RemoteStream::release() {
#if HAVE_OPUS
    if (decoder) {
        opus_decoder_destroy(decoder);
        decoder = nullptr;
    }
#endif
    fifo.reset();
    audioBuffer.clear();
    active.store(false);
}

//==============================================================================
void AudioStreamingManager::prepare(double /*sampleRate*/, int blockSize) {
#if HAVE_OPUS
    currentSampleRate = 48000.0;
    currentBlockSize = blockSize;
    
    initializeEncoder();
    
    // Pre-allocate all stream slots
    for (auto& stream : remoteStreams) {
        if (!stream) {
            stream = std::make_unique<RemoteStream>();
            stream->prepare();
        }
    }
    
    encodingBufferFill.store(0);
    activeStreamCount.store(0);
#else
    juce::ignoreUnused(blockSize);
#endif
}

void AudioStreamingManager::release() {
#if HAVE_OPUS
    if (encoder) {
        opus_encoder_destroy(encoder);
        encoder = nullptr;
    }
    
    for (auto& stream : remoteStreams) {
        if (stream) {
            stream->release();
        }
    }
    
    encodingBufferFill.store(0);
    activeStreamCount.store(0);
#endif
}

void AudioStreamingManager::initializeEncoder() {
#if HAVE_OPUS
    if (encoder) {
        opus_encoder_destroy(encoder);
        encoder = nullptr;
    }
    
    int error;
    encoder = opus_encoder_create(48000, 2, OPUS_APPLICATION_AUDIO, &error);
    
    if (error != OPUS_OK || !encoder) {
        DBG("Opus Encoder Error: " << error);
        return;
    }
    
    opus_encoder_ctl(encoder, OPUS_SET_VBR(1));
    opus_encoder_ctl(encoder, OPUS_SET_BITRATE(128000));
    opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(5));
#endif
}

//==============================================================================
// Audio Thread: Encode outgoing audio
std::vector<juce::uint8> AudioStreamingManager::encodeAudio(const juce::AudioBuffer<float>& buffer) {
#if HAVE_OPUS
    if (!encoder) return {};

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    if (numSamples <= 0) return {};
    
    const float* left = buffer.getReadPointer(0);
    const float* right = numChannels > 1 ? buffer.getReadPointer(1) : left;
    
    int fill = encodingBufferFill.load();
    
    // Check if adding these samples would overflow
    if (fill + numSamples > OPUS_FRAME_SIZE) {
        return {};
    }
    
    // Interleave samples
    for (int i = 0; i < numSamples; ++i) {
        if (fill >= OPUS_FRAME_SIZE) break;
        inputInterleaved[fill * 2] = left[i];
        inputInterleaved[fill * 2 + 1] = right[i];
        ++fill;
    }
    
    encodingBufferFill.store(fill);
    
    // Encode when we have a full frame
    if (fill >= OPUS_FRAME_SIZE) {
        int bytes = opus_encode_float(
            encoder,
            inputInterleaved.data(),
            OPUS_FRAME_SIZE,
            encodedBuffer.data(),
            static_cast<opus_int32>(encodedBuffer.size())
        );
        
        encodingBufferFill.store(0);
        
        if (bytes > 0) {
            std::vector<juce::uint8> packet(static_cast<size_t>(bytes));
            std::memcpy(packet.data(), encodedBuffer.data(), static_cast<size_t>(bytes));
            return packet;
        }
    }
#else
    juce::ignoreUnused(buffer);
#endif
    return {};
}

//==============================================================================
// Network Thread: Decode incoming packet
void AudioStreamingManager::decodePacket(const juce::uint8* data, int size, const juce::String& userId) {
#if HAVE_OPUS
    if (!data || size <= 0) return;
    
    RemoteStream* stream = findOrCreateStream(userId);
    if (!stream || !stream->decoder) return;
    
    float decodedInterleaved[OPUS_FRAME_SIZE * 2];
    int samplesDecoded = opus_decode_float(
        stream->decoder,
        data,
        size,
        decodedInterleaved,
        OPUS_FRAME_SIZE,
        0
    );
    
    if (samplesDecoded > 0) {
        writeToRingBuffer(*stream, decodedInterleaved, samplesDecoded * 2);
    }
    
    stream->lastPacketTime.store(juce::Time::currentTimeMillis());
#else
    juce::ignoreUnused(data, size, userId);
#endif
}

bool AudioStreamingManager::writeToRingBuffer(RemoteStream& stream, const float* interleavedData, int numSamples) {
    if (!stream.fifo) return false;
    
    auto writeHandle = stream.fifo->write(numSamples);
    
    if (writeHandle.blockSize1 > 0) {
        std::memcpy(stream.audioBuffer.data() + writeHandle.startIndex1,
                    interleavedData,
                    writeHandle.blockSize1 * sizeof(float));
    }
    if (writeHandle.blockSize2 > 0) {
        std::memcpy(stream.audioBuffer.data() + writeHandle.startIndex2,
                    interleavedData + writeHandle.blockSize1,
                    writeHandle.blockSize2 * sizeof(float));
    }
    
    return true;
}

//==============================================================================
// Audio Thread: Mix remote streams into output
void AudioStreamingManager::getNextAudioBlock(juce::AudioBuffer<float>& outputBuffer) {
#if HAVE_OPUS
    const int numSamples = outputBuffer.getNumSamples();
    const int numChannels = outputBuffer.getNumChannels();
    
    if (numChannels < 2) return;
    
    // Mix all active streams - NO LOCKS!
    for (auto& streamPtr : remoteStreams) {
        if (!streamPtr || !streamPtr->active.load()) continue;
        
        RemoteStream& stream = *streamPtr;
        
        // Check for timeout (network thread will clean up)
        juce::int64 lastTime = stream.lastPacketTime.load();
        juce::int64 now = juce::Time::currentTimeMillis();
        
        if (now - lastTime > 5000) { // 5 second timeout
            stream.pendingRemoval.store(true);
            continue;
        }
        
        readFromRingBuffer(stream, outputBuffer, numSamples);
    }
#else
    juce::ignoreUnused(outputBuffer);
#endif
}

void AudioStreamingManager::readFromRingBuffer(RemoteStream& stream, juce::AudioBuffer<float>& outputBuffer, int numSamples) {
    if (!stream.fifo) return;
    
    const int samplesToRead = numSamples * 2; // Stereo interleaved
    
    // Temporary buffer for de-interleaving
    thread_local std::vector<float> tempBuffer;
    if (tempBuffer.size() < static_cast<size_t>(samplesToRead)) {
        tempBuffer.resize(samplesToRead);
    }
    
    auto readHandle = stream.fifo->read(samplesToRead);
    int actualRead = readHandle.blockSize1 + readHandle.blockSize2;
    
    if (actualRead == 0) {
        // Underrun - could add concealment here
        return;
    }
    
    // Copy from ring buffer to temp
    if (readHandle.blockSize1 > 0) {
        std::memcpy(tempBuffer.data(),
                    stream.audioBuffer.data() + readHandle.startIndex1,
                    readHandle.blockSize1 * sizeof(float));
    }
    if (readHandle.blockSize2 > 0) {
        std::memcpy(tempBuffer.data() + readHandle.blockSize1,
                    stream.audioBuffer.data() + readHandle.startIndex2,
                    readHandle.blockSize2 * sizeof(float));
    }
    
    // De-interleave and add to output
    float* left = outputBuffer.getWritePointer(0);
    float* right = outputBuffer.getWritePointer(1);
    
    for (int i = 0; i < numSamples && (i * 2 + 1) < actualRead; ++i) {
        left[i] += tempBuffer[i * 2];
        right[i] += tempBuffer[i * 2 + 1];
    }
}

//==============================================================================
// Find or create stream (Network thread only)
AudioStreamingManager::RemoteStream* AudioStreamingManager::findOrCreateStream(const juce::String& userId) {
    RemoteStream* existing = findStream(userId);
    if (existing) return existing;
    
    // Find free slot
    int slot = findFreeStreamSlot();
    if (slot < 0) {
        DBG("AudioStreamingManager: No free stream slots!");
        return nullptr;
    }
    
    auto& stream = remoteStreams[slot];
    
#if HAVE_OPUS
    int error;
    stream->decoder = opus_decoder_create(48000, 2, &error);
    if (error != OPUS_OK || !stream->decoder) {
        DBG("Opus Decoder Error: " << error);
        return nullptr;
    }
#endif
    
    stream->userId = userId;
    stream->lastPacketTime.store(juce::Time::currentTimeMillis());
    stream->pendingRemoval.store(false);
    stream->active.store(true);
    
    // Reset ring buffer
    stream->fifo->reset();
    std::fill(stream->audioBuffer.begin(), stream->audioBuffer.end(), 0.0f);
    
    activeStreamCount.fetch_add(1);
    return stream.get();
}

AudioStreamingManager::RemoteStream* AudioStreamingManager::findStream(const juce::String& userId) const {
    for (auto& streamPtr : remoteStreams) {
        if (streamPtr && streamPtr->userId == userId && streamPtr->active.load()) {
            return streamPtr.get();
        }
    }
    return nullptr;
}

int AudioStreamingManager::findFreeStreamSlot() const {
    for (int i = 0; i < MAX_REMOTE_STREAMS; ++i) {
        if (!remoteStreams[i] || !remoteStreams[i]->active.load()) {
            return i;
        }
    }
    return -1;
}

//==============================================================================
// UI thread: Get active stream IDs
std::vector<juce::String> AudioStreamingManager::getActiveStreamIds() const {
    std::vector<juce::String> ids;
    ids.reserve(MAX_REMOTE_STREAMS);
    
    for (auto& streamPtr : remoteStreams) {
        if (streamPtr && streamPtr->active.load() && !streamPtr->pendingRemoval.load()) {
            ids.push_back(streamPtr->userId);
        }
    }
    
    return ids;
}

} // namespace network
} // namespace zenith
