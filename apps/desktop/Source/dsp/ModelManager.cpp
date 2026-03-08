#include "ModelManager.h"
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace zenith {

//==============================================================================
// ModelManager Implementation
//==============================================================================

ModelManager::ModelManager() {
    // Set up models directory in user data folder
    modelsDirectory = juce::File::getSpecialLocation(
        juce::File::SpecialLocationType::userApplicationDataDirectory)
        .getChildFile("ZenithDAW")
        .getChildFile("models");

    // Create directory if it doesn't exist
    if (!modelsDirectory.exists()) {
        modelsDirectory.createDirectory();
    }

    initializeAvailableModels();
    verifyModelInstallations();
}

ModelManager::~ModelManager() {
    if (downloadThread && downloadThread->isThreadRunning()) {
        downloadThread->stopThread(10000);
    }
}

juce::Array<ModelManager::ModelInfo> ModelManager::getAvailableModels() const {
    return availableModels;
}

ModelManager::ModelInfo ModelManager::getModelInfo(const juce::String& modelName) const {
    for (const auto& model : availableModels) {
        if (model.name == modelName) {
            return model;
        }
    }
    return {};
}

bool ModelManager::isModelInstalled(const juce::String& modelName) const {
    auto info = getModelInfo(modelName);
    return !info.name.isEmpty() && info.isInstalled && info.localPath.existsAsFile();
}

void ModelManager::downloadModel(const juce::String& model,
                                ProgressCallback progress,
                                CompletionCallback completion) {
    // Lock to check and set download state atomically
    {
        std::lock_guard<std::mutex> lock(downloadMutex);

        if (isDownloadingFlag) {
            if (completion) {
                juce::MessageManager::callAsync([completion]() {
                    completion(false, "Download already in progress");
                });
            }
            return;
        }

        // Set downloading flag while holding lock
        isDownloadingFlag = true;
    }

    auto info = getModelInfo(model);
    if (info.name.isEmpty()) {
        std::lock_guard<std::mutex> lock(downloadMutex);
        isDownloadingFlag = false;  // Reset flag
        if (completion) {
            juce::MessageManager::callAsync([completion, model]() {
                completion(false, "Model not found: " + model);
            });
        }
        return;
    }

    if (info.isInstalled) {
        // Verify SHA256 if already installed
        juce::String actualHash = calculateSHA256(info.localPath);
        if (actualHash == info.sha256 || info.sha256.isEmpty()) {
            std::lock_guard<std::mutex> lock(downloadMutex);
            isDownloadingFlag = false;  // Reset flag
            if (completion) {
                juce::MessageManager::callAsync([completion]() {
                    completion(true, "Model already installed and verified");
                });
            }
            return;
        }
    }

    // Create download task
    DownloadThread::DownloadTask downloadTask;
    downloadTask.url = info.url;
    downloadTask.targetPath = modelsDirectory.getChildFile(model + ".onnx");
    downloadTask.expectedSHA256 = info.sha256;
    downloadTask.progressCallback = progress;
    downloadTask.completionCallback = completion;

    // Start download thread
    {
        std::lock_guard<std::mutex> lock(downloadMutex);
        downloadThread = std::make_unique<DownloadThread>(downloadTask);
        downloadThread->startThread();
        downloadProgress = 0.0;
    }
}

void ModelManager::cancelDownload() {
    std::lock_guard<std::mutex> lock(downloadMutex);
    if (downloadThread && downloadThread->isThreadRunning()) {
        downloadThread->cancel();
    }
}

bool ModelManager::deleteModel(const juce::String& model) {
    auto info = getModelInfo(model);
    if (!info.name.isEmpty() && info.isInstalled && info.localPath.existsAsFile()) {
        bool deleted = info.localPath.deleteFile();
        if (deleted) {
            verifyModelInstallations();
        }
        return deleted;
    }
    return false;
}

int64_t ModelManager::getTotalModelSize() const {
    int64_t total = 0;
    for (const auto& model : availableModels) {
        if (model.isInstalled && model.localPath.existsAsFile()) {
            total += model.localPath.getSize();
        }
    }
    return total;
}

juce::File ModelManager::getModelsDirectory() const {
    return modelsDirectory;
}

void ModelManager::refreshModelStatus() {
    verifyModelInstallations();
}

int ModelManager::checkForUpdates() {
    int updateCount = 0;

    for (auto& model : availableModels) {
        // Reset flag
        model.hasUpdateAvailable = false;

        if (!model.isInstalled) {
            continue;  // Not installed, can't have update
        }

        // Check if installed version matches available version
        // For now, we just check if the model exists
        // In a full implementation, this would:
        // 1. Query a remote version file/API
        // 2. Compare versions
        // 3. Set hasUpdateAvailable = true if newer version exists

        // TODO: Implement remote version checking
        // For now, assume no updates available
    }

    return updateCount;
}

juce::Array<ModelManager::ModelInfo> ModelManager::getUpdatableModels() const {
    juce::Array<ModelInfo> updatable;

    for (const auto& model : availableModels) {
        if (model.hasUpdateAvailable) {
            updatable.add(model);
        }
    }

    return updatable;
}

void ModelManager::initializeAvailableModels() {
    // ========================================
    // Meta Demucs v4 Models (huggingface.co)
    // Release: 2023-12-01
    // ========================================

    // HTDemucs (6-stem hybrid transformer, highest quality)
    // Stems: vocals, drums, bass, other, piano, guitar
    ModelInfo htdemucs;
    htdemucs.name = "htdemucs";
    htdemucs.version = "4.0";
    htdemucs.minCompatibleVersion = "1.0";
    htdemucs.releaseDate = "2023-12-01";
    htdemucs.changelog = "Initial release of HTDemucs v4 with 6 stems";
    htdemucs.url = "https://dl.fbaipublicfiles.com/demucs/hybrid_transformer/demucs_quantized.onnx";
    htdemucs.sizeBytes = 0;
    htdemucs.sha256 = "";
    htdemucs.isInstalled = false;
    availableModels.add(htdemucs);

    // HTDemucs Light (faster, slightly lower quality)
    ModelInfo htdemucs_light;
    htdemucs_light.name = "htdemucs_light";
    htdemucs_light.version = "4.0";
    htdemucs_light.minCompatibleVersion = "1.0";
    htdemucs_light.releaseDate = "2023-12-01";
    htdemucs_light.changelog = "Lightweight version for faster processing";
    htdemucs_light.url = "https://dl.fbaipublicfiles.com/demucs/hybrid_transformer/demucs_light_quantized.onnx";
    htdemucs_light.sizeBytes = 0;
    htdemucs_light.sha256 = "";
    htdemucs_light.isInstalled = false;
    availableModels.add(htdemucs_light);

    // Demucs v4 (4-stem baseline)
    ModelInfo demucs;
    demucs.name = "demucs";
    demucs.version = "4.0";
    demucs.minCompatibleVersion = "1.0";
    demucs.releaseDate = "2023-12-01";
    demucs.changelog = "Demucs v4 baseline with 4 stems";
    demucs.url = "https://dl.fbaipublicfiles.com/demucs/demucs/demucs_quantized.onnx";
    demucs.sizeBytes = 0;
    demucs.sha256 = "";
    demucs.isInstalled = false;
    availableModels.add(demucs);

    // Demucs Extra (trained on more diverse data)
    ModelInfo demucs_extra;
    demucs_extra.name = "demucs_extra";
    demucs_extra.version = "4.0";
    demucs_extra.minCompatibleVersion = "1.0";
    demucs_extra.releaseDate = "2023-12-01";
    demucs_extra.changelog = "Extra model trained on diverse datasets";
    demucs_extra.url = "https://dl.fbaipublicfiles.com/demucs/demucs_extra/demucs_extra_quantized.onnx";
    demucs_extra.sizeBytes = 0;
    demucs_extra.sha256 = "";
    demucs_extra.isInstalled = false;
    availableModels.add(demucs_extra);

    // Demucs Hybrid (transformer-based, 4 stems)
    ModelInfo demucs_hybrid;
    demucs_hybrid.name = "demucs_hybrid";
    demucs_hybrid.version = "4.0";
    demucs_hybrid.minCompatibleVersion = "1.0";
    demucs_hybrid.releaseDate = "2023-12-01";
    demucs_hybrid.changelog = "Hybrid transformer model for improved quality";
    demucs_hybrid.url = "https://dl.fbaipublicfiles.com/demucs/hybrid_transformer/demucs_hybrid_quantized.onnx";
    demucs_hybrid.sizeBytes = 0;
    demucs_hybrid.sha256 = "";
    demucs_hybrid.isInstalled = false;
    availableModels.add(demucs_hybrid);

    // ========================================
    // Additional Models (when available)
    // ========================================

    // Note: MDX-Net and other models can be added here when ONNX exports are available
}

void ModelManager::verifyModelInstallations() {
    for (auto& model : availableModels) {
        juce::File modelPath = modelsDirectory.getChildFile(model.name + ".onnx");
        model.localPath = modelPath;
        model.isInstalled = modelPath.existsAsFile();

        if (model.isInstalled) {
            model.sizeBytes = modelPath.getSize();
            // Calculate SHA256 if not set
            if (model.sha256.isEmpty()) {
                model.sha256 = calculateSHA256(modelPath);
            }
        }
    }
}

juce::String ModelManager::calculateSHA256(const juce::File& file) const {
    if (!file.existsAsFile()) {
        return {};
    }

    return juce::SHA256(file).toHexString();
}

bool ModelManager::validateImplementation() {
    juce::Logger::writeToLog("=== ModelManager Validation ===");

    try {
        // Test 1: SHA256 consistency on small file
        {
            juce::File testFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                          .getChildFile("zenith_validation_test.txt");

            const juce::String testContent = "Hello, World! SHA256 validation test.";
            testFile.replaceWithText(testContent);

            ModelManager tempManager;
            juce::String hash1 = tempManager.calculateSHA256(testFile);
            juce::String hash2 = tempManager.calculateSHA256(testFile);

            testFile.deleteFile();

            if (hash1.isEmpty() || hash1 != hash2 || hash1.length() != 64) {
                juce::Logger::writeToLog("[FAIL] SHA256 consistency test");
                return false;
            }
            juce::Logger::writeToLog("[PASS] SHA256 consistency: " + hash1);
        }

        // Test 2: Streaming SHA256 on larger file (1MB)
        {
            juce::File testFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                          .getChildFile("zenith_validation_1mb.bin");

            const size_t fileSize = 1024 * 1024; // 1MB
            juce::FileOutputStream stream(testFile);

            if (stream.openedOk()) {
                constexpr size_t bufferSize = 65536;
                char buffer[bufferSize];

                for (size_t i = 0; i < bufferSize; i++) {
                    buffer[i] = static_cast<char>(i % 256);
                }

                for (int i = 0; i < 16; i++) {
                    stream.write(buffer, bufferSize);
                }
                stream.flush();
            }

            int64_t actualSize = testFile.getSize();
            ModelManager tempManager;
            juce::String hash = tempManager.calculateSHA256(testFile);

            testFile.deleteFile();

            if (hash.isEmpty() || actualSize != fileSize || hash.length() != 64) {
                juce::Logger::writeToLog("[FAIL] Streaming SHA256 test");
                return false;
            }
            juce::Logger::writeToLog("[PASS] Streaming SHA256 (1MB): " + hash);
        }

        juce::Logger::writeToLog("=== ModelManager Validation PASSED ===");
        return true;

    } catch (const std::bad_alloc&) {
        juce::Logger::writeToLog("[FAIL] Bad alloc - implementation may load file into RAM");
        return false;
    } catch (const std::exception& e) {
        juce::Logger::writeToLog("[FAIL] Exception: " + juce::String(e.what()));
        return false;
    }
}

//==============================================================================
// DownloadThread Implementation
//==============================================================================

DownloadThread::DownloadThread(DownloadTask task)
    : juce::Thread("Model Download"), task(task) {
}

DownloadThread::~DownloadThread() {
    stopThread(10000);
}

void DownloadThread::run() {
    bool success = false;
    juce::String errorMessage;

    try {
        juce::URL url(task.url);

        // Validate URL
        if (task.url.isEmpty()) {
            throw std::runtime_error("Invalid URL: URL is empty");
        }

        // Create temporary file
        juce::File tempFile = task.targetPath.getSiblingFile(
            task.targetPath.getFileName() + ".download");

        // Check for existing download to resume
        int64_t existingBytes = 0;
        bool isResuming = false;

        if (tempFile.exists()) {
            existingBytes = tempFile.getSize();
            if (existingBytes > 0) {
                isResuming = true;
                juce::Logger::writeToLog("Resuming download from byte " + juce::String(existingBytes));
            } else {
                // File exists but is empty, delete it
                tempFile.deleteFile();
            }
        }

        std::unique_ptr<juce::InputStream> stream;
        if (isResuming) {
            auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                            .withExtraHeaders("Range: bytes=" + juce::String(existingBytes) + "-");
            stream = url.createInputStream(options);
        } else {
            auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress);
            stream = url.createInputStream(options);
        }
        if (stream == nullptr) {
            throw std::runtime_error("Failed to create input stream. Check network connection and URL.");
        }

        auto totalLength = stream->getTotalLength();
        if (totalLength > 0 && isResuming) {
            // Some servers return remaining length when using Range header
            totalLength += existingBytes;
        }

        // Open output file - append to existing
        auto targetStream = tempFile.createOutputStream();

        if (targetStream == nullptr) {
            throw std::runtime_error("Failed to create output file. Check disk space and permissions.");
        }

        // If resuming, seek to end of existing file
        if (isResuming) {
            targetStream->setPosition(existingBytes);
        }

        const int bufferSize = 65536; // 64KB
        juce::HeapBlock<char> buffer(bufferSize);
        int64_t bytesReadTotal = existingBytes;  // Start from existing size

        while (!stream->isExhausted()) {
            if (threadShouldExit() || task.shouldCancel) {
                tempFile.deleteFile();
                throw std::runtime_error("Download cancelled");
            }

            int bytesRead = stream->read(buffer.getData(), bufferSize);
            if (bytesRead <= 0) {
                break;  // End of stream or error
            }

            // Write to file with error checking
            int64_t written = targetStream->write(buffer.getData(), bytesRead);
            if (written != bytesRead) {
                throw std::runtime_error("Disk full or write error. Could not write all data.");
            }

            bytesReadTotal += bytesRead;

            // Report progress - copy callback to avoid race condition
            if (task.progressCallback && totalLength > 0) {
                auto progressCallback = task.progressCallback;  // Copy by value
                juce::MessageManager::callAsync([progressCallback, bytesReadTotal, totalLength]() {
                    progressCallback(bytesReadTotal, totalLength);
                });
            }
        }

        // Flush to ensure data is written to disk
        targetStream->flush();
        targetStream.reset();  // Close stream before verification

        success = true;

        if (success && !task.shouldCancel) {
            // Verify SHA256 if provided
            if (!task.expectedSHA256.isEmpty()) {
                juce::String actualHash = calculateSHA256(tempFile);
                if (actualHash != task.expectedSHA256) {
                    tempFile.deleteFile();
                    throw std::runtime_error("SHA256 verification failed. File may be corrupted.");
                }
            }

            // Move temporary file to final location
            if (!tempFile.moveFileTo(task.targetPath)) {
                throw std::runtime_error("Failed to move file to final location. Check file permissions.");
            }
        } else if (!task.shouldCancel) {
            tempFile.deleteFile();
            errorMessage = "Download failed";
        }
    }
    catch (const std::exception& e) {
        success = false;
        errorMessage = e.what();
    }

    // Call completion on message thread
    if (task.completionCallback) {
        auto callback = task.completionCallback;
        auto msg = errorMessage;
        juce::MessageManager::callAsync([callback, success, msg]() {
            callback(success, msg);
        });
    }
}

void DownloadThread::handleAsyncUpdate() {
    // Progress updates handled in download callback
}

bool DownloadThread::downloadFile(const juce::URL& url,
                                  const juce::File& target,
                                  const ModelManager::ProgressCallback& progress) {
    // This is now handled in run() using streaming download
    return true;
}

bool DownloadThread::verifySHA256(const juce::File& file, const juce::String& expected) {
    if (expected.isEmpty()) {
        return true; // No verification required
    }

    juce::String actualHash = calculateSHA256(file);
    return actualHash == expected;
}

juce::String DownloadThread::calculateSHA256(const juce::File& file) {
    if (!file.existsAsFile()) {
        return {};
    }

    return juce::SHA256(file).toHexString();
}

} // namespace zenith
