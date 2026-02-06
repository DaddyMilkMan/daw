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

#pragma once

#include "SkiaComponent.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../../network/AuthenticationService.h"
#include <juce_gui_basics/juce_gui_basics.h>
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
    juce::TextEditor usernameField_;
    juce::TextEditor emailField_;      // Only visible in signup mode
    juce::TextEditor passwordField_;
    
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
