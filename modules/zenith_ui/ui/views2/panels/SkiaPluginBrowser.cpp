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

 * @file SkiaPluginBrowser.cpp
 * @brief Skia-based plugin browser implementation
 */



//==============================================================================
// Construction/Destruction
//==============================================================================

SkiaPluginBrowser::SkiaPluginBrowser(Engine& engine)
    : engine(engine), selectedCategory_(CategoryFilter::All) {
    setWantsKeyboardFocus(true);
    addKeyListener(this);

    // Initialize with some sample data for demo
    initializeDemoPlugins();
    updateFilteredList();
}

SkiaPluginBrowser::~SkiaPluginBrowser() {
    removeKeyListener(this);
}

//==============================================================================
// Component Interface
//==============================================================================

void SkiaPluginBrowser::drawSkia(SkCanvas* canvas) {
    auto bounds = getLocalBounds().toFloat();
    SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

    // Draw main background
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Elevated;
    opts.cornerRadius = 16.0f;
    opts.useBackdropBlur = true;
    opts.customTintColor = SkColorSetA(design::colors::BG_00, 240);
    GlassmorphicPanel::drawWithOptions(canvas, skBounds, opts);

    // Calculate layout regions
    float y = kPanelPadding;

    headerRect_ = SkRect::MakeXYWH(kPanelPadding, y,
                                   skBounds.width() - (2 * kPanelPadding), kHeaderHeight);
    y += headerRect_.height() + 16.0f;

    searchBarRect_ = SkRect::MakeXYWH(kPanelPadding, y,
                                      skBounds.width() - (2 * kPanelPadding), kSearchBarHeight);
    y += searchBarRect_.height() + 16.0f;

    categoryFilterRect_ = SkRect::MakeXYWH(kPanelPadding, y,
                                          skBounds.width() - (2 * kPanelPadding), kCategoryFilterHeight);
    y += categoryFilterRect_.height() + 16.0f;

    listRect_ = SkRect::MakeXYWH(kPanelPadding, y,
                                skBounds.width() - (2 * kPanelPadding),
                                skBounds.height() - y - kFooterHeight - kPanelPadding);

    footerRect_ = SkRect::MakeXYWH(kPanelPadding, listRect_.bottom() + 16.0f,
                                  skBounds.width() - (2 * kPanelPadding), kFooterHeight);

    // Draw sections
    drawHeader(canvas, headerRect_);
    drawSearchBar(canvas, searchBarRect_);
    drawCategoryFilter(canvas, categoryFilterRect_);
    drawPluginList(canvas, listRect_);
    drawFooter(canvas, footerRect_);
}

void SkiaPluginBrowser::resized() {
    // Layout is handled in drawSkia with dynamic positioning
}

//==============================================================================
// Drawing Methods
//==============================================================================

void SkiaPluginBrowser::drawHeader(SkCanvas* canvas, const SkRect& bounds) {
    // Title
    SkFont titleFont = design::getDisplayFont(20.0f, design::FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setColor(design::colors::TEXT_PRIMARY);
    titlePaint.setAntiAlias(true);

    juce::String title = "Plugin Browser";
    std::string titleStr = title.toStdString();
    float titleWidth = titleFont.measureText(titleStr.c_str(), titleStr.length(), SkTextEncoding::kUTF8);
    float x = bounds.centerX() - (titleWidth / 2.0f);
    float y = bounds.centerY() + 6.0f;

    canvas->drawString(titleStr.c_str(), x, y, titleFont, titlePaint);

    // Close button
    float btnSize = 32.0f;
    float padding = 16.0f;
    SkRect closeBtn = SkRect::MakeXYWH(bounds.right() - btnSize - padding,
                                       bounds.centerY() - (btnSize/2),
                                       btnSize, btnSize);

    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetA(design::colors::BG_01, 100));
    canvas->drawRoundRect(closeBtn, 8.0f, 8.0f, bgPaint);

    // Draw X icon
    SkPaint iconPaint;
    iconPaint.setColor(design::colors::TEXT_SECONDARY);
    iconPaint.setStyle(SkPaint::kStroke_Style);
    iconPaint.setStrokeWidth(2.0f);
    iconPaint.setAntiAlias(true);

    canvas->drawLine(closeBtn.left() + 8, closeBtn.top() + 8,
                     closeBtn.right() - 8, closeBtn.bottom() - 8, iconPaint);
    canvas->drawLine(closeBtn.right() - 8, closeBtn.top() + 8,
                     closeBtn.left() + 8, closeBtn.bottom() - 8, iconPaint);
}

