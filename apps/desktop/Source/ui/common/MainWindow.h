/*
    MainWindow.h - Main application window for Zenith DAW

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

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace zenith {

class CommandAPI;
class Engine;
class ProjectState;

/**
 * @class MainWindow
 * @brief Main application window for Zenith DAW
 *
 * This is the main window that contains all the UI components.
 */
class MainWindow : public juce::DocumentWindow {
public:
    /**
     * @brief Constructor
     * @param application Reference to the main application
     */
    MainWindow(juce::String name);

    /**
     * @brief Destructor
     */
    ~MainWindow() override;

    /**
     * @brief Called when the user tries to close the window
     */
    void closeButtonPressed() override;

    /**
     * @brief Get the project state from the main window
     * @return Pointer to project state, or nullptr if not available
     */
    zenith::ProjectState* getProjectState();

    /**
     * @brief Check for unsaved changes and handle quit confirmation
     */
    void checkUnsavedAndQuit();

private:
    // Keep these alive for the lifetime of the window.
    std::unique_ptr<Engine> engine_;
    std::unique_ptr<ProjectState> projectState_;
    std::unique_ptr<CommandAPI> commandAPI_;

    juce::TooltipWindow tooltipWindow_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

} // namespace zenith
