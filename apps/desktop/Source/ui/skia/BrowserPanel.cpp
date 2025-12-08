/*
  ==============================================================================

    BrowserPanel.cpp
    Refactored: 2025-12-05
    Author:  Zenith DAW

    Full implementation with:
    - Async background scanning with progress bar
    - Audio preview with waveform display
    - Drag-and-drop to tracks

  ==============================================================================
*/

#include "BrowserPanel.h"
#include <cmath>

#ifdef ZENITH_USE_SKIA

namespace zenith {

BrowserPanel::BrowserPanel(BrowserModel& model)
    : model_(model)
{
    model_.addChangeListener(this);
    setSize(300, 600);
    
    // Initialize with root
    currentRoot_ = model_.getRoot();
    updateDisplayItems();
    
    // Setup scanner callbacks
    scanner_.onProgressUpdated = [this](float progress, const juce::String& msg) {
        juce::ignoreUnused(progress, msg);
        // Will trigger repaint via timer
    };
    
    scanner_.onScanComplete = [this]() {
        auto items = scanner_.getNewItems();
        for (auto& item : items)
        {
            model_.addScannedItem(item);
        }
        updateDisplayItems();
        repaint();
    };
    
    // Start timer for UI updates (30fps for smooth animations)
    startTimerHz(30);
}

BrowserPanel::~BrowserPanel()
{
    stopTimer();
    scanner_.cancelScan();
    model_.removeChangeListener(this);
}

void BrowserPanel::timerCallback()
{
    // Update progress bar if scanning
    if (scanner_.isScanning())
    {
        repaint();
    }
    
    // Update waveform playback position
    if (previewEngine_.isPlaying())
    {
        repaint();
    }
}

void BrowserPanel::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &model_)
    {
        updateDisplayItems();
        repaint();
    }
}

void BrowserPanel::updateDisplayItems()
{
    displayItems_.clear();
    
    if (searchText_.isNotEmpty())
    {
        displayItems_ = model_.search(searchText_);
    }
    else if (currentRoot_)
    {
        displayItems_ = currentRoot_->children;
    }
    
    // Sort: Folders first, then alphabetically
    std::sort(displayItems_.begin(), displayItems_.end(), 
        [](const std::shared_ptr<BrowserItem>& a, const std::shared_ptr<BrowserItem>& b)
        {
            if (a->isDirectory != b->isDirectory)
                return a->isDirectory > b->isDirectory;
            return a->name.compareNatural(b->name) < 0;
        });
}

void BrowserPanel::navigateTo(std::shared_ptr<BrowserItem> folder)
{
    if (folder && folder->isDirectory)
    {
        currentRoot_ = folder;
        searchText_ = "";
        scrollOffset_ = 0;
        selectedIndex_ = -1;
        updateDisplayItems();
        repaint();
    }
}

void BrowserPanel::navigateUp()
{
    if (currentRoot_)
    {
        auto parent = currentRoot_->parent.lock();
        if (parent)
        {
            currentRoot_ = parent;
        }
        else
        {
            auto root = model_.getRoot();
            if (currentRoot_ != root)
                currentRoot_ = root;
        }
        
        searchText_ = "";
        scrollOffset_ = 0;
        selectedIndex_ = -1;
        updateDisplayItems();
        repaint();
    }
}

void BrowserPanel::loadWaveform(const juce::File& file)
{
    if (file == waveformFile_ && !waveformData_.empty())
        return; // Already loaded
    
    waveformData_.clear();
    waveformFile_ = file;
    
    if (!file.existsAsFile())
        return;
    
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    
    if (reader == nullptr)
        return;
    
    // Sample the waveform at ~200 points
    const int numPoints = 200;
    const int64_t samplesPerPoint = reader->lengthInSamples / numPoints;
    
    if (samplesPerPoint <= 0)
        return;
    
    juce::AudioBuffer<float> buffer(1, static_cast<int>(samplesPerPoint));
    waveformData_.reserve(numPoints);
    
    for (int i = 0; i < numPoints; ++i)
    {
        int64_t startSample = i * samplesPerPoint;
        buffer.clear();
        reader->read(&buffer, 0, static_cast<int>(samplesPerPoint), startSample, true, false);
        
        float maxVal = 0.0f;
        for (int s = 0; s < buffer.getNumSamples(); ++s)
        {
            maxVal = std::max(maxVal, std::abs(buffer.getSample(0, s)));
        }
        waveformData_.push_back(maxVal);
    }
}

//==============================================================================
// Rendering
//==============================================================================

