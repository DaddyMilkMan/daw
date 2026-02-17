/*
  ==============================================================================

    LoginComponent.cpp
    Created: 2025-12-28
    Author:  Zenith DAW Team

    Premium login UI with glassmorphism and Skia rendering.

  ==============================================================================
*/

#include "LoginComponent.h"
#include "GlassmorphicPanel.h"
#include "ZenithIcons.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../../engine/ZenithLogger.h"

namespace zenith {

using namespace design;

//==============================================================================
LoginComponent::LoginComponent(LoginSuccessCallback onSuccess)
    : onSuccess_(std::move(onSuccess)) {
    
    setWantsKeyboardFocus(true);
    
    // Register for auth state changes
    if (auto* auth = AuthenticationService::getInstance()) {
        auth->addListener(this);
    }
    
    // Initialize cached fonts
    titleFont_ = design::getSkFont(28.0f, design::FontWeight::Bold);
    labelFont_ = design::getSkFont(14.0f, design::FontWeight::Medium);
    buttonFont_ = design::getSkFont(16.0f, design::FontWeight::Bold);
    linkFont_ = design::getSkFont(14.0f, design::FontWeight::Regular);
    
    textPaint_.setAntiAlias(true);
    textPaint_.setColor(colors::TEXT_PRIMARY);
    
    // Setup input fields with Skia controls
    auto setupField = [](SkiaTextEditor& field, const juce::String& placeholder) {
        field.setMultiLine(false);
        field.setTextToShowWhenEmpty(placeholder, SkColorSetRGB(100, 100, 110));
        field.setBackgroundColour(SkColorSetRGB(30, 30, 40));
        field.setTextColour(SkColorSetRGB(240, 240, 245));
    };

    usernameField_ = std::make_unique<SkiaTextEditor>("login_username");
    emailField_ = std::make_unique<SkiaTextEditor>("login_email");
    passwordField_ = std::make_unique<SkiaTextEditor>("login_password");
    setupField(*usernameField_, "Username or Email");
    setupField(*emailField_, "Email Address");
    setupField(*passwordField_, "Password");

    addAndMakeVisible(usernameField_.get());
    addChildComponent(emailField_.get()); // Hidden by default (only in signup)
    addAndMakeVisible(passwordField_.get());

    // Enter key submits form
    usernameField_->onReturnKey = [this]() {
        if (passwordField_) passwordField_->grabKeyboardFocus();
    };
    emailField_->onReturnKey = [this]() {
        if (passwordField_) passwordField_->grabKeyboardFocus();
    };
    passwordField_->onReturnKey = [this]() { handleSubmit(); };
    
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr)
        startTimerHz(60);
}

LoginComponent::~LoginComponent() {
    stopTimer();
    if (auto* auth = AuthenticationService::getInstance()) {
        auth->removeListener(this);
    }
}

void LoginComponent::authStateChanged(bool isLoggedIn, const AuthUser& user) {
    if (isLoggedIn) {
        isLoading_ = false;
        if (onSuccess_) {
            onSuccess_(user);
        }
    }
    repaint();
}

void LoginComponent::resized() {
    layoutFields();
}

void LoginComponent::layoutFields() {
    auto bounds = getLocalBounds().toFloat();
    float panelW = std::min(400.0f, bounds.getWidth() * 0.9f);
    float panelH = mode_ == Mode::SignUp ? 450.0f : 380.0f;
    float panelX = (bounds.getWidth() - panelW) / 2.0f;
    float panelY = (bounds.getHeight() - panelH) / 2.0f;
    
    panelBounds_ = SkRect::MakeXYWH(panelX, panelY, panelW, panelH);
    
    float fieldX = panelX + 30;
    float fieldW = panelW - 60;
    float fieldH = 40;
    
    // Layout from top of panel
    float yPos = panelY + 80; // After title
    
    // Google button
    googleButtonBounds_ = SkRect::MakeXYWH(fieldX, yPos, fieldW, 48);
    yPos += 68; // 48 + 20 gap
    
    // "or" divider takes space
    yPos += 30;
    
    // Username field
    if (usernameField_) usernameField_->setBounds((int)fieldX, (int)yPos, (int)fieldW, (int)fieldH);
    yPos += fieldH + 16;
    
    // Email field (signup only)
    if (mode_ == Mode::SignUp) {
        if (emailField_) emailField_->setBounds((int)fieldX, (int)yPos, (int)fieldW, (int)fieldH);
        yPos += fieldH + 16;
    }
    
    // Password field
    if (passwordField_) passwordField_->setBounds((int)fieldX, (int)yPos, (int)fieldW, (int)fieldH);
    yPos += fieldH + 24;
    
    // Submit button
    submitButtonBounds_ = SkRect::MakeXYWH(fieldX, yPos, fieldW, 48);
    yPos += 68;
    
    // Toggle link
    toggleLinkBounds_ = SkRect::MakeXYWH(fieldX, yPos, fieldW, 24);
    
    updateFieldVisibility();
}

