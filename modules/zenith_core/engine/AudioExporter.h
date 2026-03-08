/*
  ==============================================================================

    AudioExporter.h
    Created: 2026-02-03
    Author: Claude (Anthropic)
    
    PRODUCTION-READY Audio Export System
    
    Features:
    - Input validation
    - Disk space checking
    - File overwrite protection
    - Atomic writes (temp file + rename)
    - Comprehensive error handling
    - Thread-safe Engine access
    - Cleanup on failure

  ==============================================================================
*/

#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>

namespace zenith {

// Forward declaration
class Engine;

/**
 * @brief Production-ready audio export with comprehensive error handling
 * 
 * This class provides safe, reliable audio export with:
 * - Input validation (sample rate, bit depth, format compatibility)
 * - Disk space checking before export
 * - File overwrite confirmation
 * - Atomic writes (temp file + rename on success)
 * - Automatic cleanup on failure
 * - Thread-safe Engine access
 * - Accurate progress tracking
 * - Safe cancellation
 * 
 * Thread Safety: All public methods called from Message Thread.
 * Export happens on background thread with proper synchronization.
 */
class AudioExporter
{
public:
    //==========================================================================
    // Export Settings
    //==========================================================================
    struct Settings
    {
        enum class Format
        {
            WAV,    ///< Uncompressed PCM (16/24/32-bit)
            FLAC,   ///< Lossless compression (16/24-bit only)
            AIFF,   ///< Apple format (16/24/32-bit)
            OGG     ///< Lossy compression (16-bit only, quality-based)
        };
        
        juce::File outputFile;        ///< Output file path (extension added automatically)
        Format format = Format::WAV;  ///< Audio format
        int sampleRate = 48000;       ///< Output sample rate (8000-192000)
        int bitDepth = 24;            ///< Bit depth (16, 24, or 32)
        bool normalize = true;        ///< Peak normalize to -0.1 dBFS
        bool exportStems = false;     ///< One file per track
        double startTime = 0.0;       ///< Start time in seconds
        double endTime = -1.0;        ///< End time (-1 = auto-detect)
        
        // File handling
        bool overwriteWithoutAsking = false;  ///< If false, returns error if file exists
        bool useAtomicWrites = true;          ///< Write to .tmp then rename (recommended)
        
        /** Get file extension for format */
        juce::String getFileExtension() const
        {
            switch (format)
            {
                case Format::WAV:  return ".wav";
                case Format::FLAC: return ".flac";
                case Format::AIFF: return ".aiff";
                case Format::OGG:  return ".ogg";
                default:           return ".wav";
            }
        }
        
        /** Validate settings and return error message if invalid */
        juce::String validate() const
        {
            // Check output file
            if (outputFile == juce::File{})
                return "Output file not specified";
            
            // Check parent directory exists and is writable
            auto parentDir = outputFile.getParentDirectory();
            if (!parentDir.exists())
                return "Output directory does not exist: " + parentDir.getFullPathName();
            
            if (!parentDir.hasWriteAccess())
                return "No write permission for directory: " + parentDir.getFullPathName();
            
            // Check sample rate
            if (sampleRate < 8000 || sampleRate > 192000)
                return "Sample rate must be between 8000 and 192000 Hz";
            
            // Common sample rates check
            const int validRates[] = {8000, 11025, 16000, 22050, 44100, 48000, 88200, 96000, 176400, 192000};
            bool validRate = false;
            for (int rate : validRates)
            {
                if (sampleRate == rate)
                {
                    validRate = true;
                    break;
                }
            }
            if (!validRate)
                return "Unsupported sample rate. Use standard rates (44100, 48000, 96000, etc.)";
            
            // Check bit depth
            if (bitDepth != 16 && bitDepth != 24 && bitDepth != 32)
                return "Bit depth must be 16, 24, or 32";
            
            // Format-specific validation
            switch (format)
            {
                case Format::FLAC:
                    if (bitDepth == 32)
                        return "FLAC format does not support 32-bit depth. Use 16 or 24-bit.";
                    break;
                    
                case Format::OGG:
                    if (bitDepth != 16)
                        return "OGG format only supports 16-bit depth";
                    break;
                    
                default:
                    break;
            }
            
            // Check time range
            if (startTime < 0.0)
                return "Start time cannot be negative";
            
            if (endTime > 0.0 && endTime <= startTime)
                return "End time must be greater than start time";
            
            return {};  // Valid
        }
        
