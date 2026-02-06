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

#include "PluginBlacklistPanel.h"
#include "../design-system/ZenithDesignSystem.h"

namespace zenith {

//==============================================================================
PluginBlacklistPanel::PluginBlacklistPanel(PluginBlacklist& blacklist, PluginHost& pluginHost)
    : blacklist_(blacklist),
      pluginHost_(pluginHost)
{
    setSize(700, 500);
    
    // Title
    titleLabel_.setText("Plugin Blacklist Manager", juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(20.0f, juce::Font::bold));
    titleLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel_);
    
    // Stats label
    statsLabel_.setJustificationType(juce::Justification::left);
    addAndMakeVisible(statsLabel_);
    
    // Blacklist list
    blacklistList_.setModel(this);
    blacklistList_.setColour(juce::ListBox::outlineColourId,
                             juce::Colour(design::colors::BORDER_SUBTLE));
    blacklistList_.setOutlineThickness(1);
    addAndMakeVisible(blacklistList_);
    
    // Details editor
    detailsEditor_.setMultiLine(true);
    detailsEditor_.setReadOnly(true);
    detailsEditor_.setScrollbarsShown(true);
    detailsEditor_.setColour(juce::TextEditor::backgroundColourId,
                             juce::Colour(design::colors::BG_03));
    detailsEditor_.setText("Select a plugin to view details...");
    addAndMakeVisible(detailsEditor_);
    
    // Buttons
    removeButton_.addListener(this);
    addAndMakeVisible(removeButton_);
    
    clearAllButton_.addListener(this);
    addAndMakeVisible(clearAllButton_);
    
    refreshButton_.addListener(this);
    addAndMakeVisible(refreshButton_);
    
    closeButton_.addListener(this);
    addAndMakeVisible(closeButton_);
    
    // Initial refresh
    refreshBlacklist();
}

PluginBlacklistPanel::~PluginBlacklistPanel()
{
    blacklistList_.setModel(nullptr);
}

//==============================================================================
void PluginBlacklistPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(design::colors::BG_01));
}

void PluginBlacklistPanel::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // Title at top
    titleLabel_.setBounds(area.removeFromTop(30));
    area.removeFromTop(5);
    
    // Stats below title
    statsLabel_.setBounds(area.removeFromTop(20));
    area.removeFromTop(10);
    
    // Button row
    auto buttonRow = area.removeFromBottom(40);
    removeButton_.setBounds(buttonRow.removeFromLeft(150).reduced(5));
    clearAllButton_.setBounds(buttonRow.removeFromLeft(120).reduced(5));
    refreshButton_.setBounds(buttonRow.removeFromLeft(100).reduced(5));
    closeButton_.setBounds(buttonRow.removeFromRight(100).reduced(5));
    area.removeFromBottom(10);
    
    // Split remaining area: list on left, details on right
    auto listWidth = juce::jmin(350, area.getWidth() / 2);
    auto listArea = area.removeFromLeft(listWidth);
    
    blacklistList_.setBounds(listArea);
    
    // Details on right
    detailsEditor_.setBounds(area.reduced(5));
}

//==============================================================================
// ListBoxModel overrides
//==============================================================================
int PluginBlacklistPanel::getNumRows()
{
    return static_cast<int>(entries_.size());
}

void PluginBlacklistPanel::paintListBoxItem(int rowNumber, juce::Graphics& g, 
                                            int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= entries_.size())
        return;
    
    auto& entry = entries_[rowNumber];
    
    // Background
    if (rowIsSelected) {
        g.fillAll(juce::Colour(design::colors::ACCENT_PRIMARY).withAlpha(0.2f));
    } else if (rowNumber % 2 == 0) {
        g.fillAll(juce::Colour(design::colors::BG_02));
    } else {
        g.fillAll(juce::Colour(design::colors::BG_03));
    }
    
    // Text
    g.setColour(juce::Colour(design::colors::TEXT_PRIMARY));
    
    // Plugin name (extract from path)
    juce::File file(entry.filePath);
    auto name = file.getFileNameWithoutExtension();
    
    // Error type indicator
    juce::String prefix;
    if (entry.errorType == "crash")
        prefix = "[CRASH] ";
    else if (entry.errorType == "timeout")
        prefix = "[TIMEOUT] ";
    else if (entry.userBlacklisted)
        prefix = "[USER] ";
    else
        prefix = "[" + entry.errorType.toUpperCase() + "] ";
    
    g.setFont(14.0f);
    g.drawText(prefix + name, 5, 0, width - 10, height - 12, 
               juce::Justification::centredLeft, true);
    
    // Path (smaller, gray)
    g.setColour(juce::Colour(design::colors::TEXT_SECONDARY));
    g.setFont(10.0f);
    g.drawText(entry.filePath, 5, height - 12, width - 10, 12,
               juce::Justification::centredLeft, true);
}

void PluginBlacklistPanel::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    showEntryDetails(row);
}

void PluginBlacklistPanel::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    showEntryDetails(row);
}

juce::var PluginBlacklistPanel::getDragSourceDescription(const juce::SparseSet<int>& rowsToDescribe)
{
    return juce::var();  // No drag support
}

