/*
  ==============================================================================
    ProjectManagerUI.h
    Project management UI with comprehensive features
    Phase 4: User Interface
  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../visualizations/WaveformDisplay.h"
#include "../system/UndoRedoSystem.h"
#include <memory>
#include <vector>

namespace zenith {
namespace ui {

// Project information
struct ProjectInfo {
    juce::String name;
    juce::String filePath;
    juce::Time created;
    juce::Time lastModified;
    juce::Time lastSaved;
    juce::String author;
    juce::String description;
    juce::String genre;
    float tempo = 120.0f;
    juce::String key = "C";
    int sampleRate = 44100;
    int bitDepth = 24;
    float duration = 0.0f;  // seconds
    size_t fileSize = 0;    // bytes
    bool isModified = false;
    bool autoSaveEnabled = true;
    int autoSaveInterval = 300;  // seconds
};

// Track information
struct TrackInfo {
    juce::String id;
    juce::String name;
    juce::String type;  // "audio", "midi", "bus", "master"
    bool isMuted = false;
    bool isSoloed = false;
    bool isArmed = false;
    float volume = 0.0f;  // dB
    float pan = 0.0f;     // -1.0 to 1.0
    int channelCount = 2;
    bool isStereo = true;
    juce::Colour color = juce::Colours::blue;
    juce::Time created;
    juce::Time lastModified;
};

// Effect chain information
struct EffectInfo {
    juce::String id;
    juce::String name;
    juce::String type;  // "eq", "compressor", "reverb", "delay", etc.
    bool isEnabled = true;
    bool isBypassed = false;
    int position = 0;    // position in chain
    juce::var parameters;
    juce::String presetName;
    bool hasCustomPreset = false;
};

// Project session information
struct ProjectSession {
    juce::String id;
    juce::String name;
    juce::Time created;
    juce::Time lastAccessed;
    ProjectInfo projectInfo;
    std::vector<TrackInfo> tracks;
    std::unordered_map<juce::String, std::vector<EffectInfo>> effectChains;
    juce::var globalSettings;
    bool isAutoSave = false;
};

// Project manager UI
class ProjectManagerUI : public juce::Component,
                        public juce::Button::Listener,
                        public juce::ComboBox::Listener,
                        public juce::TextEditor::Listener,
                        public juce::FileBrowserListener,
                        public juce::Timer {
public:
    ProjectManagerUI();
    ~ProjectManagerUI() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Project operations
    bool newProject(const juce::String& name = "");
    bool openProject(const juce::File& file);
    bool saveProject();
    bool saveProjectAs(const juce::File& file);
    bool closeProject();
    bool revertProject();
    
    // Project information
    ProjectInfo getCurrentProject() const;
    bool hasProject() const;
    bool isModified() const;
    
    // Track management
    void addTrack(const TrackInfo& track);
    void removeTrack(const juce::String& trackId);
    void moveTrack(const juce::String& trackId, int newPosition);
    TrackInfo getTrack(const juce::String& trackId) const;
    std::vector<TrackInfo> getAllTracks() const;
    
    // Effect management
    void addEffect(const juce::String& trackId, const EffectInfo& effect);
    void removeEffect(const juce::String& trackId, const juce::String& effectId);
    void moveEffect(const juce::String& trackId, const juce::String& effectId, int newPosition);
    void toggleEffect(const juce::String& trackId, const juce::String& effectId);
    std::vector<EffectInfo> getEffects(const juce::String& trackId) const;
    
    // Auto-save
    void enableAutoSave(bool enabled);
    void setAutoSaveInterval(int seconds);
    bool isAutoSaveEnabled() const;
    int getAutoSaveInterval() const;
    
    // Recent projects
    void addRecentProject(const juce::File& file);
    std::vector<juce::File> getRecentProjects() const;
    void clearRecentProjects();
    
    // Project templates
    void loadTemplate(const juce::String& templateName);
    void saveAsTemplate(const juce::String& templateName);
    std::vector<juce::String> getAvailableTemplates() const;
    
    // Export/Import
    bool exportProject(const juce::File& file);
    bool importProject(const juce::File& file);
    bool exportTrack(const juce::String& trackId, const juce::File& file);
    bool importTrack(const juce::File& file);
    
    // Search and filter
    void searchProjects(const juce::String& searchTerm);
    void filterByType(const juce::String& type);
    void filterByDate(const juce::Time& startDate, const juce::Time& endDate);
    
    // Timer callback for auto-save
    void timerCallback() override;
    
    // Listeners
    struct Listener {
        virtual ~Listener() = default;
        virtual void projectOpened(const ProjectInfo& project) {}
        virtual void projectSaved(const ProjectInfo& project) {}
        virtual void projectClosed() {}
        virtual void trackAdded(const TrackInfo& track) {}
        virtual void trackRemoved(const juce::String& trackId) {}
        virtual void effectAdded(const juce::String& trackId, const EffectInfo& effect) {}
        virtual void effectRemoved(const juce::String& trackId, const juce::String& effectId) {}
        virtual void projectModified() {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    // Current project state
    std::unique_ptr<ProjectInfo> currentProject;
    std::vector<TrackInfo> tracks;
    std::unordered_map<juce::String, std::vector<EffectInfo>> effectChains;
    
    // Recent projects
    std::vector<juce::File> recentProjects;
    static constexpr int MAX_RECENT_PROJECTS = 10;
    
    // Auto-save
    bool autoSaveEnabled = true;
    int autoSaveInterval = 300;  // 5 minutes
    juce::Time lastAutoSave;
    
    // UI components
    std::unique_ptr<juce::Viewport> mainViewport;
    std::unique_ptr<juce::Component> mainComponent;
    
    // Project info panel
    std::unique_ptr<juce::Label> projectNameLabel;
    std::unique_ptr<juce::TextEditor> projectNameEditor;
    std::unique_ptr<juce::Label> projectPathLabel;
    std::unique_ptr<juce::Label> projectCreatedLabel;
    std::unique_ptr<juce::Label> projectModifiedLabel;
    std::unique_ptr<juce::Label> projectAuthorLabel;
    std::unique_ptr<juce::TextEditor> projectAuthorEditor;
    std::unique_ptr<juce::Label> projectDescriptionLabel;
    std::unique_ptr<juce::TextEditor> projectDescriptionEditor;
    std::unique_ptr<juce::Label> projectGenreLabel;
    std::unique_ptr<juce::ComboBox> projectGenreComboBox;
    std::unique_ptr<juce::Label> projectTempoLabel;
    std::unique_ptr<juce::Slider> projectTempoSlider;
    std::unique_ptr<juce::Label> projectKeyLabel;
    std::unique_ptr<juce::ComboBox> projectKeyComboBox;
    std::unique_ptr<juce::Label> projectSampleRateLabel;
    std::unique_ptr<juce::ComboBox> projectSampleRateComboBox;
    std::unique_ptr<juce::Label> projectBitDepthLabel;
    std::unique_ptr<juce::ComboBox> projectBitDepthComboBox;
    
    // Project controls
    std::unique_ptr<juce::TextButton> newProjectButton;
    std::unique_ptr<juce::TextButton> openProjectButton;
    std::unique_ptr<juce::TextButton> saveProjectButton;
    std::unique_ptr<juce::TextButton> saveAsProjectButton;
    std::unique_ptr<juce::TextButton> closeProjectButton;
    std::unique_ptr<juce::TextButton> revertProjectButton;
    
    // Track management
    std::unique_ptr<juce::ListBox> trackListBox;
    std::unique_ptr<juce::TextButton> addTrackButton;
    std::unique_ptr<juce::TextButton> removeTrackButton;
    std::unique_ptr<juce::TextButton> moveTrackUpButton;
    std::unique_ptr<juce::TextButton> moveTrackDownButton;
    std::unique_ptr<juce::TextButton> duplicateTrackButton;
    
    // Effect management
    std::unique_ptr<juce::ListBox> effectListBox;
    std::unique_ptr<juce::TextButton> addEffectButton;
    std::unique_ptr<juce::TextButton> removeEffectButton;
    std::unique_ptr<juce::TextButton> moveEffectUpButton;
    std::unique_ptr<juce::TextButton> moveEffectDownButton;
    std::unique_ptr<juce::TextButton> toggleEffectButton;
    
    // Recent projects
    std::unique_ptr<juce::ComboBox> recentProjectsComboBox;
    std::unique_ptr<juce::TextButton> clearRecentButton;
    
    // Templates
    std::unique_ptr<juce::ComboBox> templateComboBox;
    std::unique_ptr<juce::TextButton> loadTemplateButton;
    std::unique_ptr<juce::TextButton> saveAsTemplateButton;
    
    // Auto-save settings
    std::unique_ptr<juce::ToggleButton> autoSaveToggle;
    std::unique_ptr<juce::Slider> autoSaveIntervalSlider;
    std::unique_ptr<juce::Label> lastAutoSaveLabel;
    
    // Search and filter
    std::unique_ptr<juce::TextEditor> searchEditor;
    std::unique_ptr<juce::TextButton> clearSearchButton;
    std::unique_ptr<juce::ComboBox> filterComboBox;
    
    // File browser
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::FileBrowserComponent> fileBrowser;
    
    // Status
    std::unique_ptr<juce::Label> statusLabel;
    std::unique_ptr<juce::ProgressBar> saveProgressBar;
    
    // Listeners
    std::vector<Listener*> listeners;
    
    // Undo/redo integration
    std::unique_ptr<UndoRedoManager> undoRedoManager;
    
    // UI creation
    void createProjectInfoPanel();
    void createProjectControls();
    void createTrackManagement();
    void createEffectManagement();
    void createRecentProjectsPanel();
    void createTemplatePanel();
    void createAutoSavePanel();
    void createSearchAndFilter();
    void createStatusBar();
    
    // Event handlers
    void onProjectNameChanged();
    void onProjectPropertyChanged();
    void onTrackSelectionChanged();
    void onEffectSelectionChanged();
    
    // File operations
    bool loadProjectFromFile(const juce::File& file);
    bool saveProjectToFile(const juce::File& file);
    bool createProjectDirectory(const juce::File& file);
    
    // Project validation
    bool validateProject() const;
    std::vector<juce::String> getValidationErrors() const;
    
    // Auto-save
    void performAutoSave();
    void updateLastAutoSaveTime();
    
    // Recent projects management
    void loadRecentProjects();
    void saveRecentProjects();
    void updateRecentProjectsUI();
    
    // Templates
    void loadTemplates();
    void saveTemplates();
    std::vector<juce::String> getTemplateFiles() const;
    
    // UI updates
    void updateProjectInfo();
    void updateTrackList();
    void updateEffectList();
    void updateStatusBar();
    void updateControlStates();
    
    // Notification
    void notifyProjectOpened(const ProjectInfo& project);
    void notifyProjectSaved(const ProjectInfo& project);
    void notifyProjectClosed();
    void notifyTrackAdded(const TrackInfo& track);
    void notifyTrackRemoved(const juce::String& trackId);
    void notifyEffectAdded(const juce::String& trackId, const EffectInfo& effect);
    void notifyEffectRemoved(const juce::String& trackId, const juce::String& effectId);
    void notifyProjectModified();
    
    // Utility
    juce::File getProjectDirectory() const;
    juce::File getTemplatesDirectory() const;
    juce::File getRecentProjectsFile() const;
    juce::String generateProjectId() const;
    juce::String generateTrackId() const;
    juce::String generateEffectId() const;
    
    // FileBrowserListener overrides
    void selectionChanged() override;
    void fileClicked(const juce::File& file, const juce::MouseEvent& e) override;
    void fileDoubleClicked(const juce::File& file) override;
    void browserRootChanged(const juce::File& newRoot) override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectManagerUI)
};

// Track list model
class TrackListModel : public juce::ListBoxModel {
public:
    TrackListModel(const std::vector<TrackInfo>& tracks);
    ~TrackListModel() = default;
    
    // ListBoxModel overrides
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height) override;
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    
    // Data management
    void setTracks(const std::vector<TrackInfo>& tracks);
    void addTrack(const TrackInfo& track);
    void removeTrack(int index);
    void moveTrack(int fromIndex, int toIndex);
    
private:
    std::vector<TrackInfo> tracks;
    
    juce::String formatTrackInfo(const TrackInfo& track) const;
    juce::Colour getTrackColor(const TrackInfo& track) const;
};

// Effect list model
class EffectListModel : public juce::ListBoxModel {
public:
    EffectListModel(const std::vector<EffectInfo>& effects);
    ~EffectListModel() = default;
    
    // ListBoxModel overrides
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height) override;
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    
    // Data management
    void setEffects(const std::vector<EffectInfo>& effects);
    void addEffect(const EffectInfo& effect);
    void removeEffect(int index);
    void moveEffect(int fromIndex, int toIndex);
    
private:
    std::vector<EffectInfo> effects;
    
    juce::String formatEffectInfo(const EffectInfo& effect) const;
    juce::Colour getEffectColor(const EffectInfo& effect) const;
};

// Project template
class ProjectTemplate {
public:
    ProjectTemplate(const juce::String& name, const juce::String& description = "");
    ~ProjectTemplate() = default;
    
    // Template information
    juce::String getName() const { return name; }
    juce::String getDescription() const { return description; }
    
    // Template content
    void setProjectInfo(const ProjectInfo& info) { projectInfo = info; }
    void addTrack(const TrackInfo& track) { tracks.push_back(track); }
    void addEffect(const juce::String& trackId, const EffectInfo& effect);
    
    // Access methods
    const ProjectInfo& getProjectInfo() const { return projectInfo; }
    const std::vector<TrackInfo>& getTracks() const { return tracks; }
    const std::unordered_map<juce::String, std::vector<EffectInfo>>& getEffectChains() const { return effectChains; }
    
    // File operations
    bool saveToFile(const juce::File& file) const;
    bool loadFromFile(const juce::File& file);
    
private:
    juce::String name;
    juce::String description;
    ProjectInfo projectInfo;
    std::vector<TrackInfo> tracks;
    std::unordered_map<juce::String, std::vector<EffectInfo>> effectChains;
    
    juce::var serialize() const;
    bool deserialize(const juce::var& data);
};

// Project template manager
class ProjectTemplateManager {
public:
    ProjectTemplateManager();
    ~ProjectTemplateManager() = default;
    
    // Template management
    bool addTemplate(const ProjectTemplate& template);
    bool removeTemplate(const juce::String& name);
    ProjectTemplate getTemplate(const juce::String& name) const;
    std::vector<juce::String> getTemplateNames() const;
    
    // File operations
    bool saveTemplates(const juce::File& directory) const;
    bool loadTemplates(const juce::File& directory);
    
    // Built-in templates
    void createBuiltinTemplates();
    
private:
    std::unordered_map<juce::String, ProjectTemplate> templates;
    juce::File templatesDirectory;
    
    void createEmptyTemplate();
    void createMusicTemplate();
    void createPodcastTemplate();
    void createMasteringTemplate();
};

} // namespace ui
} // namespace zenith