void SkiaPluginBrowser::drawSearchBar(SkCanvas* canvas, const SkRect& bounds) {
    // Search background
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Subtle;
    opts.cornerRadius = 8.0f;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);

    // Search icon
    SkFont iconFont = design::getSkFont(16.0f);
    SkPaint iconPaint;
    iconPaint.setColor(design::colors::TEXT_TERTIARY);
    iconPaint.setAntiAlias(true);

    canvas->drawString("🔍", bounds.left() + 16, bounds.centerY() + 6, iconFont, iconPaint);

    // Search text (or placeholder)
    SkFont textFont = design::getSkFont(14.0f);
    SkPaint textPaint;

    if (searchText_.isEmpty() && !isSearching_) {
        textPaint.setColor(design::colors::TEXT_TERTIARY);
        juce::String placeholder = "Search plugins...";
        std::string placeholderStr = placeholder.toStdString();
        canvas->drawString(placeholderStr.c_str(), bounds.left() + 40, bounds.centerY() + 6, textFont, textPaint);
    } else {
        textPaint.setColor(design::colors::TEXT_PRIMARY);
        std::string searchTextStr = searchText_.toStdString();
        canvas->drawString(searchTextStr.c_str(), bounds.left() + 40, bounds.centerY() + 6, textFont, textPaint);
    }
}

void SkiaPluginBrowser::drawCategoryFilter(SkCanvas* canvas, const SkRect& bounds) {
    float buttonWidth = 120.0f;
    float spacing = 12.0f;
    float startX = bounds.left();
    float y = bounds.centerY() - 20.0f;

    // Category buttons
    const std::pair<CategoryFilter, juce::String> categories[] = {
        {CategoryFilter::All, "All"},
        {CategoryFilter::Instruments, "Instruments"},
        {CategoryFilter::Effects, "Effects"},
        {CategoryFilter::Synths, "Synths"},
        {CategoryFilter::Drums, "Drums"}
    };

    for (const auto& [category, label] : categories) {
        SkRect btnRect = SkRect::MakeXYWH(startX, y, buttonWidth, 36.0f);

        bool isActive = (selectedCategory_ == category);
        bool isHovered = (hoveredIndex_ == (category - CategoryFilter::All) + 1000); // Special range for categories

        drawCategoryButton(canvas, btnRect, label, category, isActive, isHovered);

        startX += buttonWidth + spacing;
    }
}

void SkiaPluginBrowser::drawPluginList(SkCanvas* canvas, const SkRect& bounds) {
    // List background
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Subtle;
    opts.cornerRadius = 8.0f;
    GlassmorphicPanel::drawWithOptions(canvas, bounds, opts);

    // Draw scrollbar
    float scrollWidth = 8.0f;
    float scrollX = bounds.right() - scrollWidth - 4.0f;
    float scrollHeight = bounds.height();
    float scrollY = bounds.top();

    // Scrollbar track
    SkRect scrollTrack = SkRect::MakeXYWH(scrollX, scrollY, scrollWidth, scrollHeight);
    SkPaint scrollTrackPaint;
    scrollTrackPaint.setColor(SkColorSetA(design::colors::BG_02, 100));
    canvas->drawRoundRect(scrollTrack, 4.0f, 4.0f, scrollTrackPaint);

    // Draw plugin items
    float itemY = bounds.top() + 8.0f;
    float availableHeight = bounds.height() - 16.0f;

    for (size_t i = 0; i < filteredItems.size() && itemY < bounds.bottom(); ++i) {
        float itemHeight = kItemHeight;
        SkRect itemBounds = SkRect::MakeXYWH(bounds.left() + 8.0f, itemY,
                                            bounds.width() - 24.0f, itemHeight);

        bool isSelected = (selectedIndex_ == static_cast<int>(i));
        bool isHovered = (hoveredIndex_ == static_cast<int>(i));

        drawPluginItem(canvas, itemBounds, filteredItems[i], static_cast<int>(i), isSelected, isHovered);

        itemY += itemHeight + 4.0f;
    }

    // Empty state
    if (filteredItems.empty()) {
        SkFont font = design::getSkFont(14.0f);
        SkPaint paint;
        paint.setColor(design::colors::TEXT_TERTIARY);
        paint.setAntiAlias(true);

        juce::String emptyMsg = searchText_.isNotEmpty() ? "No plugins found" : "No plugins available";
        std::string emptyMsgStr = emptyMsg.toStdString();
        float msgWidth = font.measureText(emptyMsgStr.c_str(), emptyMsgStr.length(), SkTextEncoding::kUTF8);
        float x = bounds.centerX() - (msgWidth / 2.0f);
        float y = bounds.centerY();

        canvas->drawString(emptyMsgStr.c_str(), x, y, font, paint);
    }
}

