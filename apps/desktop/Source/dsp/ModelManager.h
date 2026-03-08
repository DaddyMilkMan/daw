#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_cryptography/juce_cryptography.h>
#include <functional>
#include <memory>
#include <vector>
#include <mutex>

namespace zenith {

// Forward declaration
class DownloadThread;

/**
 * @brief Manages download and installation of AI models
 *
 * Handles downloading, verifying, and managing machine learning models
 * for features like stem separation.
 */
class ModelManager {
public:
    struct ModelInfo {
        juce::String name;           // e.g., "htdemucs", "demucs"
        juce::String version;        // e.g., "4.0"
        juce::String url;            // Download URL
        juce::String sha256;         // Checksum for verification
        int64_t sizeBytes = 0;       // Expected file size
        bool isInstalled = false;    // Currently available locally
        juce::File localPath;        // Path to installed model

        // Versioning support
        juce::String minCompatibleVersion;  // Minimum DAW version required
        juce::String releaseDate;           // ISO date: YYYY-MM-DD
        juce::String changelog;             // Brief description of changes
        bool hasUpdateAvailable = false;    // Newer version exists
    };

    using ProgressCallback = std::function<void(int64_t bytesDownloaded, int64_t totalBytes)>;
    using CompletionCallback = std::function<void(bool success, const juce::String& message)>;

    ModelManager();
    ~ModelManager();

    /**
     * @brief Get available models
     */
    juce::Array<ModelInfo> getAvailableModels() const;

    /**
     * @brief Get info about a specific model
     */
    ModelInfo getModelInfo(const juce::String& modelName) const;

    /**
     * @brief Check if a model is installed
     */
    bool isModelInstalled(const juce::String& modelName) const;

    /**
     * @brief Download a model
     * @param model Name of model to download
     * @param progress Callback for download progress (bytes, total)
     * @param completion Callback when download completes
     */
    void downloadModel(const juce::String& model,
                      ProgressCallback progress,
                      CompletionCallback completion);

    /**
     * @brief Cancel active download
     */
    void cancelDownload();

    /**
     * @brief Delete a model to free disk space
     */
    bool deleteModel(const juce::String& model);

    /**
     * @brief Get total disk space used by models
     */
    int64_t getTotalModelSize() const;

    /**
     * @brief Get models directory
     */
    juce::File getModelsDirectory() const;

    /**
     * @brief Refresh installed model status
     */
    void refreshModelStatus();

    /**
     * @brief Check for model updates
     *
     * Compares installed model versions with available versions.
     * Sets hasUpdateAvailable flag on models with newer versions.
     *
     * @return Number of models with updates available
     */
    int checkForUpdates();

    /**
     * @brief Get models that have updates available
     */
    juce::Array<ModelInfo> getUpdatableModels() const;

    /**
     * @brief Validate implementation (for runtime testing)
     *
     * Performs self-tests to verify SHA256 calculation works correctly.
     * Can be called during development/testing to verify implementation.
     *
     * @return true if all validation tests pass
     */
    static bool validateImplementation();

    /**
     * @brief Check if download is in progress
     */
    bool isDownloading() const {
        std::lock_guard<std::mutex> lock(downloadMutex);
        return isDownloadingFlag;
    }

    /**
     * @brief Get current download progress (0.0 to 1.0)
     */
    double getDownloadProgress() const {
        std::lock_guard<std::mutex> lock(downloadMutex);
        return downloadProgress;
    }

private:
    juce::File modelsDirectory;

    // Thread for downloads
    std::unique_ptr<DownloadThread> downloadThread;
    bool isDownloadingFlag = false;
    double downloadProgress = 0.0;
    mutable std::mutex downloadMutex;  // Protects download state (mutable for const methods)

    // Available models registry
    juce::Array<ModelInfo> availableModels;

    void initializeAvailableModels();
    void verifyModelInstallations();
    juce::String calculateSHA256(const juce::File& file) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModelManager)
};

/**
 * @brief Background thread for downloading models
 */
class DownloadThread : public juce::Thread,
                       public juce::AsyncUpdater {
public:
    struct DownloadTask {
        juce::String url;
        juce::File targetPath;
        juce::String expectedSHA256;
        ModelManager::ProgressCallback progressCallback;
        ModelManager::CompletionCallback completionCallback;
        bool shouldCancel = false;
    };

    DownloadThread(DownloadTask task);
    ~DownloadThread() override;

    void cancel() { task.shouldCancel = true; }

private:
    friend class ModelManager;
    DownloadTask task;

    void run() override;
    void handleAsyncUpdate() override;

    bool downloadFile(const juce::URL& url, const juce::File& target,
                      const ModelManager::ProgressCallback& progress);

    bool verifySHA256(const juce::File& file, const juce::String& expected);
    juce::String calculateSHA256(const juce::File& file);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DownloadThread)
};

} // namespace zenith