void LoginComponent::updateFieldVisibility() {
    if (emailField_) emailField_->setVisible(mode_ == Mode::SignUp);
}

void LoginComponent::drawSkia(SkCanvas* canvas) {
    if (panelBounds_.isEmpty()) return;
    
    // Semi-transparent backdrop
    SkPaint backdropPaint;
    backdropPaint.setColor(SkColorSetARGB(180, 10, 10, 15));
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), backdropPaint);
    
    // Glass panel
    GlassmorphicPanel::draw(canvas, panelBounds_, GlassmorphicPanel::Style::Elevated);
    
    // Title
    textPaint_.setColor(colors::TEXT_PRIMARY);
    juce::String title = (mode_ == Mode::SignIn) ? "Sign In to Zenith" : "Create Your Account";
    
    SkRect titleBounds = SkRect::MakeXYWH(panelBounds_.fLeft + 30, panelBounds_.fTop + 25, 
                                          panelBounds_.width() - 60, 40);
    
    SkString skTitle(title.toRawUTF8());
    canvas->drawString(skTitle, titleBounds.fLeft, titleBounds.fTop + 30, titleFont_, textPaint_);
    
    // Google button
    drawGoogleButton(canvas);
    
    // "or" divider
    float dividerY = googleButtonBounds_.fBottom + 25;
    SkPaint linePaint;
    linePaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.2f));
    linePaint.setStrokeWidth(1.0f);
    
    float lineStartX = panelBounds_.fLeft + 30;
    float lineEndX = panelBounds_.fRight - 30;
    float lineMiddle = (lineStartX + lineEndX) / 2.0f;
    
    canvas->drawLine(lineStartX, dividerY, lineMiddle - 20, dividerY, linePaint);
    canvas->drawLine(lineMiddle + 20, dividerY, lineEndX, dividerY, linePaint);
    
    SkPaint orPaint = textPaint_;
    orPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.5f));
    canvas->drawString("or", lineMiddle - 8, dividerY + 5, labelFont_, orPaint);
    
    // Field labels
    SkPaint labelPaint = textPaint_;
    labelPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.7f));
    
    if (usernameField_) {
        canvas->drawString("Username or Email", (float)usernameField_->getX(), (float)usernameField_->getY() - 8.0f, labelFont_, labelPaint);
    }

    if (mode_ == Mode::SignUp && emailField_) {
        canvas->drawString("Email Address", (float)emailField_->getX(), (float)emailField_->getY() - 8.0f, labelFont_, labelPaint);
    }

    if (passwordField_) {
        canvas->drawString("Password", (float)passwordField_->getX(), (float)passwordField_->getY() - 8.0f, labelFont_, labelPaint);
    }
    
    // Submit button
    drawSubmitButton(canvas);
    
    // Toggle link
    drawToggleLink(canvas);
    
    // Error message
    if (errorMessage_.isNotEmpty()) {
        drawError(canvas);
    }
    
    // Loading spinner
    if (isLoading_) {
        drawLoadingSpinner(canvas);
    }
}