void SkiaPluginBrowser::drawFooter(SkCanvas* canvas, const SkRect& bounds) {
    // Track selector
    SkFont trackFont = design::getSkFont(12.0f);
    SkPaint trackPaint;
    trackPaint.setColor(design::colors::TEXT_PRIMARY);
    trackPaint.setAntiAlias(true);

    juce::String trackText = "Track: " + trackNames_[selectedTrack_];
    std::string trackTextStr = trackText.toStdString();
    canvas->drawString(trackTextStr.c_str(), bounds.left() + 16, bounds.centerY() + 4, trackFont, trackPaint);

    // Load button
    float btnWidth = 120.0f;
    float btnHeight = 36.0f;
    SkRect loadBtn = SkRect::MakeXYWH(bounds.right() - btnWidth - 16,
                                      bounds.centerY() - (btnHeight/2),
                                      btnWidth, btnHeight);

    SkPaint loadPaint;
    loadPaint.setColor(selectedIndex_ >= 0 ? design::colors::ACCENT_PRIMARY : SkColorSetA(design::colors::BG_02, 100));
    canvas->drawRoundRect(loadBtn, 8.0f, 8.0f, loadPaint);

    // Button text
    SkPaint textPaint;
    textPaint.setColor(selectedIndex_ >= 0 ? design::colors::TEXT_PRIMARY : design::colors::TEXT_TERTIARY);
    textPaint.setAntiAlias(true);

    juce::String btnText = "Load Plugin";
    std::string btnTextStr = btnText.toStdString();
    float textWidth = trackFont.measureText(btnTextStr.c_str(), btnTextStr.length(), SkTextEncoding::kUTF8);
    float x = loadBtn.centerX() - (textWidth / 2.0f);
    float y = loadBtn.centerY() + 4.0f;

    canvas->drawString(btnTextStr.c_str(), x, y, trackFont, textPaint);
}

void SkiaPluginBrowser::drawPluginItem(SkCanvas* canvas, const SkRect& bounds,
                                       const PluginItem& item, int index, bool isSelected, bool isHovered) {
    // Item background
    SkPaint bgPaint;
    if (isSelected) {
        bgPaint.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
    } else if (isHovered) {
        bgPaint.setColor(SkColorSetA(design::colors::BG_01, 50));
    } else {
        bgPaint.setColor(SkColorSetA(design::colors::BG_00, 20));
    }
    canvas->drawRoundRect(bounds, 6.0f, 6.0f, bgPaint);

    // Plugin name
    SkFont nameFont = design::getSkFont(14.0f, design::FontWeight::Semibold);
    SkPaint namePaint;
    namePaint.setColor(design::colors::TEXT_PRIMARY);
    namePaint.setAntiAlias(true);

    canvas->drawString(item.name.toStdString().c_str(), bounds.left() + 12, bounds.top() + 20, nameFont, namePaint);

    // Plugin manufacturer
    SkFont manufFont = design::getSkFont(12.0f);
    SkPaint manufPaint;
    manufPaint.setColor(design::colors::TEXT_SECONDARY);
    manufPaint.setAntiAlias(true);

    canvas->drawString(item.manufacturer.toStdString().c_str(), bounds.left() + 12, bounds.top() + 36, manufPaint, manufFont);

    // Category badge
    if (item.category.isNotEmpty()) {
        SkRect badgeRect = SkRect::MakeXYWH(bounds.right() - 80, bounds.top() + 8, 70, 20);
        SkPaint badgePaint;
        badgePaint.setColor(getCategoryColor(selectedCategory_));
        canvas->drawRoundRect(badgeRect, 10.0f, 10.0f, badgePaint);

        SkPaint badgeTextPaint;
        badgeTextPaint.setColor(design::colors::TEXT_PRIMARY);
        badgeTextPaint.setAntiAlias(true);

        SkFont badgeFont = design::getSkFont(10.0f);
        canvas->drawString(item.category.toStdString().c_str(),
                          badgeRect.centerX() - badgeFont.measureText(item.category.toStdString().c_str(),
                            item.category.length(), SkTextEncoding::kUTF8) / 2.0f,
                          badgeRect.centerY() + 4, badgeFont, badgeTextPaint);
    }
}

