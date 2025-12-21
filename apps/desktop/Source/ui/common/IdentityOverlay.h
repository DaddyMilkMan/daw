/*
  ==============================================================================

    IdentityOverlay.h
    Created: 2025-12-20
    Author:  Zenith DAW Team

    Native Skia-based identity overlay for Login and Signup.

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include "../../network/IdentityManager.h"
#include "../widgets/SkiaTextEditor.h"
#include <memory>

namespace zenith {

class IdentityOverlay : public SkiaComponent {
public:
  enum class Mode { Login, Signup };

  explicit IdentityOverlay(std::function<void()> onDismiss);
  ~IdentityOverlay() override;

  void setMode(Mode mode);
  void show();
  void hide();

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;

private:
  Mode mode_ = Mode::Login;
  std::function<void()> onDismiss_;

  // UI State
  AnimatedValue alpha_{0.0f};
  
  // Widgets
  SkiaTextEditor emailEditor_;
  SkiaTextEditor usernameEditor_;
  SkiaTextEditor passwordEditor_;

  // Layout
  SkRect containerBounds_;
  SkRect loginButtonBounds_;
  SkRect googleButtonBounds_;
  SkRect modeSwitchBounds_;
  SkRect closeButtonBounds_;

  // Interaction
  bool isLoginHovered_ = false;
  bool isGoogleHovered_ = false;
  bool isModeSwitchHovered_ = false;
  bool isCloseHovered_ = false;

  // Fonts
  SkFont titleFont_;
  SkFont subFont_;
  SkFont labelFont_;
  SkFont buttonFont_;

  void updateLayout();
  void attemptAction();
  void attemptGoogleAuth();

  JU_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IdentityOverlay)
};

} // namespace zenith
