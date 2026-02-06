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

#include "ClipEditorWindow.h"
#include "../../engine/AudioEngine.h"
#include "../../design-system/ColorBridge.h"
#include "../../design-system/ZenithDesignSystem.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace zenith::ui {

using namespace design;

//==============================================================================
// Waveform Cache
//==============================================================================

/**
 * @brief Cached waveform data for efficient display
 */
class WaveformCache {
public:
    struct ChannelData {
        std::vector<float> minValues;
        std::vector<float> maxValues;
        std::vector<float> rmsValues;
    };
    
    bool isValid() const { return !channels.empty() && sampleRate > 0; }
    void clear() { channels.clear(); sampleRate = 0; numSamples = 0; }
    
    std::vector<ChannelData> channels;
    double sampleRate = 0.0;
    int64_t numSamples = 0;
    juce::String sourceFile;
};

//==============================================================================
// Content Component
//==============================================================================

class ClipEditorWindow::ContentComponent : public SkiaComponent,
                                           public juce::ChangeListener,
                                           public juce::ScrollBar::Listener {
public:
    ContentComponent(zenith::ProjectState& projectState,
                    const juce::String& clipId,
                    const juce::String& trackId)
        : projectState_(projectState)
        , clipId_(clipId)
        , trackId_(trackId)
        , thumbnailCache_(5)
        , thumbnail_(512, formatManager_, thumbnailCache_)
        , zoomLevel_(1.0)
        , scrollOffset_(0.0)
        , isLoading_(false)
    {
        setName("ClipEditorContent");
        
        // Initialize audio format manager
        formatManager_.registerBasicFormats();
        
        // Setup scrollbars
        verticalScrollBar_.setAutoHide(false);
        horizontalScrollBar_.setAutoHide(false);
        addAndMakeVisible(verticalScrollBar_);
        addAndMakeVisible(horizontalScrollBar_);
        
        verticalScrollBar_.addListener(this);
        horizontalScrollBar_.addListener(this);
        
        // Load clip data
        loadClipData();
        
        // Start background loading
        if (clipType_ == "audio" && !audioFilePath_.isEmpty()) {
            loadAudioWaveform();
        }
        
        // Listen for project changes
        projectState_.addChangeListener(this);
    }
    
    ~ContentComponent() override {
        projectState_.removeChangeListener(this);
        verticalScrollBar_.removeListener(this);
        horizontalScrollBar_.removeListener(this);
        
        // Cancel any pending loads
        if (loadThread_.joinable()) {
            cancelLoad_ = true;
            loadThread_.join();
        }
    }
    
    void resized() override {
        auto bounds = getLocalBounds();
        
        // Reserve space for scrollbars
        const int scrollbarSize = 16;
        auto contentBounds = bounds.reduced(0, 0);
        contentBounds.removeFromRight(scrollbarSize);
        contentBounds.removeFromBottom(scrollbarSize);
        
        // Position scrollbars
        verticalScrollBar_.setBounds(contentBounds.getRight(), contentBounds.getY(),
                                     scrollbarSize, contentBounds.getHeight());
        horizontalScrollBar_.setBounds(contentBounds.getX(), contentBounds.getBottom(),
                                       contentBounds.getWidth(), scrollbarSize);
        
        // Update scrollbar ranges
        updateScrollRanges();
        
        markDirty();
    }
    
    void drawSkia(SkCanvas* canvas) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        SkPaint bgPaint;
        bgPaint.setColor(toSkColor(colors::BG_DARK));
        canvas->drawRect(SkRect::MakeWH(bounds.width(), bounds.height()), bgPaint);
        
        // Header bar
        drawHeader(canvas, bounds);
        
        // Main content area (below header)
        float headerHeight = 40.0f;
        SkRect contentBounds = SkRect::MakeXYWH(0, headerHeight, 
                                                bounds.width() - 16,  // Account for scrollbar
                                                bounds.height() - headerHeight - 16);
        
        // Clip content
        if (clipType_ == "audio") {
            drawAudioClipEditor(canvas, contentBounds);
        } else {
            drawMidiClipEditor(canvas, contentBounds);
        }
        
        // Loading indicator
        if (isLoading_) {
            drawLoadingIndicator(canvas, bounds.centerX(), bounds.centerY());
        }
    }