void SkiaPluginBrowser::drawCategoryButton(SkCanvas* canvas, const SkRect& bounds,
                                           const juce::String& label, CategoryFilter filter,
                                           bool isActive, bool isHovered) {
    SkPaint bgPaint;
    if (isActive) {
        bgPaint.setColor(design::colors::ACCENT_PRIMARY);
    } else if (isHovered) {
        bgPaint.setColor(SkColorSetA(design::colors::ACCENT_PRIMARY, 30));
    } else {
        bgPaint.setColor(SkColorSetA(design::colors::BG_01, 100));
    }
    canvas->drawRoundRect(bounds, 18.0f, 18.0f, bgPaint);

    // Button text
    SkFont font = design::getSkFont(12.0f, design::FontWeight::Medium);
    SkPaint textPaint;
    textPaint.setColor(isActive ? design::colors::TEXT_PRIMARY : design::colors::TEXT_SECONDARY);
    textPaint.setAntiAlias(true);

    float textWidth = font.measureText(label.toStdString().c_str(), label.length(), SkTextEncoding::kUTF8);
    float x = bounds.centerX() - (textWidth / 2.0f);
    float y = bounds.centerY() + 4.0f;

    canvas->drawString(label.toStdString().c_str(), x, y, font, textPaint);
}

//==============================================================================
// Input Handling
//==============================================================================

void SkiaPluginBrowser::mouseDown(const juce::MouseEvent& e) {
    float x = (float)e.getPosition().x;
    float y = (float)e.getPosition().y;

    // Check close button
    if (headerRect_.contains(x, y)) {
        float btnSize = 32.0f;
        float padding = 16.0f;
        SkRect closeBtn = SkRect::MakeXYWH(headerRect_.right() - btnSize - padding,
                                           headerRect_.centerY() - (btnSize/2),
                                           btnSize, btnSize);
        if (closeBtn.contains(x, y)) {
            if (onClose) onClose();
            return;
        }
    }

    // Check search bar
    if (searchBarRect_.contains(x, y)) {
        isSearching_ = true;
        // TODO: Focus search text input
        return;
    }

    // Check category buttons
    if (categoryFilterRect_.contains(x, y)) {
        float buttonWidth = 120.0f;
        float spacing = 12.0f;
        float startX = categoryFilterRect_.left();
        float y = categoryFilterRect_.centerY() - 20.0f;

        const std::pair<CategoryFilter, juce::String> categories[] = {
            {CategoryFilter::All, "All"},
            {CategoryFilter::Instruments, "Instruments"},
            {CategoryFilter::Effects, "Effects"},
            {CategoryFilter::Synths, "Synths"},
            {CategoryFilter::Drums, "Drums"}
        };

        for (const auto& [category, label] : categories) {
            SkRect btnRect = SkRect::MakeXYWH(startX, y, buttonWidth, 36.0f);
            if (btnRect.contains(x, y)) {
                selectCategory(category);
                return;
            }
            startX += buttonWidth + spacing;
        }
    }

    // Check plugin list items
    if (listRect_.contains(x, y)) {
        float itemY = listRect_.top() + 8.0f;

        for (size_t i = 0; i < filteredItems.size(); ++i) {
            float itemHeight = kItemHeight;
            SkRect itemBounds = SkRect::MakeXYWH(listRect_.left() + 8.0f, itemY,
                                                listRect_.width() - 24.0f, itemHeight);

            if (itemBounds.contains(x, y)) {
                selectedIndex_ = static_cast<int>(i);
                if (e.getNumberOfClicks() == 2) {
                    loadPluginAtIndex(static_cast<int>(i));
                }
                markDirty();
                return;
            }

            itemY += itemHeight + 4.0f;
        }
    }

    // Check load button
    if (footerRect_.contains(x, y)) {
        float btnWidth = 120.0f;
        float btnHeight = 36.0f;
        SkRect loadBtn = SkRect::MakeXYWH(footerRect_.right() - btnWidth - 16,
                                          footerRect_.centerY() - (btnHeight/2),
                                          btnWidth, btnHeight);

        if (loadBtn.contains(x, y) && selectedIndex_ >= 0) {
            loadPluginAtIndex(selectedIndex_);
        }
    }
}