void BrowserPanel::drawSkia(SkCanvas* canvas)
{
    auto bounds = getLocalBounds();
    
    // Calculate areas
    int y = 0;
    
    // Header area
    juce::Rectangle<int> headerArea(0, y, bounds.getWidth(), headerHeight_);
    y += headerHeight_;
    
    // Filter bar
    filterBarBounds_ = juce::Rectangle<int>(0, y, bounds.getWidth(), filterBarHeight_);
    y += filterBarHeight_;
    
    // Progress bar (only visible when scanning)
    progressBarBounds_ = juce::Rectangle<int>(0, y, bounds.getWidth(), progressBarHeight_);
    y += progressBarHeight_;
    
    // Preview area (bottom)
    previewAreaBounds_ = juce::Rectangle<int>(0, bounds.getHeight() - previewHeight_, 
                                               bounds.getWidth(), previewHeight_);
    
    // List area (remaining)
    listAreaBounds_ = juce::Rectangle<int>(0, y, bounds.getWidth(), 
                                            bounds.getHeight() - y - previewHeight_);
    
    // 1. Premium Background - subtle gradient
    SkPoint bgGradPoints[2] = {{0, 0}, {0, static_cast<float>(bounds.getHeight())}};
    SkColor bgGradColors[3] = {
        SkColorSetRGB(22, 22, 28),   // Top - slightly cooler
        SkColorSetRGB(18, 18, 22),   // Middle - darkest
        SkColorSetRGB(20, 20, 25)    // Bottom
    };
    float bgPositions[3] = {0.0f, 0.5f, 1.0f};
    auto bgGradient = SkGradientShader::MakeLinear(bgGradPoints, bgGradColors, bgPositions, 3, SkTileMode::kClamp);
    
    SkPaint bgPaint;
    bgPaint.setShader(bgGradient);
    canvas->drawRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), bgPaint);
    
    // 2. Subtle side accent glow
    SkPaint accentGlow;
    SkPoint glowPoints[2] = {{0, 0}, {40, 0}};
    SkColor glowColors[2] = {SkColorSetARGB(25, 0, 200, 255), SkColorSetARGB(0, 0, 200, 255)};
    accentGlow.setShader(SkGradientShader::MakeLinear(glowPoints, glowColors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRect(SkRect::MakeWH(40, bounds.getHeight()), accentGlow);
    
    // 3. Draw components
    drawHeader(canvas);
    drawFilterBar(canvas);
    drawProgressBar(canvas);
    drawItemList(canvas);
    drawPreviewArea(canvas);
}

void BrowserPanel::drawHeader(SkCanvas* canvas)
{
    // Premium header with gradient and subtle border
    SkPoint headerGradPoints[2] = {{0, 0}, {0, static_cast<float>(headerHeight_)}};
    SkColor headerGradColors[2] = {SkColorSetRGB(38, 38, 45), SkColorSetRGB(28, 28, 32)};
    auto headerGradient = SkGradientShader::MakeLinear(headerGradPoints, headerGradColors, nullptr, 2, SkTileMode::kClamp);
    
    SkPaint headerBg;
    headerBg.setShader(headerGradient);
    canvas->drawRect(SkRect::MakeWH(getWidth(), headerHeight_), headerBg);
    
    // Bottom highlight line
    SkPaint highlightPaint;
    highlightPaint.setColor(SkColorSetARGB(30, 255, 255, 255));
    canvas->drawLine(0, 0.5f, getWidth(), 0.5f, highlightPaint);
    
    // Bottom border
    SkPaint borderPaint;
    borderPaint.setColor(SkColorSetRGB(50, 50, 58));
    canvas->drawLine(0, headerHeight_ - 0.5f, getWidth(), headerHeight_ - 0.5f, borderPaint);
    
    bool showBack = (currentRoot_ != model_.getRoot()) && searchText_.isEmpty();
    
    if (showBack)
    {
        // Enhanced back button with glow
        backButtonBounds_ = juce::Rectangle<int>(8, 10, 28, 28);
        
        // Button background with gradient
        SkPaint backBtnBg;
        backBtnBg.setColor(SkColorSetARGB(80, 255, 255, 255));
        backBtnBg.setAntiAlias(true);
        canvas->drawRoundRect(SkRect::MakeXYWH(backButtonBounds_.getX(), backButtonBounds_.getY(),
                                               backButtonBounds_.getWidth(), backButtonBounds_.getHeight()),
                              6, 6, backBtnBg);
        
        // Modern chevron icon
        SkPath chevron;
        float cx = backButtonBounds_.getCentreX();
        float cy = backButtonBounds_.getCentreY();
        chevron.moveTo(cx + 3, cy - 6);
        chevron.lineTo(cx - 4, cy);
        chevron.lineTo(cx + 3, cy + 6);
        
        SkPaint chevronPaint;
        chevronPaint.setColor(SK_ColorWHITE);
        chevronPaint.setStyle(SkPaint::kStroke_Style);
        chevronPaint.setStrokeWidth(2.5f);
        chevronPaint.setStrokeCap(SkPaint::kRound_Cap);
        chevronPaint.setAntiAlias(true);
        canvas->drawPath(chevron, chevronPaint);
        
        // Folder title - clean without glow
        SkFont titleFont;
        titleFont.setSize(15.0f);
        titleFont.setEdging(SkFont::Edging::kSubpixelAntiAlias);
        
        SkPaint textPaint;
        textPaint.setColor(SK_ColorWHITE);
        textPaint.setAntiAlias(true);
        canvas->drawString(currentRoot_->name.toStdString().c_str(), 44, 28, titleFont, textPaint);
    }
    else
    {
        backButtonBounds_ = juce::Rectangle<int>();
        
        // Premium Search Box with inner glow
        searchBoxBounds_ = juce::Rectangle<int>(12, 9, getWidth() - 60, searchBoxHeight_ - 2);
        
        // Search box gradient background
        SkRect searchRect = SkRect::MakeXYWH(searchBoxBounds_.getX(), searchBoxBounds_.getY(), 
                                              searchBoxBounds_.getWidth(), searchBoxBounds_.getHeight());
        
        SkPoint searchGradPoints[2] = {{0, searchRect.fTop}, {0, searchRect.fBottom}};
        SkColor searchGradColors[2] = {SkColorSetRGB(42, 42, 48), SkColorSetRGB(35, 35, 40)};
        auto searchGradient = SkGradientShader::MakeLinear(searchGradPoints, searchGradColors, nullptr, 2, SkTileMode::kClamp);
        
        SkPaint searchBgPaint;
        searchBgPaint.setShader(searchGradient);
        searchBgPaint.setAntiAlias(true);
        canvas->drawRoundRect(searchRect, 8.0f, 8.0f, searchBgPaint);
        
        // Inner shadow
        SkPaint innerShadow;
        innerShadow.setColor(SkColorSetARGB(40, 0, 0, 0));
        innerShadow.setStyle(SkPaint::kStroke_Style);
        innerShadow.setStrokeWidth(1.0f);
        innerShadow.setAntiAlias(true);
        canvas->drawRoundRect(searchRect.makeInset(0.5f, 0.5f), 7.5f, 7.5f, innerShadow);
        
        // Search icon - magnifying glass
        float iconX = searchBoxBounds_.getX() + 18;
        float iconY = searchBoxBounds_.getCentreY();
        
        SkPaint iconPaint;
        iconPaint.setColor(SkColorSetARGB(150, 255, 255, 255));
        iconPaint.setStyle(SkPaint::kStroke_Style);
        iconPaint.setStrokeWidth(1.8f);
        iconPaint.setAntiAlias(true);
        canvas->drawCircle(iconX, iconY - 1, 5, iconPaint);
        canvas->drawLine(iconX + 4, iconY + 3, iconX + 7, iconY + 6, iconPaint);
        
        // Search text
        SkFont searchFont;
        searchFont.setSize(13.0f);
        searchFont.setEdging(SkFont::Edging::kSubpixelAntiAlias);
        
        SkPaint textPaint;
        textPaint.setColor(searchText_.isEmpty() ? SkColorSetARGB(90, 255, 255, 255) : SkColorSetRGB(230, 230, 240));
        textPaint.setAntiAlias(true);
        
        juce::String displayText = searchText_.isEmpty() ? "Search samples, plugins, presets..." : searchText_;
        canvas->drawString(displayText.toStdString().c_str(), 
                           searchBoxBounds_.getX() + 32, searchBoxBounds_.getCentreY() + 4, 
                           searchFont, textPaint);
        
        // Add Folder button - premium style
        addFolderButtonBounds_ = juce::Rectangle<int>(getWidth() - 40, 9, 32, searchBoxHeight_ - 2);
        
        // Button gradient
        SkRect addBtnRect = SkRect::MakeXYWH(addFolderButtonBounds_.getX(), addFolderButtonBounds_.getY(),
                                              addFolderButtonBounds_.getWidth(), addFolderButtonBounds_.getHeight());
        
        SkPoint btnGradPoints[2] = {{0, addBtnRect.fTop}, {0, addBtnRect.fBottom}};
        SkColor btnGradColors[2] = {SkColorSetRGB(55, 75, 55), SkColorSetRGB(40, 60, 40)};
        auto btnGradient = SkGradientShader::MakeLinear(btnGradPoints, btnGradColors, nullptr, 2, SkTileMode::kClamp);
        
        SkPaint addBtnPaint;
        addBtnPaint.setShader(btnGradient);
        addBtnPaint.setAntiAlias(true);
        canvas->drawRoundRect(addBtnRect, 6, 6, addBtnPaint);
        
        // Button border
        SkPaint btnBorder;
        btnBorder.setColor(SkColorSetRGB(80, 120, 80));
        btnBorder.setStyle(SkPaint::kStroke_Style);
        btnBorder.setStrokeWidth(1.0f);
        btnBorder.setAntiAlias(true);
        canvas->drawRoundRect(addBtnRect, 6, 6, btnBorder);
        
        // Plus icon with glow
        float cx = addFolderButtonBounds_.getCentreX();
        float cy = addFolderButtonBounds_.getCentreY();
        
        // Glow
        SkPaint glowPaint;
        glowPaint.setColor(SkColorSetARGB(80, 100, 255, 100));
        glowPaint.setStyle(SkPaint::kStroke_Style);
        glowPaint.setStrokeWidth(3.5f);
        glowPaint.setAntiAlias(true);
        canvas->drawLine(cx - 6, cy, cx + 6, cy, glowPaint);
        canvas->drawLine(cx, cy - 6, cx, cy + 6, glowPaint);
        
        // Main plus
        SkPaint plusPaint;
        plusPaint.setColor(SkColorSetRGB(150, 255, 150));
        plusPaint.setStyle(SkPaint::kStroke_Style);
        plusPaint.setStrokeWidth(2.2f);
        plusPaint.setStrokeCap(SkPaint::kRound_Cap);
        plusPaint.setAntiAlias(true);
        canvas->drawLine(cx - 5, cy, cx + 5, cy, plusPaint);
        canvas->drawLine(cx, cy - 5, cx, cy + 5, plusPaint);
    }
}

void BrowserPanel::drawProgressBar(SkCanvas* canvas)
{
    if (!scanner_.isScanning())
        return;
    
    float progress = scanner_.getProgress();
    float w = progressBarBounds_.getWidth();
    float h = progressBarBounds_.getHeight();
    float y = progressBarBounds_.getY();
    
    // Background with subtle gradient
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetRGB(25, 25, 30));
    canvas->drawRect(SkRect::MakeXYWH(0, y, w, h), bgPaint);
    
    // Progress fill with multi-color gradient
    float progressWidth = w * progress;
    SkPoint gradientPoints[2] = {{0, 0}, {progressWidth, 0}};
    SkColor gradientColors[3] = {
        SkColorSetRGB(0, 150, 255),   // Cyan
        SkColorSetRGB(0, 255, 200),   // Teal
        SkColorSetRGB(100, 255, 150)  // Green
    };
    float positions[3] = {0.0f, 0.5f, 1.0f};
    
    auto gradient = SkGradientShader::MakeLinear(gradientPoints, gradientColors, positions, 3, SkTileMode::kClamp);
    SkPaint progressPaint;
    progressPaint.setShader(gradient);
    canvas->drawRect(SkRect::MakeXYWH(0, y, progressWidth, h), progressPaint);
    
    // Subtle highlight at leading edge (not animated - professional)
    SkPaint edgePaint;
    edgePaint.setColor(SkColorSetARGB(100, 255, 255, 255));
    canvas->drawLine(progressWidth - 1, y, progressWidth - 1, y + h, edgePaint);
    
    // Top highlight
    SkPaint highlightPaint;
    highlightPaint.setColor(SkColorSetARGB(40, 255, 255, 255));
    canvas->drawLine(0, y, progressWidth, y, highlightPaint);
}

