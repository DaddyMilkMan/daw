/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================
    SkiaProjectManager.h
    Skia-based project management UI with comprehensive features
  ==============================================================================
*/


#include "../../design-system/ZenithTheme.h"
#include "engine/Engine.h"
#include "engine/ProjectState.h"
#include "engine/RecentProjectManager.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
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
    float pan = 0.0f;     // -1 to 1
    juce::Colour color;
    juce::Array<juce::PluginDescription> plugins;
    juce::StringArray clips;
};

// Template information
struct ProjectTemplate {
    juce::String id;
    juce::String name;
    juce::String category;
    juce::String description;
    juce::String thumbnailPath;
    juce::File templatePath;
    int estimatedDuration = 180;  // seconds
    bool isPopular = false;
};

/**
 * @class SkiaProjectManager
 * @brief Skia-based project management interface
 *
 * Features:
 * - Project metadata editing
 * - Track management (add, remove, reorder)
 * - Effect chain management
 * - Auto-save configuration
 * - Recent projects
 * - Template system
 * - Glassmorphic design
 */
class SkiaProjectManager : public SkiaComponent {
public:
    //==========================================================================
    explicit SkiaProjectManager(Engine& engine, ProjectState& projectState);
    ~SkiaProjectManager() override;

    //==========================================================================
    // Component interface
    //==========================================================================

    void drawSkia(SkCanvas* canvas) override;
    void resized() override;

    //==========================================================================
    // Project management
    //==========================================================================

    /**
     * @brief Load a project from file
     */
    bool loadProject(const juce::File& file);

    /**
     * @brief Save current project
     */
    bool saveProject();

    /**
     * @brief Save project as new file
     */
    bool saveProjectAs(const juce::File& file);

    /**
     * @brief Create new project from template
     */
    bool createProjectFromTemplate(const ProjectTemplate& template);

    /**
     * @brief Get current project info
     */
    const ProjectInfo& getCurrentProject() const { return currentProject_; }

    /**
     * @brief Get list of tracks
     */
    const std::vector<TrackInfo>& getTracks() const { return tracks_; }

    /**
     * @brief Add a new track
     */
    void addTrack(const TrackInfo& trackInfo);

    /**
     * @brief Remove a track by ID
     */
    void removeTrack(const juce::String& trackId);

    /**
     * @brief Move track to new position
     */
    void moveTrack(const juce::String& trackId, int newIndex);

    /**
     * @brief Update track properties
     */
    void updateTrack(const juce::String& trackId, const TrackInfo& trackInfo);

    /**
     * @brief Get recent projects
     */
    std::vector<ProjectInfo> getRecentProjects() const;

    /**
     * @brief Get project templates
     */
    std::vector<ProjectTemplate> getProjectTemplates() const;

    //==========================================================================
    // Callbacks
    //==========================================================================

    std::function<void(const juce::File&)> onProjectLoaded;
    std::function<void()> onProjectSaved;
    std::function<void(const juce::String&)> onTrackAdded;
    std::function<void(const juce::String&)> onTrackRemoved;
    std::function<void(const juce::String&)> onTrackChanged;
    std::function<void()> onExportRequested;
    std::function<void()> onImportRequested;

private:
    //==========================================================================
    // Types
    //==========================================================================

    enum class ViewMode {
        RecentProjects,
        Templates,
        CurrentProject,
        ProjectSettings
    };

    enum class TrackEditMode {
        Add,
        Remove,
        Edit
    };

    struct ProjectTab {
        ViewMode mode;
        juce::String label;
        SkRect bounds;
        bool isActive = false;
        bool isHovered = false;
    };

    struct TrackRow {
        TrackInfo info;
        SkRect bounds;
        bool isHovered = false;
        bool isSelected = false;
        juce::Component* editor = nullptr;
    };

    //==========================================================================
    // Layout Constants
    //==========================================================================

