/*
  ==============================================================================

    IdentityOverlay.cpp
    Created: 2025-12-20
    Author:  Zenith DAW Team

  ==============================================================================
*/

#include "IdentityOverlay.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ZenithLayout.h"
#include "../design-system/ZenithIcons.h"
#include "GlassmorphicPanel.h"

namespace zenith {

using namespace design;

IdentityOverlay::IdentityOverlay(std::function<void()> onDismiss)
    : onDismiss_(std::move(onDismiss)) {
  
  addChildComponent(&emailEditor_);
  addChildComponent(&usernameEditor_);
  addChildComponent(&passwordEditor_);

  emailEditor_.setTextToShowWhenEmpty("Email or Username", withAlpha(colors::TEXT_PRIMARY, 0.4f));
  usernameEditor_.setTextToShowWhenEmpty("Create Username", withAlpha(colors::TEXT_PRIMARY, 0.4f));
  passwordEditor_.setTextToShowWhenEmpty("Password", withAlpha(colors::TEXT_PRIMARY, 0.4f));
  passwordEditor_.setPasswordMode(true);

  // Styling
  auto fieldFont = getSkFont(16.0f, FontWeight::Regular);
  for (auto* ed : {&emailEditor_, &usernameEditor_, &passwordEditor_}) {
    ed->setFont(fieldFont);
    ed->setBackgroundColour(withAlpha(colors::BG_DARKEST, 0.5f));
  }

  titleFont_ = getSkFont(32.0f, FontWeight::Bold);
  subFont_ = getSkFont(16.0f, FontWeight::Regular);
  labelFont_ = getSkFont(14.0f, FontWeight::Medium);
  buttonFont_ = getSkFont(18.0f, FontWeight::Bold);

  setMode(Mode::Login);
}

IdentityOverlay::~IdentityOverlay() = default;

void IdentityOverlay::setMode(Mode mode) {
  mode_ = mode;
  usernameEditor_.setVisible(mode_ == Mode::Signup);
  updateLayout();
  repaint();
}

void IdentityOverlay::show() {
  alpha_.setTarget(1.0f, 400, AnimatedValue::EasingCurve::EaseOut);
  setVisible(true);
  startTimerHz(60);
}

void IdentityOverlay::hide() {
  alpha_.setTarget(0.0f, 300, AnimatedValue::EasingCurve::EaseIn);
}

void IdentityOverlay::drawSkia(SkCanvas *canvas) {
  float currentAlpha = alpha_.getCurrentValue();
  if (currentAlpha <= 0.001f) {
    if (alpha_.getTarget() == 0.0f) setVisible(false);
    return;
  }

  canvas->saveLayerAlpha(nullptr, (U8CPU)(currentAlpha * 255));
  
  // Backdrop Dim
  SkPaint dimPaint;
  dimPaint.setColor(SkColorSetARGB(180, 0, 0, 0));
  canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), dimPaint);

  // Main Container
  GlassmorphicPanel::draw(canvas, containerBounds_, GlassmorphicPanel::Style::Floating);

  // Title
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(colors::TEXT_PRIMARY);

  float ty = containerBounds_.fTop + 50.0f;
  canvas->drawString("Hi, welcome to Zenith", containerBounds_.centerX(), ty, titleFont_, textPaint);

  // Subtitle
  textPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.6f));
  juce::String subText = (mode_ == Mode::Login) ? "Sign in to access your cloud projects" : "Create an account to start your journey";
  canvas->drawString(subText.toRawUTF8(), containerBounds_.centerX(), ty + 30.0f, subFont_, textPaint);

  // Login Button
  GlassmorphicPanel::Options btnOpts;
  btnOpts.style = isLoginHovered_ ? GlassmorphicPanel::Style::ActiveGlow : GlassmorphicPanel::Style::Elevated;
  btnOpts.accentColor = colors::CYAN;
  btnOpts.cornerRadius = 8.0f;
  GlassmorphicPanel::drawWithOptions(canvas, loginButtonBounds_, btnOpts);
  
  textPaint.setColor(colors::TEXT_PRIMARY);
  juce::String btnText = (mode_ == Mode::Login) ? "Login" : "Create Account";
  canvas->drawString(btnText.toRawUTF8(), loginButtonBounds_.centerX(), loginButtonBounds_.centerY() + 6.0f, buttonFont_, textPaint);

  // Divider
  float dy = loginButtonBounds_.bottom() + 30.0f;
  GlassmorphicPanel::drawDivider(canvas, containerBounds_.fLeft + 40, dy, containerBounds_.fRight - 40);
  
  SkPaint circlePaint;
  circlePaint.setColor(colors::BG_DARK);
  circlePaint.setAntiAlias(true);
  canvas->drawCircle(containerBounds_.centerX(), dy, 15.0f, circlePaint);
  
  textPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.4f));
  canvas->drawString("or", containerBounds_.centerX(), dy + 5.0f, labelFont_, textPaint);

  // Google Button
  GlassmorphicPanel::Options gBtnOpts;
  gBtnOpts.style = isGoogleHovered_ ? GlassmorphicPanel::Style::ActiveGlow : GlassmorphicPanel::Style::Elevated;
  gBtnOpts.accentColor = colors::AMBER;
  gBtnOpts.cornerRadius = 8.0f;
  GlassmorphicPanel::drawWithOptions(canvas, googleButtonBounds_, gBtnOpts);

  icons::IconStyle gIconStyle;
  gIconStyle.color = isGoogleHovered_ ? colors::AMBER : colors::TEXT_PRIMARY;
  
  SkRect gIconRect = SkRect::MakeXYWH(googleButtonBounds_.fLeft + 12, googleButtonBounds_.fTop + 10, 20, 20);
  icons::drawIconCentered(canvas, icons::Google(), gIconRect, 18.0f, gIconStyle);
  
  textPaint.setColor(colors::TEXT_PRIMARY);
  textPaint.setTextSize(14.0f);
  canvas->drawString("Google", googleButtonBounds_.fLeft + 38, googleButtonBounds_.centerY() + 5.0f, labelFont_, textPaint);

  // Mode Switch Button (Next to Google)
  GlassmorphicPanel::Options mBtnOpts;
  mBtnOpts.style = isModeSwitchHovered_ ? GlassmorphicPanel::Style::ActiveGlow : GlassmorphicPanel::Style::Elevated;
  mBtnOpts.accentColor = colors::VIOLET;
  mBtnOpts.cornerRadius = 8.0f;
  GlassmorphicPanel::drawWithOptions(canvas, modeSwitchBounds_, mBtnOpts);

  textPaint.setColor(colors::TEXT_PRIMARY);
  juce::String switchLink = (mode_ == Mode::Login) ? "Sign Up" : "Sign In";
  canvas->drawString(switchLink.toRawUTF8(), modeSwitchBounds_.centerX(), modeSwitchBounds_.centerY() + 5.0f, labelFont_, textPaint);

  // Close Button
  icons::IconStyle closeStyle;
  closeStyle.color = isCloseHovered_ ? colors::NEON_PINK : colors::TEXT_SECONDARY;
  icons::drawIconCentered(canvas, icons::Close(), closeButtonBounds_, 20.0f, closeStyle);

  canvas->restore();
}

