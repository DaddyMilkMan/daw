/*
  ==============================================================================
    WaveformDisplay.cpp
    Real-time waveform visualization implementation
  ==============================================================================
*/

#include "WaveformDisplay.h"
#include <algorithm>
#include <cmath>

namespace zenith {
namespace ui {

// WaveformDisplay Implementation
WaveformDisplay::WaveformDisplay() {
    // Start timer for real-time updates
    startTimerHz(20);  // 20 FPS
    
    // Enable mouse interaction
    setMouseCursor(juce::MouseCursor::IBeam);
    setWantsKeyboardFocus(true);
    
    // Initialize waveform image
    waveformImage = std::make_unique<juce::Image>(juce::Image::RGB, 1, 1, true);
    spectrogramImage = std::make_unique<juce::Image>(juce::Image::RGB, 1, 1, true);
}

WaveformDisplay::~WaveformDisplay() {
    stopTimer();
}

void WaveformDisplay::setAudioData(const juce::AudioBuffer<float>& buffer, double sampleRate) {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    currentSampleRate = sampleRate;
    audioDuration = buffer.getNumSamples() / sampleRate;
    
    waveformBuffer.clear();
    processAudioData(buffer);
    
    needsRedraw = true;
    invalidateWaveform();
}

void WaveformDisplay::addAudioData(const juce::AudioBuffer<float>& buffer, double sampleRate) {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    if (currentSampleRate != sampleRate) {
        currentSampleRate = sampleRate;
        waveformBuffer.clear();
    }
    
    processAudioData(buffer);
    
    // Optimize buffer size
    if (waveformBuffer.size() > MAX_BUFFER_SIZE) {
        optimizeBuffer();
    }
    
    needsRedraw = true;
}

void WaveformDisplay::clearAudioData() {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    waveformBuffer.clear();
    audioDuration = 0.0;
    currentSampleRate = 44100.0;
    
    needsRedraw = true;
    invalidateWaveform();
}

void WaveformDisplay::setMode(WaveformMode mode) {
    if (currentMode != mode) {
        currentMode = mode;
        needsRedraw = true;
        invalidateWaveform();
    }
}

void WaveformDisplay::setColors(const WaveformColors& newColors) {
    colors = newColors;
    needsRedraw = true;
    repaint();
}

void WaveformDisplay::setShowGrid(bool show) {
    showGrid = show;
    repaint();
}

void WaveformDisplay::setShowLabels(bool show) {
    showLabels = show;
    repaint();
}

void WaveformDisplay::setShowRMS(bool show) {
    showRMS = show;
    repaint();
}

void WaveformDisplay::setShowPeaks(bool show) {
    showPeaks = show;
    repaint();
}

void WaveformDisplay::setZoomLevel(float zoom) {
    zoomLevel = juce::jlimit(0.1f, 100.0f, zoom);
    updateViewRange();
    repaint();
}

void WaveformDisplay::setScrollPosition(float position) {
    scrollPosition = juce::jlimit(0.0f, 1.0f, position);
    updateViewRange();
    repaint();
}

void WaveformDisplay::setViewRange(double startTime, double endTime) {
    viewStartTime = juce::jlimit(0.0, audioDuration, startTime);
    viewEndTime = juce::jlimit(viewStartTime, audioDuration, endTime);
    
    // Update zoom and scroll
    double viewDuration = viewEndTime - viewStartTime;
    if (audioDuration > 0.0) {
        zoomLevel = static_cast<float>(audioDuration / viewDuration);
        scrollPosition = static_cast<float>(viewStartTime / audioDuration);
    }
    
    needsRedraw = true;
    repaint();
}

void WaveformDisplay::fitToWindow() {
    if (audioDuration > 0.0) {
        setViewRange(0.0, audioDuration);
    }
}

void WaveformDisplay::setSelection(double startTime, double endTime) {
    selectionStart = juce::jlimit(0.0, audioDuration, startTime);
    selectionEnd = juce::jlimit(selectionStart, audioDuration, endTime);
    hasSelectionFlag = true;
    repaint();
}

void WaveformDisplay::clearSelection() {
    hasSelectionFlag = false;
    repaint();
}

std::pair<double, double> WaveformDisplay::getSelection() const {
    if (hasSelectionFlag) {
        return {selectionStart, selectionEnd};
    }
    return {0.0, 0.0};
}

bool WaveformDisplay::hasSelection() const {
    return hasSelectionFlag;
}

void WaveformDisplay::setPlaybackPosition(double time) {
    playbackPosition = juce::jlimit(0.0, audioDuration, time);
    repaint();
}

void WaveformDisplay::clearPlaybackPosition() {
    playbackPosition = -1.0;
    repaint();
}

double WaveformDisplay::getPlaybackPosition() const {
    return playbackPosition;
}

void WaveformDisplay::showGenreDetection(bool show) {
    showGenreDetectionFlag = show;
    repaint();
}

void WaveformDisplay::showQualityAnalysis(bool show) {
    showQualityAnalysisFlag = show;
    repaint();
}

void WaveformDisplay::showSpectrumAnalysis(bool show) {
    showSpectrumAnalysisFlag = show;
    repaint();
}

void WaveformDisplay::updateAnalysisData(const juce::String& analysis) {
    currentAnalysis = analysis;
    repaint();
}

juce::Image WaveformDisplay::exportWaveform(int width, int height) const {
    juce::Image image(juce::Image::RGB, width, height, true);
    juce::Graphics g(image);
    
    // Save current state
    auto savedBounds = getBounds();
    auto savedZoom = zoomLevel;
    auto savedScroll = scrollPosition;
    
    // Temporarily set size for export
    const_cast<WaveformDisplay*>(this)->setSize(width, height);
    
    // Draw
    paint(g);
    
    // Restore state
    const_cast<WaveformDisplay*>(this)->setSize(savedBounds.getWidth(), savedBounds.getHeight());
    const_cast<WaveformDisplay*>(this)->setZoomLevel(savedZoom);
    const_cast<WaveformDisplay*>(this)->setScrollPosition(savedScroll);
    
    return image;
}

void WaveformDisplay::exportToFile(const juce::File& file, int width, int height) const {
    auto image = exportWaveform(width, height);
    juce::PNGImageFormat format;
    file.createOutputStream()->writeFromInputStream(juce::MemoryInputStream(image.getData(), image.getSize()), image.getSize());
}

void WaveformDisplay::paint(juce::Graphics& g) {
    // Draw background
    drawBackground(g);
    
    // Draw grid
    if (showGrid) {
        drawGrid(g);
    }
    
    // Draw waveform based on mode
    drawWaveform(g);
    
    // Draw RMS and peaks
    if (showRMS) {
        drawRMS(g);
    }
    if (showPeaks) {
        drawPeaks(g);
    }
    
    // Draw selection
    if (hasSelectionFlag) {
        drawSelection(g);
    }
    
    // Draw playback position
    if (playbackPosition >= 0.0) {
        drawPlaybackPosition(g);
    }
    
    // Draw labels
    if (showLabels) {
        drawLabels(g);
    }
    
    // Draw analysis overlay
    if (showGenreDetectionFlag || showQualityAnalysisFlag || showSpectrumAnalysisFlag) {
        drawAnalysisOverlay(g);
    }
}

void WaveformDisplay::resized() {
    needsRedraw = true;
    updateViewRange();
}

void WaveformDisplay::mouseDown(const juce::MouseEvent& event) {
    if (event.mods.isLeftButtonDown()) {
        double time = xToTime(event.position.x);
        startSelection(time);
    }
}

void WaveformDisplay::mouseDrag(const juce::MouseEvent& event) {
    if (isSelecting && event.mods.isLeftButtonDown()) {
        double time = xToTime(event.position.x);
        updateSelection(time);
    }
}

void WaveformDisplay::mouseUp(const juce::MouseEvent& event) {
    if (isSelecting) {
        endSelection();
    }
}

void WaveformDisplay::mouseMove(const juce::MouseEvent& event) {
    double time = xToTime(event.position.x);
    float amplitude = yToAmplitude(event.position.y);
    
    // Update tooltip
    juce::String tooltip = formatTime(time) + " | " + formatAmplitude(amplitude);
    setTooltip(tooltip);
}

void WaveformDisplay::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) {
    if (event.mods.isCommandDown()) {
        // Zoom with Ctrl+wheel
        float zoomDelta = wheel.deltaY > 0 ? 1.1f : 0.9f;
        setZoomLevel(zoomLevel * zoomDelta);
    } else {
        // Pan with wheel
        float scrollDelta = wheel.deltaY * 0.1f;
        setScrollPosition(scrollPosition + scrollDelta);
    }
}

void WaveformDisplay::timerCallback() {
    if (needsRedraw) {
        updateWaveformImage();
        needsRedraw = false;
    }
    
    repaint();
}

double WaveformDisplay::timeToX(double time) const {
    if (viewEndTime <= viewStartTime) return 0.0;
    
    double viewDuration = viewEndTime - viewStartTime;
    double relativeTime = time - viewStartTime;
    double normalized = relativeTime / viewDuration;
    
    return normalized * getWidth();
}

double WaveformDisplay::xToTime(double x) const {
    double normalized = x / getWidth();
    double relativeTime = normalized * (viewEndTime - viewStartTime);
    return viewStartTime + relativeTime;
}

float WaveformDisplay::amplitudeToY(float amplitude) const {
    float normalized = (amplitude + 1.0f) * 0.5f;  // Convert -1..1 to 0..1
    return (1.0f - normalized) * getHeight();  // Flip Y axis
}

float WaveformDisplay::yToAmplitude(float y) const {
    float normalized = 1.0f - (y / getHeight());  // Flip Y axis
    return normalized * 2.0f - 1.0f;  // Convert 0..1 to -1..1
}

void WaveformDisplay::drawBackground(juce::Graphics& g) {
    g.fillAll(colors.background);
}

void WaveformDisplay::drawGrid(juce::Graphics& g) {
    g.setColour(colors.grid.withAlpha(gridOpacity));
    g.setOpacity(gridOpacity);
    
    int width = getWidth();
    int height = getHeight();
    
    // Calculate grid spacing
    double viewDuration = viewEndTime - viewStartTime;
    double timePerPixel = viewDuration / width;
    
    // Major grid lines
    for (int i = 0; i <= static_cast<int>(viewDuration); i += majorGridInterval) {
        double time = viewStartTime + i;
        float x = static_cast<float>(timeToX(time));
        
        g.drawVerticalLine(x, 0, height);
        
        // Time label
        if (showLabels) {
            g.setColour(colors.text);
            g.setFont(10.0f);
            g.drawText(formatTime(time), x + 2, height - 15, 50, 12, juce::Justification::left);
        }
    }
    
    // Minor grid lines
    for (int i = 0; i < majorGridInterval; ++i) {
        double time = viewStartTime + i * (viewDuration / majorGridInterval);
        float x = static_cast<float>(timeToX(time));
        
        g.setColour(colors.grid.withAlpha(gridOpacity * 0.5f));
        g.drawVerticalLine(x, 0, height);
    }
    
    // Horizontal lines
    g.setColour(colors.grid.withAlpha(gridOpacity));
    g.drawHorizontalLine(height / 2, 0, width);  // 0 dB line
    g.drawHorizontalLine(height * 0.25f, 0, width);  // +6 dB line
    g.drawHorizontalLine(height * 0.75f, 0, width);  // -6 dB line
}

void WaveformDisplay::drawWaveform(juce::Graphics& g) {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    if (waveformBuffer.empty()) {
        return;
    }
    
    switch (currentMode) {
        case WaveformMode::Normal:
            drawNormalWaveform(g);
            break;
        case WaveformMode::Stereo:
            drawStereoWaveform(g);
            break;
        case WaveformMode::Spectrogram:
            drawSpectrogram(g);
            break;
        case WaveformMode::Phase:
            drawPhaseDisplay(g);
            break;
        default:
            drawNormalWaveform(g);
            break;
    }
}

void WaveformDisplay::drawNormalWaveform(juce::Graphics& g) {
    g.setColour(colors.waveform);
    
    int width = getWidth();
    int height = getHeight();
    
    // Draw waveform as connected lines
    juce::Path leftChannelPath;
    juce::Path rightChannelPath;
    
    bool firstPoint = true;
    
    for (const auto& data : waveformBuffer) {
        double time = data.timestamp.toMilliseconds() / 1000.0;
        
        if (time >= viewStartTime && time <= viewEndTime) {
            float x = static_cast<float>(timeToX(time));
            float leftY = amplitudeToY(data.leftChannel);
            float rightY = amplitudeToY(data.rightChannel);
            
            if (firstPoint) {
                leftChannelPath.startNewSubPath(x, leftY);
                rightChannelPath.startNewSubPath(x, rightY);
                firstPoint = false;
            } else {
                leftChannelPath.lineTo(x, leftY);
                rightChannelPath.lineTo(x, rightY);
            }
        }
    }
    
    // Draw paths
    g.strokePath(leftChannelPath, juce::PathStrokeType(1.0f));
    
    if (waveformBuffer.size() > 0 && waveformBuffer[0].rightChannel != 0.0f) {
        g.setColour(colors.waveform.withAlpha(0.5f));
        g.strokePath(rightChannelPath, juce::PathStrokeType(1.0f));
    }
}

void WaveformDisplay::drawStereoWaveform(juce::Graphics& g) {
    // Draw left channel in upper half, right channel in lower half
    int halfHeight = getHeight() / 2;
    
    g.setColour(colors.waveform);
    
    // Left channel (top half)
    juce::Path leftPath;
    bool firstPoint = true;
    
    for (const auto& data : waveformBuffer) {
        double time = data.timestamp.toMilliseconds() / 1000.0;
        
        if (time >= viewStartTime && time <= viewEndTime) {
            float x = static_cast<float>(timeToX(time));
            float y = amplitudeToY(data.leftChannel) * 0.5f;  // Scale to upper half
            
            if (firstPoint) {
                leftPath.startNewSubPath(x, y);
                firstPoint = false;
            } else {
                leftPath.lineTo(x, y);
            }
        }
    }
    
    g.strokePath(leftPath, juce::PathStrokeType(1.0f));
    
    // Right channel (bottom half)
    juce::Path rightPath;
    firstPoint = true;
    
    for (const auto& data : waveformBuffer) {
        double time = data.timestamp.toMilliseconds() / 1000.0;
        
        if (time >= viewStartTime && time <= viewEndTime) {
            float x = static_cast<float>(timeToX(time));
            float y = amplitudeToY(data.rightChannel) * 0.5f + halfHeight;  // Scale to lower half
            
            if (firstPoint) {
                rightPath.startNewSubPath(x, y);
                firstPoint = false;
            } else {
                rightPath.lineTo(x, y);
            }
        }
    }
    
    g.strokePath(rightPath, juce::PathStrokeType(1.0f));
    
    // Draw divider line
    g.setColour(colors.grid);
    g.drawHorizontalLine(halfHeight, 0, getWidth());
}

void WaveformDisplay::drawRMS(juce::Graphics& g) {
    if (!showRMS) return;
    
    g.setColour(colors.rms);
    
    int width = getWidth();
    int height = getHeight();
    
    juce::Path rmsPath;
    bool firstPoint = true;
    
    for (const auto& data : waveformBuffer) {
        double time = data.timestamp.toMilliseconds() / 1000.0;
        
        if (time >= viewStartTime && time <= viewEndTime) {
            float x = static_cast<float>(timeToX(time));
            float y = amplitudeToY(data.rms);
            
            if (firstPoint) {
                rmsPath.startNewSubPath(x, y);
                firstPoint = false;
            } else {
                rmsPath.lineTo(x, y);
            }
        }
    }
    
    g.strokePath(rmsPath, juce::PathStrokeType(2.0f));
}

void WaveformDisplay::drawPeaks(juce::Graphics& g) {
    if (!showPeaks) return;
    
    g.setColour(colors.peak);
    
    int width = getWidth();
    int height = getHeight();
    
    // Draw peak markers
    for (const auto& data : waveformBuffer) {
        if (data.isClipping) {
            double time = data.timestamp.toMilliseconds() / 1000.0;
            
            if (time >= viewStartTime && time <= viewEndTime) {
                float x = static_cast<float>(timeToX(time));
                
                // Draw clipping indicator
                g.setColour(colors.clipping);
                g.drawVerticalLine(x, 0, height);
                g.drawEllipse(x - 3, 3, 6, 6, 2.0f);
            }
        }
    }
}

void WaveformDisplay::drawSelection(juce::Graphics& g) {
    if (!hasSelectionFlag) return;
    
    float startX = static_cast<float>(timeToX(selectionStart));
    float endX = static_cast<float>(timeToX(selectionEnd));
    
    if (startX > endX) {
        std::swap(startX, endX);
    }
    
    g.setColour(colors.selection);
    g.fillRect(startX, 0, endX - startX, getHeight());
    
    // Draw selection handles
    g.setColour(colors.waveform);
    g.drawRect(startX, 0, endX - startX, getHeight(), 2.0f);
}

void WaveformDisplay::drawPlaybackPosition(juce::Graphics& g) {
    if (playbackPosition < 0.0) return;
    
    float x = static_cast<float>(timeToX(playbackPosition));
    
    g.setColour(juce::Colours::white);
    g.drawVerticalLine(x, 0, getHeight());
    
    // Draw playback triangle
    juce::Path triangle;
    triangle.startNewSubPath(x - 5, 0);
    triangle.lineTo(x + 5, 0);
    triangle.lineTo(x, 10);
    triangle.closeSubPath();
    
    g.fillPath(triangle);
}

void WaveformDisplay::drawLabels(juce::Graphics& g) {
    g.setColour(colors.text);
    g.setFont(12.0f);
    
    // Draw amplitude labels
    g.drawText("+6 dB", 5, 5, 40, 15, juce::Justification::left);
    g.drawText("0 dB", 5, getHeight() / 2 - 7, 40, 15, juce::Justification::left);
    g.drawText("-6 dB", 5, getHeight() - 20, 40, 15, juce::Justification::left);
    
    // Draw time labels
    g.drawText(formatTime(viewStartTime), 5, getHeight() - 35, 60, 15, juce::Justification::left);
    g.drawText(formatTime(viewEndTime), getWidth() - 65, getHeight() - 35, 60, 15, juce::Justification::right);
}

void WaveformDisplay::drawAnalysisOverlay(juce::Graphics& g) {
    if (currentAnalysis.isEmpty()) return;
    
    // Draw analysis text overlay
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    
    juce::Rectangle<int> textBounds(10, 10, getWidth() - 20, 30);
    g.drawText(currentAnalysis, textBounds, juce::Justification::centred);
}

void WaveformDisplay::processAudioData(const juce::AudioBuffer<float>& buffer) {
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    
    // Process samples in chunks
    int chunkSize = 1024;  // Process 1024 samples at a time
    
    for (int start = 0; start < numSamples; start += chunkSize) {
        int end = std::min(start + chunkSize, numSamples);
        int chunkLength = end - start;
        
        WaveformData data;
        data.leftChannel = 0.0f;
        data.rightChannel = 0.0f;
        data.rms = 0.0f;
        data.peak = 0.0f;
        data.isClipping = false;
        data.timestamp = juce::Time::getCurrentTime();
        
        // Calculate RMS and peak
        for (int ch = 0; ch < numChannels; ++ch) {
            for (int i = start; i < end; ++i) {
                float sample = buffer.getSample(ch, i);
                
                if (ch == 0) {
                    data.leftChannel += sample;
                } else if (ch == 1) {
                    data.rightChannel += sample;
                }
                
                data.rms += sample * sample;
                data.peak = std::max(data.peak, std::abs(sample));
                
                if (std::abs(sample) > 0.99f) {
                    data.isClipping = true;
                }
            }
        }
        
        // Average the values
        data.leftChannel /= chunkLength;
        if (numChannels > 1) {
            data.rightChannel /= chunkLength;
        }
        
        data.rms = std::sqrt(data.rms / (chunkLength * numChannels));
        
        waveformBuffer.push_back(data);
    }
}

void WaveformDisplay::updateViewRange() {
    if (audioDuration > 0.0) {
        double viewDuration = audioDuration / zoomLevel;
        viewStartTime = scrollPosition * audioDuration;
        viewEndTime = viewStartTime + viewDuration;
        
        // Clamp to audio duration
        if (viewEndTime > audioDuration) {
            viewEndTime = audioDuration;
            viewStartTime = viewEndTime - viewDuration;
        }
        
        if (viewStartTime < 0.0) {
            viewStartTime = 0.0;
            viewEndTime = viewDuration;
        }
    }
}

void WaveformDisplay::optimizeBuffer() {
    if (waveformBuffer.size() > MAX_BUFFER_SIZE) {
        // Remove oldest samples
        size_t removeCount = waveformBuffer.size() - MAX_BUFFER_SIZE;
        waveformBuffer.erase(waveformBuffer.begin(), waveformBuffer.begin() + removeCount);
    }
}

void WaveformDisplay::updateWaveformImage() {
    // Update cached waveform image for performance
    int width = getWidth();
    int height = getHeight();
    
    if (width > 0 && height > 0) {
        *waveformImage = juce::Image(juce::Image::RGB, width, height, true);
        
        juce::Graphics g(*waveformImage);
        drawBackground(g);
        drawWaveform(g);
    }
}

void WaveformDisplay::invalidateWaveform() {
    needsRedraw = true;
}

void WaveformDisplay::startSelection(double time) {
    isSelecting = true;
    selectionAnchor = time;
    selectionStart = time;
    selectionEnd = time;
    hasSelectionFlag = true;
}

void WaveformDisplay::updateSelection(double time) {
    if (isSelecting) {
        selectionStart = std::min(selectionAnchor, time);
        selectionEnd = std::max(selectionAnchor, time);
        repaint();
    }
}

void WaveformDisplay::endSelection() {
    isSelecting = false;
    hasSelectionFlag = (std::abs(selectionEnd - selectionStart) > 0.01);  // Min 10ms selection
    repaint();
}

juce::String WaveformDisplay::formatTime(double time) const {
    int minutes = static_cast<int>(time) / 60;
    int seconds = static_cast<int>(time) % 60;
    int milliseconds = static_cast<int>((time - static_cast<int>(time)) * 1000);
    
    if (minutes > 0) {
        return juce::String::formatted("%02d:%02d.%03d", minutes, seconds, milliseconds);
    } else {
        return juce::String::formatted("%d.%03d", seconds, milliseconds);
    }
}

juce::String WaveformDisplay::formatAmplitude(float amplitude) const {
    float db = juce::Decibels::gainToDecibels(std::abs(amplitude));
    return juce::String(db, 1) + " dB";
}

// WaveformControlPanel Implementation
WaveformControlPanel::WaveformControlPanel(WaveformDisplay& display)
    : waveformDisplay(display) {
    
    createDisplayControls();
    createZoomControls();
    createNavigationControls();
    createAnalysisControls();
    createExportControls();
}

WaveformControlPanel::~WaveformControlPanel() = default;

void WaveformControlPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkgrey);
}