    static constexpr float kHeaderHeight = 80.0f;
    static constexpr float kTabHeight = 50.0f;
    static constexpr float kSidebarWidth = 300.0f;
    static constexpr float kFooterHeight = 60.0f;
    static constexpr float kPanelPadding = 20.0f;
    static constexpr float kTrackHeight = 60.0f;
    static constexpr float kTrackSpacing = 8.0f;

    //==========================================================================
    // Drawing Methods
    //==========================================================================

    void drawHeader(SkCanvas* canvas, const SkRect& bounds);
    void drawTabs(SkCanvas* canvas, const SkRect& bounds);
    void drawSidebar(SkCanvas* canvas, const SkRect& bounds);
    void drawMainContent(SkCanvas* canvas, const SkRect& bounds);
    void drawFooter(SkCanvas* canvas, const SkRect& bounds);

    void drawProjectTab(SkCanvas* canvas, const ProjectTab& tab);
    void drawRecentProjectItem(SkCanvas* canvas, const SkRect& bounds,
                              const ProjectInfo& project, bool isSelected, bool isHovered);
    void drawProjectTemplateItem(SkCanvas* canvas, const SkRect& bounds,
                               const ProjectTemplate& template, bool isSelected, bool isHovered);
    void drawCurrentProjectView(SkCanvas* canvas, const SkRect& bounds);
    void drawProjectSettingsView(SkCanvas* canvas, const SkRect& bounds);
    void drawTrackList(SkCanvas* canvas, const SkRect& bounds);
    void drawTrackRow(SkCanvas* canvas, const TrackRow& row);

    // Individual track element renderers
    void drawTrackName(SkCanvas* canvas, const TrackRow& row);
    void drawTrackVolume(SkCanvas* canvas, const TrackRow& row);
    void drawTrackPan(SkCanvas* canvas, const TrackRow& row);
    void drawTrackMuteSolo(SkCanvas* canvas, const TrackRow& row);
    void drawTrackColor(SkCanvas* canvas, const TrackRow& row);

    //==========================================================================
    // Input Handling
    //==========================================================================

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;

    //==========================================================================
    // Helper Methods
    //==========================================================================

    void updateLayout();
    void selectView(ViewMode mode);
    void updateRecentProjects();
    void updateProjectTemplates();
    void loadProjectInfo();
    void saveProjectInfo();
    void addDefaultTracks();
    void selectTemplate(const ProjectTemplate& template);
    juce::String formatFileSize(size_t bytes) const;
    juce::String formatDuration(float seconds) const;
    juce::String getTrackTypeIcon(const juce::String& type) const;
    SkColor getTrackTypeColor(const juce::String& type) const;

    //==========================================================================
    // Member variables
    //==========================================================================

    Engine& engine;
    ProjectState& projectState;
    RecentProjectManager& recentProjects_;

    // Current project state
    ProjectInfo currentProject_;
    std::vector<TrackInfo> tracks_;
    std::vector<ProjectInfo> recentProjects_;
    std::vector<ProjectTemplate> projectTemplates_;

    // UI State
    ViewMode currentView_ = ViewMode::RecentProjects;
    std::vector<ProjectTab> projectTabs_;
    TrackEditMode trackEditMode_ = TrackEditMode::Edit;
    juce::String selectedTrackId_;
    juce::String searchText_;

    // Layout
    SkRect headerRect_;
    SkRect tabsRect_;
    SkRect sidebarRect_;
    SkRect mainContentRect_;
    SkRect footerRect_;

    // State
    bool hasUnsavedChanges_ = false;
    juce::File lastSaveLocation_;
    ProjectTemplate selectedTemplate_;

    // Track editor state
    std::unique_ptr<juce::TextEditor> trackNameEditor_;
    std::unique_ptr<juce::Slider> volumeSlider_;
    std::unique_ptr<juce::Slider> panSlider_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaProjectManager)
};

} // namespace ui
} // namespace zenith
