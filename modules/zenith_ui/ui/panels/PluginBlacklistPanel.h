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

    PluginBlacklistPanel.h
    Created: 2026-01-29
    Author:  Zenith DAW

    UI panel for managing the plugin blacklist and viewing scan results.

  ==============================================================================

*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../engine/PluginBlacklist.h"
#include "../../engine/PluginHost.h"

namespace zenith {

//==============================================================================
/**
    UI panel for managing plugin blacklist and viewing scan results.
    
    Features:
    - View all blacklisted plugins with error details
    - Remove plugins from blacklist (retry scanning)
    - View last scan results with statistics
    - Clear entire blacklist
    - Export/import blacklist
*/
class PluginBlacklistPanel : public juce::Component,
                             public juce::ListBoxModel,
                             public juce::Button::Listener
{
public:
    //==============================================================================
    PluginBlacklistPanel(PluginBlacklist& blacklist, PluginHost& pluginHost);
    ~PluginBlacklistPanel() override;

    //==============================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==============================================================================
    // ListBoxModel overrides
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    juce::var getDragSourceDescription(const juce::SparseSet<int>& rowsToDescribe) override;

    //==============================================================================
    // Button::Listener override
    void buttonClicked(juce::Button* button) override;

    //==============================================================================
    // Content management
    void refreshBlacklist();
    void setScanResults(const std::vector<PluginHost::ScanResult>& results);
    
    //==============================================================================
    // Static helper to show as dialog
    static void showAsDialog(PluginBlacklist& blacklist, PluginHost& pluginHost);

private:
    //==============================================================================
    PluginBlacklist& blacklist_;
    PluginHost& pluginHost_;
    
    // UI Components
    juce::ListBox blacklistList_;
    juce::TextButton removeButton_{"Remove from Blacklist"};
    juce::TextButton clearAllButton_{"Clear All"};
    juce::TextButton refreshButton_{"Refresh"};
    juce::TextButton closeButton_{"Close"};
    juce::Label titleLabel_;
    juce::Label statsLabel_;
    juce::TextEditor detailsEditor_;
    
    // Data
    std::vector<PluginBlacklist::BlacklistEntry> entries_;
    std::vector<PluginHost::ScanResult> lastScanResults_;
    
    //==============================================================================
    void updateStatsLabel();
    void showEntryDetails(int row);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBlacklistPanel)
};

} // namespace zenith