private:
    void drawHeader(SkCanvas* canvas, const SkRect& bounds) {
        SkRect headerRect = SkRect::MakeXYWH(0, 0, bounds.width(), 40.0f);
        
        SkPaint headerBg;
        headerBg.setColor(toSkColor(withAlpha(colors::BG_LIGHT, 0.5f)));
        canvas->drawRect(headerRect, headerBg);
        
        // Clip name
        SkFont titleFont = getSkFont(14.0f, FontWeight::SemiBold);
        SkPaint titlePaint;
        titlePaint.setColor(toSkColor(colors::TEXT_PRIMARY));
        titlePaint.setAntiAlias(true);
        
        juce::String title = clipName_.isEmpty() ? "Untitled Clip" : clipName_;
        canvas->drawString(title.toStdString().c_str(), 16.0f, 26.0f, titleFont, titlePaint);
        
        // Clip info (type, length)
        SkFont infoFont = getSkFont(11.0f, FontWeight::Regular);
        SkPaint infoPaint;
        infoPaint.setColor(toSkColor(colors::TEXT_SECONDARY));
        infoPaint.setAntiAlias(true);
        
        juce::String info = clipType_.toUpperCase() + " | " + 
                           juce::String(lengthBeats_, 1) + " beats";
        canvas->drawString(info.toStdString().c_str(), 200.0f, 26.0f, infoFont, infoPaint);
        
        // Zoom controls
        drawZoomControls(canvas, headerRect);
    }
    
    void drawZoomControls(SkCanvas* canvas, const SkRect& headerBounds) {
        float btnSize = 24.0f;
        float btnY = (headerBounds.height() - btnSize) / 2.0f;
        float rightX = headerBounds.right() - 100.0f;
        
        // Zoom out button
        SkRect zoomOutRect = SkRect::MakeXYWH(rightX, btnY, btnSize, btnSize);
        SkPaint btnBg;
        btnBg.setColor(toSkColor(withAlpha(colors::BG_DARK, 0.8f)));
        btnBg.setAntiAlias(true);
        canvas->drawRoundRect(zoomOutRect, 4.0f, 4.0f, btnBg);
        
        SkFont btnFont = getSkFont(14.0f, FontWeight::Bold);
        SkPaint btnText;
        btnText.setColor(toSkColor(colors::TEXT_PRIMARY));
        btnText.setAntiAlias(true);
        canvas->drawString("-", zoomOutRect.centerX() - 3.0f, zoomOutRect.centerY() + 5.0f, 
                          btnFont, btnText);
        
        // Zoom level display
        SkFont zoomFont = getSkFont(11.0f, FontWeight::Regular);
        SkPaint zoomPaint;
        zoomPaint.setColor(toSkColor(colors::TEXT_SECONDARY));
        zoomPaint.setAntiAlias(true);
        juce::String zoomText = juce::String(static_cast<int>(zoomLevel_ * 100)) + "%";
        canvas->drawString(zoomText.toStdString().c_str(), rightX + 32.0f, 
                          headerBounds.centerY() + 4.0f, zoomFont, zoomPaint);
        
        // Zoom in button
        SkRect zoomInRect = SkRect::MakeXYWH(rightX + 70.0f, btnY, btnSize, btnSize);
        canvas->drawRoundRect(zoomInRect, 4.0f, 4.0f, btnBg);
        canvas->drawString("+", zoomInRect.centerX() - 4.0f, zoomInRect.centerY() + 5.0f,
                          btnFont, btnText);
    }
    
    void drawAudioClipEditor(SkCanvas* canvas, const SkRect& bounds) {
        // Background
        SkPaint bgPaint;
        bgPaint.setColor(toSkColor(withAlpha(colors::BG_DARKER, 0.5f)));
        canvas->drawRect(bounds, bgPaint);
        
        // Grid lines
        drawTimeGrid(canvas, bounds);
        
        // Waveform
        if (thumbnail_.getTotalLength() > 0.0) {
            drawWaveform(canvas, bounds);
        } else if (!audioFilePath_.isEmpty() && !isLoading_) {
            // Show placeholder while loading
            drawWaveformPlaceholder(canvas, bounds);
        } else {
            // No audio file
            drawNoAudioMessage(canvas, bounds);
        }
        
        // Playhead
        drawPlayhead(canvas, bounds);
    }
    
    void drawWaveform(SkCanvas* canvas, const SkRect& bounds) {
        float channelHeight = bounds.height() / thumbnail_.getNumChannels();
        
        for (int channel = 0; channel < thumbnail_.getNumChannels(); ++channel) {
            float y = bounds.top() + channel * channelHeight;
            SkRect channelBounds = SkRect::MakeXYWH(bounds.left(), y, 
                                                    bounds.width(), channelHeight);
            
            // Draw waveform for this channel
            SkPaint wavePaint;
            wavePaint.setColor(toSkColor(colors::ACCENT_PRIMARY));
            wavePaint.setAntiAlias(true);
            wavePaint.setStyle(SkPaint::kStroke_Style);
            wavePaint.setStrokeWidth(1.0f);
            
            // Use JUCE's thumbnail to draw into Skia
            juce::Graphics g(juce::Image(juce::Image::ARGB, 1, 1, false));
            
            // Calculate visible range
            double totalLength = thumbnail_.getTotalLength();
            double visibleStart = scrollOffset_ / zoomLevel_;
            double visibleLength = (bounds.width() / zoomLevel_) / 100.0; // Scale factor
            
            if (visibleStart < 0) visibleStart = 0;
            if (visibleStart + visibleLength > totalLength)
                visibleLength = totalLength - visibleStart;
            
            // Draw waveform line by line
            const int numSamples = static_cast<int>(bounds.width());
            SkPath wavePath;
            
            for (int x = 0; x < numSamples; ++x) {
                double time = visibleStart + (x / static_cast<double>(numSamples)) * visibleLength;
                
                float minVal, maxVal;
                thumbnail_.getApproximateMinMax(time, time + visibleLength / numSamples, 
                                                minVal, maxVal, channel);
                
                float yMin = channelBounds.centerY() - minVal * channelHeight * 0.45f;
                float yMax = channelBounds.centerY() - maxVal * channelHeight * 0.45f;
                
                if (x == 0) {
                    wavePath.moveTo(bounds.left() + x, yMin);
                } else {
                    wavePath.lineTo(bounds.left() + x, yMin);
                }
            }
            
            // Draw top outline
            canvas->drawPath(wavePath, wavePaint);
            
            // Draw filled area
            wavePath.lineTo(bounds.right(), channelBounds.centerY());
            wavePath.lineTo(bounds.left(), channelBounds.centerY());
            wavePath.close();
            
            SkPaint fillPaint;
            fillPaint.setColor(toSkColor(withAlpha(colors::ACCENT_PRIMARY, 0.3f)));
            fillPaint.setAntiAlias(true);
            canvas->drawPath(wavePath, fillPaint);
            
            // Channel label
            if (thumbnail_.getNumChannels() > 1) {
                SkFont labelFont = getSkFont(10.0f, FontWeight::Regular);
                SkPaint labelPaint;
                labelPaint.setColor(toSkColor(colors::TEXT_SECONDARY));
                labelPaint.setAntiAlias(true);
                juce::String label = channel == 0 ? "L" : "R";
                canvas->drawString(label.toStdString().c_str(), bounds.left() + 5.0f, 
                                  y + 15.0f, labelFont, labelPaint);
            }
        }
    }
    
    void drawWaveformPlaceholder(SkCanvas* canvas, const SkRect& bounds) {
        // Draw a placeholder representation while loading
        SkPaint placeholderPaint;
        placeholderPaint.setColor(toSkColor(withAlpha(colors::ACCENT_PRIMARY, 0.2f)));
        placeholderPaint.setStyle(SkPaint::kStroke_Style);
        placeholderPaint.setStrokeWidth(1.0f);
        placeholderPaint.setPathEffect(SkDashPathEffect::Make(new float[]{5, 5}, 2, 0));
        
        float centerY = bounds.centerY();
        
        SkPath placeholderPath;
        placeholderPath.moveTo(bounds.left(), centerY);
        
        for (float x = bounds.left(); x < bounds.right(); x += 5.0f) {
            float y = centerY + std::sin(x * 0.05f) * bounds.height() * 0.3f;
            placeholderPath.lineTo(x, y);
        }
        
        canvas->drawPath(placeholderPath, placeholderPaint);
        
        // Loading text
        SkFont textFont = getSkFont(12.0f, FontWeight::Regular);
        SkPaint textPaint;
        textPaint.setColor(toSkColor(colors::TEXT_SECONDARY));
        textPaint.setAntiAlias(true);
        
        juce::String text = "Loading waveform...";
        canvas->drawString(text.toStdString().c_str(), bounds.centerX() - 50.0f, 
                          bounds.centerY() + 40.0f, textFont, textPaint);
    }
    
    void drawNoAudioMessage(SkCanvas* canvas, const SkRect& bounds) {
        SkFont textFont = getSkFont(14.0f, FontWeight::Regular);
        SkPaint textPaint;
        textPaint.setColor(toSkColor(colors::TEXT_SECONDARY));
        textPaint.setAntiAlias(true);
        
        juce::String text = "No audio file associated with this clip";
        float textWidth = textFont.measureText(text.toRawUTF8(), text.getNumBytesAsUTF8());
        
        canvas->drawString(text.toStdString().c_str(), 
                          bounds.centerX() - textWidth / 2.0f,
                          bounds.centerY(), textFont, textPaint);
    }
    
    void drawTimeGrid(SkCanvas* canvas, const SkRect& bounds) {
        SkPaint gridPaint;
        gridPaint.setColor(toSkColor(withAlpha(colors::BORDER_DEFAULT, 0.3f)));
        gridPaint.setStrokeWidth(1.0f);
        
        // Draw vertical beat lines
        float pixelsPerBeat = 100.0f * zoomLevel_;
        float offset = std::fmod(scrollOffset_, pixelsPerBeat);
        
        for (float x = bounds.left() - offset; x < bounds.right(); x += pixelsPerBeat) {
            canvas->drawLine(x, bounds.top(), x, bounds.bottom(), gridPaint);
        }
        
        // Draw horizontal center line
        float centerY = bounds.centerY();
        canvas->drawLine(bounds.left(), centerY, bounds.right(), centerY, gridPaint);
    }
    
    void drawPlayhead(SkCanvas* canvas, const SkRect& bounds) {
        // Calculate playhead position
        double playPosition = 0.0; // Would get from transport
        float playheadX = bounds.left() + static_cast<float>(playPosition * 100.0 * zoomLevel_) - scrollOffset_;
        
        if (playheadX >= bounds.left() && playheadX <= bounds.right()) {
            SkPaint playheadPaint;
            playheadPaint.setColor(toSkColor(colors::NEON_RED));
            playheadPaint.setStrokeWidth(2.0f);
            playheadPaint.setAntiAlias(true);
            
            canvas->drawLine(playheadX, bounds.top(), playheadX, bounds.bottom(), playheadPaint);
            
            // Playhead handle
            SkPaint handlePaint;
            handlePaint.setColor(toSkColor(colors::NEON_RED));
            handlePaint.setAntiAlias(true);
            canvas->drawCircle(playheadX, bounds.top() + 8.0f, 6.0f, handlePaint);
        }
    }
    
    void drawMidiClipEditor(SkCanvas* canvas, const SkRect& bounds) {
        // Background
        SkPaint bgPaint;
        bgPaint.setColor(toSkColor(colors::BG_DARKER));
        canvas->drawRect(bounds, bgPaint);
        
        // Draw piano roll grid
        drawPianoRollGrid(canvas, bounds);
        
        // Placeholder for MIDI notes
        SkFont textFont = getSkFont(14.0f, FontWeight::Regular);
        SkPaint textPaint;
        textPaint.setColor(toSkColor(colors::TEXT_SECONDARY));
        textPaint.setAntiAlias(true);
        
        juce::String text = "MIDI Editor - Note editing coming soon";
        float textWidth = textFont.measureText(text.toRawUTF8(), text.getNumBytesAsUTF8());
        
        canvas->drawString(text.toStdString().c_str(),
                          bounds.centerX() - textWidth / 2.0f,
                          bounds.centerY(), textFont, textPaint);
    }
    
    void drawPianoRollGrid(SkCanvas* canvas, const SkRect& bounds) {
        const int numKeys = 128;
        const float keyHeight = bounds.height() / 24.0f; // Show 2 octaves by default
        
        for (int i = 0; i < 24; ++i) {
            int note = 60 - i; // Start from middle C
            float y = bounds.top() + i * keyHeight;
            
            bool isBlackKey = ((note % 12) == 1) || ((note % 12) == 3) || 
                             ((note % 12) == 6) || ((note % 12) == 8) || ((note % 12) == 10);
            
            SkPaint keyPaint;
            if (isBlackKey) {
                keyPaint.setColor(toSkColor(withAlpha(colors::BG_LIGHT, 0.5f)));
            } else {
                keyPaint.setColor(toSkColor(colors::BG_DARKER));
            }
            
            SkRect keyRect = SkRect::MakeXYWH(bounds.left(), y, 60.0f, keyHeight);
            canvas->drawRect(keyRect, keyPaint);
            
            // Key border
            SkPaint borderPaint;
            borderPaint.setColor(toSkColor(withAlpha(colors::BORDER_DEFAULT, 0.3f)));
            borderPaint.setStrokeWidth(0.5f);
            canvas->drawLine(bounds.left(), y + keyHeight, bounds.left() + 60.0f, y + keyHeight, borderPaint);
        }
        
        // Vertical separator
        SkPaint sepPaint;
        sepPaint.setColor(toSkColor(colors::BORDER_DEFAULT));
        sepPaint.setStrokeWidth(1.0f);
        canvas->drawLine(bounds.left() + 60.0f, bounds.top(), 
                        bounds.left() + 60.0f, bounds.bottom(), sepPaint);
    }
    
    void drawLoadingIndicator(SkCanvas* canvas, float x, float y) {
        static float rotation = 0.0f;
        rotation += 0.1f;
        
        SkPaint paint;
        paint.setColor(toSkColor(colors::ACCENT_PRIMARY));
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(3.0f);
        paint.setAntiAlias(true);
        
        float radius = 15.0f;
        SkRect oval = SkRect::MakeXYWH(x - radius, y - radius, radius * 2, radius * 2);
        
        canvas->drawArc(oval, rotation * 57.2958f, 270.0f, false, paint);
    }
    
    // ProjectState::ChangeListener
    void changeListenerCallback(juce::ChangeBroadcaster*) override {
        // Reload if clip data changed
        loadClipData();
        markDirty();
    }
    
    // ScrollBar::Listener
    void scrollBarMoved(juce::ScrollBar* bar, double newRangeStart) override {
        if (bar == &horizontalScrollBar_) {
            scrollOffset_ = newRangeStart;
        }
        markDirty();
    }

