/*
  ==============================================================================

    AudioEncoder.cpp
    Created: 2025-02-03
    Author:  Zenith DAW

    Implementation of external audio encoders for MP3 and AAC formats.

  ==============================================================================
*/

#include "AudioEncoder.h"

namespace zenith {

//==============================================================================
// Constructor / Destructor
//==============================================================================

AudioEncoder::AudioEncoder() = default;

AudioEncoder::~AudioEncoder() {
    cancel();
}

//==============================================================================
// Static Utilities
//==============================================================================

bool AudioEncoder::isEncoderAvailable(EncoderType type) {
    juce::String toolName;
    
    switch (type) {
        case EncoderType::MP3:
            // Try LAME first, then FFmpeg
            {
                juce::ChildProcess lameCheck;
                if (lameCheck.start("lame --version")) {
                    lameCheck.waitForProcessToFinish(2000);
                    return true;
                }
            }
            toolName = "ffmpeg";
            break;
            
        case EncoderType::AAC:
            toolName = "ffmpeg";
            break;
    }
    
    // Check if tool is available
    juce::ChildProcess check;
    if (check.start(toolName + " -version")) {
        check.waitForProcessToFinish(2000);
        return true;
    }
    
    return false;
}

juce::String AudioEncoder::getEncoderName(EncoderType type) {
    switch (type) {
        case EncoderType::MP3: {
            // Prefer LAME for MP3
            juce::ChildProcess lameCheck;
            if (lameCheck.start("lame --version")) {
                lameCheck.waitForProcessToFinish(2000);
                return "LAME";
            }
            return "FFmpeg";
        }
        case EncoderType::AAC:
            return "FFmpeg";
    }
    return "Unknown";
}

//==============================================================================
// Command Building
//==============================================================================

juce::String AudioEncoder::buildFFmpegCommand(const juce::File& input,
                                               const juce::File& output,
                                               const EncoderConfig& config) const {
    juce::StringArray args;
    args.add("ffmpeg");
    args.add("-y");  // Overwrite output
    args.add("-i");
    args.add(input.getFullPathName());
    
    // Audio codec settings
    if (config.type == EncoderType::MP3) {
        args.add("-c:a");
        args.add("libmp3lame");
        
        if (config.useVBR) {
            // VBR quality (0 = best, 9 = worst)
            int vbrQuality = 2; // Default to high quality
            switch (config.quality) {
                case EncoderQuality::Low:     vbrQuality = 6; break;
                case EncoderQuality::Medium:  vbrQuality = 4; break;
                case EncoderQuality::High:    vbrQuality = 2; break;
                case EncoderQuality::Maximum: vbrQuality = 0; break;
            }
            args.add("-q:a");
            args.add(juce::String(vbrQuality));
        } else {
            args.add("-b:a");
            args.add(juce::String(config.getEffectiveBitrate()) + "k");
        }
    } else if (config.type == EncoderType::AAC) {
        args.add("-c:a");
        args.add("aac");
        args.add("-b:a");
        args.add(juce::String(config.getEffectiveBitrate()) + "k");
    }
    
    // Sample rate
    if (config.sampleRate > 0) {
        args.add("-ar");
        args.add(juce::String(static_cast<int>(config.sampleRate)));
    }
    
    // Channels
    args.add("-ac");
    args.add(juce::String(config.numChannels));
    
    // Output
    args.add(output.getFullPathName());
    
    return args.joinIntoString(" ");
}

juce::String AudioEncoder::buildLameCommand(const juce::File& input,
                                             const juce::File& output,
                                             const EncoderConfig& config) const {
    juce::StringArray args;
    args.add("lame");
    
    if (config.useVBR) {
        // VBR mode
        args.add("-V");
        int vbrQuality = 2;
        switch (config.quality) {
            case EncoderQuality::Low:     vbrQuality = 6; break;
            case EncoderQuality::Medium:  vbrQuality = 4; break;
            case EncoderQuality::High:    vbrQuality = 2; break;
            case EncoderQuality::Maximum: vbrQuality = 0; break;
        }
        args.add(juce::String(vbrQuality));
    } else {
        // CBR mode
        args.add("-b");
        args.add(juce::String(config.getEffectiveBitrate()));
    }
    
    // Input and output
    args.add(input.getFullPathName());
    args.add(output.getFullPathName());
    
    return args.joinIntoString(" ");
}

//==============================================================================
// Encoding
//==============================================================================

bool AudioEncoder::encodeFile(const juce::File& inputWavFile,
                               const juce::File& outputFile,
                               const EncoderConfig& config,
                               ProgressCallback progressCallback) {
    cancelled_.store(false);
    lastError_.clear();
    
    if (!inputWavFile.existsAsFile()) {
        lastError_ = "Input file does not exist: " + inputWavFile.getFullPathName();
        return false;
    }
    
    // Delete existing output file
    if (outputFile.existsAsFile()) {
        if (!outputFile.deleteFile()) {
            lastError_ = "Cannot overwrite output file: " + outputFile.getFullPathName();
            return false;
        }
    }
    
    // Build command based on encoder type and availability
    juce::String command;
    
    if (config.type == EncoderType::MP3) {
        // Try LAME first for MP3
        juce::ChildProcess lameCheck;
        if (lameCheck.start("lame --version")) {
            lameCheck.waitForProcessToFinish(2000);
            command = buildLameCommand(inputWavFile, outputFile, config);
        } else {
            // Fall back to FFmpeg
            command = buildFFmpegCommand(inputWavFile, outputFile, config);
        }
    } else {
        // AAC always uses FFmpeg
        command = buildFFmpegCommand(inputWavFile, outputFile, config);
    }
    
    if (progressCallback) {
        progressCallback(0.1f, "Starting encoder...");
    }
    
    // Run the encoder
    if (!runEncoderProcess(command, progressCallback)) {
        return false;
    }
    
    // Verify output was created
    if (!outputFile.existsAsFile()) {
        lastError_ = "Encoding failed - output file was not created";
        return false;
    }
    
    if (progressCallback) {
        progressCallback(1.0f, "Encoding complete!");
    }
    
    return true;
}

bool AudioEncoder::encodeBuffer(const juce::AudioBuffer<float>& buffer,
                                 double sampleRate,
                                 const juce::File& outputFile,
                                 const EncoderConfig& config,
                                 ProgressCallback progressCallback) {
    cancelled_.store(false);
    lastError_.clear();
    
    // Create temporary WAV file
    juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    juce::File tempWav = tempDir.getChildFile(
        "zenith_encode_temp_" + juce::String(juce::Time::currentTimeMillis()) + ".wav");
    
    if (progressCallback) {
        progressCallback(0.0f, "Preparing audio...");
    }
    
    // Write buffer to temp WAV
    if (!writeBufferToTempWav(buffer, sampleRate, tempWav)) {
        return false;
    }
    
    // Encode the temp file
    EncoderConfig configCopy = config;
    configCopy.sampleRate = sampleRate;
    configCopy.numChannels = buffer.getNumChannels();
    
    bool result = encodeFile(tempWav, outputFile, configCopy, progressCallback);
    
    // Clean up temp file
    tempWav.deleteFile();
    
    return result;
}

bool AudioEncoder::writeBufferToTempWav(const juce::AudioBuffer<float>& buffer,
                                         double sampleRate,
                                         const juce::File& tempFile) {
    tempFile.deleteFile();
    
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::OutputStream> stream(tempFile.createOutputStream());
    
    if (!stream) {
        lastError_ = "Failed to create temporary WAV file";
        return false;
    }
    
    auto writerOptions = juce::AudioFormatWriterOptions()
        .withSampleRate(sampleRate)
        .withNumChannels(buffer.getNumChannels())
        .withBitsPerSample(24);  // 24-bit for intermediate file
    
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(stream.get(), writerOptions));
    
