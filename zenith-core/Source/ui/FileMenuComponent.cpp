/**
 * @file FileMenuComponent.cpp
 * @brief File menu implementation with smooth animations and modern design
 */

#include "FileMenuComponent.h"
#include "../../include/ProjectState.h"
#include "../../include/Engine.h"

namespace zenith {

//==============================================================================
FileMenuComponent::FileMenuComponent(ProjectState& state, Engine& eng)
    : projectState_(state), engine_(eng)
{
    setSize(280, 40);

    // Setup file button
    fileButton.setButtonText("File");
    fileButton.setClickingTogglesState(true);
    fileButton.addListener(this);
    addAndMakeVisible(fileButton);

    // Setup menu appearance
    fileButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff0055ff).withAlpha(0.8f));
    fileButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff0055ff));
    fileButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffffff));
    fileButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffffff));
}

FileMenuComponent::~FileMenuComponent()
{
}

//==============================================================================
void FileMenuComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));

    // If menu is open, draw dropdown menu
    if (menuOpen_ && menuAlpha_ > 0.01f) {
        auto menuBounds = bounds.withTop(bounds.getHeight()).withWidth(MENU_WIDTH);

        // Menu background with shadow
        g.setColour(juce::Colour(0x00000000).withAlpha(0.4f * menuAlpha_));
        g.fillRect(menuBounds.expanded(0, 8));

        g.setColour(juce::Colour(0xff2a2a2a).withAlpha(menuAlpha_));
        g.fillRoundedRectangle(menuBounds.toFloat().expanded(1.0f), 8.0f);

        // Menu border
        g.setColour(juce::Colour(0xff4a4a4a).withAlpha(menuAlpha_));
        g.drawRoundedRectangle(menuBounds.toFloat().expanded(1.0f), 8.0f, 1.0f);

        // Menu items
        int y = menuBounds.getY();

        // New Project
        newProjectBounds_ = juce::Rectangle<int>(menuBounds.getX(), y, menuBounds.getWidth(), MENU_ITEM_HEIGHT);
        paintMenuItem(g, "New Project", "Ctrl+N", 0, hoveredMenuItemIndex_ == 0);
        y += MENU_ITEM_HEIGHT;

        // Open Project
        openProjectBounds_ = juce::Rectangle<int>(menuBounds.getX(), y, menuBounds.getWidth(), MENU_ITEM_HEIGHT);
        paintMenuItem(g, "Open Project", "Ctrl+O", 1, hoveredMenuItemIndex_ == 1);
        y += MENU_ITEM_HEIGHT;

        // Separator
        g.setColour(juce::Colour(0xff3a3a3a).withAlpha(menuAlpha_));
        g.drawHorizontalLine(y + SEPARATOR_HEIGHT / 2, (float)menuBounds.getX() + 8.0f,
                           (float)menuBounds.getRight() - 8.0f);
        y += SEPARATOR_HEIGHT;

        // Save Project
        saveProjectBounds_ = juce::Rectangle<int>(menuBounds.getX(), y, menuBounds.getWidth(), MENU_ITEM_HEIGHT);
        paintMenuItem(g, "Save Project", "Ctrl+S", 2, hoveredMenuItemIndex_ == 2);
        y += MENU_ITEM_HEIGHT;

        // Save As
        saveAsBounds_ = juce::Rectangle<int>(menuBounds.getX(), y, menuBounds.getWidth(), MENU_ITEM_HEIGHT);
        paintMenuItem(g, "Save As", "Ctrl+Shift+S", 3, hoveredMenuItemIndex_ == 3);
        y += MENU_ITEM_HEIGHT;

        // Separator
        g.setColour(juce::Colour(0xff3a3a3a).withAlpha(menuAlpha_));
        g.drawHorizontalLine(y + SEPARATOR_HEIGHT / 2, (float)menuBounds.getX() + 8.0f,
                           (float)menuBounds.getRight() - 8.0f);
        y += SEPARATOR_HEIGHT;

        // Recent Files header
        if (!recentFiles_.isEmpty()) {
            g.setColour(juce::Colour(0xff999999).withAlpha(menuAlpha_ * 0.7f));
            g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
            g.drawText("Recent Files", juce::Rectangle<int>(menuBounds.getX() + 12, y,
                                                            menuBounds.getWidth() - 24, 20),
                      juce::Justification::centredLeft, false);
            y += 20;

            // Recent files list
            for (int i = 0; i < juce::jmin(recentFiles_.size(), 5); ++i) {
                recentFileBounds_.set(i, juce::Rectangle<int>(menuBounds.getX(), y,
                                                             menuBounds.getWidth(), MENU_ITEM_HEIGHT));
                bool isHovered = hoveredMenuItemIndex_ == (4 + i);

                // Background highlight
                if (isHovered) {
                    g.setColour(juce::Colour(0xff0055ff).withAlpha(0.15f * menuAlpha_));
                    g.fillRoundedRectangle(recentFileBounds_[i].reduced(4, 2).toFloat(), 4.0f);
                }

                // Text
                g.setColour(juce::Colour(0xffcccccc).withAlpha(menuAlpha_));
                g.setFont(juce::FontOptions(11.0f));
                juce::String filename = juce::File(recentFiles_[i]).getFileNameWithoutExtension();
                g.drawText(filename, recentFileBounds_[i].reduced(12, 2),
                          juce::Justification::centredLeft, true);

                y += MENU_ITEM_HEIGHT;
            }
        }
    }
}