void WaveformControlPanel::resized() {
    auto bounds = getLocalBounds();
    
    // Layout controls in rows
    int rowHeight = 30;
    int margin = 5;
    
    // Row 1: Display controls
    int y = margin;
    modeComboBox->setBounds(margin, y, 100, rowHeight);
    gridToggle->setBounds(110, y, 60, rowHeight);
    labelsToggle->setBounds(175, y, 60, rowHeight);
    rmsToggle->setBounds(240, y, 50, rowHeight);
    peaksToggle->setBounds(295, y, 50, rowHeight);
    
    // Row 2: Zoom controls
    y += rowHeight + margin;
    zoomSlider->setBounds(margin, y, 200, rowHeight);
    zoomInButton->setBounds(210, y, 40, rowHeight);
    zoomOutButton->setBounds(255, y, 40, rowHeight);
    fitToWindowButton->setBounds(300, y, 80, rowHeight);
    
    // Row 3: Navigation controls
    y += rowHeight + margin;
    goStartButton->setBounds(margin, y, 60, rowHeight);
    goEndButton->setBounds(70, y, 60, rowHeight);
    playPauseButton->setBounds(140, y, 60, rowHeight);
    loopButton->setBounds(210, y, 60, rowHeight);
    
    // Row 4: Analysis controls
    y += rowHeight + margin;
    genreToggle->setBounds(margin, y, 80, rowHeight);
    qualityToggle->setBounds(90, y, 80, rowHeight);
    spectrumToggle->setBounds(180, y, 80, rowHeight);
    
    // Row 5: Export controls
    y += rowHeight + margin;
    exportButton->setBounds(margin, y, 60, rowHeight);
    screenshotButton->setBounds(70, y, 80, rowHeight);
}