        /** Check if settings are valid */
        bool isValid() const
        {
            return validate().isEmpty();
        }
        
        /** Estimate output file size in bytes */
        juce::int64 estimateFileSize(double durationSeconds) const
        {
            // Calculate uncompressed size
            juce::int64 bytesPerSample = bitDepth / 8;
            juce::int64 samplesPerSecond = sampleRate * 2;  // Stereo
            juce::int64 uncompressedSize = (juce::int64)(durationSeconds * samplesPerSecond * bytesPerSample);
            
            // Apply compression estimates
            switch (format)
            {
                case Format::WAV:
                case Format::AIFF:
                    return uncompressedSize + 1024;  // Header overhead
                    
                case Format::FLAC:
                    return (juce::int64)(uncompressedSize * 0.6);  // ~40% compression
                    
                case Format::OGG:
                    return (juce::int64)(uncompressedSize * 0.15); // ~85% compression (quality dependent)
                    
                default:
                    return uncompressedSize;
            }
        }
    };
    
    //==========================================================================
    // Callback Types
    //==========================================================================
    
    /** Called periodically with progress (0.0 to 1.0) and status message */
    using ProgressCallback = std::function<void(float progress, juce::String status)>;
    
    /** Called when export completes or fails */
    using CompletionCallback = std::function<void(bool success, juce::String message)>;
    
    /** Called to confirm file overwrite (return true to proceed) */
    using OverwriteCallback = std::function<bool(juce::File existingFile)>;
    
    //==========================================================================
    // Constructor / Destructor
    //==========================================================================
    AudioExporter();
    ~AudioExporter();
    
    //==========================================================================
    // Export Operations
    //==========================================================================
    
    /**
     * @brief Export project with full validation and error handling
     * 
     * This method performs:
     * 1. Input validation
     * 2. Disk space checking
     * 3. File overwrite confirmation (if needed)
     * 4. Atomic export (temp file + rename)
     * 5. Automatic cleanup on failure
     * 
     * @param engine The audio engine
     * @param settings Export configuration
     * @param onProgress Progress callback (0.0 to 1.0)
     * @param onComplete Completion callback
     * @param onOverwrite Overwrite confirmation callback (optional)
     * 
     * @return true if export started successfully, false with error message
     */
    bool exportProject(
        Engine& engine,
        const Settings& settings,
        ProgressCallback onProgress,
        CompletionCallback onComplete,
        OverwriteCallback onOverwrite = nullptr);
    
    /**
     * @brief Export individual stems with validation
     */
    bool exportStems(
        Engine& engine,
        const Settings& baseSettings,
        ProgressCallback onProgress,
        CompletionCallback onComplete,
        OverwriteCallback onOverwrite = nullptr);
    
    /**
     * @brief Cancel ongoing export
     * 
     * This safely stops the export and cleans up temp files.
     */
    void cancelExport();
    
    /**
     * @brief Check if export is currently running
     */
    bool isExporting() const { return isExporting_.load(); }
    
    /**
     * @brief Get estimated time remaining (seconds)
     * 
     * @return Estimated seconds remaining, or -1 if not exporting
     */
    double getEstimatedTimeRemaining() const;
    
    /**
     * @brief Get current progress (0.0 to 1.0)
     */
    float getCurrentProgress() const { return currentProgress_.load(); }
    
    /**
     * @brief Get available disk space for output path
     * 
     * @param outputFile The target output file
     * @return Available bytes, or -1 on error
     */
    static juce::int64 getAvailableDiskSpace(const juce::File& outputFile);
    
private:
    //==========================================================================
    // Internal Methods
    //==========================================================================
    
    /** Pre-flight checks before starting export */
    juce::String performPreflightChecks(const Settings& settings, double estimatedDuration);
    
    /** Clean up temporary files */
    void cleanupTempFiles();
    
    //==========================================================================
    // Member Variables
    //==========================================================================
    std::atomic<bool> isExporting_{false};
    std::atomic<bool> shouldCancel_{false};
    std::atomic<float> currentProgress_{0.0f};
    std::atomic<double> estimatedTimeRemaining_{-1.0};
    
    std::unique_ptr<std::thread> exportThread_;
    std::chrono::steady_clock::time_point exportStartTime_;
    
    juce::File tempFile_;  // Temporary file for atomic writes
    juce::CriticalSection tempFileLock_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioExporter)
};

} // namespace zenith