void BrowserPanel::drawItemList(SkCanvas* canvas)
{
    canvas->save();
    canvas->clipRect(SkRect::MakeXYWH(listAreaBounds_.getX(), listAreaBounds_.getY(),
                                       listAreaBounds_.getWidth(), listAreaBounds_.getHeight()));
    
    int maxVisible = listAreaBounds_.getHeight() / itemHeight_;
    int visibleStart = scrollOffset_;
    int visibleEnd = juce::jmin(visibleStart + maxVisible + 1, (int)displayItems_.size());
    
    for (int i = visibleStart; i < visibleEnd; ++i)
    {
        int itemY = listAreaBounds_.getY() + (i - scrollOffset_) * itemHeight_;
        auto itemBounds = juce::Rectangle<int>(0, itemY, getWidth(), itemHeight_);
        
        drawBrowserItem(canvas, i, itemBounds);
    }
    
    // Scrollbar
    if (displayItems_.size() > maxVisible)
    {
        float ratio = (float)maxVisible / (float)displayItems_.size();
        float scrollbarHeight = ratio * listAreaBounds_.getHeight();
        float scrollbarY = listAreaBounds_.getY() + 
                          ((float)scrollOffset_ / (float)displayItems_.size()) * listAreaBounds_.getHeight();
        
        SkPaint scrollPaint;
        scrollPaint.setColor(SkColorSetARGB(60, 255, 255, 255));
        canvas->drawRoundRect(SkRect::MakeXYWH(getWidth() - 6, scrollbarY, 4, scrollbarHeight), 2, 2, scrollPaint);
    }
    
    canvas->restore();
}