void WaveformControlPanel::buttonClicked(juce::Button* button) {
    if (button == zoomInButton.get()) {
        waveformDisplay.setZoomLevel(waveformDisplay.getZoomLevel() * 1.5f);
    } else if (button == zoomOutButton.get()) {
        waveformDisplay.setZoomLevel(waveformDisplay.getZoomLevel() / 1.5f);
    } else if (button == fitToWindowButton.get()) {
        waveformDisplay.fitToWindow();
    } else if (button == goStartButton.get()) {
        waveformDisplay.setScrollPosition(0.0f);
    } else if (button == goEndButton.get()) {
        waveformDisplay.setScrollPosition(1.0f);
    } else if (button == playPauseButton.get()) {
        // Toggle playback
    } else if (button == loopButton.get()) {
        // Toggle loop
    } else if (button == exportButton.get()) {
        // Export waveform
    } else if (button == screenshotButton.get()) {
        // Take screenshot
    }
}

void WaveformControlPanel::comboBoxChanged(juce::ComboBox* comboBox) {
    if (comboBox == modeComboBox.get()) {
        int selectedId = comboBox->getSelectedId();
        switch (selectedId) {
            case 1: waveformDisplay.setMode(WaveformMode::Normal); break;
            case 2: waveformDisplay.setMode(WaveformMode::Stereo); break;
            case 3: waveformDisplay.setMode(WaveformMode::Spectrogram); break;
            case 4: waveformDisplay.setMode(WaveformMode::Phase); break;
            default: waveformDisplay.setMode(WaveformMode::Normal); break;
        }
    }
}

