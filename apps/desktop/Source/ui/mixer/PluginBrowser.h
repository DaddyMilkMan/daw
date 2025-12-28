/*
  ==============================================================================

    PluginBrowser.h
    Created: 2025
    Author:  Zenith DAW

    Skia-based Plugin Browser Component.
    Displays a list of available plugins with search and filtering.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../framework/SkiaComponent.h"
#include "../../engine/PluginHost.h"

namespace zenith {

class PluginBrowser : public SkiaComponent, 
                      public juce::TextEditor::Listener {
public:
    // Callback signature: void(std::unique_ptr<juce::AudioPluginInstance>)
    using Callback = std::function<void(const juce::PluginDescription&)>;

    PluginBrowser(PluginHost& pluginHost, Callback callback);
    ~PluginBrowser() override;

    void paint(juce::Graphics& g) override; // Fallback
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    // TextEditor::Listener
    void textEditorTextChanged(juce::TextEditor&) override;
    void textEditorEscapeKeyPressed(juce::TextEditor&) override;
    
    // Timer
    void timerCallback() override;

private:
    PluginHost& pluginHost_;
    Callback callback_;

    struct PluginItem {
        juce::PluginDescription desc;
        juce::String name;
        juce::String category;
        juce::String manufacturer;
        bool isFavorite = false;
        
        // UI Layout
        SkRect bounds;
    };

    std::vector<PluginItem> allPlugins_;
    std::vector<PluginItem*> visiblePlugins_;

    // Search
    juce::TextEditor searchBar_;
    juce::String currentSearch_;
    
    // Scroll
    float scrollY_ = 0.0f;
    float maxScrollY_ = 0.0f;
    float targetScrollY_ = 0.0f; // Smooth scroll target
    
    // Selection / Hover
    int hoveredIndex_ = -1;
    int selectedIndex_ = -1; // Keyboard nav (future)

    // Layout constants
    static constexpr float kHeaderHeight = 50.0f;
    static constexpr float kItemHeight = 32.0f;
    static constexpr float kSearchBarHeight = 28.0f;

    void refreshPluginList();
    void filterPlugins();
    void onItemClicked(const PluginItem& item);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowser)
};

} // namespace zenith
