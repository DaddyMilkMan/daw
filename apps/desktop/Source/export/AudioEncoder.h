/*
  ==============================================================================

    AudioEncoder.h
    Created: 2025-02-03
    Author:  Zenith DAW

    External audio encoders for MP3 and AAC formats.
    Uses FFmpeg/LAME command-line tools for encoding.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <functional>
#include <memory>
#include <atomic>

namespace zenith {

/**
 * @brief Encoder type for lossy audio formats
 */
enum class EncoderType {
    MP3,    ///< MP3 encoding via LAME or FFmpeg
    AAC     ///< AAC encoding via FFmpeg
};

/**
 * @brief Quality preset for lossy encoding
 */
enum class EncoderQuality {
    Low,      ///< ~128 kbps
    Medium,   ///< ~192 kbps  
    High,     ///< ~256 kbps
    Maximum   ///< ~320 kbps (MP3 max) or highest AAC quality
};

/**
 * @brief Configuration for external audio encoding
 */
struct EncoderConfig {
    EncoderType type = EncoderType::MP3;
    EncoderQuality quality = EncoderQuality::High;
    int bitrate = 256;           ///< Target bitrate in kbps (overrides quality if > 0)
    bool useVBR = true;          ///< Use variable bitrate (recommended)
    double sampleRate = 44100.0;
    int numChannels = 2;
    
    /// Get bitrate based on quality preset if not explicitly set
    int getEffectiveBitrate() const {
        if (bitrate > 0) return bitrate;
        
        switch (quality) {
            case EncoderQuality::Low:     return 128;
            case EncoderQuality::Medium:  return 192;
            case EncoderQuality::High:    return 256;
            case EncoderQuality::Maximum: return 320;
        }
        return 256;
    }
};

/**
 * @class AudioEncoder
 * @brief Encodes audio data to MP3 or AAC using external tools
 * 
 * This class provides a bridge between JUCE audio data and external encoders
 * like FFmpeg or LAME. It first exports audio to a temporary WAV file, then
 * encodes it to the target format using command-line tools.
 * 
 * Thread-safe for single encoding operation at a time.
 */
class AudioEncoder {
public:
    /// Progress callback: (progress 0.0-1.0, status message)
    using ProgressCallback = std::function<void(float, const juce::String&)>;
    
    AudioEncoder();
    ~AudioEncoder();
    
    /**
     * @brief Check if an encoder is available on the system
     * @param type Encoder type to check
     * @return true if the encoder tool is found in PATH
     */
    static bool isEncoderAvailable(EncoderType type);
    
    /**
     * @brief Get the name of the encoder tool
     * @param type Encoder type
     * @return Name of the tool (e.g., "ffmpeg", "lame")
     */
    static juce::String getEncoderName(EncoderType type);
    
    /**
     * @brief Encode a WAV file to MP3 or AAC
     * @param inputWavFile Source WAV file
     * @param outputFile Destination file (extension determines format if config not set)
     * @param config Encoder configuration
     * @param progressCallback Optional progress callback
     * @return true on success
     */
    bool encodeFile(const juce::File& inputWavFile,
                    const juce::File& outputFile,
                    const EncoderConfig& config,
                    ProgressCallback progressCallback = nullptr);
    
    /**
     * @brief Encode audio buffer directly to file
     * @param buffer Audio data to encode
     * @param sampleRate Sample rate of the audio
     * @param outputFile Destination file
     * @param config Encoder configuration
     * @param progressCallback Optional progress callback
     * @return true on success
     */
    bool encodeBuffer(const juce::AudioBuffer<float>& buffer,
                      double sampleRate,
                      const juce::File& outputFile,
                      const EncoderConfig& config,
                      ProgressCallback progressCallback = nullptr);
    
    /**
     * @brief Get the last error message
     */
    juce::String getLastError() const { return lastError_; }
    
    /**
     * @brief Cancel ongoing encoding
     */
    void cancel();
    
    /**
     * @brief Check if encoding was cancelled
     */
    bool isCancelled() const { return cancelled_.load(); }

private:
    juce::String lastError_;
    std::atomic<bool> cancelled_{false};
    std::unique_ptr<juce::ChildProcess> encoderProcess_;
    
    juce::String buildFFmpegCommand(const juce::File& input,
                                    const juce::File& output,
                                    const EncoderConfig& config) const;
    
    juce::String buildLameCommand(const juce::File& input,
                                  const juce::File& output,
                                  const EncoderConfig& config) const;
    
    bool writeBufferToTempWav(const juce::AudioBuffer<float>& buffer,
                              double sampleRate,
                              const juce::File& tempFile);
    
    bool runEncoderProcess(const juce::String& command,
                           ProgressCallback progressCallback);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEncoder)
};

/**
 * @brief Get file extension for encoder type
 */
inline juce::String getExtensionForEncoder(EncoderType type) {
    switch (type) {
        case EncoderType::MP3: return ".mp3";
        case EncoderType::AAC: return ".m4a";
    }
    return ".mp3";
}

} // namespace zenith