void WaveformControlPanel::sliderValueChanged(juce::Slider* slider) {
    if (slider == zoomSlider.get()) {
        waveformDisplay.setZoomLevel(slider->getValue());
    }
}

void WaveformControlPanel::createDisplayControls() {
    modeComboBox = std::make_unique<juce::ComboBox>();
    modeComboBox->addItem("Normal", 1);
    modeComboBox->addItem("Stereo", 2);
    modeComboBox->addItem("Spectrogram", 3);
    modeComboBox->addItem("Phase", 4);
    modeComboBox->setSelectedId(1);
    modeComboBox->addListener(this);
    addAndMakeVisible(*modeComboBox);
    
    gridToggle = std::make_unique<juce::ToggleButton>("Grid");
    gridToggle->setToggleState(true, juce::dontSendNotification);
    gridToggle->addListener(this);
    addAndMakeVisible(*gridToggle);
    
    labelsToggle = std::make_unique<juce::ToggleButton>("Labels");
    labelsToggle->setToggleState(true, juce::dontSendNotification);
    labelsToggle->addListener(this);
    addAndMakeVisible(*labelsToggle);
    
    rmsToggle = std::make_unique<juce::ToggleButton>("RMS");
    rmsToggle->setToggleState(true, juce::dontSendNotification);
    rmsToggle->addListener(this);
    addAndMakeVisible(*rmsToggle);
    
    peaksToggle = std::make_unique<juce::ToggleButton>("Peaks");
    peaksToggle->setToggleState(true, juce::dontSendNotification);
    peaksToggle->addListener(this);
    addAndMakeVisible(*peaksToggle);
}