private:
    void loadClipData() {
        // Query project state for clip data
        auto clip = projectState_.getClip(clipId_);
        if (clip.isValid()) {
            clipName_ = clip.getProperty("name", "Untitled Clip").toString();
            clipType_ = clip.getProperty("type", "audio").toString();
            startBeats_ = clip.getProperty("start", 0.0);
            lengthBeats_ = clip.getProperty("length", 4.0);
            audioFilePath_ = clip.getProperty("audioFile", "").toString();
        }
    }
    
    void loadAudioWaveform() {
        if (audioFilePath_.isEmpty()) return;
        
        isLoading_ = true;
        markDirty();
        
        // Load in background thread
        if (loadThread_.joinable()) {
            cancelLoad_ = true;
            loadThread_.join();
        }
        
        cancelLoad_ = false;
        loadThread_ = std::thread([this]() {
            juce::File audioFile(audioFilePath_);
            
            if (!audioFile.existsAsFile()) {
                isLoading_ = false;
                juce::MessageManager::callAsync([this]() { markDirty(); });
                return;
            }
            
            // Load into thumbnail
            juce::AudioFormatReader* reader = formatManager_.createReaderFor(audioFile);
            
            if (reader != nullptr && !cancelLoad_) {
                thumbnail_.setReader(reader, audioFile.hashCode());
            }
            
            isLoading_ = false;
            
            juce::MessageManager::callAsync([this]() {
                updateScrollRanges();
                markDirty();
            });
        });
    }
    
    void updateScrollRanges() {
        float contentWidth = static_cast<float>(thumbnail_.getTotalLength() * 100.0 * zoomLevel_);
        float viewWidth = getWidth() - 32.0f;
        
        horizontalScrollBar_.setRangeLimits(0.0, contentWidth);
        horizontalScrollBar_.setCurrentRange(scrollOffset_, viewWidth);
    }