void BrowserPanel::drawBrowserItem(SkCanvas* canvas, int index, const juce::Rectangle<int>& bounds)
{
    auto item = displayItems_[index];
    float x = static_cast<float>(bounds.getX());
    float y = static_cast<float>(bounds.getY());
    float w = static_cast<float>(bounds.getWidth());
    float h = static_cast<float>(bounds.getHeight());
    
    // Selection / Hover Background with premium styling
    if (index == selectedIndex_)
    {
        // Selection gradient
        SkPoint selGradPoints[2] = {{x, 0}, {x + w, 0}};
        SkColor selGradColors[2] = {SkColorSetARGB(60, 0, 200, 255), SkColorSetARGB(20, 0, 200, 255)};
        auto selGradient = SkGradientShader::MakeLinear(selGradPoints, selGradColors, nullptr, 2, SkTileMode::kClamp);
        
        SkPaint selPaint;
        selPaint.setShader(selGradient);
        canvas->drawRect(SkRect::MakeXYWH(x, y, w, h), selPaint);
        
        // Glowing selection indicator bar
        SkPaint barGlowPaint;
        barGlowPaint.setColor(SkColorSetARGB(80, 0, 200, 255));
        canvas->drawRect(SkRect::MakeXYWH(0, y, 6, h), barGlowPaint);
        
        SkPaint barPaint;
        barPaint.setColor(SkColorSetRGB(0, 220, 255));
        canvas->drawRect(SkRect::MakeXYWH(0, y + 2, 3, h - 4), barPaint);
        
        // Top highlight on selected items
        SkPaint topHighlight;
        topHighlight.setColor(SkColorSetARGB(30, 255, 255, 255));
        canvas->drawLine(x + 10, y, x + w - 10, y, topHighlight);
    }
    else if (index == hoverIndex_)
    {
        // Subtle hover gradient
        SkPoint hovGradPoints[2] = {{x, 0}, {x + w, 0}};
        SkColor hovGradColors[2] = {SkColorSetARGB(35, 255, 255, 255), SkColorSetARGB(5, 255, 255, 255)};
        auto hovGradient = SkGradientShader::MakeLinear(hovGradPoints, hovGradColors, nullptr, 2, SkTileMode::kClamp);
        
        SkPaint hovPaint;
        hovPaint.setShader(hovGradient);
        canvas->drawRoundRect(SkRect::MakeXYWH(x + 4, y + 1, w - 8, h - 2), 4, 4, hovPaint);
    }
    
    // Icon (no glow - clean professional look)
    drawIcon(canvas, item->type, x + 18, bounds.getCentreY(), 14);
    
    // Text with subpixel antialiasing
    SkFont font;
    font.setSize(13.0f);
    font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
    
    SkPaint textPaint;
    if (index == selectedIndex_)
        textPaint.setColor(SkColorSetRGB(150, 235, 255)); // Brighter cyan
    else
        textPaint.setColor(SkColorSetRGB(220, 220, 225));
    textPaint.setAntiAlias(true);
    
    canvas->drawString(item->name.toStdString().c_str(), 
                       x + 38, bounds.getCentreY() + 4, 
                       font, textPaint);
    
    // Folder arrow indicator - modern chevron
    if (item->isDirectory)
    {
        float arrowX = w - 20;
        float arrowY = bounds.getCentreY();
        
        SkPath chevron;
        chevron.moveTo(arrowX - 2, arrowY - 5);
        chevron.lineTo(arrowX + 4, arrowY);
        chevron.lineTo(arrowX - 2, arrowY + 5);
        
        SkPaint chevronPaint;
        chevronPaint.setColor(index == hoverIndex_ ? SkColorSetARGB(180, 255, 255, 255) : SkColorSetARGB(80, 255, 255, 255));
        chevronPaint.setStyle(SkPaint::kStroke_Style);
        chevronPaint.setStrokeWidth(1.8f);
        chevronPaint.setStrokeCap(SkPaint::kRound_Cap);
        chevronPaint.setAntiAlias(true);
        canvas->drawPath(chevron, chevronPaint);
    }
    
    // Metadata (duration for audio) with pill background
    if (item->type == BrowserItemType::AudioFile && item->metadata.duration > 0)
    {
        int seconds = static_cast<int>(item->metadata.duration);
        juce::String durStr = juce::String::formatted("%d:%02d", seconds / 60, seconds % 60);
        
        // Background pill
        SkPaint pillPaint;
        pillPaint.setColor(SkColorSetARGB(40, 0, 0, 0));
        pillPaint.setAntiAlias(true);
        canvas->drawRoundRect(SkRect::MakeXYWH(w - 55, bounds.getCentreY() - 9, 45, 18), 9, 9, pillPaint);
        
        SkFont metaFont;
        metaFont.setSize(10.0f);
        metaFont.setEdging(SkFont::Edging::kSubpixelAntiAlias);
        SkPaint metaPaint;
        metaPaint.setColor(SkColorSetARGB(140, 255, 255, 255));
        metaPaint.setAntiAlias(true);
        
        canvas->drawString(durStr.toStdString().c_str(), w - 48, bounds.getCentreY() + 3, metaFont, metaPaint);
    }
}

void BrowserPanel::drawPreviewArea(SkCanvas* canvas)
{
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetRGB(30, 30, 35));
    canvas->drawRect(SkRect::MakeXYWH(previewAreaBounds_.getX(), previewAreaBounds_.getY(),
                                       previewAreaBounds_.getWidth(), previewAreaBounds_.getHeight()), bgPaint);
    
    // Top border
    SkPaint borderPaint;
    borderPaint.setColor(SkColorSetRGB(50, 50, 55));
    canvas->drawLine(0, previewAreaBounds_.getY(), getWidth(), previewAreaBounds_.getY(), borderPaint);
    
    // Layout preview controls
    int padding = 8;
    int buttonSize = 28;
    int x = padding;
    int y = previewAreaBounds_.getY() + padding;
    
    // Play/Pause button
    playButtonBounds_ = juce::Rectangle<int>(x, y, buttonSize, buttonSize);
    drawButton(canvas, playButtonBounds_, previewEngine_.isPlaying() ? "||" : ">", 
               previewEngine_.isPlaying(), false);
    x += buttonSize + 4;
    
    // Stop button
    stopButtonBounds_ = juce::Rectangle<int>(x, y, buttonSize, buttonSize);
    drawButton(canvas, stopButtonBounds_, "[]", false, false);
    x += buttonSize + 4;
    
    // Loop button
    loopButtonBounds_ = juce::Rectangle<int>(x, y, buttonSize, buttonSize);
    drawButton(canvas, loopButtonBounds_, "O", previewEngine_.isLooping(), false);
    x += buttonSize + 8;
    
    // Auto-play toggle
    autoPlayButtonBounds_ = juce::Rectangle<int>(x, y, buttonSize + 20, buttonSize);
    drawButton(canvas, autoPlayButtonBounds_, "AUTO", previewEngine_.isAutoPlayEnabled(), false);
    
    // Waveform area
    waveformBounds_ = juce::Rectangle<int>(padding, y + buttonSize + 4, 
                                           previewAreaBounds_.getWidth() - padding * 2, 
                                           previewAreaBounds_.getHeight() - buttonSize - padding * 2 - 4);
    
    if (waveformBounds_.getHeight() > 10)
    {
        drawWaveform(canvas, SkRect::MakeXYWH(waveformBounds_.getX(), waveformBounds_.getY(),
                                               waveformBounds_.getWidth(), waveformBounds_.getHeight()));
    }
    
    // File name
    if (previewEngine_.isLoaded())
    {
        SkFont nameFont;
        nameFont.setSize(11.0f);
        nameFont.setEdging(SkFont::Edging::kAntiAlias);
        SkPaint namePaint;
        namePaint.setColor(SK_ColorWHITE);
        namePaint.setAntiAlias(true);
        
        juce::String fileName = previewEngine_.getCurrentFile().getFileNameWithoutExtension();
        if (fileName.length() > 25)
            fileName = fileName.substring(0, 23) + "...";
        
        canvas->drawString(fileName.toStdString().c_str(), 
                          autoPlayButtonBounds_.getRight() + 10, y + 18, nameFont, namePaint);
    }
}

