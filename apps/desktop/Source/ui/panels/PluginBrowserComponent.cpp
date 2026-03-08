/**
 * @file PluginBrowserComponent.cpp
 * @brief Plugin browser implementation
 */

#include "PluginBrowserComponent.h"
#include "Engine.h"
#include "../controls/SkiaAlertWindow.h"
#include "../design-system/ColorBridge.h"
#include "../design-system/ZenithTheme.h"
#include "../engine/PluginHost.h"
#include "../engine/Track.h"

using namespace zenith;

PluginBrowserComponent::PluginBrowserComponent(Engine& eng)
    : engine(eng), pluginList("plugin-list")
{
    setSize(860, 620);

    searchBox.setPlaceholder("Search plugins by name, maker, or category");
    searchBox.setPillShape(false);
    searchBox.onTextChanged = [this](const juce::String&) { updateFilteredList(); };
    addAndMakeVisible(searchBox);

    trackSelector.setTextWhenNothingSelected("Select target track");
    trackSelector.onChange = [this]()
    {
        const int trackIndex = trackSelector.getSelectedId() - 2;
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

    pluginList.setModel(this);
    pluginList.setRowHeight(34);
    pluginList.setOutlineThickness(1);
    pluginList.setColour(design::toSkColor(ZenithTheme::Colors::bg_02));
    pluginList.onRowDoubleClicked = [this](int row)
    {
        if (row >= 0 && row < filteredPlugins.size())
        {
            pluginList.selectRow(row);
            loadPluginAtIndex(row);
        }
    };
    addAndMakeVisible(pluginList);

    loadButton.setText("Load On Track");
    loadButton.setStyle(SkiaButton::Style::Primary);
    loadButton.onClick = [this]() { loadSelectedPlugin(); };
    addAndMakeVisible(loadButton);

    statusLabel.setText("No target track selected", juce::dontSendNotification);
    statusLabel.setJustification(SkiaLabel::Justification::Left);
    statusLabel.setTextColour(design::toSkColor(ZenithTheme::Colors::text_secondary));
    addAndMakeVisible(statusLabel);

    refresh();
}

PluginBrowserComponent::~PluginBrowserComponent() = default;

void PluginBrowserComponent::paint(juce::Graphics& g)
{
    g.fillAll(ZenithTheme::Colors::bg_00);

    auto bounds = getLocalBounds().reduced(12);

    auto titleArea = bounds.removeFromTop(36);
    g.setColour(ZenithTheme::Colors::text_primary);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawText("Plugin Browser", titleArea, juce::Justification::centredLeft, false);

    auto searchArea = bounds.removeFromTop(34);
    g.setColour(ZenithTheme::Colors::text_secondary);
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("Search", searchArea.removeFromLeft(72), juce::Justification::centredLeft, false);

    bounds.removeFromTop(8);

    auto trackArea = bounds.removeFromTop(34);
    g.drawText("Target", trackArea.removeFromLeft(72), juce::Justification::centredLeft, false);

    bounds.removeFromTop(12);

    auto tableHeader = bounds.removeFromTop(24);
    g.setColour(ZenithTheme::Colors::bg_02);
    g.fillRoundedRectangle(tableHeader.toFloat(), 4.0f);

    const int w = tableHeader.getWidth();
    const int nameW = static_cast<int>(w * 0.42f);
    const int categoryW = static_cast<int>(w * 0.20f);
    const int manufacturerW = static_cast<int>(w * 0.26f);
    auto col = tableHeader;

    g.setColour(ZenithTheme::Colors::text_secondary);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("Name", col.removeFromLeft(nameW).reduced(10, 0), juce::Justification::centredLeft, false);
    g.drawText("Category", col.removeFromLeft(categoryW).reduced(10, 0), juce::Justification::centredLeft, false);
    g.drawText("Manufacturer", col.removeFromLeft(manufacturerW).reduced(10, 0), juce::Justification::centredLeft, false);
    g.drawText("Format", col.reduced(10, 0), juce::Justification::centredLeft, false);
}

void PluginBrowserComponent::resized()
{
    auto bounds = getLocalBounds().reduced(12);

    bounds.removeFromTop(36);

    auto searchArea = bounds.removeFromTop(34);
    searchArea.removeFromLeft(72);
    searchBox.setBounds(searchArea);

    bounds.removeFromTop(8);

    auto trackArea = bounds.removeFromTop(34);
    trackArea.removeFromLeft(72);
    trackSelector.setBounds(trackArea.withWidth(320));

    bounds.removeFromTop(12);
    bounds.removeFromTop(24);

    auto bottomBar = bounds.removeFromBottom(42);
    loadButton.setBounds(bottomBar.removeFromRight(160));
    statusLabel.setBounds(bottomBar.reduced(6, 0));

    bounds.removeFromBottom(8);
    pluginList.setBounds(bounds);
}

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
    return pluginList.getSelectedRow();
}