void SkiaPluginBrowser::mouseMove(const juce::MouseEvent& e) {
    float x = (float)e.getPosition().x;
    float y = (float)e.getPosition().y;

    int previousHovered = hoveredIndex_;
    hoveredIndex_ = -1;

    // Check plugin list hover
    if (listRect_.contains(x, y)) {
        float itemY = listRect_.top() + 8.0f;

        for (size_t i = 0; i < filteredItems.size(); ++i) {
            float itemHeight = kItemHeight;
            SkRect itemBounds = SkRect::MakeXYWH(listRect_.left() + 8.0f, itemY,
                                                listRect_.width() - 24.0f, itemHeight);

            if (itemBounds.contains(x, y)) {
                hoveredIndex_ = static_cast<int>(i);
                break;
            }

            itemY += itemHeight + 4.0f;
        }
    }

    // Check category buttons hover
    if (categoryFilterRect_.contains(x, y)) {
        float buttonWidth = 120.0f;
        float spacing = 12.0f;
        float startX = categoryFilterRect_.left();
        float catY = categoryFilterRect_.centerY() - 20.0f;

        const std::pair<CategoryFilter, juce::String> categories[] = {
            {CategoryFilter::All, "All"},
            {CategoryFilter::Instruments, "Instruments"},
            {CategoryFilter::Effects, "Effects"},
            {CategoryFilter::Synths, "Synths"},
            {CategoryFilter::Drums, "Drums"}
        };

        for (const auto& [category, label] : categories) {
            SkRect btnRect = SkRect::MakeXYWH(startX, catY, buttonWidth, 36.0f);
            if (btnRect.contains(x, y)) {
                hoveredIndex_ = (category - CategoryFilter::All) + 1000; // Special range
                break;
            }
            startX += buttonWidth + spacing;
        }
    }

    if (previousHovered != hoveredIndex_) {
        markDirty();
    }
}

void SkiaPluginBrowser::mouseExit(const juce::MouseEvent& e) {
    hoveredIndex_ = -1;
    markDirty();
}

bool SkiaPluginBrowser::keyPressed(const juce::KeyPress& key, juce::Component* origin) {
    if (key == juce::KeyPress::escapeKey) {
        if (onClose) onClose();
        return true;
    }

    if (key == juce::KeyPress::returnKey) {
        if (selectedIndex_ >= 0) {
            loadPluginAtIndex(selectedIndex_);
            return true;
        }
    }

    // Search handling
    if (key.isKeyCode(juce::KeyPress::backspaceKey) && searchText_.isNotEmpty()) {
        searchText_ = searchText_.dropLastCharacters(1);
        updateFilteredList();
        markDirty();
        return true;
    }

    // Handle text input for search
    if (key.isKeyCurrentlyDown(juce::KeyPress::spaceKey) ||
        (key.isKeyCode('A') && key.getModifiers().isShiftDown())) {
        // TODO: Implement proper text input
        return true;
    }

    return false;
}

//==============================================================================
// Helper Methods
//==============================================================================