void IdentityOverlay::resized() {
  updateLayout();
}

void IdentityOverlay::updateLayout() {
  auto bounds = getLocalBounds().toFloat();
  float w = std::min(450.0f, bounds.getWidth() * 0.9f);
  float h = (mode_ == Mode::Login) ? 550.0f : 620.0f;
  
  containerBounds_ = SkRect::MakeXYWH((bounds.getWidth() - w) * 0.5f, (bounds.getHeight() - h) * 0.5f, w, h);
  
  float fieldW = w - 80.0f;
  float fieldH = 45.0f;
  float startY = containerBounds_.fTop + 120.0f;

  emailEditor_.setBounds(juce::Rectangle<int>(containerBounds_.fLeft + 40, startY, fieldW, fieldH));
  
  float nextY = startY + 60.0f;
  if (mode_ == Mode::Signup) {
    usernameEditor_.setBounds(juce::Rectangle<int>(containerBounds_.fLeft + 40, nextY, fieldW, fieldH));
    nextY += 60.0f;
  }
  
  passwordEditor_.setBounds(juce::Rectangle<int>(containerBounds_.fLeft + 40, nextY, fieldW, fieldH));
  
  loginButtonBounds_ = SkRect::MakeXYWH(containerBounds_.fLeft + 40, nextY + 60.0f, fieldW, 45.0f);
  
  float secondaryBtnW = (fieldW - 20.0f) * 0.5f;
  googleButtonBounds_ = SkRect::MakeXYWH(containerBounds_.fLeft + 40, loginButtonBounds_.bottom() + 40.0f, secondaryBtnW, 40.0f);
  modeSwitchBounds_ = SkRect::MakeXYWH(googleButtonBounds_.fRight + 20.0f, googleButtonBounds_.fTop, secondaryBtnW, 40.0f);
  
  closeButtonBounds_ = SkRect::MakeXYWH(containerBounds_.fRight - 35, containerBounds_.fTop + 15, 20, 20);
}

void IdentityOverlay::mouseDown(const juce::MouseEvent &e) {
  SkPoint pt = {(float)e.x, (float)e.y};
  
  if (loginButtonBounds_.contains(pt.fX, pt.fY)) {
    attemptAction();
  } else if (googleButtonBounds_.contains(pt.fX, pt.fY)) {
    attemptGoogleAuth();
  } else if (modeSwitchBounds_.contains(pt.fX, pt.fY)) {
    setMode(mode_ == Mode::Login ? Mode::Signup : Mode::Login);
  } else if (closeButtonBounds_.contains(pt.fX, pt.fY)) {
    hide();
  } else if (!containerBounds_.contains(pt.fX, pt.fY)) {
    hide();
  }
}

void IdentityOverlay::mouseMove(const juce::MouseEvent &e) {
  SkPoint pt = {(float)e.x, (float)e.y};
  
  isLoginHovered_ = loginButtonBounds_.contains(pt.fX, pt.fY);
  isGoogleHovered_ = googleButtonBounds_.contains(pt.fX, pt.fY);
  isModeSwitchHovered_ = modeSwitchBounds_.contains(pt.fX, pt.fY);
  isCloseHovered_ = closeButtonBounds_.contains(pt.fX, pt.fY);
  
  repaint();
}

void IdentityOverlay::attemptAction() {
  auto email = emailEditor_.getText();
  auto pass = passwordEditor_.getText();
  
  if (email.isEmpty() || pass.isEmpty()) return;
  
  if (mode_ == Mode::Login) {
    IdentityManager::getInstance().login(email, pass);
  } else {
    // Signup logic would go here
    IdentityManager::getInstance().login(email, pass); // Mocking for now
  }
  
  hide();
}

void IdentityOverlay::attemptGoogleAuth() {
  // Placeholder for real OAuth redirect
  IdentityManager::getInstance().login("google_user@gmail.com", "oauth_token");
  hide();
}

} // namespace zenith
