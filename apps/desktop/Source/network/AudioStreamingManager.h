/*
  ==============================================================================
    AudioStreamingManager.h
    Handles Opus encoding/decoding for real-time P2P audio streaming
  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <memory>
#include <atomic>
#include <array>

// Forward declare Opus types to avoid exposing headers globally
typedef struct OpusEncoder OpusEncoder;
typedef struct OpusDecoder OpusDecoder;

namespace zenith {
namespace network {

class AudioStreamingManager {
public:
    AudioStreamingManager();
    ~AudioStreamingManager();

    // Configuration (called from UI thread)
    void prepare(double sampleRate, int blockSize);
    void release();

    // Encoding (Local Audio -> Network) - Called from audio thread
    // Returns a vector of encoded bytes. Empty if encoding failed or buffer not full.
    std::vector<juce::uint8> encodeAudio(const juce::AudioBuffer<float>& buffer);

    // Decoding (Network -> Remote Audio) - Called from network thread
    // Decodes a packet and adds it to the lock-free buffer
    void decodePacket(const juce::uint8* data, int size, const juce::String& userId);

    // Rendering (Remote Audio -> Mixer) - Called from audio thread
    // Mixes all active remote streams into the output buffer
    void getNextAudioBlock(juce::AudioBuffer<float>& outputBuffer);

    // UI thread: Get list of active stream user IDs
    std::vector<juce::String> getActiveStreamIds() const;

private:
    static constexpr int MAX_REMOTE_STREAMS = 8;
    static constexpr int OPUS_FRAME_SIZE = 960; // 20ms @ 48kHz
    static constexpr int RING_BUFFER_SAMPLES = 48000 * 2; // 2 seconds stereo

    struct RemoteStream {
        std::atomic<bool> active{false};
        std::atomic<bool> pendingRemoval{false};
        std::atomic<juce::int64> lastPacketTime{0};
        
        // Lock-free ring buffer using AbstractFifo
        std::unique_ptr<juce::AbstractFifo> fifo;
        std::vector<float> audioBuffer; // Interleaved stereo
        
        // Decoder - only accessed by network thread
        void* decoder = nullptr;
        juce::String userId;
        
        void prepare();
        void release();
    };

    double currentSampleRate = 48000.0;
    int currentBlockSize = 512;
    
    // Opus State - encoder only accessed by audio thread
    void* encoder = nullptr;
    std::vector<float> inputInterleaved;
    std::vector<juce::uint8> encodedBuffer;
    
    // Encoding buffer
    std::vector<float> encodingResampleBuffer;
    std::atomic<int> encodingBufferFill{0};

    // Remote Peers - lock-free array for audio thread access
    std::array<std::unique_ptr<RemoteStream>, MAX_REMOTE_STREAMS> remoteStreams;
    
    // Atomic indices for stream management (UI/Network thread only)
    std::atomic<int> activeStreamCount{0};

    void initializeEncoder();
    RemoteStream* findOrCreateStream(const juce::String& userId);
    RemoteStream* findStream(const juce::String& userId) const;
    int findFreeStreamSlot() const;
    
    // Lock-free write to ring buffer (called from network thread)
    bool writeToRingBuffer(RemoteStream& stream, const float* interleavedData, int numSamples);
    
    // Lock-free read from ring buffer (called from audio thread)
    void readFromRingBuffer(RemoteStream& stream, juce::AudioBuffer<float>& outputBuffer, int numSamples);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioStreamingManager)
};

} // namespace network
} // namespace zenith