//==============================================================================
// Button::Listener override
//==============================================================================
void PluginBlacklistPanel::buttonClicked(juce::Button* button)
{
    if (button == &removeButton_)
    {
        auto selected = blacklistList_.getSelectedRow();
        if (selected >= 0 && selected < entries_.size())
        {
            blacklist_.removeFromBlacklist(entries_[selected].filePath);
            refreshBlacklist();
            detailsEditor_.setText("Plugin removed from blacklist. It will be scanned again on next plugin scan.");
        }
    }
    else if (button == &clearAllButton_)
    {
        auto result = juce::AlertWindow::showOkCancelBox(
            juce::AlertWindow::WarningIcon,
            "Clear Blacklist",
            "Are you sure you want to clear the entire blacklist?\n\n"
            "This will attempt to scan all previously blacklisted plugins again, "
            "which may cause crashes.",
            "Clear All",
            "Cancel");
        
        if (result == 1)  // OK clicked
        {
            blacklist_.clearBlacklist();
            refreshBlacklist();
            detailsEditor_.setText("Blacklist cleared. All plugins will be scanned on next plugin scan.");
        }
    }
    else if (button == &refreshButton_)
    {
        refreshBlacklist();
    }
    else if (button == &closeButton_)
    {
        // Find parent dialog and close it
        auto* parent = findParentComponentOfClass<juce::DialogWindow>();
        if (parent != nullptr)
            parent->exitModalState(0);
    }
}

//==============================================================================
// Content management
//==============================================================================
void PluginBlacklistPanel::refreshBlacklist()
{
    entries_ = blacklist_.getAllEntries();
    blacklistList_.updateContent();
    blacklistList_.repaint();
    updateStatsLabel();
}

void PluginBlacklistPanel::setScanResults(const std::vector<PluginHost::ScanResult>& results)
{
    lastScanResults_ = results;
    
    // Count statistics
    int success = 0, crashed = 0, timedOut = 0, blacklisted = 0, errors = 0;
    for (const auto& r : results)
    {
        if (r.success) success++;
        else if (r.errorType == "crash") crashed++;
        else if (r.errorType == "timeout") timedOut++;
        else if (r.errorType == "blacklisted") blacklisted++;
        else errors++;
    }
    
    juce::String stats;
    stats << "Last Scan: " << results.size() << " plugins | ";
    stats << "Success: " << success << " | ";
    if (crashed > 0) stats << "Crashed: " << crashed << " | ";
    if (timedOut > 0) stats << "Timed out: " << timedOut << " | ";
    if (blacklisted > 0) stats << "Skipped: " << blacklisted << " | ";
    if (errors > 0) stats << "Errors: " << errors;
    
    statsLabel_.setText(stats, juce::dontSendNotification);
}

void PluginBlacklistPanel::updateStatsLabel()
{
    int total = blacklist_.getBlacklistCount();
    int userBlacklisted = 0;
    int autoBlacklisted = 0;
    
    for (const auto& entry : entries_)
    {
        if (entry.userBlacklisted)
            userBlacklisted++;
        else
            autoBlacklisted++;
    }
    
    juce::String text;
    text << "Blacklisted plugins: " << total;
    if (total > 0)
    {
        text << " (Auto: " << autoBlacklisted;
        if (userBlacklisted > 0)
            text << ", User: " << userBlacklisted;
        text << ")";
    }
    
    statsLabel_.setText(text, juce::dontSendNotification);
}

void PluginBlacklistPanel::showEntryDetails(int row)
{
    if (row < 0 || row >= entries_.size())
    {
        detailsEditor_.setText("Select a plugin to view details...");
        return;
    }
    
    auto& entry = entries_[row];
    
    juce::String details;
    details << "=== Plugin Details ===\n\n";
    details << "File: " << entry.filePath << "\n\n";
    details << "Error Type: " << entry.errorType.toUpperCase() << "\n";
    
    if (entry.errorMessage.isNotEmpty())
        details << "Error Message: " << entry.errorMessage << "\n";
    
    details << "\n";
    details << "First Seen: " << entry.timestamp.toString(true, true, false, true) << "\n";
    details << "Failure Count: " << entry.failureCount << "\n";
    
    if (entry.userBlacklisted)
        details << "\n[User manually blacklisted]\n";
    else
        details << "\n[Automatically blacklisted after crash/timeout]\n";
    
    details << "\n=== Actions ===\n";
    details << "Click 'Remove from Blacklist' to retry scanning this plugin.\n";
    details << "Note: If the plugin still crashes, it will be re-blacklisted.";
    
    detailsEditor_.setText(details);
}

//==============================================================================
// Static helper to show as dialog
//==============================================================================
void PluginBlacklistPanel::showAsDialog(PluginBlacklist& blacklist, PluginHost& pluginHost)
{
    auto* panel = new PluginBlacklistPanel(blacklist, pluginHost);
    
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(panel);
    options.dialogTitle = "Plugin Blacklist Manager";
    options.dialogBackgroundColour = juce::Colours::lightgrey;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    
    auto* window = options.create();
    window->centreWithSize(700, 500);
    window->setVisible(true);
}

} // namespace zenith