    if (!writer) {
        lastError_ = "Failed to create WAV writer";
        return false;
    }
    
    // Writer takes ownership of stream
    stream.release();
    
    if (!writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples())) {
        lastError_ = "Failed to write audio to temporary file";
        return false;
    }
    
    return true;
}

bool AudioEncoder::runEncoderProcess(const juce::String& command,
                                      ProgressCallback progressCallback) {
    DBG("AudioEncoder: Running command: " + command);
    
    encoderProcess_ = std::make_unique<juce::ChildProcess>();
    
    if (!encoderProcess_->start(command)) {
        lastError_ = "Failed to start encoder process";
        encoderProcess_.reset();
        return false;
    }
    
    // Wait for process with progress updates
    int pollCount = 0;
    while (encoderProcess_->isRunning()) {
        if (cancelled_.load()) {
            encoderProcess_->kill();
            lastError_ = "Encoding cancelled by user";
            encoderProcess_.reset();
            return false;
        }
        
        juce::Thread::sleep(100);
        pollCount++;
        
        // Simulate progress (we can't get real progress from most encoders)
        if (progressCallback && pollCount % 5 == 0) {
            float fakeProgress = juce::jmin(0.9f, 0.1f + (pollCount * 0.01f));
            progressCallback(fakeProgress, "Encoding audio...");
        }
    }
    
    // Check exit code
    uint32_t exitCode = encoderProcess_->getExitCode();
    encoderProcess_.reset();
    
    if (exitCode != 0) {
        lastError_ = "Encoder exited with error code " + juce::String(exitCode);
        return false;
    }
    
    return true;
}

void AudioEncoder::cancel() {
    cancelled_.store(true);
    
    if (encoderProcess_ && encoderProcess_->isRunning()) {
        encoderProcess_->kill();
    }
}

} // namespace zenith
