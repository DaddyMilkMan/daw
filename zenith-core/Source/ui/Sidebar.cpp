/**
 * @file Sidebar.cpp
 * @brief Implementation of sidebar component
 */

#include "Sidebar.h"

//==============================================================================
Sidebar::Sidebar()
{
    // Tab buttons
    tracksTab.setButtonText("Tracks");
    tracksTab.setClickingTogglesState(true);
    tracksTab.setToggleState(true, juce::dontSendNotification);
    tracksTab.onClick = [this]() { setCurrentTab(0); };
    addAndMakeVisible(tracksTab);

    filesTab.setButtonText("Files");
    filesTab.setClickingTogglesState(true);
    filesTab.onClick = [this]() { setCurrentTab(1); };
    addAndMakeVisible(filesTab);

    pluginsTab.setButtonText("Plugins");
    pluginsTab.setClickingTogglesState(true);
    pluginsTab.onClick = [this]() { setCurrentTab(2); };
    addAndMakeVisible(pluginsTab);

    favoritesTab.setButtonText("★");
    favoritesTab.setClickingTogglesState(true);
    favoritesTab.onClick = [this]() { setCurrentTab(3); };
    addAndMakeVisible(favoritesTab);

    // Search box
    searchBox.setTextToShowWhenEmpty("Search...", ZenithColours::textSecondary);
    searchBox.setColour(juce::TextEditor::backgroundColourId, ZenithColours::backgroundLight);
    searchBox.setColour(juce::TextEditor::textColourId, ZenithColours::textPrimary);
    searchBox.setColour(juce::TextEditor::outlineColourId, ZenithColours::border);
    addAndMakeVisible(searchBox);

    // Track list (visible by default)
    trackList.setColour(juce::ListBox::backgroundColourId, ZenithColours::backgroundDark);
    addAndMakeVisible(trackList);

    // File tree (hidden by default)
    fileTree.setVisible(false);
    addAndMakeVisible(fileTree);

    // Add some default tracks for testing
    addTrack("Audio 1");
    addTrack("Audio 2");
    addTrack("MIDI 1");
}

void Sidebar::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(ZenithColours::backgroundMedium);

    // Right border
    g.setColour(ZenithColours::border);
    g.drawLine((float)getWidth(), 0.0f, (float)getWidth(), (float)getHeight(), 1.0f);
}

void Sidebar::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    // Tab buttons at top
    auto tabRow = bounds.removeFromTop(32);
    int tabWidth = tabRow.getWidth() / 4;

    tracksTab.setBounds(tabRow.removeFromLeft(tabWidth).reduced(2));
    filesTab.setBounds(tabRow.removeFromLeft(tabWidth).reduced(2));
    pluginsTab.setBounds(tabRow.removeFromLeft(tabWidth).reduced(2));
    favoritesTab.setBounds(tabRow.reduced(2));

    bounds.removeFromTop(8); // Spacing

    // Search box
    searchBox.setBounds(bounds.removeFromTop(28));

    bounds.removeFromTop(8); // Spacing

    // Content area (remaining space)
    trackList.setBounds(bounds);
    fileTree.setBounds(bounds);
}

void Sidebar::addTrack(const juce::String& trackName)
{
    trackNames.add(trackName);
    trackList.updateContent();
}

void Sidebar::removeTrack(int trackIndex)
{
    if (juce::isPositiveAndBelow(trackIndex, trackNames.size()))
    {
        trackNames.remove(trackIndex);
        trackList.updateContent();
    }
}

void Sidebar::clearTracks()
{
    trackNames.clear();
    trackList.updateContent();
}

void Sidebar::setCurrentTab(int tabIndex)
{
    currentTab = tabIndex;

    // Update tab button states
    tracksTab.setToggleState(tabIndex == 0, juce::dontSendNotification);
    filesTab.setToggleState(tabIndex == 1, juce::dontSendNotification);
    pluginsTab.setToggleState(tabIndex == 2, juce::dontSendNotification);
    favoritesTab.setToggleState(tabIndex == 3, juce::dontSendNotification);

    // Show/hide appropriate content
    trackList.setVisible(tabIndex == 0);
    fileTree.setVisible(tabIndex == 1);

    // TODO: Implement plugins and favorites views
}
