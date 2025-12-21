/**
 * @file PluginBrowserComponent.cpp
 * @brief Plugin browser implementation
 */

#include "../../include/ui/PluginBrowserComponent.h"
#include "../../include/Engine.h"
#include "../engine/Track.h"
#include "../engine/PluginHost.h"

using namespace zenith;

//==============================================================================
// PluginBrowserComponent Implementation
//==============================================================================

PluginBrowserComponent::PluginBrowserComponent(Engine& eng)
    : engine(eng)
{
    setSize(800, 600);

    // Title label
    titleLabel.setText("Plugin Browser", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    // Search label and box
    searchLabel.setText("Search:", juce::dontSendNotification);
    searchLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(searchLabel);

    searchBox.setMultiLine(false);
    searchBox.setReturnKeyStartsNewLine(false);
    searchBox.setTextToShowWhenEmpty("Type to filter plugins...", juce::Colours::grey);
    searchBox.addListener(this);
    addAndMakeVisible(searchBox);

    // Track selector
    trackLabel.setText("Target Track:", juce::dontSendNotification);
    trackLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(trackLabel);

    trackSelector.addItem("(Select Track)", 1);
    trackSelector.setSelectedId(1);
    trackSelector.onChange = [this]()
    {
        int trackIndex = trackSelector.getSelectedId() - 2;  // -2 because ID 1 is "(Select Track)"
        if (trackIndex >= 0 && trackIndex < engine.getNumTracks())
        {
            const auto& tracks = engine.tracks();
            targetTrack = tracks[trackIndex].get();
            statusLabel.setText("Target: " + juce::String(targetTrack->getName()), juce::dontSendNotification);
        }
        else
        {
            targetTrack = nullptr;
            statusLabel.setText("No target track selected", juce::dontSendNotification);
        }
    };
    addAndMakeVisible(trackSelector);

    // Plugin table
    pluginTable.setModel(this);
    pluginTable.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff2e2e2e));
    pluginTable.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff555555));
    pluginTable.setOutlineThickness(1);
    pluginTable.setMultipleSelectionEnabled(false);

    // Add columns
    pluginTable.getHeader().addColumn("Name", 1, 300, 100, 500, juce::TableHeaderComponent::defaultFlags);
    pluginTable.getHeader().addColumn("Category", 2, 150, 100, 300, juce::TableHeaderComponent::defaultFlags);
    pluginTable.getHeader().addColumn("Manufacturer", 3, 200, 100, 400, juce::TableHeaderComponent::defaultFlags);
    pluginTable.getHeader().addColumn("Format", 4, 80, 60, 120, juce::TableHeaderComponent::defaultFlags);

    addAndMakeVisible(pluginTable);

    // Load button
    loadButton.setButtonText("Load on Track");
    loadButton.onClick = [this]() { loadSelectedPlugin(); };
    addAndMakeVisible(loadButton);

    // Status label
    statusLabel.setText("No target track selected", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(statusLabel);

    // Populate track selector
    refresh();
}

PluginBrowserComponent::~PluginBrowserComponent()
{
    searchBox.removeListener(this);
}

void PluginBrowserComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));  // Dark grey background
}

void PluginBrowserComponent::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Title
    titleLabel.setBounds(bounds.removeFromTop(40).reduced(5));

    // Search area
    auto searchArea = bounds.removeFromTop(30);
    searchLabel.setBounds(searchArea.removeFromLeft(80));
    searchBox.setBounds(searchArea.reduced(5, 2));

    bounds.removeFromTop(10);  // Spacing

    // Track selector
    auto trackArea = bounds.removeFromTop(30);
    trackLabel.setBounds(trackArea.removeFromLeft(100));
    trackSelector.setBounds(trackArea.removeFromLeft(300).reduced(5, 2));

    bounds.removeFromTop(10);  // Spacing

    // Bottom controls
    auto bottomBar = bounds.removeFromBottom(40);
    loadButton.setBounds(bottomBar.removeFromRight(150).reduced(5));
    statusLabel.setBounds(bottomBar.reduced(5));

    // Table takes remaining space
    pluginTable.setBounds(bounds);
}

//==============================================================================
// Plugin browser interface
//==============================================================================

void PluginBrowserComponent::setTargetTrack(zenith::Track* track)
{
    targetTrack = track;
    if (targetTrack != nullptr)
    {
        statusLabel.setText("Target: " + juce::String(targetTrack->getName()), juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText("No target track selected", juce::dontSendNotification);
    }
}

int PluginBrowserComponent::getSelectedPluginIndex() const
{
    return pluginTable.getSelectedRow();
}

bool PluginBrowserComponent::loadSelectedPlugin()
{
    int selectedIndex = getSelectedPluginIndex();
    if (selectedIndex < 0 || selectedIndex >= filteredPlugins.size())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "No Plugin Selected",
            "Please select a plugin from the list.",
            "OK");
        return false;
    }

    if (targetTrack == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "No Target Track",
            "Please select a target track first.",
            "OK");
        return false;
    }

    loadPluginAtIndex(selectedIndex);
    return true;
}