void BrowserPanel::drawWaveform(SkCanvas* canvas, const SkRect& bounds)
{
    // Premium background with inner shadow
    SkPaint bgPaint;
    SkPoint bgGradPoints[2] = {{0, bounds.fTop}, {0, bounds.fBottom}};
    SkColor bgGradColors[2] = {SkColorSetRGB(15, 15, 20), SkColorSetRGB(25, 25, 30)};
    bgPaint.setShader(SkGradientShader::MakeLinear(bgGradPoints, bgGradColors, nullptr, 2, SkTileMode::kClamp));
    canvas->drawRoundRect(bounds, 6, 6, bgPaint);
    
    // Inner border
    SkPaint borderPaint;
    borderPaint.setColor(SkColorSetARGB(50, 0, 150, 255));
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setAntiAlias(true);
    canvas->drawRoundRect(bounds.makeInset(0.5f, 0.5f), 5.5f, 5.5f, borderPaint);
    
    if (waveformData_.empty())
    {
        // No waveform - show placeholder with icon
        SkFont font;
        font.setSize(12.0f);
        font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
        SkPaint textPaint;
        textPaint.setColor(SkColorSetARGB(60, 255, 255, 255));
        textPaint.setAntiAlias(true);
        canvas->drawString("Select audio to preview", bounds.centerX() - 65, bounds.centerY() + 4, font, textPaint);
        
        // Waveform icon placeholder
        SkPaint iconPaint;
        iconPaint.setColor(SkColorSetARGB(30, 0, 200, 255));
        iconPaint.setAntiAlias(true);
        for (int i = -3; i <= 3; ++i) {
            float barHeight = (3 - std::abs(i)) * 4.0f + 4.0f;
            canvas->drawRoundRect(SkRect::MakeXYWH(bounds.centerX() + i * 8 - 2, bounds.centerY() - barHeight/2, 4, barHeight), 
                                  2, 2, iconPaint);
        }
        return;
    }
    
    // Draw waveform
    float centerY = bounds.centerY();
    float maxHeight = bounds.height() * 0.42f;
    float pointWidth = bounds.width() / waveformData_.size();
    
    SkPath waveformPath;
    bool pathStarted = false;
    
    for (size_t i = 0; i < waveformData_.size(); ++i)
    {
        float x = bounds.x() + i * pointWidth;
        float height = waveformData_[i] * maxHeight;
        
        if (!pathStarted)
        {
            waveformPath.moveTo(x, centerY - height);
            pathStarted = true;
        }
        else
        {
            waveformPath.lineTo(x, centerY - height);
        }
    }
    
    // Mirror bottom
    for (int i = static_cast<int>(waveformData_.size()) - 1; i >= 0; --i)
    {
        float x = bounds.x() + i * pointWidth;
        float height = waveformData_[i] * maxHeight;
        waveformPath.lineTo(x, centerY + height);
    }
    waveformPath.close();
    
    // Main waveform gradient fill (no outer glow - clean look)
    SkPoint gradientPoints[2] = {{0, bounds.fTop}, {0, bounds.fBottom}};
    SkColor gradientColors[3] = {
        SkColorSetARGB(200, 0, 220, 255),   // Top - bright cyan
        SkColorSetARGB(150, 0, 180, 220),   // Middle
        SkColorSetARGB(100, 0, 100, 180)    // Bottom - deeper blue
    };
    float positions[3] = {0.0f, 0.5f, 1.0f};
    auto gradient = SkGradientShader::MakeLinear(gradientPoints, gradientColors, positions, 3, SkTileMode::kClamp);
    
    SkPaint waveformPaint;
    waveformPaint.setShader(gradient);
    waveformPaint.setAntiAlias(true);
    canvas->drawPath(waveformPath, waveformPaint);
    
    // Waveform outline
    SkPaint outlinePaint;
    outlinePaint.setColor(SkColorSetARGB(100, 0, 255, 255));
    outlinePaint.setStyle(SkPaint::kStroke_Style);
    outlinePaint.setStrokeWidth(1.0f);
    outlinePaint.setAntiAlias(true);
    canvas->drawPath(waveformPath, outlinePaint);
    
    // Center line
    SkPaint centerLinePaint;
    centerLinePaint.setColor(SkColorSetARGB(30, 255, 255, 255));
    canvas->drawLine(bounds.fLeft + 4, centerY, bounds.fRight - 4, centerY, centerLinePaint);
    
    // Playback position indicator (clean, no glow)
    if (previewEngine_.isLoaded())
    {
        float posX = bounds.x() + previewEngine_.getPlaybackPosition() * bounds.width();
        
        // Main line
        SkPaint posPaint;
        posPaint.setColor(SkColorSetRGB(255, 255, 255)); // White for visibility
        posPaint.setStrokeWidth(1.5f);
        posPaint.setAntiAlias(true);
        canvas->drawLine(posX, bounds.y() + 4, posX, bounds.y() + bounds.height() - 4, posPaint);
        
        // Playhead triangle (subtle)
        SkPath playhead;
        playhead.moveTo(posX - 3, bounds.y() + 1);
        playhead.lineTo(posX + 3, bounds.y() + 1);
        playhead.lineTo(posX, bounds.y() + 5);
        playhead.close();
        
        SkPaint playheadPaint;
        playheadPaint.setColor(SkColorSetRGB(255, 255, 255));
        playheadPaint.setAntiAlias(true);
        canvas->drawPath(playhead, playheadPaint);
    }
}

