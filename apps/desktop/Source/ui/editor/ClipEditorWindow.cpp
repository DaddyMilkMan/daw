/*
  ==============================================================================

    ClipEditorWindow.cpp
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Implementation of Clip Editor Window.

  ==============================================================================
*/

#include "ClipEditorWindow.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"

namespace zenith::ui {

//==============================================================================
// ContentComponent
//==============================================================================

ClipEditorWindow::ContentComponent::ContentComponent(
    zenith::ProjectState& projectState,
    const juce::String& clipId,
    const juce::String& trackId)
    : projectState_(projectState)
    , clipId_(clipId)
    , trackId_(trackId) {

    // Load clip data from project state
    auto [trackTree, clipTree] = projectState_.findClip(clipId_);
    if (clipTree.isValid()) {
        clipName_ = clipTree.getProperty(zenith::ProjectState::PROP_NAME, "Untitled Clip").toString();
        clipType_ = clipTree.getProperty(zenith::ProjectState::PROP_TYPE, "audio").toString();
        startBeats_ = clipTree.getProperty(zenith::ProjectState::PROP_START_BEATS, 0.0);
        lengthBeats_ = clipTree.getProperty(zenith::ProjectState::PROP_LENGTH_BEATS, 4.0);

        if (clipType_ == "audio") {
            audioFilePath_ = clipTree.getProperty(zenith::ProjectState::PROP_AUDIO_FILE, "").toString();
        }
    }
}

ClipEditorWindow::ContentComponent::~ContentComponent() {
}

void ClipEditorWindow::ContentComponent::resized() {
    // Layout child components if any
}

#ifdef ZENITH_USE_SKIA
void ClipEditorWindow::ContentComponent::drawSkia(SkCanvas* canvas) {
    using namespace design;

    auto width = static_cast<float>(getWidth());
    auto height = static_cast<float>(getHeight());

    // Background
    SkPaint bgPaint;
    bgPaint.setColor(toSkColor(colors::background::dark));
    canvas->drawRect(SkRect::MakeWH(width, height), bgPaint);

    // Header bar
    SkPaint headerPaint;
    headerPaint.setColor(toSkColor(colors::surface::overlay));
    canvas->drawRect(SkRect::MakeWH(width, 40.0f), headerPaint);

    // Clip name in header
    SkPaint textPaint;
    textPaint.setColor(toSkColor(colors::text::primary));
    textPaint.setAntiAlias(true);

    SkFont headerFont = typography::getSkFont(typography::FONT_SM, FontWeight::Medium);
    canvas->drawSimpleText(clipName_.toUTF8(), clipName_.length(),
                          headerFont, 12.0f, 16.0f, 26.0f, textPaint);

    // Editor area content
    if (clipType_ == "audio") {
        // Draw audio clip editor content
        drawAudioClipEditor(canvas, width, height);
    } else if (clipType_ == "midi") {
        // Draw MIDI clip editor content
        drawMidiClipEditor(canvas, width, height);
    }
}

void ClipEditorWindow::ContentComponent::drawAudioClipEditor(SkCanvas* canvas, float width, float height) {
    using namespace design;

    // Draw waveform placeholder or actual waveform
    SkPaint waveformPaint;
    waveformPaint.setColor(toSkColor(colors::accent::primary).withAlpha(0.6f));

    float centerY = 40.0f + (height - 40.0f) * 0.5f;

    // Center line
    SkPaint centerLinePaint;
    centerLinePaint.setColor(toSkColor(colors::text::tertiary).withAlpha(0.3f));
    centerLinePaint.setStrokeWidth(1.0f);
    canvas->drawLine(10.0f, centerY, width - 10.0f, centerY, centerLinePaint);

    // Draw "Audio Clip Editor" label
    SkPaint textPaint;
    textPaint.setColor(toSkColor(colors::text::secondary));
    textPaint.setAntiAlias(true);

    SkFont labelFont = typography::getSkFont(typography::FONT_SM, FontWeight::Regular);
    canvas->drawSimpleText("Audio Clip Editor", 18, labelFont, 12.0f, height / 2.0f + 4.0f, textPaint);

    // If audio file exists, show waveform (placeholder for now)
    if (!audioFilePath_.isEmpty()) {
        juce::File audioFile(audioFilePath_);
        if (audioFile.existsAsFile()) {
            // TODO: Load and display actual waveform
            // For now, show a simple visualization
        }
    }
}

void ClipEditorWindow::ContentComponent::drawMidiClipEditor(SkCanvas* canvas, float width, float height) {
    using namespace design;

    // Draw "MIDI Clip Editor" label
    SkPaint textPaint;
    textPaint.setColor(toSkColor(colors::text::secondary));
    textPaint.setAntiAlias(true);

    SkFont labelFont = typography::getSkFont(typography::FONT_SM, FontWeight::Regular);
    canvas->drawSimpleText("MIDI Clip Editor", 18, labelFont, 12.0f, height / 2.0f + 4.0f, textPaint);
}
#endif

//==============================================================================
// ClipEditorWindow
//==============================================================================

ClipEditorWindow::ClipEditorWindow(zenith::ProjectState& projectState,
                                   juce::String clipId,
                                   juce::String trackId)
    : juce::DocumentWindow("Clip Editor",
                           juce::Colours::darkgrey,
                           juce::DocumentWindow::allButtons)
    , projectState_(projectState)
    , clipId_(clipId)
    , trackId_(trackId) {

    // Get clip name for window title
    auto [trackTree, clipTree] = projectState.findClip(clipId);
    if (clipTree.isValid()) {
        juce::String name = clipTree.getProperty(zenith::ProjectState::PROP_NAME, "Untitled Clip").toString();
        setName(name + " - Clip Editor");
    }

    // Create and set content component
    setContentOwned(new ContentComponent(projectState, clipId, trackId), true);

    // Set initial size and position
    setSize(800, 500);
    setCentrePosition(800, 500);

    // Set visible
    setVisible(true);
}

ClipEditorWindow::~ClipEditorWindow() {
}

void ClipEditorWindow::closeButtonPressed() {
    // Delete window when closed
    delete this;
}

} // namespace zenith::ui