void WaveformControlPanel::createZoomControls() {
    zoomSlider = std::make_unique<juce::Slider>("Zoom");
    zoomSlider->setRange(0.1f, 100.0f, 0.1f);
    zoomSlider->setValue(1.0f);
    zoomSlider->setSliderStyle(juce::Slider::LinearHorizontal);
    zoomSlider->addListener(this);
    addAndMakeVisible(*zoomSlider);
    
    zoomInButton = std::make_unique<juce::TextButton>("+");
    zoomInButton->addListener(this);
    addAndMakeVisible(*zoomInButton);
    
    zoomOutButton = std::make_unique<juce::TextButton>("-");
    zoomOutButton->addListener(this);
    addAndMakeVisible(*zoomOutButton);
    
    fitToWindowButton = std::make_unique<juce::TextButton>("Fit");
    fitToWindowButton->addListener(this);
    addAndMakeVisible(*fitToWindowButton);
}

void WaveformControlPanel::createNavigationControls() {
    goStartButton = std::make_unique<juce::TextButton("|<<");
    goStartButton->addListener(this);
    addAndMakeVisible(*goStartButton);
    
    goEndButton = std::make_unique<juce::TextButton(">>|");
    goEndButton->addListener(this);
    addAndMakeVisible(*goEndButton);
    
    playPauseButton = std::make_unique<juce::TextButton("Play");
    playPauseButton->addListener(this);
    addAndMakeVisible(*playPauseButton);
    
    loopButton = std::make_unique<juce::TextButton("Loop");
    loopButton->addListener(this);
    addAndMakeVisible(*loopButton);
}