void LoginComponent::drawGoogleButton(SkCanvas* canvas) {
    SkRRect rrect = SkRRect::MakeRectXY(googleButtonBounds_, 8.0f, 8.0f);
    
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    bgPaint.setColor(isGoogleHovered_ ? SkColorSetRGB(255, 255, 255) : SkColorSetRGB(240, 240, 245));
    canvas->drawRRect(rrect, bgPaint);
    
    // Google "G" icon placeholder (use actual icon in production)
    float iconSize = 24;
    float iconX = googleButtonBounds_.fLeft + 16;
    float iconY = googleButtonBounds_.centerY() - iconSize / 2;
    
    SkPaint gPaint;
    gPaint.setColor(SkColorSetRGB(66, 133, 244)); // Google blue
    gPaint.setAntiAlias(true);
    canvas->drawCircle(iconX + iconSize / 2, iconY + iconSize / 2, iconSize / 2, gPaint);
    
    // Button text
    SkPaint textPaint;
    textPaint.setColor(SkColorSetRGB(60, 60, 60));
    textPaint.setAntiAlias(true);
    canvas->drawString("Sign in with Google", iconX + iconSize + 12, googleButtonBounds_.centerY() + 6, buttonFont_, textPaint);
}

void LoginComponent::drawSubmitButton(SkCanvas* canvas) {
    SkRRect rrect = SkRRect::MakeRectXY(submitButtonBounds_, 8.0f, 8.0f);
    
    // Gradient background
    SkPaint bgPaint;
    bgPaint.setAntiAlias(true);
    
    SkColor startColor = isSubmitHovered_ ? colors::CYAN : SkColorSetRGB(0, 150, 200);
    SkColor endColor = isSubmitHovered_ ? colors::BLUE : SkColorSetRGB(0, 100, 180);
    
    SkPoint pts[2] = {{submitButtonBounds_.fLeft, submitButtonBounds_.fTop},
                      {submitButtonBounds_.fRight, submitButtonBounds_.fBottom}};
    SkColor colors[2] = {startColor, endColor};
    bgPaint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));
    
    canvas->drawRRect(rrect, bgPaint);
    
    // Button text
    juce::String label = (mode_ == Mode::SignIn) ? "Sign In" : "Create Account";
    SkString skLabel(label.toRawUTF8());
    
    SkRect textBounds;
    buttonFont_.measureText(skLabel.c_str(), skLabel.size(), SkTextEncoding::kUTF8, &textBounds);
    
    float textX = submitButtonBounds_.centerX() - textBounds.width() / 2;
    float textY = submitButtonBounds_.centerY() + textBounds.height() / 3;
    
    SkPaint textPaint;
    textPaint.setColor(colors::TEXT_PRIMARY);
    textPaint.setAntiAlias(true);
    canvas->drawString(skLabel, textX, textY, buttonFont_, textPaint);
}

void LoginComponent::drawToggleLink(SkCanvas* canvas) {
    juce::String text = (mode_ == Mode::SignIn) 
        ? "Don't have an account? Sign up" 
        : "Already have an account? Sign in";
    
    SkPaint linkPaint;
    linkPaint.setColor(isToggleHovered_ ? colors::CYAN : withAlpha(colors::TEXT_PRIMARY, 0.6f));
    linkPaint.setAntiAlias(true);
    
    canvas->drawString(SkString(text.toRawUTF8()), toggleLinkBounds_.fLeft, toggleLinkBounds_.centerY() + 5, linkFont_, linkPaint);
}

void LoginComponent::drawError(SkCanvas* canvas) {
    SkPaint errorPaint;
    errorPaint.setColor(colors::DANGER);
    errorPaint.setAntiAlias(true);
    
    float errorY = submitButtonBounds_.fTop - 10;
    canvas->drawString(SkString(errorMessage_.toRawUTF8()), panelBounds_.fLeft + 30, errorY, labelFont_, errorPaint);
}

void LoginComponent::drawLoadingSpinner(SkCanvas* canvas) {
    // Simple rotating arc
    float cx = panelBounds_.centerX();
    float cy = panelBounds_.centerY();
    float radius = 20.0f;
    
    SkPaint spinnerPaint;
    spinnerPaint.setStyle(SkPaint::kStroke_Style);
    spinnerPaint.setStrokeWidth(3.0f);
    spinnerPaint.setColor(colors::CYAN);
    spinnerPaint.setAntiAlias(true);
    spinnerPaint.setStrokeCap(SkPaint::kRound_Cap);
    
    // Animate rotation based on timer
    float angle = fmod(juce::Time::getMillisecondCounter() / 5.0f, 360.0f);
    
    SkRect arcRect = SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2, radius * 2);
    canvas->drawArc(arcRect, angle, 270, false, spinnerPaint);
}