private:
    zenith::ProjectState& projectState_;
    juce::String clipId_;
    juce::String trackId_;
    
    juce::String clipName_;
    juce::String clipType_;
    double startBeats_ = 0.0;
    double lengthBeats_ = 0.0;
    juce::String audioFilePath_;
    
    // Audio waveform
    juce::AudioFormatManager formatManager_;
    juce::AudioThumbnailCache thumbnailCache_;
    juce::AudioThumbnail thumbnail_;
    
    // View state
    float zoomLevel_;
    double scrollOffset_;
    bool isLoading_;
    std::atomic<bool> cancelLoad_{false};
    std::thread loadThread_;
    
    // UI
    juce::ScrollBar verticalScrollBar_{true};
    juce::ScrollBar horizontalScrollBar_{false};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ContentComponent)
};

//==============================================================================
// Clip Editor Window
//==============================================================================

ClipEditorWindow::ClipEditorWindow(zenith::ProjectState& projectState,
                                 juce::String clipId,
                                 juce::String trackId)
    : DocumentWindow("Clip Editor", 
                    juce::Colours::darkgrey,
                    DocumentWindow::allButtons)
    , projectState_(projectState)
    , clipId_(std::move(clipId))
    , trackId_(std::move(trackId))
{
    setContentOwned(new ContentComponent(projectState_, clipId_, trackId_), true);
    setResizable(true, true);
    setSize(800, 400);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
}

ClipEditorWindow::~ClipEditorWindow() = default;

void ClipEditorWindow::closeButtonPressed() {
    setVisible(false);
}

} // namespace zenith::ui
