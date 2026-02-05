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
