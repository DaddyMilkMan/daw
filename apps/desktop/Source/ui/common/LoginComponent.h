/*
  ==============================================================================

    LoginComponent.h
    Created: 2025-12-28
    Author:  Zenith DAW Team

    Premium login UI component with Sign In / Sign Up forms,
    Google OAuth button, and SylorLabs username/password authentication.

  ==============================================================================
*/

#pragma once

#include "SkiaComponent.h"
#include "../controls/SkiaTextEditor.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../../network/AuthenticationService.h"
#include <functional>

namespace zenith {

/**
 * @class LoginComponent
 * @brief Premium Skia-rendered login form with glassmorphism styling.
 *
 * Supports:
 * - Google OAuth "Sign in with Google" button
 * - SylorLabs username/password login
 * - Account creation (signup)
 * - Toggle between login and signup modes
 */
class LoginComponent : public SkiaComponent,
                       public AuthenticationService::Listener {
public:
    /**
     * @brief Callback when login succeeds and component should close
     */
    using LoginSuccessCallback = std::function<void(const AuthUser& user)>;
    
    explicit LoginComponent(LoginSuccessCallback onSuccess);
    ~LoginComponent() override;
    
    void drawSkia(SkCanvas* canvas) override;
    void resized() override;
    
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    
    // AuthenticationService::Listener
    void authStateChanged(bool isLoggedIn, const AuthUser& user) override;
    
private:
    LoginSuccessCallback onSuccess_;
    
    // UI State
    enum class Mode { SignIn, SignUp };
    Mode mode_ = Mode::SignIn;
    
    bool isLoading_ = false;
    juce::String errorMessage_;
    
    // Input fields
    std::unique_ptr<SkiaTextEditor> usernameField_;
    std::unique_ptr<SkiaTextEditor> emailField_;      // Only visible in signup mode
    std::unique_ptr<SkiaTextEditor> passwordField_;
    
    // Layout bounds (calculated in resized/drawSkia)
    SkRect panelBounds_;
    SkRect googleButtonBounds_;
    SkRect submitButtonBounds_;
    SkRect toggleLinkBounds_;
    
    // Hover states
    bool isGoogleHovered_ = false;
    bool isSubmitHovered_ = false;
    bool isToggleHovered_ = false;
    
    // Cached fonts/paints
    SkFont titleFont_;
    SkFont labelFont_;
    SkFont buttonFont_;
    SkFont linkFont_;
    SkPaint textPaint_;
    
    // Drawing helpers
    void drawGoogleButton(SkCanvas* canvas);
    void drawSubmitButton(SkCanvas* canvas);
    void drawToggleLink(SkCanvas* canvas);
    void drawError(SkCanvas* canvas);
    void drawLoadingSpinner(SkCanvas* canvas);
    
    // Actions
    void handleGoogleLogin();
    void handleSubmit();
    void toggleMode();
    
    void updateFieldVisibility();
    void layoutFields();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoginComponent)
};

} // namespace zenith