void SkiaPluginBrowser::updateFilteredList() {
    filteredItems.clear();

    for (const auto& item : pluginItems) {
        bool matchesSearch = true;
        if (!searchText_.isEmpty()) {
            matchesSearch = item.name.containsIgnoreCase(searchText_) ||
                           item.manufacturer.containsIgnoreCase(searchText_) ||
                           item.description.containsIgnoreCase(searchText_);
        }

        bool matchesCategory = (selectedCategory_ == CategoryFilter::All);
        if (!matchesCategory) {
            // Simple category matching
            switch (selectedCategory_) {
                case CategoryFilter::Instruments:
                    matchesCategory = (item.category == "Instrument" || item.category == "Synth" || item.category == "Drums");
                    break;
                case CategoryFilter::Effects:
                    matchesCategory = (item.category == "Effect" || item.category == "Plugin");
                    break;
                case CategoryFilter::Synths:
                    matchesCategory = (item.category == "Synth");
                    break;
                case CategoryFilter::Drums:
                    matchesCategory = (item.category == "Drum" || item.category == "Percussion");
                    break;
                default:
                    matchesCategory = true;
            }
        }

        if (matchesSearch && matchesCategory) {
            filteredItems.push_back(item);
        }
    }

    selectedIndex_ = -1;
    markDirty();
}

void SkiaPluginBrowser::loadPluginAtIndex(int index) {
    if (index >= 0 && index < static_cast<int>(filteredItems.size())) {
        if (onLoadPlugin) {
            onLoadPlugin(true);
        }
        // TODO: Actually load the plugin
    }
}

void SkiaPluginBrowser::selectCategory(CategoryFilter filter) {
    selectedCategory_ = filter;
    updateFilteredList();
}

SkColor SkiaPluginBrowser::getCategoryColor(CategoryFilter category) const {
    switch (category) {
        case CategoryFilter::Instruments:
            return SkColorSetARGB(255, 147, 51, 234); // Purple
        case CategoryFilter::Effects:
            return SkColorSetARGB(255, 59, 130, 246); // Blue
        case CategoryFilter::Synths:
            return SkColorSetARGB(255, 16, 185, 129); // Green
        case CategoryFilter::Drums:
            return SkColorSetARGB(255, 239, 68, 68); // Red
        default:
            return SkColorSetARGB(255, 156, 163, 175); // Gray
    }
}

void SkiaPluginBrowser::initializeDemoPlugins() {
    // Create some demo plugins for testing
    PluginItem demo1;
    demo1.name = "Serum";
    demo1.manufacturer = "Xfer Records";
    demo1.category = "Synth";
    demo1.description = "Advanced wavetable synthesizer";
    pluginItems.push_back(demo1);

    PluginItem demo2;
    demo2.name = "Reverb";
    demo2.manufacturer = "Valhalla DSP";
    demo2.category = "Effect";
    demo2.description = "High quality reverb plugin";
    pluginItems.push_back(demo2);

    PluginItem demo3;
    demo3.name = "Drum Rack";
    demo3.manufacturer = "Ableton";
    demo3.category = "Drum";
    demo3.description = "Drum machine and sample player";
    pluginItems.push_back(demo3);

    PluginItem demo4;
    demo4.name = "Ozone";
    demo4.manufacturer = "iZotope";
    demo4.category = "Effect";
    demo4.description = "Mastering suite";
    pluginItems.push_back(demo4);
}

void SkiaPluginBrowser::setTargetTrack(zenith::Track* track) {
    targetTrack = track;
}

int SkiaPluginBrowser::getSelectedPluginIndex() const {
    return selectedIndex_;
}

bool SkiaPluginBrowser::loadSelectedPlugin() {
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(filteredItems.size())) {
        // TODO: Implement actual plugin loading
        return true;
    }
    return false;
}

void SkiaPluginBrowser::refresh() {
    // TODO: Actually refresh from engine's plugin list
    updateFilteredList();

    if (onRefresh) {
        onRefresh();
    }
}

//==============================================================================
// PluginBrowserWindow implementation
//==============================================================================

SkiaPluginBrowserWindow::SkiaPluginBrowserWindow(Engine& engine)
    : DocumentWindow("Plugin Browser", juce::Colours::black,
                     juce::DocumentWindow::closeButton |
                     juce::DocumentWindow::minimiseButton |
                     juce::DocumentWindow::maximiseButton,
                     true) {

    browserComponent = std::make_unique<SkiaPluginBrowser>(engine);
    setContentOwned(browserComponent.get(), true);
    setUsingNativeTitleBar(true);
    setResizable(true, true);

    setSize(800, 600);
    centreWithSize(getWidth(), getHeight());
}

SkiaPluginBrowserWindow::~SkiaPluginBrowserWindow() {
    clearContentComponent();
}

void SkiaPluginBrowserWindow::closeButtonPressed() {
    delete this;
}

} // namespace zenith::ui