void BrowserPanel::drawIcon(SkCanvas* canvas, BrowserItemType type, float x, float y, float size)
{
    SkPaint iconPaint;
    iconPaint.setStyle(SkPaint::kFill_Style);
    iconPaint.setAntiAlias(true);
    
    switch (type)
    {
        case BrowserItemType::Folder:
        {
            iconPaint.setColor(SkColorSetRGB(220, 180, 80));
            // Folder shape
            SkPath folder;
            folder.moveTo(x - 6, y - 4);
            folder.lineTo(x - 2, y - 4);
            folder.lineTo(x, y - 6);
            folder.lineTo(x + 6, y - 6);
            folder.lineTo(x + 6, y + 4);
            folder.lineTo(x - 6, y + 4);
            folder.close();
            canvas->drawPath(folder, iconPaint);
            break;
        }
        case BrowserItemType::AudioFile:
            iconPaint.setColor(SkColorSetRGB(100, 220, 100));
            // Waveform icon
            canvas->drawRect(SkRect::MakeXYWH(x - 6, y - 2, 3, 4), iconPaint);
            canvas->drawRect(SkRect::MakeXYWH(x - 2, y - 5, 3, 10), iconPaint);
            canvas->drawRect(SkRect::MakeXYWH(x + 2, y - 3, 3, 6), iconPaint);
            break;
            
        case BrowserItemType::MidiFile:
            iconPaint.setColor(SkColorSetRGB(220, 100, 220));
            // Piano keys icon
            canvas->drawRect(SkRect::MakeXYWH(x - 5, y - 4, 10, 8), iconPaint);
            iconPaint.setColor(SkColorSetRGB(40, 40, 45));
            canvas->drawRect(SkRect::MakeXYWH(x - 3, y - 4, 2, 5), iconPaint);
            canvas->drawRect(SkRect::MakeXYWH(x + 1, y - 4, 2, 5), iconPaint);
            break;
            
        case BrowserItemType::Plugin:
        case BrowserItemType::Instrument:
            iconPaint.setColor(SkColorSetRGB(100, 150, 255));
            canvas->drawRoundRect(SkRect::MakeXYWH(x - 5, y - 5, 10, 10), 2, 2, iconPaint);
            iconPaint.setColor(SkColorSetRGB(200, 220, 255));
            canvas->drawCircle(x, y, 3, iconPaint);
            break;
            
        case BrowserItemType::Preset:
            iconPaint.setColor(SkColorSetRGB(200, 100, 200));
            canvas->drawCircle(x, y, size * 0.4f, iconPaint);
            break;
            
        default:
            iconPaint.setColor(SK_ColorGRAY);
            canvas->drawCircle(x, y, size * 0.3f, iconPaint);
            break;
    }
}

void BrowserPanel::drawButton(SkCanvas* canvas, const juce::Rectangle<int>& bounds, 
                               const juce::String& icon, bool active, bool hovered)
{
    SkPaint bgPaint;
    if (active)
        bgPaint.setColor(SkColorSetRGB(0, 150, 200));
    else if (hovered)
        bgPaint.setColor(SkColorSetRGB(60, 60, 70));
    else
        bgPaint.setColor(SkColorSetRGB(45, 45, 55));
    
    canvas->drawRoundRect(SkRect::MakeXYWH(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight()),
                          4, 4, bgPaint);
    
    SkFont font;
    font.setSize(11.0f);
    font.setEdging(SkFont::Edging::kAntiAlias);
    
    SkPaint textPaint;
    textPaint.setColor(active ? SK_ColorWHITE : SkColorSetARGB(180, 255, 255, 255));
    textPaint.setAntiAlias(true);
    
    // Center text
    SkRect textBounds;
    font.measureText(icon.toStdString().c_str(), icon.length(), SkTextEncoding::kUTF8, &textBounds);
    float textX = bounds.getCentreX() - textBounds.width() / 2;
    float textY = bounds.getCentreY() + textBounds.height() / 2 - 1;
    
    canvas->drawString(icon.toStdString().c_str(), textX, textY, font, textPaint);
}

//==============================================================================
// Events
//==============================================================================

void BrowserPanel::mouseDown(const juce::MouseEvent& e)
{
    dragStartPos_ = e.getPosition();
    isDragging_ = false;
    
    // Check preview controls
    if (previewAreaBounds_.contains(e.getPosition()))
    {
        if (playButtonBounds_.contains(e.getPosition()))
        {
            previewEngine_.togglePlayback();
            repaint();
            return;
        }
        if (stopButtonBounds_.contains(e.getPosition()))
        {
            previewEngine_.stop();
            repaint();
            return;
        }
        if (loopButtonBounds_.contains(e.getPosition()))
        {
            previewEngine_.setLooping(!previewEngine_.isLooping());
            repaint();
            return;
        }
        if (autoPlayButtonBounds_.contains(e.getPosition()))
        {
            previewEngine_.setAutoPlayEnabled(!previewEngine_.isAutoPlayEnabled());
            repaint();
            return;
        }
        return;
    }
    
    // Check filter tabs
    if (filterBarBounds_.contains(e.getPosition()))
    {
        if (filterAllBounds_.contains(e.getPosition()))
        {
            model_.clearFilter();
            updateDisplayItems();
            repaint();
            return;
        }
        if (filterAudioBounds_.contains(e.getPosition()))
        {
            model_.setActiveFilter(BrowserItemType::AudioFile);
            updateDisplayItems();
            repaint();
            return;
        }
        if (filterMidiBounds_.contains(e.getPosition()))
        {
            model_.setActiveFilter(BrowserItemType::MidiFile);
            updateDisplayItems();
            repaint();
            return;
        }
        if (filterPluginBounds_.contains(e.getPosition()))
        {
            model_.setActiveFilter(BrowserItemType::Plugin);
            updateDisplayItems();
            repaint();
            return;
        }
    }
    
    // Check back button
    if (backButtonBounds_.contains(e.getPosition()) && !backButtonBounds_.isEmpty())
    {
        navigateUp();
        return;
    }
    
    // Check add folder button
    if (addFolderButtonBounds_.contains(e.getPosition()) && !addFolderButtonBounds_.isEmpty())
    {
        showAddFolderDialog();
        return;
    }
    
    // Check item list
    if (listAreaBounds_.contains(e.getPosition()))
    {
        int clickedIndex = getItemIndexAt(e.y);
        
        if (clickedIndex >= 0 && clickedIndex < (int)displayItems_.size())
        {
            selectedIndex_ = clickedIndex;
            
            // Right-click for context menu
            if (e.mods.isRightButtonDown())
            {
                showContextMenu(clickedIndex, e.getScreenPosition());
                return;
            }
            
            // Load preview for audio files
            auto item = displayItems_[clickedIndex];
            if (item->type == BrowserItemType::AudioFile)
            {
                juce::File file(item->id);
                loadWaveform(file);
                previewEngine_.loadFile(file, previewEngine_.isAutoPlayEnabled());
            }
            
            repaint();
        }
    }
}