void LoginComponent::mouseMove(const juce::MouseEvent& e) {
    SkPoint pt = {(float)e.x, (float)e.y};
    
    bool gHover = googleButtonBounds_.contains(pt.fX, pt.fY);
    bool sHover = submitButtonBounds_.contains(pt.fX, pt.fY);
    bool tHover = toggleLinkBounds_.contains(pt.fX, pt.fY);
    
    if (gHover != isGoogleHovered_ || sHover != isSubmitHovered_ || tHover != isToggleHovered_) {
        isGoogleHovered_ = gHover;
        isSubmitHovered_ = sHover;
        isToggleHovered_ = tHover;
        repaint();
    }
}

void LoginComponent::mouseDown(const juce::MouseEvent& e) {
    SkPoint pt = {(float)e.x, (float)e.y};
    
    if (googleButtonBounds_.contains(pt.fX, pt.fY)) {
        handleGoogleLogin();
        return;
    }
    
    if (submitButtonBounds_.contains(pt.fX, pt.fY)) {
        handleSubmit();
        return;
    }
    
    if (toggleLinkBounds_.contains(pt.fX, pt.fY)) {
        toggleMode();
        return;
    }
}

void LoginComponent::mouseUp(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
}

void LoginComponent::handleGoogleLogin() {
    if (isLoading_) return;
    
    ZENITH_LOG_INFO("[LoginUI] Google login clicked");
    isLoading_ = true;
    errorMessage_.clear();
    repaint();
    
    if (auto* auth = AuthenticationService::getInstance()) {
        auth->loginWithGoogle([this](bool success, juce::String error) {
            juce::MessageManager::callAsync([this, success, error]() {
                isLoading_ = false;
                if (!success) {
                    errorMessage_ = error.isEmpty() ? "Google login failed" : error;
                }
                repaint();
            });
        });
    }
}

void LoginComponent::handleSubmit() {
    if (isLoading_) return;
    
    juce::String username = usernameField_ ? usernameField_->getText().trim() : juce::String();
    juce::String email = emailField_ ? emailField_->getText().trim() : juce::String();
    juce::String password = passwordField_ ? passwordField_->getText() : juce::String();
    
    errorMessage_.clear();
    
    if (username.isEmpty()) {
        errorMessage_ = "Username is required";
        repaint();
        return;
    }
    
    if (password.isEmpty()) {
        errorMessage_ = "Password is required";
        repaint();
        return;
    }
    
    isLoading_ = true;
    repaint();
    
    if (mode_ == Mode::SignIn) {
        ZENITH_LOG_INFO("[LoginUI] SylorLabs login for: " + username);
        if (auto* auth = AuthenticationService::getInstance()) {
            auth->loginWithSylorLabs(username, password, 
                [this](bool success, juce::String error) {
                    juce::MessageManager::callAsync([this, success, error]() {
                        isLoading_ = false;
                        if (!success) {
                            errorMessage_ = error.isEmpty() ? "Login failed" : error;
                        }
                        repaint();
                    });
                });
        }
    } else {
        // Signup
        if (email.isEmpty() || !email.contains("@")) {
            isLoading_ = false;
            errorMessage_ = "Valid email is required";
            repaint();
            return;
        }
        
        ZENITH_LOG_INFO("[LoginUI] SylorLabs signup for: " + username);
        if (auto* auth = AuthenticationService::getInstance()) {
            auth->signupWithSylorLabs(username, email, password,
                [this](bool success, juce::String error) {
                    juce::MessageManager::callAsync([this, success, error]() {
                        isLoading_ = false;
                        if (!success) {
                            errorMessage_ = error.isEmpty() ? "Signup failed" : error;
                        }
                        repaint();
                    });
                });
        }
    }
}

void LoginComponent::toggleMode() {
    mode_ = (mode_ == Mode::SignIn) ? Mode::SignUp : Mode::SignIn;
    errorMessage_.clear();
    layoutFields();
    repaint();
}

} // namespace zenith
