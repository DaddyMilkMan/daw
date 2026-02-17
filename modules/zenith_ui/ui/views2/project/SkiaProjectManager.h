/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "../../framework/SkiaComponent.h"
#include "zenith_core/engine/Engine.h"
#include "zenith_core/engine/ProjectState.h"
#include <juce_core/juce_core.h>

namespace zenith::ui {

/**
 * @brief Project management component with recent projects and templates
 */
class SkiaProjectManager : public SkiaComponent {
public:
    SkiaProjectManager(Engine& engine, ProjectState& projectState);
    ~SkiaProjectManager() override;

    void paint(SkCanvas* canvas) override;
    void resized() override;

    bool keyPressed(const juce::KeyPress& key) override;

    /**
     * @brief Load a project from file
     */
    bool loadProject(const juce::File& file);

    /**
     * @brief Save current project
     */
    bool saveProject();

    /**
     * @brief Create new project
     */
    void createNewProject();

private:
    Engine& engine_;
    ProjectState& projectState_;

    struct ProjectInfo {
        juce::String name;
        juce::String path;
        juce::String lastModified;
    };

    struct ProjectTemplate {
        juce::String name;
        juce::String description;
    };

    std::vector<ProjectInfo> recentProjects_;
    std::vector<ProjectTemplate> templates_;

    bool hasUnsavedChanges_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkiaProjectManager)
};

} // namespace zenith::ui
