#include "../../include/ui/InstrumentBrowserPanel.h"
#include "../instruments/InstrumentRegistry.h"
#include "../instruments/InstrumentMetadata.h"

namespace zenith {

InstrumentBrowserPanel::InstrumentBrowserPanel(InstrumentRegistry& registry)
    : registry_(registry)
{
    // Title
    addAndMakeVisible(titleLabel_);
    titleLabel_.setText("Instruments", juce::dontSendNotification);
    titleLabel_.setJustificationType(juce::Justification::centred);
    titleLabel_.setFont(juce::FontOptions(16.0f).withStyle("Bold"));

    // Search box
    addAndMakeVisible(searchBox_);
    searchBox_.setTextToShowWhenEmpty("Search instruments...", juce::Colours::grey);
    searchBox_.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff2a2a2a));
    searchBox_.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    searchBox_.setInputRestrictions(100); // Max 100 characters for search

    searchBox_.onTextChange = [this]()
    {
        filterInstruments(searchBox_.getText());
    };

    // Instrument list
    addAndMakeVisible(instrumentList_);
    instrumentList_.setModel(this);
    instrumentList_.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff1e1e1e));

    // Load initial list
    updateInstrumentList();
}

void InstrumentBrowserPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
}

void InstrumentBrowserPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Title
    titleLabel_.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(5);

    // Search box
    searchBox_.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(10);

    // List takes remaining space
    instrumentList_.setBounds(bounds);
}

void InstrumentBrowserPanel::setLoadInstrumentCallback(std::function<void(const juce::String&)> callback)
{
    onLoadInstrument_ = callback;
}

int InstrumentBrowserPanel::getNumRows()
{
    return static_cast<int>(filteredIds_.size());
}

void InstrumentBrowserPanel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(filteredIds_.size()))
        return;

    // Background
    if (rowIsSelected)
        g.fillAll(juce::Colour(0xff4a9eff));
    else if (rowNumber % 2 == 0)
        g.fillAll(juce::Colour(0xff242424));
    else
        g.fillAll(juce::Colour(0xff1e1e1e));

    // Text
    g.setColour(rowIsSelected ? juce::Colours::white : juce::Colours::lightgrey);
    g.setFont(14.0f);

    const auto& instrumentId = filteredIds_[rowNumber];

    // Get metadata from cache (O(log n) instead of O(n))
    juce::String displayText = instrumentId;
    auto it = metadataCache_.find(instrumentId);
    if (it != metadataCache_.end() && it->second.name.isNotEmpty())
        displayText = it->second.name;

    g.drawText(displayText, 10, 0, width - 20, height, juce::Justification::centredLeft, true);
}

void InstrumentBrowserPanel::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (row < 0 || row >= static_cast<int>(filteredIds_.size()))
        return;

    const auto& instrumentId = filteredIds_[row];

    if (instrumentId.isEmpty())
    {
        DBG("InstrumentBrowserPanel: Cannot load instrument with empty ID");
        return;
    }

    if (onLoadInstrument_)
    {
        try
        {
            onLoadInstrument_(instrumentId);
        }
        catch (const std::exception& e)
        {
            DBG("InstrumentBrowserPanel: Failed to load instrument '" << instrumentId << "': " << e.what());
        }
    }
}

void InstrumentBrowserPanel::updateInstrumentList()
{
    try
    {
        auto ids = registry_.getAllInstrumentIds();
        instrumentIds_.clear();
        metadataCache_.clear();

        // Build metadata cache
        for (const auto& id : ids)
        {
            try
            {
                instrumentIds_.push_back(id);
                metadataCache_[id] = registry_.getInstrumentMetadata(id);
            }
            catch (const std::exception& e)
            {
                DBG("InstrumentBrowserPanel: Failed to load metadata for instrument '" << id << "': " << e.what());
                // Continue with next instrument
            }
        }

        filteredIds_ = instrumentIds_;
        instrumentList_.updateContent();
    }
    catch (const std::exception& e)
    {
        DBG("InstrumentBrowserPanel: Critical error updating instrument list: " << e.what());
        // Ensure we have at least an empty list to avoid crashes
        filteredIds_.clear();
        instrumentList_.updateContent();
    }
}

void InstrumentBrowserPanel::filterInstruments(const juce::String& searchTerm)
{
    // Input validation
    if (searchTerm.length() > 100)
    {
        DBG("InstrumentBrowserPanel: Search term too long (max 100 characters)");
        return;
    }

    if (searchTerm.isEmpty())
    {
        filteredIds_ = instrumentIds_;
    }
    else
    {
        filteredIds_.clear();

        juce::String lowerSearch = searchTerm.toLowerCase();

        for (const auto& id : instrumentIds_)
        {
            // Use cached metadata (O(log n) instead of O(n))
            juce::String displayName = id;
            auto it = metadataCache_.find(id);
            if (it != metadataCache_.end() && it->second.name.isNotEmpty())
                displayName = it->second.name;

            if (displayName.toLowerCase().contains(lowerSearch) ||
                id.toLowerCase().contains(lowerSearch))
            {
                filteredIds_.push_back(id);
            }
        }
    }

    instrumentList_.updateContent();
}

} // namespace zenith