void PluginBrowserComponent::refresh()
{
    // Update track selector
    trackSelector.clear();
    trackSelector.addItem("(Select Track)", 1);

    const auto& tracks = engine.tracks();
    for (int i = 0; i < engine.getNumTracks(); ++i)
    {
        if (tracks[i] != nullptr)
        {
            trackSelector.addItem(tracks[i]->getName(), i + 2);  // +2 because ID 1 is reserved
        }
    }

    trackSelector.setSelectedId(1);

    // Update plugin list
    updateFilteredList();
}

//==============================================================================
// TableListBoxModel interface
//==============================================================================

int PluginBrowserComponent::getNumRows()
{
    return filteredPlugins.size();
}

void PluginBrowserComponent::paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colour(0xff4a4a4a));
    else if (rowNumber % 2 == 0)
        g.fillAll(juce::Colour(0xff2a2a2a));
    else
        g.fillAll(juce::Colour(0xff2e2e2e));
}

void PluginBrowserComponent::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    g.setColour(rowIsSelected ? juce::Colours::white : juce::Colours::lightgrey);
    g.setFont(14.0f);

    if (rowNumber >= 0 && rowNumber < filteredPlugins.size())
    {
        const auto& desc = filteredPlugins[rowNumber];
        juce::String text;

        switch (columnId)
        {
            case 1: text = desc.name; break;
            case 2: text = desc.category.isEmpty() ? "Unknown" : desc.category; break;
            case 3: text = desc.manufacturerName; break;
            case 4: text = desc.pluginFormatName; break;
            default: break;
        }

        g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
    }
}

void PluginBrowserComponent::cellDoubleClicked(int rowNumber, int columnId, const juce::MouseEvent& e)
{
    juce::ignoreUnused(columnId, e);

    if (rowNumber >= 0 && rowNumber < filteredPlugins.size())
    {
        pluginTable.selectRow(rowNumber);
        loadPluginAtIndex(rowNumber);
    }
}

//==============================================================================
// TextEditor::Listener interface
//==============================================================================

void PluginBrowserComponent::textEditorTextChanged(juce::TextEditor& editor)
{
    juce::ignoreUnused(editor);
    updateFilteredList();
}

//==============================================================================
// Helper methods
//==============================================================================

void PluginBrowserComponent::updateFilteredList()
{
    filteredPlugins.clear();
    currentFilter = searchBox.getText().toLowerCase();

    auto& knownPlugins = engine.getPluginHost().getKnownPlugins();

    for (const auto& desc : knownPlugins.getTypes())
    {
        // Filter by search text
        if (currentFilter.isEmpty() ||
            desc.name.toLowerCase().contains(currentFilter) ||
            desc.manufacturerName.toLowerCase().contains(currentFilter) ||
            desc.category.toLowerCase().contains(currentFilter))
        {
            filteredPlugins.add(desc);
        }
    }

    pluginTable.updateContent();
    pluginTable.repaint();
}

void PluginBrowserComponent::loadPluginAtIndex(int index)
{
    if (index < 0 || index >= filteredPlugins.size() || targetTrack == nullptr)
        return;

    const auto& desc = filteredPlugins[index];

    // Show loading message
    statusLabel.setText("Loading: " + desc.name + "...", juce::dontSendNotification);

    // Load plugin asynchronously
    juce::String errorMessage;
    auto& formatManager = engine.getPluginFormatManager();

    // Create plugin instance
    formatManager.createPluginInstanceAsync(
        desc,
        engine.getSampleRate(),
        engine.getBufferSize(),
        [this, desc, weakThis = juce::Component::SafePointer<PluginBrowserComponent>(this)](
            std::unique_ptr<juce::AudioPluginInstance> instance,
            const juce::String& error)
        {
            // Check if component is still valid
            if (weakThis == nullptr)
                return;

            if (instance != nullptr && targetTrack != nullptr)
            {
                // Successfully loaded plugin
                targetTrack->addPlugin(std::move(instance));

                statusLabel.setText("Loaded: " + desc.name + " on " + juce::String(targetTrack->getName()),
                                  juce::dontSendNotification);

                DBG("PluginBrowser: Loaded plugin " + desc.name + " on track " + targetTrack->getName());
            }
            else
            {
                // Failed to load
                statusLabel.setText("Failed to load: " + desc.name, juce::dontSendNotification);

                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Plugin Load Failed",
                    "Failed to load plugin: " + desc.name + "\n\nError: " + error,
                    "OK");

                DBG("PluginBrowser: Failed to load plugin " + desc.name + ": " + error);
            }
        });
}

//==============================================================================
// PluginBrowserWindow Implementation
//==============================================================================

PluginBrowserWindow::PluginBrowserWindow(Engine& engine)
    : DocumentWindow("Plugin Browser",
                     juce::Desktop::getInstance().getDefaultLookAndFeel()
                         .findColour(juce::ResizableWindow::backgroundColourId),
                     DocumentWindow::allButtons)
{
    browserComponent = std::make_unique<PluginBrowserComponent>(engine);

    setUsingNativeTitleBar(true);
    setContentOwned(browserComponent.get(), true);
    setResizable(true, false);

    centreWithSize(getWidth(), getHeight());
    setVisible(true);
}

PluginBrowserWindow::~PluginBrowserWindow()
{
    clearContentComponent();
}

void PluginBrowserWindow::closeButtonPressed()
{
    setVisible(false);
}