bool PluginBrowserComponent::loadSelectedPlugin()
{
    const int selectedIndex = getSelectedPluginIndex();
    if (selectedIndex < 0 || selectedIndex >= filteredPlugins.size())
    {
        SkiaAlertWindow::showMessageBoxAsync(
            SkiaAlertWindow::IconType::WarningIcon,
            "No Plugin Selected",
            "Select a plugin from the list.",
            "OK");
        return false;
    }

    if (targetTrack == nullptr)
    {
        SkiaAlertWindow::showMessageBoxAsync(
            SkiaAlertWindow::IconType::WarningIcon,
            "No Target Track",
            "Select a target track first.",
            "OK");
        return false;
    }

    loadPluginAtIndex(selectedIndex);
    return true;
}

void PluginBrowserComponent::refresh()
{
    trackSelector.clear();
    trackSelector.addItem("(Select Track)", 1);

    const auto& tracks = engine.tracks();
    for (int i = 0; i < engine.getNumTracks(); ++i)
    {
        if (tracks[i] != nullptr)
        {
            trackSelector.addItem(tracks[i]->getName(), i + 2);
        }
    }

    trackSelector.setSelectedId(1);
    updateFilteredList();
}

int PluginBrowserComponent::getNumRows()
{
    return filteredPlugins.size();
}

void PluginBrowserComponent::paintListBoxItem(int rowNumber, SkCanvas& canvas, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= filteredPlugins.size())
        return;

    const auto& desc = filteredPlugins[rowNumber];

    SkPaint rowPaint;
    if (rowIsSelected)
    {
        rowPaint.setColor(design::toSkColor(ZenithTheme::Colors::accent_subtle));
    }
    else
    {
        rowPaint.setColor(design::toSkColor((rowNumber % 2 == 0) ? ZenithTheme::Colors::bg_01 : ZenithTheme::Colors::bg_02));
    }
    canvas.drawRect(SkRect::MakeWH(static_cast<float>(width), static_cast<float>(height)), rowPaint);

    SkFont font;
    font.setSize(13.0f);

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(design::toSkColor(rowIsSelected ? ZenithTheme::Colors::text_primary : ZenithTheme::Colors::text_secondary));

    const int nameW = static_cast<int>(width * 0.42f);
    const int categoryW = static_cast<int>(width * 0.20f);
    const int manufacturerW = static_cast<int>(width * 0.26f);
    const float baseline = static_cast<float>(height) * 0.62f;

    const juce::String category = desc.category.isEmpty() ? "Unknown" : desc.category;

    canvas.drawString(desc.name.toRawUTF8(), 10.0f, baseline, font, textPaint);
    canvas.drawString(category.toRawUTF8(), static_cast<float>(nameW) + 10.0f, baseline, font, textPaint);
    canvas.drawString(desc.manufacturerName.toRawUTF8(), static_cast<float>(nameW + categoryW) + 10.0f, baseline, font, textPaint);
    canvas.drawString(desc.pluginFormatName.toRawUTF8(), static_cast<float>(nameW + categoryW + manufacturerW) + 10.0f, baseline, font, textPaint);
}

void PluginBrowserComponent::listBoxItemDoubleClicked(int rowNumber, const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);

    if (rowNumber >= 0 && rowNumber < filteredPlugins.size())
    {
        pluginList.selectRow(rowNumber);
        loadPluginAtIndex(rowNumber);
    }
}

void PluginBrowserComponent::updateFilteredList()
{
    filteredPlugins.clear();
    currentFilter = searchBox.getText().toLowerCase();

    auto& knownPlugins = engine.getPluginHost().getKnownPlugins();
    for (const auto& desc : knownPlugins.getTypes())
    {
        if (currentFilter.isEmpty()
            || desc.name.toLowerCase().contains(currentFilter)
            || desc.manufacturerName.toLowerCase().contains(currentFilter)
            || desc.category.toLowerCase().contains(currentFilter))
        {
            filteredPlugins.add(desc);
        }
    }

    pluginList.updateContent();
}

void PluginBrowserComponent::loadPluginAtIndex(int index)
{
    if (index < 0 || index >= filteredPlugins.size() || targetTrack == nullptr)
        return;

    const auto& desc = filteredPlugins[index];
    statusLabel.setText("Loading: " + desc.name + "...", juce::dontSendNotification);

    auto& formatManager = engine.getPluginFormatManager();
    formatManager.createPluginInstanceAsync(
        desc,
        engine.getSampleRate(),
        engine.getBufferSize(),
        [this, desc, weakThis = juce::Component::SafePointer<PluginBrowserComponent>(this)](
            std::unique_ptr<juce::AudioPluginInstance> instance,
            const juce::String& error)
        {
            if (weakThis == nullptr)
                return;

            if (instance != nullptr && targetTrack != nullptr)
            {
                targetTrack->addPlugin(std::move(instance));
                statusLabel.setText("Loaded: " + desc.name + " on " + juce::String(targetTrack->getName()),
                                    juce::dontSendNotification);
                DBG("PluginBrowser: Loaded plugin " + desc.name + " on track " + targetTrack->getName());
            }
            else
            {
                statusLabel.setText("Failed to load: " + desc.name, juce::dontSendNotification);
                SkiaAlertWindow::showMessageBoxAsync(
                    SkiaAlertWindow::IconType::WarningIcon,
                    "Plugin Load Failed",
                    "Failed to load plugin: " + desc.name + "\n\nError: " + error,
                    "OK");
                DBG("PluginBrowser: Failed to load plugin " + desc.name + ": " + error);
            }
        });
}
