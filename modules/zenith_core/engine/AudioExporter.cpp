/*
  ==============================================================================

    AudioExporter.cpp
    Created: 2026-02-03
    Author: Claude (Anthropic)
    
    PRODUCTION-READY implementation with comprehensive error handling

  ==============================================================================
*/

#include "../../modules/zenith_core/engine/AudioExporter.h"
#include "../../modules/zenith_core/engine/Engine.h"
#include <chrono>

namespace zenith {

AudioExporter::AudioExporter()
{
    exportStartTime_ = std::chrono::steady_clock::now();
}

AudioExporter::~AudioExporter()
{
    cancelExport();
    if (exportThread_ && exportThread_->joinable())
        exportThread_->join();
    
    cleanupTempFiles();
}

juce::int64 AudioExporter::getAvailableDiskSpace(const juce::File& outputFile)
{
    auto parentDir = outputFile.getParentDirectory();
    if (!parentDir.exists())
        return -1;
    
    return parentDir.getBytesFreeOnVolume();
}

juce::String AudioExporter::performPreflightChecks(const Settings& settings, double estimatedDuration)
{
    // Validate settings
    auto validationError = settings.validate();
    if (validationError.isNotEmpty())
        return validationError;
    
    // Check if file exists and overwrite policy
    auto finalFile = settings.outputFile.withFileExtension(settings.getFileExtension());
    
    if (finalFile.existsAsFile() && !settings.overwriteWithoutAsking)
    {
        return "File already exists: " + finalFile.getFullPathName() + 
               " (set overwriteWithoutAsking=true or provide overwrite callback)";
    }
    
    // Estimate file size
    juce::int64 estimatedSize = settings.estimateFileSize(estimatedDuration);
    
    // Check disk space (require 2x estimated size for safety)
    juce::int64 availableSpace = getAvailableDiskSpace(finalFile);
    
    if (availableSpace < 0)
    {
        return "Cannot determine available disk space";
    }
    
    juce::int64 requiredSpace = estimatedSize * 2;  // 2x for temp file + final file
    
    if (availableSpace < requiredSpace)
    {
        double requiredMB = requiredSpace / (1024.0 * 1024.0);
        double availableMB = availableSpace / (1024.0 * 1024.0);
        
        return juce::String::formatted(
            "Insufficient disk space. Required: %.1f MB, Available: %.1f MB",
            requiredMB, availableMB);
    }
    
    return {};  // All checks passed
}

void AudioExporter::cleanupTempFiles()
{
    const juce::ScopedLock lock(tempFileLock_);
    
    if (tempFile_ != juce::File{} && tempFile_.existsAsFile())
    {
        DBG("AudioExporter: Cleaning up temp file: " << tempFile_.getFullPathName());
        tempFile_.deleteFile();
        tempFile_ = juce::File{};
    }
}

bool AudioExporter::exportProject(
    Engine& engine,
    const Settings& settings,
    ProgressCallback onProgress,
    CompletionCallback onComplete,
    OverwriteCallback onOverwrite)
{
    if (isExporting_)
    {
        if (onComplete)
            onComplete(false, "Export already in progress");
        return false;
    }
    
    // Validate settings
    auto validationError = settings.validate();
    if (validationError.isNotEmpty())
    {
        if (onComplete)
            onComplete(false, "Validation error: " + validationError);
        return false;
    }
    
    // Check file overwrite
    auto finalFile = settings.outputFile.withFileExtension(settings.getFileExtension());
    
    if (finalFile.existsAsFile() && !settings.overwriteWithoutAsking)
    {
        if (onOverwrite)
        {
            bool shouldOverwrite = onOverwrite(finalFile);
            if (!shouldOverwrite)
            {
                if (onComplete)
                    onComplete(false, "Export cancelled by user (file already exists)");
                return false;
            }
        }
        else
        {
            if (onComplete)
                onComplete(false, "File already exists: " + finalFile.getFullPathName());
            return false;
        }
    }
    
    // Estimate duration for disk space check
    double estimatedDuration = 60.0;  // Default 1 minute
    if (settings.endTime > 0.0)
    {
        estimatedDuration = settings.endTime - settings.startTime;
    }
    
    // Perform pre-flight checks
    auto preflightError = performPreflightChecks(settings, estimatedDuration);
    if (preflightError.isNotEmpty())
    {
        if (onComplete)
            onComplete(false, "Pre-flight check failed: " + preflightError);
        return false;
    }
    
    isExporting_ = true;
    shouldCancel_ = false;
    currentProgress_ = 0.0f;
    exportStartTime_ = std::chrono::steady_clock::now();
    
    // Create temp file path for atomic writes
    juce::File tempFileToUse;
    if (settings.useAtomicWrites)
    {
        tempFileToUse = finalFile.getSiblingFile(
            finalFile.getFileNameWithoutExtension() + "_temp_" + 
            juce::String(juce::Random::getSystemRandom().nextInt()) +
            settings.getFileExtension());
        
        const juce::ScopedLock lock(tempFileLock_);
        tempFile_ = tempFileToUse;
    }
    else
    {
        tempFileToUse = finalFile;
    }
    
    // Launch export in background thread
    exportThread_ = std::make_unique<std::thread>(
        [this, &engine, settings, tempFileToUse, finalFile, onProgress, onComplete]()
    {
        bool success = false;
        juce::String message;
        
        try
        {
            // Convert settings to Engine::ExportOptions
            Engine::ExportOptions engineOptions;
            engineOptions.outputFile = tempFileToUse;
            engineOptions.sampleRate = settings.sampleRate;
            engineOptions.bitDepth = settings.bitDepth;
            engineOptions.normalize = settings.normalize;
            engineOptions.normalizeDb = settings.normalize ? -0.1 : 0.0;
            
            // Map format
            switch (settings.format)
            {
                case Settings::Format::WAV:
                    engineOptions.format = Engine::ExportFormat::WAV;
                    break;
                case Settings::Format::FLAC:
                    engineOptions.format = Engine::ExportFormat::FLAC;
                    break;
                case Settings::Format::AIFF:
                    engineOptions.format = Engine::ExportFormat::AIFF;
                    break;
                case Settings::Format::OGG:
                    engineOptions.format = Engine::ExportFormat::OGG;
                    break;
            }
            
            // Calculate duration
            if (settings.endTime > 0.0)
            {
                engineOptions.duration = settings.endTime - settings.startTime;
            }
            else
            {
                engineOptions.duration = 0.0;  // Auto-detect
            }
            
            // Set up progress callback
            if (onProgress)
            {
                engineOptions.progressCallback = [this, onProgress](float progress, const juce::String& status)
                {
                    if (shouldCancel_)
                        return;  // Don't update if cancelled
                    
                    currentProgress_ = progress;
                    
                    // Calculate estimated time remaining
                    if (progress > 0.01f)
                    {
                        auto now = std::chrono::steady_clock::now();
                        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now - exportStartTime_).count() / 1000.0;
                        
                        double estimatedTotal = elapsed / progress;
                        estimatedTimeRemaining_ = estimatedTotal - elapsed;
                    }
                    
                    // Call user's progress callback on message thread
                    juce::MessageManager::callAsync([onProgress, progress, status]()
                    {
                        onProgress(progress, status);
                    });
                };
            }
            
            // Perform export
            DBG("AudioExporter: Starting export to " << tempFileToUse.getFullPathName());
            success = engine.exportProject(engineOptions);
            
            if (success && !shouldCancel_)
            {
                // Atomic rename if using temp file
                if (settings.useAtomicWrites && tempFileToUse != finalFile)
                {
                    DBG("AudioExporter: Performing atomic rename");
                    
                    // Delete existing file if it exists
                    if (finalFile.existsAsFile())
                        finalFile.deleteFile();
                    
                    // Rename temp to final
                    if (!tempFileToUse.moveFileTo(finalFile))
                    {
                        success = false;
                        message = "Failed to rename temp file to final destination";
                    }
                    else
                    {
                        message = "Export complete: " + finalFile.getFullPathName();
                        
                        // Clear temp file reference
                        const juce::ScopedLock lock(tempFileLock_);
                        tempFile_ = juce::File{};
                    }
                }
                else
                {
                    message = "Export complete: " + finalFile.getFullPathName();
                }
            }
            else if (shouldCancel_)
            {
                success = false;
                message = "Export cancelled by user";
                
                // Clean up temp file
                cleanupTempFiles();
            }
            else
            {
                success = false;
                message = "Export failed (Engine returned false)";
                
                // Clean up temp file
                cleanupTempFiles();
            }
        }
        catch (const std::exception& e)
        {
            success = false;
            message = juce::String("Export failed with exception: ") + e.what();
            
            // Clean up temp file
            cleanupTempFiles();
        }
        catch (...)
        {
            success = false;
            message = "Export failed with unknown exception";
            
            // Clean up temp file
            cleanupTempFiles();
        }
        
        // Call completion callback on message thread
        if (onComplete)
        {
            juce::MessageManager::callAsync([onComplete, success, message]()
            {
                onComplete(success, message);
            });
        }
        
        isExporting_ = false;
        currentProgress_ = 0.0f;
        estimatedTimeRemaining_ = -1.0;
    });
    
