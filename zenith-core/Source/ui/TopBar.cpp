/**
 * @file TopBar.cpp
 * @brief Implementation of top bar component
 */

#include "TopBar.h"

//==============================================================================
TopBar::TopBar()
{
    // Logo label
    logoLabel.setText("Zenith", juce::dontSendNotification);
    logoLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    logoLabel.setJustificationType(juce::Justification::centredLeft);
    logoLabel.setColour(juce::Label::textColourId, ZenithColours::accent);
    addAndMakeVisible(logoLabel);

    // Project name label (editable)
    projectNameLabel.setText("Untitled Project", juce::dontSendNotification);
    projectNameLabel.setFont(juce::Font(14.0f, juce::Font::plain));
    projectNameLabel.setJustificationType(juce::Justification::centredLeft);
    projectNameLabel.setEditable(true, true);
    projectNameLabel.setColour(juce::Label::backgroundWhenEditingColourId, ZenithColours::backgroundLight);
    projectNameLabel.onTextChange = [this]()
    {
        if (onProjectNameChanged)
            onProjectNameChanged(projectNameLabel.getText());
    };
    addAndMakeVisible(projectNameLabel);

    // AI toggle button
    aiToggleButton.setButtonText("AI");
    aiToggleButton.setClickingTogglesState(true);
    aiToggleButton.onClick = [this]()
    {
        if (onAIToggleChanged)
            onAIToggleChanged(aiToggleButton.getToggleState());
    };
    addAndMakeVisible(aiToggleButton);

    // Settings button
    settingsButton.setButtonText("⚙");  // Gear icon
    settingsButton.onClick = [this]()
    {
        if (onSettingsClicked)
            onSettingsClicked();
    };
    addAndMakeVisible(settingsButton);

    // Menu button
    menuButton.setButtonText("☰");  // Hamburger menu
    addAndMakeVisible(menuButton);
}

void TopBar::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(ZenithColours::backgroundMedium);

    // Bottom border
    g.setColour(ZenithColours::border);
    g.drawLine(0.0f, (float)getHeight(), (float)getWidth(), (float)getHeight(), 1.0f);
}

void TopBar::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    // Logo (left)
    logoLabel.setBounds(bounds.removeFromLeft(100));

    bounds.removeFromLeft(16); // Spacing

    // Project name (left, flexible width)
    projectNameLabel.setBounds(bounds.removeFromLeft(200));

    // Right side controls (work from right to left)
    menuButton.setBounds(bounds.removeFromRight(buttonSize));
    bounds.removeFromRight(8); // Spacing

    settingsButton.setBounds(bounds.removeFromRight(buttonSize));
    bounds.removeFromRight(8); // Spacing

    aiToggleButton.setBounds(bounds.removeFromRight(buttonSize + 16)); // Wider for "AI" text
}

void TopBar::setProjectName(const juce::String& name)
{
    projectNameLabel.setText(name, juce::dontSendNotification);
}

juce::String TopBar::getProjectName() const
{
    return projectNameLabel.getText();
}

void TopBar::setAIEnabled(bool enabled)
{
    aiToggleButton.setToggleState(enabled, juce::dontSendNotification);
}
