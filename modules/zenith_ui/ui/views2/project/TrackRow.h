/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
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
namespace j = juce;
namespace ui {

// Project information
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
    std::unique_ptr<j::TextEditor> trackNameEditor_;
    std::unique_ptr<j::Slider> volumeSlider_;
    std::unique_ptr<j::Slider> panSlider_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaProjectManager)
};

} // namespace ui
} // namespace zenith
