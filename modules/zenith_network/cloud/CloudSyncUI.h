/*
  ==============================================================================
    CloudSyncSystem.h
    Cloud synchronization and backup system with version control
    Phase 5: Advanced Features
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_network/juce_network.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>

namespace zenith {
namespace cloud {

// Sync status
enum class SyncStatus {
    Idle,
    Syncing,
    Uploading,
    Downloading,
    Conflict,
    Error,
    Offline
};

// File sync information
class CloudSyncUI : public juce::Component,
                   public CloudSyncSystem::Listener,
                   public juce::Button::Listener,
                   public juce::Timer {
public:
    CloudSyncUI();
    ~CloudSyncUI() override;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Sync system access
    void setSyncSystem(CloudSyncSystem* syncSystem);
    CloudSyncSystem* getSyncSystem() const { return syncSystem; }

    // UI control
    void showSyncStatus(bool show);
    void showProjects(bool show);
    void showBackups(bool show);
    void showSettings(bool show);

    // CloudSyncSystem::Listener
    void syncStatusChanged(SyncStatus status) override;
    void projectSynced(const juce::String& projectId) override;
    void fileSynced(const juce::String& filePath) override;
    void syncError(const juce::String& error) override;
    void conflictDetected(const juce::String& filePath) override;
    void storageInfoUpdated(size_t used, size_t total) override;

    // Button::Listener
    void buttonClicked(juce::Button* button) override;

    // Timer callback for UI updates
    void timerCallback() override;

private:
    CloudSyncSystem* syncSystem = nullptr;

    // UI components
    std::unique_ptr<juce::Viewport> mainViewport;
    std::unique_ptr<juce::Component> mainComponent;

    // Status panel
    std::unique_ptr<juce::Component> statusPanel;
    std::unique_ptr<juce::Label> statusLabel;
    std::unique_ptr<juce::ProgressBar> progressBar;
    std::unique_ptr<juce::TextButton> syncButton;
    std::unique_ptr<juce::TextButton> pauseButton;

    // Storage info
    std::unique_ptr<juce::Label> storageLabel;
    std::unique_ptr<juce::ProgressBar> storageBar;

    // Projects list
    std::unique_ptr<juce::ListBox> projectListBox;
    std::unique_ptr<juce::TextButton> addProjectButton;
    std::unique_ptr<juce::TextButton> removeProjectButton;

    // Backups list
    std::unique_ptr<juce::ListBox> backupListBox;
    std::unique_ptr<juce::TextButton> createBackupButton;
    std::unique_ptr<juce::TextButton> restoreBackupButton;

    // Settings
    std::unique_ptr<juce::TextButton> settingsButton;
    std::unique_ptr<juce::ToggleButton> autoSyncToggle;
    std::unique_ptr<juce::Slider> syncIntervalSlider;

    // Conflict dialog
    std::unique_ptr<juce::DialogWindow> conflictDialog;

    // UI creation
    void createStatusPanel();
    void createProjectsList();
    void createBackupsList();
    void createSettingsPanel();

    // Updates
    void updateStatusDisplay();
    void updateStorageDisplay();
    void updateProjectsList();
    void updateBackupsList();

    // Dialogs
    void showAddProjectDialog();
    void showConflictDialog(const juce::String& filePath);
    void showSettingsDialog();
    void showBackupDialog();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudSyncUI)
};

// Cloud backup manager

} // namespace
