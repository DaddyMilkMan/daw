/*
  ==============================================================================
    CollabPanel.h
    Skia-based Real-Time Collaboration UI Panel
  ==============================================================================
*/
#pragma once

#include "../../network/CollaborationManager.h"
#include "SkiaComponent.h"
#include <juce_core/juce_core.h>

namespace zenith {

class CollabPanel : public SkiaComponent, public juce::ChangeListener {
public:
    CollabPanel();
    ~CollabPanel() override;

    void changeListenerCallback(juce::ChangeBroadcaster *) override;
    void timerCallback() override;
    void drawSkia(SkCanvas *canvas) override;

    void mouseDown(const juce::MouseEvent &e) override;
    bool keyPressed(const juce::KeyPress &key) override;
    void resized() override;

private:
    enum class Field { None, Name, Code };
    Field activeField_ = Field::None;

    juce::String userName_ = "User";
    juce::String sessionCode_;

    SkRect nameInputBounds_;
    SkRect hostButtonBounds_;
    SkRect codeInputBounds_;
    SkRect joinButtonBounds_;
    SkRect disconnectButtonBounds_;

    void drawSection(SkCanvas *canvas, const char *title, float y, float w);
    void drawTextField(SkCanvas *canvas, const SkRect &bounds,
                     const juce::String &text, const char *placeholder,
                     bool focused);
    void drawButton(SkCanvas *canvas, const SkRect &bounds, const char *label,
                  bool active, SkColor accentColor);
    void drawStatusIndicator(SkCanvas *canvas,
                           CollaborationManager::ConnectionState state, float y,
                           float w);
};

} // namespace zenith