void FileMenuComponent::paintMenuItem(juce::Graphics& g, const juce::String& label,
                                     const juce::String& shortcut, int index, bool isHovered)
{
    juce::Rectangle<int> bounds;

    if (index == 0) bounds = newProjectBounds_;
    else if (index == 1) bounds = openProjectBounds_;
    else if (index == 2) bounds = saveProjectBounds_;
    else if (index == 3) bounds = saveAsBounds_;

    if (!bounds.isEmpty()) {
        // Background highlight
        if (isHovered) {
            g.setColour(juce::Colour(0xff0055ff).withAlpha(0.2f * menuAlpha_));
            g.fillRoundedRectangle(bounds.reduced(4, 2).toFloat(), 4.0f);

            // Subtle border
            g.setColour(juce::Colour(0xff0055ff).withAlpha(0.4f * menuAlpha_));
            g.drawRoundedRectangle(bounds.reduced(4, 2).toFloat(), 4.0f, 1.0f);
        }

        // Label text
        g.setColour(juce::Colour(0xffcccccc).withAlpha(menuAlpha_));
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText(label, bounds.removeFromLeft(bounds.getWidth() - 80).reduced(12, 0),
                  juce::Justification::centredLeft, false);

        // Shortcut text
        g.setColour(juce::Colour(0xff999999).withAlpha(menuAlpha_ * 0.7f));
        g.setFont(juce::FontOptions(9.0f));
        g.drawText(shortcut, bounds.reduced(12, 0),
                  juce::Justification::centredRight, false);
    }
}

void FileMenuComponent::resized()
{
    fileButton.setBounds(getLocalBounds().removeFromTop(40));
}

//==============================================================================
void FileMenuComponent::buttonClicked(juce::Button* button)
{
    if (button == &fileButton) {
        if (menuOpen_) {
            hideMenu();
        } else {
            showMenu();
        }
    }
}

//==============================================================================
void FileMenuComponent::showMenu()
{
    menuOpen_ = true;
    // Menu will animate in during paint() calls via menuAlpha_
}

void FileMenuComponent::hideMenu()
{
    menuOpen_ = false;
    fileButton.setToggleState(false, juce::dontSendNotification);
}

//==============================================================================
void FileMenuComponent::handleNewProject()
{
    projectState_.newProject();
    hideMenu();
    DBG("New project created");
}

void FileMenuComponent::handleOpenProject()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Open Zenith Project",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.zth");

    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file.existsAsFile()) {
            projectState_.loadFromFile(file);
            hideMenu();
            DBG("Project opened: " + file.getFullPathName());
        }
    });
}

void FileMenuComponent::handleSaveProject()
{
    // TODO(zenith-core#1): Save to current file, or show save as dialog if new project
    projectState_.saveToFile(juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                            .getChildFile("untitled.zth"));
    hideMenu();
    DBG("Project saved");
}

void FileMenuComponent::handleSaveAs()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Save Zenith Project",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.zth");

    auto chooserFlags = juce::FileBrowserComponent::saveMode
                      | juce::FileBrowserComponent::warnAboutOverwriting;

    chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file.getFullPathName().isNotEmpty()) {
            projectState_.saveToFile(file);
            hideMenu();
            DBG("Project saved as: " + file.getFullPathName());
        }
    });
}

void FileMenuComponent::refreshRecentFiles()
{
    // TODO(zenith-core#1): Load recent files list from preferences
    recentFiles_.clear();
}

}  // namespace zenith

