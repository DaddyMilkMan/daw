/*
  ==============================================================================

    PluginBrowser.cpp
    Created: 2025
    Author:  Zenith DAW

  ==============================================================================
*/

#include "PluginBrowser.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"
#include "../design-system/ZenithTheme.h"

namespace zenith {

using namespace design;

PluginBrowser::PluginBrowser(PluginHost& pluginHost, Callback callback)
    : pluginHost_(pluginHost), callback_(callback) {
    
    addAndMakeVisible(searchBar_);
    searchBar_.addListener(this);
    searchBar_.setTextToShowWhenEmpty("Search Plugins...", ZenithTheme::Colors::text_tertiary);
    searchBar_.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    searchBar_.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    searchBar_.setColour(juce::TextEditor::textColourId, ZenithTheme::Colors::text_primary);

    refreshPluginList();
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(60); // Animation timer
}

PluginBrowser::~PluginBrowser() {
    searchBar_.removeListener(this);
}

void PluginBrowser::resized() {
    auto bounds = getLocalBounds().toFloat();
    searchBar_.setBounds(10, 10, bounds.getWidth() - 20, kSearchBarHeight);
    
    // Update Layout if needed
}

void PluginBrowser::paint(juce::Graphics& g) {
    // Skia rendering used - see drawSkia()
    juce::ignoreUnused(g);
}

void PluginBrowser::drawSkia(SkCanvas* canvas) {
    using namespace zenith::design;
    
    float width = (float)getLocalBounds().getWidth();
    float height = (float)getLocalBounds().getHeight();
    
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(colors::BG_DARKEST);
    canvas->drawRect(SkRect::MakeWH(width, height), bgPaint);
    
    // Header background
    SkPaint headerPaint;
    headerPaint.setColor(colors::BG_DARKER);
    canvas->drawRect(SkRect::MakeWH(width, kHeaderHeight), headerPaint);
    
    // List content
    canvas->save();
    canvas->clipRect(SkRect::MakeLTRB(0, kHeaderHeight, width, height));
    canvas->translate(0, -scrollY_ + kHeaderHeight);
    
    float y = 0;
    
    SkFont font = typography::getSkFont(typography::FONT_SM, FontWeight::Regular);
    // Better: use typography bridge if available, otherwise just use SkFont directly or via Theme
    // ZenithTheme doesn't return SkFont directly usually. 
    // Let's assume typography::getSkFont exists in ZenithDesignSystem.
    // Actually, I should use what works.
    // The previous error didn't complain about typography::getSkFont.
    // Only colors.
    // But let's check textPaint color.
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(toSkColor(ZenithTheme::Colors::text_primary));
    
    SkPaint hoverPaint;
    hoverPaint.setColor(toSkColor(ZenithTheme::Colors::accent_primary.withAlpha(0.1f)));
    
    for (size_t i = 0; i < visiblePlugins_.size(); ++i) {
        auto* item = visiblePlugins_[i];
        
        SkRect itemRect = SkRect::MakeXYWH(0, y, width, kItemHeight);
        item->bounds = itemRect; // Update bounds for hit testing (absolute logic needs offset)
        
        if (static_cast<int>(i) == hoveredIndex_) {
            canvas->drawRect(itemRect, hoverPaint);
        }
        
        // Icon / Type indicator (placeholder)
        // ...
        
        // Name
        canvas->drawString(item->name.toRawUTF8(), 20.0f, y + kItemHeight * 0.65f, font, textPaint);
        
        // Category (right aligned)
        SkPaint catPaint;
        catPaint.setAntiAlias(true);
        catPaint.setColor(colors::TEXT_SECONDARY);
        SkFont catFont = font; // reuse font for simplicity or fetch smaller

        
        std::string catStr = item->category.toStdString();
        SkRect catBounds;
        catFont.measureText(catStr.c_str(), catStr.length(), SkTextEncoding::kUTF8, &catBounds);
        
        canvas->drawString(catStr.c_str(), width - catBounds.width() - 20.0f, y + kItemHeight * 0.65f, catFont, catPaint);
        
        y += kItemHeight;
    }
    
    canvas->restore();
    
    // Scrollbar (simplified)
    if (maxScrollY_ > 0) {
        float viewportHeight = height - kHeaderHeight;
        float thumbHeight = std::max(20.0f, viewportHeight * (viewportHeight / (maxScrollY_ + viewportHeight)));
        float thumbY = kHeaderHeight + (scrollY_ / maxScrollY_) * (viewportHeight - thumbHeight);
        
        SkPaint scrollPaint;
        scrollPaint.setColor(SkColorSetARGB(40, 255, 255, 255));
        canvas->drawRoundRect(SkRect::MakeXYWH(width - 6, thumbY, 4, thumbHeight), 2, 2, scrollPaint);
    }
}

void PluginBrowser::refreshPluginList() {
    allPlugins_.clear();
    
    // Populate from PluginHost
    for (const auto& desc : pluginHost_.getKnownPlugins().getTypes()) {
        PluginItem item;
        item.desc = desc;
        item.name = desc.name;
        item.category = desc.category;
        item.manufacturer = desc.manufacturerName;
        allPlugins_.push_back(item);
    }
    
    filterPlugins();
}

void PluginBrowser::filterPlugins() {
    visiblePlugins_.clear();
    
    juce::String query = currentSearch_.trim().toLowerCase();
    
    for (auto& item : allPlugins_) {
        if (query.isEmpty() || item.name.toLowerCase().contains(query) || item.manufacturer.toLowerCase().contains(query)) {
            visiblePlugins_.push_back(&item);
        }
    }
    
    float viewportHeight = getHeight() - kHeaderHeight;
    float contentHeight = visiblePlugins_.size() * kItemHeight;
    maxScrollY_ = std::max(0.0f, contentHeight - viewportHeight);
    
    repaint();
}

void PluginBrowser::textEditorTextChanged(juce::TextEditor& editor) {
    currentSearch_ = editor.getText();
    filterPlugins();
    scrollY_ = 0;
    targetScrollY_ = 0;
}

void PluginBrowser::textEditorEscapeKeyPressed(juce::TextEditor& editor) {
    editor.setText("");
    currentSearch_ = "";
    filterPlugins();
    editor.giveAwayKeyboardFocus();
}

void PluginBrowser::timerCallback() {
    // Smooth scroll
    if (std::abs(targetScrollY_ - scrollY_) > 0.5f) {
        scrollY_ += (targetScrollY_ - scrollY_) * 0.2f;
        repaint();
    }
}

void PluginBrowser::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    targetScrollY_ -= wheel.deltaY * 300.0f; // Speed
    targetScrollY_ = juce::jlimit(0.0f, maxScrollY_, targetScrollY_);
}

void PluginBrowser::mouseMove(const juce::MouseEvent& e) {
    float y = e.y - kHeaderHeight + scrollY_;
    int index = (int)(y / kItemHeight);
    
    if (e.y < kHeaderHeight || index < 0 || index >= (int)visiblePlugins_.size()) {
        if (hoveredIndex_ != -1) {
            hoveredIndex_ = -1;
            repaint();
        }
    } else {
        if (hoveredIndex_ != index) {
            hoveredIndex_ = index;
            repaint();
        }
    }
}

void PluginBrowser::mouseExit(const juce::MouseEvent&) {
    if (hoveredIndex_ != -1) {
        hoveredIndex_ = -1;
        repaint();
    }
}

void PluginBrowser::mouseDown(const juce::MouseEvent& e) {
    if (hoveredIndex_ >= 0 && hoveredIndex_ < (int)visiblePlugins_.size()) {
        onItemClicked(*visiblePlugins_[hoveredIndex_]);
    }
}

void PluginBrowser::onItemClicked(const PluginItem& item) {
    if (callback_) {
        callback_(item.desc);
    }
}

} // namespace zenith