    return true;
}

bool AudioExporter::exportStems(
    Engine& engine,
    const Settings& baseSettings,
    ProgressCallback onProgress,
    CompletionCallback onComplete,
    OverwriteCallback onOverwrite)
{
    if (isExporting_)
    {
        if (onComplete)
            onComplete(false, "Export already in progress");
        return false;
    }
    
    // Validate base settings
    auto validationError = baseSettings.validate();
    if (validationError.isNotEmpty())
    {
        if (onComplete)
            onComplete(false, "Validation error: " + validationError);
        return false;
    }
    
    // Perform pre-flight checks
    double estimatedDuration = 60.0;
    if (baseSettings.endTime > 0.0)
        estimatedDuration = baseSettings.endTime - baseSettings.startTime;
    
    auto preflightError = performPreflightChecks(baseSettings, estimatedDuration);
    if (preflightError.isNotEmpty())
    {
        if (onComplete)
            onComplete(false, "Pre-flight check failed: " + preflightError);
        return false;
    }
    
    isExporting_ = true;
    shouldCancel_ = false;
    currentProgress_ = 0.0f;
    exportStartTime_ = std::chrono::steady_clock::now();
    
    // Use Engine's built-in stem export
    exportThread_ = std::make_unique<std::thread>(
        [this, &engine, baseSettings, onProgress, onComplete]()
    {
        bool success = false;
        juce::String message;
        
        try
        {
            Engine::ExportOptions engineOptions;
            engineOptions.outputFile = baseSettings.outputFile.withFileExtension(
                baseSettings.getFileExtension());
            engineOptions.sampleRate = baseSettings.sampleRate;
            engineOptions.bitDepth = baseSettings.bitDepth;
            engineOptions.normalize = baseSettings.normalize;
            engineOptions.exportStems = true;
            
            // Map format
            switch (baseSettings.format)
            {
                case Settings::Format::WAV:
                    engineOptions.format = Engine::ExportFormat::WAV;
                    break;
                case Settings::Format::FLAC:
                    engineOptions.format = Engine::ExportFormat::FLAC;
                    break;
                case Settings::Format::AIFF:
                    engineOptions.format = Engine::ExportFormat::AIFF;
                    break;
                case Settings::Format::OGG:
                    engineOptions.format = Engine::ExportFormat::OGG;
                    break;
            }
            
            // Set up progress callback
            if (onProgress)
            {
                engineOptions.progressCallback = [this, onProgress](float progress, const juce::String& status)
                {
                    currentProgress_ = progress;
                    
                    juce::MessageManager::callAsync([onProgress, progress, status]()
                    {
                        onProgress(progress, status);
                    });
                };
            }
            
            success = engine.exportProject(engineOptions);
            
            if (success && !shouldCancel_)
            {
                message = "Stem export complete";
            }
            else if (shouldCancel_)
            {
                message = "Export cancelled by user";
                success = false;
            }
            else
            {
                message = "Stem export failed";
            }
        }
        catch (const std::exception& e)
        {
            success = false;
            message = juce::String("Stem export failed: ") + e.what();
        }
        
        if (onComplete)
        {
            juce::MessageManager::callAsync([onComplete, success, message]()
            {
                onComplete(success, message);
            });
        }
        
        isExporting_ = false;
        currentProgress_ = 0.0f;
    });
    
    return true;
}

void AudioExporter::cancelExport()
{
    shouldCancel_ = true;
    
    // Clean up temp files
    cleanupTempFiles();
}

double AudioExporter::getEstimatedTimeRemaining() const
{
    if (!isExporting_)
        return -1.0;
    
    return estimatedTimeRemaining_.load();
}

} // namespace zenith