void BrowserPanel::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging_)
    {
        int distance = e.getDistanceFromDragStart();
        if (distance > dragThreshold_ && selectedIndex_ >= 0)
        {
            isDragging_ = true;
            startItemDrag(selectedIndex_);
        }
    }
}

void BrowserPanel::startItemDrag(int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= (int)displayItems_.size())
        return;
    
    auto item = displayItems_[itemIndex];
    
    // Don't allow dragging folders
    if (item->isDirectory)
        return;
    
    BrowserDragSource::startDrag(this, item);
}

void BrowserPanel::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (listAreaBounds_.contains(e.getPosition()))
    {
        int clickedIndex = getItemIndexAt(e.y);
        
        if (clickedIndex >= 0 && clickedIndex < (int)displayItems_.size())
        {
            auto item = displayItems_[clickedIndex];
            
            if (item->isDirectory)
            {
                navigateTo(item);
            }
            else
            {
                if (onItemDoubleClicked) onItemDoubleClicked(item);
            }
        }
    }
}

void BrowserPanel::mouseMove(const juce::MouseEvent& e)
{
    if (listAreaBounds_.contains(e.getPosition()))
    {
        int newHoverIndex = getItemIndexAt(e.y);
        
        if (newHoverIndex != hoverIndex_)
        {
            hoverIndex_ = newHoverIndex;
            repaint();
        }
    }
    else if (hoverIndex_ != -1)
    {
        hoverIndex_ = -1;
        repaint();
    }
}

void BrowserPanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (displayItems_.empty()) return;
    
    int delta = (wheel.deltaY > 0) ? -3 : 3;
    int maxScroll = std::max(0, (int)displayItems_.size() - (listAreaBounds_.getHeight() / itemHeight_));
    scrollOffset_ = juce::jlimit(0, maxScroll, scrollOffset_ + delta);
    repaint();
}

bool BrowserPanel::keyPressed(const juce::KeyPress& key)
{
    if (key.isKeyCode(juce::KeyPress::backspaceKey))
    {
        if (searchText_.isNotEmpty())
            setSearchText(searchText_.dropLastCharacters(1));
        else
            navigateUp();
        return true;
    }
    else if (key.isKeyCode(juce::KeyPress::returnKey))
    {
        if (selectedIndex_ >= 0 && selectedIndex_ < (int)displayItems_.size())
        {
             auto item = displayItems_[selectedIndex_];
             if (item->isDirectory) navigateTo(item);
        }
        return true;
    }
    else if (key.isKeyCode(juce::KeyPress::upKey))
    {
        if (selectedIndex_ > 0)
        {
            selectedIndex_--;
            if (selectedIndex_ < scrollOffset_)
                scrollOffset_ = selectedIndex_;
            repaint();
        }
        return true;
    }
    else if (key.isKeyCode(juce::KeyPress::downKey))
    {
        if (selectedIndex_ < (int)displayItems_.size() - 1)
        {
            selectedIndex_++;
            int maxVisible = listAreaBounds_.getHeight() / itemHeight_;
            if (selectedIndex_ >= scrollOffset_ + maxVisible)
                scrollOffset_ = selectedIndex_ - maxVisible + 1;
            repaint();
        }
        return true;
    }
    else if (key.isKeyCode(juce::KeyPress::spaceKey))
    {
        previewEngine_.togglePlayback();
        repaint();
        return true;
    }
    else if (key.getTextCharacter() >= 32 && key.getTextCharacter() < 127)
    {
        setSearchText(searchText_ + juce::String::charToString(key.getTextCharacter()));
        return true;
    }
    
    return false;
}

void BrowserPanel::setSearchText(const juce::String& text)
{
    searchText_ = text;
    scrollOffset_ = 0;
    selectedIndex_ = -1;
    updateDisplayItems();
    repaint();
}

int BrowserPanel::getItemIndexAt(int y) const
{
    if (!listAreaBounds_.contains(0, y))
        return -1;
    
    int relativeY = y - listAreaBounds_.getY();
    return (relativeY / itemHeight_) + scrollOffset_;
}

bool BrowserPanel::isInPreviewArea(int y) const
{
    return previewAreaBounds_.contains(0, y);
}

void BrowserPanel::resized()
{
}

void BrowserPanel::showAddFolderDialog()
{
    // Use async file chooser (JUCE 8 style)
    fileChooser_ = std::make_unique<juce::FileChooser>(
        "Select Sample Library Folder",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "",
        true
    );
    
    const int flags = juce::FileBrowserComponent::openMode | 
                      juce::FileBrowserComponent::canSelectDirectories;
    
    fileChooser_->launchAsync(flags, [this](const juce::FileChooser& chooser)
    {
        auto result = chooser.getResult();
        
        if (result.isDirectory())
        {
            DBG("BrowserPanel: Adding folder: " + result.getFullPathName());
            
            // Get current paths and add new one
            juce::StringArray paths = model_.getUserLibraryPaths();
            
            // Check if already added
            if (!paths.contains(result.getFullPathName()))
            {
                paths.add(result.getFullPathName());
                model_.setUserLibraryPaths(paths);
                
                // Optionally start a deep scan on the new folder
                scanner_.startScan(result, true, 5); // Max depth 5
                
                DBG("BrowserPanel: Added folder and starting scan");
            }
            else
            {
                DBG("BrowserPanel: Folder already in library");
            }
        }
    });
}

//==============================================================================
// Filter Bar
//==============================================================================

