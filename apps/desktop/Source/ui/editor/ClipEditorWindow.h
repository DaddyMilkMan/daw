/*
  ==============================================================================

    ClipEditorWindow.h
    Created: 2026-02-03
    Author:  Zenith DAW Team

    Clip editor window for editing clip contents.

  ==============================================================================
*/

#pragma once

#include "../../framework/SkiaComponent.h"
#include "../../design-system/ZenithTheme.h"
#include "../../../engine/ProjectState.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith::ui {

/**
 * @brief Clip editor window component
 *
 * Opens when double-clicking a clip in the arrangement or session view.
 * Displays clip contents for editing (waveform for audio, piano roll for MIDI).
 */
class ClipEditorWindow : public juce::DocumentWindow {
public:
    //==========================================================================
    // Construction/Destruction
    //==========================================================================

    ClipEditorWindow(zenith::ProjectState& projectState,
                     juce::String clipId,
                     juce::String trackId);

    ~ClipEditorWindow() override;

    //==========================================================================
    // DocumentWindow Overrides
    //==========================================================================

    void closeButtonPressed() override;

private:
    //==========================================================================
    // Internal Components
    //==========================================================================

    /**
     * @brief Main content component for the clip editor
     */
    class ContentComponent : public SkiaComponent {
    public:
        ContentComponent(zenith::ProjectState& projectState,
                        const juce::String& clipId,
                        const juce::String& trackId);
        ~ContentComponent() override;

        void resized() override;
#ifdef ZENITH_USE_SKIA
        void drawSkia(SkCanvas* canvas) override;
#endif

    private:
        zenith::ProjectState& projectState_;
        juce::String clipId_;
        juce::String trackId_;

        // Clip data cache
        juce::String clipName_;
        juce::String clipType_;  // "audio" or "midi"
        double startBeats_ = 0.0;
        double lengthBeats_ = 0.0;
        juce::String audioFilePath_;

#ifdef ZENITH_USE_SKIA
        void drawAudioClipEditor(SkCanvas* canvas, float width, float height);
        void drawMidiClipEditor(SkCanvas* canvas, float width, float height);
#endif

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ContentComponent)
    };

    //==========================================================================
    // Data
    //==========================================================================

    zenith::ProjectState& projectState_;
    juce::String clipId_;
    juce::String trackId_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipEditorWindow)
};

} // namespace zenith::ui
