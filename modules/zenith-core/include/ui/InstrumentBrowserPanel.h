#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <map>
#include <functional>

namespace zenith {

// Forward declarations
class InstrumentRegistry;
struct InstrumentMetadata;

/**
 * @brief Browser panel for discovering and loading instruments
 *
 * Displays available instruments from the registry and allows users to
 * load them into tracks.
 */
class InstrumentBrowserPanel : public juce::Component,
                                public juce::ListBoxModel
{
public:
    explicit InstrumentBrowserPanel(InstrumentRegistry& registry);
    ~InstrumentBrowserPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /**
     * @brief Set callback for when user selects an instrument to load
     */
    void setLoadInstrumentCallback(std::function<void(const juce::String& instrumentId)> callback);

    // ListBoxModel implementation
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;

private:
    InstrumentRegistry& registry_;
    std::function<void(const juce::String&)> onLoadInstrument_;

    juce::ListBox instrumentList_;
    juce::Label titleLabel_;
    juce::TextEditor searchBox_;

    std::vector<juce::String> instrumentIds_;
    std::vector<juce::String> filteredIds_;
    std::map<juce::String, InstrumentMetadata> metadataCache_; // Cache to avoid O(n²) lookups

    void updateInstrumentList();
    void filterInstruments(const juce::String& searchTerm);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentBrowserPanel)
};

} // namespace zenith