void BrowserPanel::drawFilterBar(SkCanvas* canvas)
{
    float y = filterBarBounds_.getY();
    float w = filterBarBounds_.getWidth();
    float h = filterBarBounds_.getHeight();
    
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetRGB(28, 28, 32));
    canvas->drawRect(SkRect::MakeXYWH(0, y, w, h), bgPaint);
    
    // Calculate tab widths
    int tabWidth = (getWidth() - 16) / 4;
    int x = 8;
    
    filterAllBounds_ = juce::Rectangle<int>(x, static_cast<int>(y) + 4, tabWidth - 4, static_cast<int>(h) - 8);
    x += tabWidth;
    filterAudioBounds_ = juce::Rectangle<int>(x, static_cast<int>(y) + 4, tabWidth - 4, static_cast<int>(h) - 8);
    x += tabWidth;
    filterMidiBounds_ = juce::Rectangle<int>(x, static_cast<int>(y) + 4, tabWidth - 4, static_cast<int>(h) - 8);
    x += tabWidth;
    filterPluginBounds_ = juce::Rectangle<int>(x, static_cast<int>(y) + 4, tabWidth - 4, static_cast<int>(h) - 8);
    
    // Draw tabs
    auto activeFilter = model_.getActiveFilter();
    drawFilterTab(canvas, filterAllBounds_, "All", !model_.hasActiveFilter());
    drawFilterTab(canvas, filterAudioBounds_, "Audio", activeFilter == BrowserItemType::AudioFile);
    drawFilterTab(canvas, filterMidiBounds_, "MIDI", activeFilter == BrowserItemType::MidiFile);
    drawFilterTab(canvas, filterPluginBounds_, "Plugins", activeFilter == BrowserItemType::Plugin);
    
    // Bottom border
    SkPaint borderPaint;
    borderPaint.setColor(SkColorSetRGB(40, 40, 45));
    canvas->drawLine(0, y + h - 0.5f, w, y + h - 0.5f, borderPaint);
}

void BrowserPanel::drawFilterTab(SkCanvas* canvas, const juce::Rectangle<int>& bounds,
                                  const juce::String& label, bool active)
{
    SkPaint tabPaint;
    
    if (active)
    {
        // Active tab with accent color
        tabPaint.setColor(SkColorSetARGB(50, 0, 200, 255));
    }
    else
    {
        tabPaint.setColor(SkColorSetARGB(30, 255, 255, 255));
    }
    tabPaint.setAntiAlias(true);
    canvas->drawRoundRect(SkRect::MakeXYWH(bounds.getX(), bounds.getY(), 
                                            bounds.getWidth(), bounds.getHeight()),
                          4, 4, tabPaint);
    
    // Active indicator line
    if (active)
    {
        SkPaint indicatorPaint;
        indicatorPaint.setColor(SkColorSetRGB(0, 200, 255));
        canvas->drawRect(SkRect::MakeXYWH(bounds.getX() + 4, bounds.getBottom() - 2, 
                                           bounds.getWidth() - 8, 2), indicatorPaint);
    }
    
    // Text
    SkFont font;
    font.setSize(11.0f);
    font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
    
    SkPaint textPaint;
    textPaint.setColor(active ? SkColorSetRGB(150, 230, 255) : SkColorSetARGB(150, 255, 255, 255));
    textPaint.setAntiAlias(true);
    
    // Center text
    SkRect textBounds;
    font.measureText(label.toStdString().c_str(), label.length(), SkTextEncoding::kUTF8, &textBounds);
    float textX = bounds.getCentreX() - textBounds.width() / 2;
    float textY = bounds.getCentreY() + 4;
    
    canvas->drawString(label.toStdString().c_str(), textX, textY, font, textPaint);
}

//==============================================================================
// Context Menu & Favorites
//==============================================================================

void BrowserPanel::showContextMenu(int itemIndex, juce::Point<int> position)
{
    if (itemIndex < 0 || itemIndex >= static_cast<int>(displayItems_.size()))
        return;
    
    auto item = displayItems_[itemIndex];
    
    juce::PopupMenu menu;
    
    // Favorite toggle
    if (item->isFavorite)
    {
        menu.addItem(1, "Remove from Favorites");
    }
    else
    {
        menu.addItem(1, "Add to Favorites");
    }
    
    menu.addSeparator();
    
    // Tagging options
    menu.addItem(2, "Add Tag...");
    
    if (!item->metadata.tags.empty())
    {
        juce::PopupMenu tagSubMenu;
        int tagId = 100;
        for (const auto& tag : item->metadata.tags)
        {
            tagSubMenu.addItem(tagId++, "Remove: " + tag);
        }
        menu.addSubMenu("Remove Tag", tagSubMenu);
    }
    
    menu.addSeparator();
    
    // File operations
    if (item->type == BrowserItemType::AudioFile || item->type == BrowserItemType::MidiFile)
    {
        menu.addItem(10, "Show in Explorer");
    }
    
    // Show menu asynchronously
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({position.x, position.y, 1, 1}),
        [this, item](int result)
        {
            if (result == 1)
            {
                toggleFavorite(item);
            }
            else if (result == 2)
            {
                // Show tag input dialog
                auto* alertWindow = new juce::AlertWindow("Add Tag", "Enter a tag for this item:", 
                                                          juce::MessageBoxIconType::NoIcon);
                alertWindow->addTextEditor("tag", "", "Tag:");
                alertWindow->addButton("Add", 1);
                alertWindow->addButton("Cancel", 0);
                
                alertWindow->enterModalState(true, juce::ModalCallbackFunction::create(
                    [this, item, alertWindow](int result)
                    {
                        if (result == 1)
                        {
                            juce::String tag = alertWindow->getTextEditorContents("tag");
                            if (tag.isNotEmpty())
                            {
                                model_.addTagToItem(item, tag);
                                repaint();
                            }
                        }
                        // AlertWindow is managed by JUCE, no manual deletion needed
                    }));
            }
            else if (result >= 100)
            {
                // Remove tag
                int tagIndex = result - 100;
                if (tagIndex < static_cast<int>(item->metadata.tags.size()))
                {
                    model_.removeTagFromItem(item, item->metadata.tags[tagIndex]);
                    repaint();
                }
            }
            else if (result == 10)
            {
                // Show in explorer
                juce::File file(item->id);
                if (file.existsAsFile())
                {
                    file.revealToUser();
                }
            }
        });
}

void BrowserPanel::toggleFavorite(std::shared_ptr<BrowserItem> item)
{
    if (!item) return;
    
    if (item->isFavorite)
    {
        model_.removeFromFavorites(item);
    }
    else
    {
        model_.addToFavorites(item);
    }
    
    repaint();
}

} // namespace zenith

#endif // ZENITH_USE_SKIA