void WaveformControlPanel::createAnalysisControls() {
    genreToggle = std::make_unique<juce::ToggleButton>("Genre");
    genreToggle->addListener(this);
    addAndMakeVisible(*genreToggle);
    
    qualityToggle = std::make_unique<juce::ToggleButton>("Quality");
    qualityToggle->addListener(this);
    addAndMakeVisible(*qualityToggle);
    
    spectrumToggle = std::make_unique<juce::ToggleButton>("Spectrum");
    spectrumToggle->addListener(this);
    addAndMakeVisible(*spectrumToggle);
}

void WaveformControlPanel::createExportControls() {
    exportButton = std::make_unique<juce::TextButton>("Export");
    exportButton->addListener(this);
    addAndMakeVisible(*exportButton);
    
    screenshotButton = std::make_unique<juce::TextButton>("Screenshot");
    screenshotButton->addListener(this);
    addAndMakeVisible(*screenshotButton);
}

// WaveformContainer Implementation
WaveformContainer::WaveformContainer() {
    waveformDisplay = std::make_unique<WaveformDisplay>();
    controlPanel = std::make_unique<WaveformControlPanel>(*waveformDisplay);
    
    addAndMakeVisible(*waveformDisplay);
    addAndMakeVisible(*controlPanel);
}

WaveformContainer::~WaveformContainer() = default;

void WaveformContainer::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkgrey);
}

void WaveformContainer::resized() {
    auto bounds = getLocalBounds();
    
    // Control panel at top
    int controlPanelHeight = 150;
    controlPanel->setBounds(bounds.removeFromTop(controlPanelHeight));
    
    // Waveform display takes remaining space
    waveformDisplay->setBounds(bounds);
}

void WaveformContainer::setColors(const WaveformColors& colors) {
    waveformDisplay->setColors(colors);
}

void WaveformContainer::setAudioData(const juce::AudioBuffer<float>& buffer, double sampleRate) {
    waveformDisplay->setAudioData(buffer, sampleRate);
}

} // namespace ui
} // namespace zenith
