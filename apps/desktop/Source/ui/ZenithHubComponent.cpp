/*
  ==============================================================================

    ZenithHubComponent.cpp
    Created: 2025-12-13
    Author:  Zenith DAW Team

  ==============================================================================
*/

#include "ZenithHubComponent.h"
#include "skia/ZenithIcons.h"
#include <array>
#include <cmath>
#include <cstdlib>

namespace zenith {

using namespace design;

ZenithHubComponent::ZenithHubComponent(std::function<void()> onDismiss)
    : onDismiss_(std::move(onDismiss)) {
  setWantsKeyboardFocus(true);
  createMockData();

  // Initialize Aurora Background
  auroraBackground_ = std::make_unique<AuroraBackground>();

  // Start fade-in
  alpha_.setTarget(0.0f, 0); // Start invisible
  alpha_.setTarget(1.0f, 600, AnimatedValue::EasingCurve::EaseOut); // Fade in

  startTimerHz(60);
}

ZenithHubComponent::~ZenithHubComponent() { stopTimer(); }

void ZenithHubComponent::createMockData() {
  // Mock Recent Projects
  recentProjects_ = {
      {"Cyberpunk City", "2 hours ago", "Electronic", colors::CYAN, {}, false},
      {"Orchestral Suite No. 1",
       "Yesterday",
       "Cinematic",
       colors::VIOLET,
       {},
       false},
      {"Late Night Jazz", "3 days ago", "Jazz", colors::NEON_PINK, {}, false},
      {"Techno Bunker", "1 week ago", "Techno", colors::NEON_GREEN, {}, false},
      {"Ambient Dreams", "2 weeks ago", "Ambient", colors::BLUE, {}, false},
      {"Rock Anthem", "1 month ago", "Rock", colors::AMBER, {}, false}};

  // Generate mock waveform data for each project
  for (auto &proj : recentProjects_) {
    proj.waveform.clear();
    // Generate 20-30 bars of random heights
    int numBars = 20 + (std::rand() % 10);
    for (int i = 0; i < numBars; ++i) {
      // Values between 0.2 and 1.0 for visual interest
      float h = 0.2f + (static_cast<float>(std::rand()) / RAND_MAX) * 0.8f;
      proj.waveform.push_back(h);
    }
  }

  // Mock Templates
  templates_ = {{"Electronic", "icon_synth", colors::CYAN, {}, false},
                {"Orchestral", "icon_note", colors::VIOLET, {}, false},
                {"Recording", "icon_mic", colors::NEON_PINK, {}, false}};
}

void ZenithHubComponent::resized() { updateLayout(); }

void ZenithHubComponent::updateLayout() {
  auto bounds = getLocalBounds().toFloat();
  float w = bounds.getWidth();
  float h = bounds.getHeight();

  // Main Glass Card in Center
  float cardW = std::min(1000.0f, w * 0.9f);
  float cardH = std::min(700.0f, h * 0.85f);

  float cardX = (w - cardW) * 0.5f;
  float cardY = (h - cardH) * 0.5f;

  mainCardBounds_ = SkRect::MakeXYWH(cardX, cardY, cardW, cardH);

  // Internal Layout
  float padding = 40.0f; // Generous padding
  float gridGap = 40.0f; // Requested 40px grid gap

  float availableW = cardW - (padding * 2);
  float colOneW = (availableW - gridGap) * 0.6f; // 60% for Recent
  float colTwoW = (availableW - gridGap) * 0.4f; // 40% for Sidebar

  // Recent Projects Area
  // Header consumes significant vertical space now due to large title
  float headerHeight = 140.0f;

  recentArea_ =
      SkRect::MakeXYWH(cardX + padding, cardY + padding + headerHeight, colOneW,
                       cardH - (padding * 2) - headerHeight);

  // Sidebar (Account + Templates)
  float sidebarX = cardX + padding + colOneW + gridGap;

  // Account (Top Right aligned with recent area top)
  float accountH = 100.0f;
  accountArea_ =
      SkRect::MakeXYWH(sidebarX, recentArea_.fTop, colTwoW, accountH);

  // New Project Button (Prominent Neon Button)
  float buttonH = 80.0f;
  newProjectButtonBounds_ = SkRect::MakeXYWH(
      sidebarX, accountArea_.bottom() + padding, colTwoW, buttonH);

  // Templates (Bottom Right)
  templatesArea_ = SkRect::MakeXYWH(
      sidebarX, newProjectButtonBounds_.bottom() + padding, colTwoW,
      recentArea_.bottom() - (newProjectButtonBounds_.bottom() + padding));

  // Update Recent Project Cards Layout (Grid)
  float gridW = recentArea_.width();
  float cardGap = 20.0f;                   // Gap between cards themselves
  float pCardW = (gridW - cardGap) / 2.0f; // 2 columns
  float pCardH = 110.0f;

  for (size_t i = 0; i < recentProjects_.size(); ++i) {
    int row = (int)i / 2;
    int col = (int)i % 2;

    float px = recentArea_.fLeft + (col * (pCardW + cardGap));
    float py = recentArea_.fTop + (row * (pCardH + cardGap));

    recentProjects_[i].bounds = SkRect::MakeXYWH(px, py, pCardW, pCardH);
  }

  // Update Template Cards Layout (List)
  float tCardH = 80.0f;
  for (size_t i = 0; i < templates_.size(); ++i) {
    float tx = templatesArea_.fLeft;
    float ty = templatesArea_.fTop + 40.0f +
               (i * (tCardH + cardGap)); // +40 for header
    templates_[i].bounds =
        SkRect::MakeXYWH(tx, ty, templatesArea_.width(), tCardH);
  }

  // Profile Button Bounds (Centered in account area)
  profileBounds_ =
      SkRect::MakeXYWH(accountArea_.fLeft, accountArea_.fTop + 20,
                       accountArea_.width(), 60.0f); // Badge height
}

void ZenithHubComponent::timerCallback() {
  animationTime_ += 0.016f; // ~60fps
  alpha_.update(16.0f);

  // Update Tilt Physics
  tiltX_.update();
  tiltY_.update();

  // Update Spring Physics for Projects
  bool needsRepaint = alpha_.isAnimating() ||
                      (std::abs(tiltX_.velocity) > 0.001f) ||
                      (std::abs(tiltY_.velocity) > 0.001f);

  for (auto &proj : recentProjects_) {
    proj.scaleSpring.update();
    if (std::abs(proj.scaleSpring.velocity) > 0.001f)
      needsRepaint = true;
  }

  for (auto &tmpl : templates_) {
    tmpl.scaleSpring.update();
    if (std::abs(tmpl.scaleSpring.velocity) > 0.001f)
      needsRepaint = true;
  }

  // Update Button Animation
  buttonGradientAngle_ += 2.0f; // Continuous rotation
  if (buttonGradientAngle_ >= 360.0f)
    buttonGradientAngle_ -= 360.0f;

  // Update Ripples
  for (int i = buttonRipples_.size() - 1; i >= 0; --i) {
    auto &r = buttonRipples_[i];
    r.radius += 5.0f;   // Expansion speed
    r.opacity -= 0.03f; // Fade speed

    if (r.opacity <= 0.0f) {
      buttonRipples_.erase(buttonRipples_.begin() + i);
    } else {
      needsRepaint = true;
    }
  }

  // Always repaint if button is visible/hovered to animate gradient?
  // Optimization: only if hovered or ripples active.
  if (isNewProjectHovered_ || !buttonRipples_.empty())
    needsRepaint = true;

  if (needsRepaint) {
    repaint();
  }
}

void ZenithHubComponent::show() {
  setVisible(true);
  alpha_.setTarget(1.0f, 600, AnimatedValue::EasingCurve::EaseOut);
}

void ZenithHubComponent::dismiss() {
  alpha_.setTarget(0.0f, 400, AnimatedValue::EasingCurve::EaseIn);
  // When alpha reaches 0, we should really hide the component, but we'll handle
  // that in draw or logic. For now, let the owner handle destruction or hiding
  // if they monitor alpha, but simplified: we trigger callback immediately or
  // after delay.

  if (onDismiss_) {
    // Delay callback slightly to allow fade out?
    // Or just let MainWindow handle it.
    // We can use a lambda in timer if we wanted to be fancy.
    onDismiss_();
  }
}

void ZenithHubComponent::drawSkia(SkCanvas *canvas) {
  float opacity = alpha_.getCurrentValue();
  if (opacity <= 0.001f)
    return;

  // Save layer for global opacity
  canvas->saveLayerAlpha(nullptr, (U8CPU)(opacity * 255));

  drawBackground(canvas);

  // Main Glass Card
  // Apply Parallax Tilt
  canvas->save();

  // Pivot around center of card
  float cx = mainCardBounds_.centerX();
  float cy = mainCardBounds_.centerY();

  SkM44 mat = SkM44::Translate(cx, cy, 0);
  mat = mat * SkM44::Rotate({1, 0, 0}, tiltX_.current); // Tilt X
  mat =
      mat * SkM44::Rotate({0, 1, 0},
                          -tiltY_.current); // Tilt Y (negate for natural feel?)
  mat = mat * SkM44::Translate(-cx, -cy, 0); // Move back

  canvas->concat(mat);

  GlassmorphicPanel::draw(canvas, mainCardBounds_,
                          GlassmorphicPanel::Style::Floating);

  // Header - Massive & Editorial (120pt Display Font)
  {
    SkFont titleFont = design::getSkFont(120.0f, design::FontWeight::Bold);
    SkPaint paint;
    paint.setAntiAlias(true);

    // Linear gradient texture for the text
    const std::array<SkPoint, 2> pts = {
        SkPoint::Make(mainCardBounds_.fLeft, mainCardBounds_.fTop),
        SkPoint::Make(mainCardBounds_.fRight, mainCardBounds_.fTop + 100)};
    const std::array<SkColor, 2> gradColors = {colors::NEON_CYAN,
                                               colors::MAGENTA};

    // Mask shader into text
    paint.setShader(SkGradientShader::MakeLinear(
        pts.data(), gradColors.data(), nullptr, 2, SkTileMode::kClamp));

    SkRect titleBounds =
        SkRect::MakeXYWH(mainCardBounds_.fLeft + 40, mainCardBounds_.fTop + 40,
                         mainCardBounds_.width(), 110);

    // Draw Title
    canvas->drawString("Zenith Hub", titleBounds.fLeft, titleBounds.bottom(),
                       titleFont, paint);

    // Subtitle / User Greeting
    SkFont subFont = design::getSkFont(18.0f, design::FontWeight::Medium);
    paint.setShader(nullptr); // Reset shader for plain text
    paint.setColor(withAlpha(colors::TEXT_SECONDARY, 0.8f));

    // Align with baseline or slightly offset
    canvas->drawString("Welcome back, User", titleBounds.fLeft + 5,
                       titleBounds.bottom() + 30, subFont, paint);
  }

  drawRecentProjects(canvas);
  drawAccount(canvas);
  drawNewProjectButton(canvas);
  drawTemplates(canvas);

  canvas->restore(); // Restore from parallax transform

  canvas->restore(); // Restore layer
}

void ZenithHubComponent::drawBackground(SkCanvas *canvas) {
  // Use "Living" Aurora Mesh Gradient Background
  auto bounds = getLocalBounds().toFloat();
  SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // Draw Aurora Background (replaces simple moving circles)
  if (auroraBackground_) {
    auroraBackground_->draw(canvas, rect, animationTime_);
  } else {
    // Fallback: solid dark background
    SkPaint bgPaint;
    bgPaint.setColor(colors::BG_DARKEST);
    canvas->drawRect(rect, bgPaint);
  }
}

void ZenithHubComponent::drawRecentProjects(SkCanvas *canvas) {
  SkFont headerFont = design::getSkFont(20.0f, design::FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  canvas->drawString("Recent Projects", recentArea_.fLeft,
                     recentArea_.fTop - 15, headerFont, textPaint);

  for (const auto &proj : recentProjects_) {
    // Save for scale animation
    canvas->save();
    float s = proj.scaleSpring.current;
    if (s != 1.0f) {
      float pivotX = proj.bounds.centerX();
      float pivotY = proj.bounds.centerY();
      canvas->translate(pivotX, pivotY);
      canvas->scale(s, s);
      canvas->translate(-pivotX, -pivotY);
    }

    // Card Background
    SkRRect rrect = SkRRect::MakeRectXY(proj.bounds, 8.0f, 8.0f);

    SkPaint cardPaint;
    cardPaint.setColor(proj.isHovered ? withAlpha(colors::BG_LIGHT, 0.2f)
                                      : withAlpha(colors::BG_LIGHT, 0.05f));
    cardPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, cardPaint);

    if (proj.isHovered) {
      // Glow border
      SkPaint borderPaint;
      borderPaint.setStyle(SkPaint::kStroke_Style);
      borderPaint.setStrokeWidth(1.0f);
      borderPaint.setColor(withAlpha(proj.accent, 0.6f));
      canvas->drawRRect(rrect, borderPaint);
    }

    SkRect imageRect =
        SkRect::MakeXYWH(proj.bounds.fLeft + 10, proj.bounds.fTop + 10, 80, 80);

    SkPaint imgBgPaint;
    imgBgPaint.setColor(withAlpha(proj.accent, 0.1f));
    imgBgPaint.setAntiAlias(true);
    canvas->drawRoundRect(imageRect, 8.0f, 8.0f, imgBgPaint);

    // Draw Waveform Thumbnail Bars behind icon
    if (!proj.waveform.empty()) {
      SkPaint barPaint;
      barPaint.setAntiAlias(true);

      float barAreaWidth = imageRect.width() - 10.0f;
      float barWidth = barAreaWidth / proj.waveform.size();
      float maxBarHeight = imageRect.height() * 0.6f;
      float barY = imageRect.centerY();

      for (size_t i = 0; i < proj.waveform.size(); ++i) {
        float h = proj.waveform[i] * maxBarHeight;
        float x = imageRect.fLeft + 5.0f + (i * barWidth);

        // Gradient from accent to darker
        float t = static_cast<float>(i) / proj.waveform.size();
        SkColor barColor =
            interpolateColor(proj.accent, darken(proj.accent, 0.4f), t);
        barPaint.setColor(withAlpha(barColor, 0.5f));

        // Draw bar centered vertically
        SkRect barRect =
            SkRect::MakeXYWH(x, barY - h * 0.5f, barWidth * 0.8f, h);
        canvas->drawRoundRect(barRect, 1.0f, 1.0f, barPaint);
      }
    }

    // Determines Icon
    SkPath iconPath = icons::Project(); // Default
    if (proj.genre == "Electronic" || proj.genre == "Techno")
      iconPath = icons::Synth();
    else if (proj.genre == "Cinematic" || proj.genre == "Orchestral" ||
             proj.genre == "Jazz")
      iconPath = icons::MusicNote();
    else if (proj.genre == "Ambient")
      iconPath = icons::Cloud();
    else if (proj.genre == "Rock")
      iconPath = icons::Waveform();

    // Draw Vector Icon with Glow
    icons::IconStyle style;
    style.color = proj.accent;
    style.strokeWidth = 1.6f;
    style.glowRadius = proj.isHovered ? 12.0f : 0.0f; // Glow on hover
    style.glowColor = proj.accent;

    icons::drawIconCentered(canvas, iconPath, imageRect, 40.0f, style);

    // === WAVEFORM THUMBNAIL ===
    // Render abstract colored bars behind project text for visual weight
    float waveformStartX = imageRect.right() + 15;
    float waveformY = proj.bounds.fTop + 35;
    float waveformWidth = proj.bounds.right() - waveformStartX - 15;
    float waveformHeight = 40.0f;

    if (!proj.waveform.empty()) {
      float barWidth = waveformWidth / (float)proj.waveform.size();
      float barGap = 1.0f;

      SkPaint wavePaint;
      wavePaint.setAntiAlias(true);

      for (size_t i = 0; i < proj.waveform.size(); ++i) {
        float h = proj.waveform[i] * waveformHeight;
        float x = waveformStartX + i * barWidth;
        float y = waveformY + (waveformHeight - h) * 0.5f; // Center vertically

        // Gradient alpha based on position (fade at edges)
        float edgeFade = 1.0f;
        if (i < 3)
          edgeFade = (i + 1) / 4.0f;
        else if (i >= proj.waveform.size() - 3)
          edgeFade = (proj.waveform.size() - i) / 4.0f;

        float alpha = proj.isHovered ? 0.35f : 0.15f;
        wavePaint.setColor(withAlpha(proj.accent, alpha * edgeFade));

        SkRect barRect = SkRect::MakeXYWH(x, y, barWidth - barGap, h);
        canvas->drawRoundRect(barRect, 1.5f, 1.5f, wavePaint);
      }
    }

    // === TEXT (drawn on top of waveform) ===
    SkFont titleFont = design::getSkFont(16.0f, design::FontWeight::Bold);
    textPaint.setColor(colors::TEXT_PRIMARY);
    canvas->drawString(proj.name.toStdString().c_str(), waveformStartX,
                       proj.bounds.fTop + 28, titleFont, textPaint);

    SkFont subFont = design::getSkFont(13.0f, design::FontWeight::Regular);
    textPaint.setColor(colors::TEXT_SECONDARY);
    canvas->drawString(proj.date.toStdString().c_str(), waveformStartX,
                       proj.bounds.fTop + 90, subFont, textPaint);

    // Genre Badge - positioned below waveform
    SkPaint badgeBgPaint;
    badgeBgPaint.setColor(withAlpha(proj.accent, 0.15f));
    badgeBgPaint.setAntiAlias(true);
    SkRect badgeRect =
        SkRect::MakeXYWH(waveformStartX, proj.bounds.fTop + 75,
                         std::min(80.0f, waveformWidth * 0.4f), 18);
    canvas->drawRoundRect(badgeRect, 4.0f, 4.0f, badgeBgPaint);

    // Badge Text
    SkFont badgeFont = design::getSkFont(11.0f, design::FontWeight::Medium);
    SkPaint badgeTextPaint;
    badgeTextPaint.setColor(proj.accent);
    badgeTextPaint.setAntiAlias(true);
    canvas->drawString(proj.genre.toStdString().c_str(), badgeRect.fLeft + 8,
                       badgeRect.centerY() + 4, badgeFont, badgeTextPaint);

    canvas->restore(); // Restore scale
  }
}

void ZenithHubComponent::drawTemplates(SkCanvas *canvas) {
  SkFont headerFont = design::getSkFont(20.0f, design::FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  canvas->drawString("Quick Start", templatesArea_.fLeft,
                     templatesArea_.fTop - 15, headerFont,
                     textPaint); // Adjusted y

  for (const auto &tmpl : templates_) {
    canvas->save();
    float s = tmpl.scaleSpring.current;
    if (s != 1.0f) {
      float pivotX = tmpl.bounds.centerX();
      float pivotY = tmpl.bounds.centerY();
      canvas->translate(pivotX, pivotY);
      canvas->scale(s, s);
      canvas->translate(-pivotX, -pivotY);
    }

    SkRRect rrect = SkRRect::MakeRectXY(tmpl.bounds, 8.0f, 8.0f);

    SkPaint cardPaint;
    cardPaint.setColor(tmpl.isHovered ? withAlpha(tmpl.color, 0.2f)
                                      : withAlpha(colors::BG_LIGHT, 0.05f));
    cardPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, cardPaint);

    // Resolve Icon
    SkPath iconPath = icons::Template(); // Default
    if (tmpl.icon == "icon_synth")
      iconPath = icons::Synth();
    else if (tmpl.icon == "icon_note")
      iconPath = icons::MusicNote();
    else if (tmpl.icon == "icon_mic")
      iconPath = icons::Microphone();

    // Draw Icon
    icons::IconStyle style;
    style.color = tmpl.color;
    style.strokeWidth = 2.0f;
    style.glowRadius = tmpl.isHovered ? 8.0f : 0.0f;
    style.glowColor = tmpl.color;

    // Use a fixed size square for icon
    SkRect iconBounds = SkRect::MakeXYWH(tmpl.bounds.fLeft + 20,
                                         tmpl.bounds.centerY() - 15, 30, 30);
    icons::drawIconCentered(canvas, iconPath, iconBounds, 24.0f, style);

    // Text
    textPaint.setColor(colors::TEXT_PRIMARY);
    SkFont nameFont = design::getSkFont(16.0f, design::FontWeight::Medium);
    canvas->drawString(tmpl.name.toStdString().c_str(), iconBounds.right() + 15,
                       tmpl.bounds.centerY() + 6, nameFont, textPaint);

    if (tmpl.isHovered) {
      SkPaint borderPaint;
      borderPaint.setStyle(SkPaint::kStroke_Style);
      borderPaint.setStrokeWidth(1.0f);
      borderPaint.setColor(withAlpha(tmpl.color, 0.8f));
      canvas->drawRRect(rrect, borderPaint);
    }

    canvas->restore();
  }
}

void ZenithHubComponent::drawAccount(SkCanvas *canvas) {
  // Redesigned to look like a "Badge" floating on the glass

  SkFont headerFont = design::getSkFont(20.0f, design::FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  // Use profileBounds_ directly for the badge
  SkRRect badgeRRect = SkRRect::MakeRectXY(profileBounds_, 30.0f,
                                           30.0f); // Fully rounded/Capsule

  // Floating Glass Badge effect
  SkPaint badgePaint;
  // Subtle gradient or solid with blur
  badgePaint.setColor(isProfileHovered_ ? withAlpha(colors::BG_LIGHT, 0.4f)
                                        : withAlpha(colors::BG_LIGHT, 0.2f));
  badgePaint.setAntiAlias(true);

  // Draw the badge background
  // (Note: Skia doesn't have setShadowLayer like Android, shadow effect would
  // need blur filter)

  canvas->drawRRect(badgeRRect, badgePaint);

  // Border for definition
  SkPaint strokePaint;
  strokePaint.setStyle(SkPaint::kStroke_Style);
  strokePaint.setStrokeWidth(1.0f);
  strokePaint.setColor(SkColorSetA(SK_ColorWHITE, 25));
  strokePaint.setAntiAlias(true);
  canvas->drawRRect(badgeRRect, strokePaint);

  // Avatar (Circle on left)
  float avatarSize = 40.0f;
  float avatarX = profileBounds_.fLeft + 10.0f;
  float avatarY = profileBounds_.centerY();

  SkPaint avatarPaint;
  avatarPaint.setColor(colors::AMBER);
  avatarPaint.setAntiAlias(true);
  canvas->drawCircle(avatarX + avatarSize / 2, avatarY, avatarSize / 2,
                     avatarPaint);

  // Text Info
  float textStartX = avatarX + avatarSize + 15.0f;

  SkFont nameFont = design::getSkFont(16.0f, design::FontWeight::Bold);
  textPaint.setColor(colors::TEXT_PRIMARY);
  canvas->drawString("SoundDesigner99", textStartX,
                     profileBounds_.centerY() + 6, nameFont, textPaint);

  // Online Indicator (Subtle dot on avatar?)
  SkPaint statusPaint;
  statusPaint.setColor(colors::NEON_GREEN);
  statusPaint.setAntiAlias(true);
  canvas->drawCircle(avatarX + avatarSize - 2, avatarY + avatarSize / 2 - 2, 5,
                     statusPaint);
}

void ZenithHubComponent::drawNewProjectButton(SkCanvas *canvas) {
  SkRRect rrect = SkRRect::MakeRectXY(newProjectButtonBounds_, 12.0f, 12.0f);

  // Clip to button shape for ripples
  canvas->save();
  canvas->clipRRect(rrect, true);

  // Neon Gradient Background
  SkPaint btnPaint;
  btnPaint.setAntiAlias(true);

  // Animated gradient based on angle
  float angleRad = buttonGradientAngle_ * (3.14159f / 180.0f);
  float cx = newProjectButtonBounds_.centerX();
  float cy = newProjectButtonBounds_.centerY();
  float r = newProjectButtonBounds_.width() * 0.7f;

  // Start/End points based on angle
  SkPoint start = {cx + cos(angleRad) * r, cy + sin(angleRad) * r};
  SkPoint end = {cx - cos(angleRad) * r, cy - sin(angleRad) * r};

  const std::array<SkPoint, 2> pts = {start, end};

  auto gradientColors =
      isNewProjectHovered_
          ? std::array<SkColor, 2>{colors::NEON_CYAN, colors::MAGENTA}
          : std::array<SkColor, 2>{colors::CYAN, colors::VIOLET};

  auto shader =
      SkGradientShader::MakeLinear(pts.data(), gradientColors.data(), nullptr,
                                   gradientColors.size(), SkTileMode::kClamp);
  btnPaint.setShader(shader);

  // Drop Shadow / Glow
  if (isNewProjectHovered_) {
    btnPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
    // Draw glow pass
    canvas->drawRRect(rrect, btnPaint);
    btnPaint.setMaskFilter(nullptr); // Reset for main body
  }

  canvas->drawRRect(rrect, btnPaint);

  // Draw Ripples
  SkPaint ripplePaint;
  ripplePaint.setColor(SK_ColorWHITE);
  ripplePaint.setAntiAlias(true);
  for (const auto &rip : buttonRipples_) {
    if (rip.opacity > 0) {
      ripplePaint.setAlphaf(rip.opacity * 0.3f); // Max 0.3
      canvas->drawCircle(rip.x, rip.y, rip.radius, ripplePaint);
    }
  }

  // Text
  SkFont btnFont = design::getSkFont(24.0f, design::FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(SK_ColorWHITE); // Start white
  textPaint.setAntiAlias(true);

  // Add subtle shadow to text
  // Shadow effect removed - Skia doesn't have setShadowLayer
  // Use MaskFilter for glow/shadow effects if needed

  // Center text
  SkString text("New Project");
  SkRect textBounds;
  btnFont.measureText(text.c_str(), text.size(), SkTextEncoding::kUTF8,
                      &textBounds);

  float tx = newProjectButtonBounds_.centerX() - (textBounds.width() / 2.0f);
  float ty =
      newProjectButtonBounds_.centerY() + (textBounds.height() / 2.0f) - 4.0f;

  canvas->drawString(text, tx, ty, btnFont, textPaint);

  // Maybe put icon to the left of text?
  // width: 24, height 24
  // For now simple text is clear enough or I can add a plus sign.

  canvas->restore();
}

void ZenithHubComponent::mouseMove(const juce::MouseEvent &e) {
  SkPoint pt = {(float)e.x, (float)e.y};
  bool needsUpdate = false;

  // Calculate Parallax Tilt
  float cx = mainCardBounds_.centerX();
  float cy = mainCardBounds_.centerY();

  // Max tilt in degrees
  float maxTilt = 2.0f;
  float targetX = -((pt.fY - cy) / (mainCardBounds_.height() * 0.5f)) * maxTilt;
  float targetY = -((pt.fX - cx) / (mainCardBounds_.width() * 0.5f)) * maxTilt;

  // Clamp
  targetX = std::clamp(targetX, -maxTilt, maxTilt);
  targetY = std::clamp(targetY, -maxTilt, maxTilt);

  // Update targets
  tiltX_.target = targetX;
  tiltY_.target = targetY;

  // Check Recent Projects
  for (auto &proj : recentProjects_) {
    bool h = proj.bounds.contains(pt.fX, pt.fY);
    if (h != proj.isHovered) {
      proj.isHovered = h;
      proj.scaleSpring.target = h ? 1.02f : 1.0f;
    }
  }

  // Check Templates
  for (auto &tmpl : templates_) {
    bool h = tmpl.bounds.contains(pt.fX, pt.fY);
    if (h != tmpl.isHovered) {
      tmpl.isHovered = h;
      tmpl.scaleSpring.target = h ? 1.02f : 1.0f;
    }
  }

  // Check Profile
  bool ph = profileBounds_.contains(pt.fX, pt.fY);
  if (ph != isProfileHovered_) {
    isProfileHovered_ = ph;
    needsUpdate = true;
  }

  // Check New Project Button
  bool nph = newProjectButtonBounds_.contains(pt.fX, pt.fY);
  if (nph != isNewProjectHovered_) {
    isNewProjectHovered_ = nph;
    needsUpdate = true;
  }

  // The physics update in timerCallback will handle the repaint requests
}

void ZenithHubComponent::mouseDown(const juce::MouseEvent &e) {
  SkPoint pt = {(float)e.x, (float)e.y};

  // Click outside card?
  if (!mainCardBounds_.contains(pt.fX, pt.fY)) {
    // Maybe nothing, forcing user to pick something?
    // Or drag window.
  }

  // Click items
  for (auto &proj : recentProjects_) {
    if (proj.bounds.contains(pt.fX, pt.fY)) {
      dismiss(); // Load project
      return;
    }
  }

  for (auto &tmpl : templates_) {
    if (tmpl.bounds.contains(pt.fX, pt.fY)) {
      dismiss(); // Load template
      return;
    }
  }

  if (profileBounds_.contains(pt.fX, pt.fY)) {
    // Open profile settings?
  }

  if (newProjectButtonBounds_.contains(pt.fX, pt.fY)) {
    // Add Ripple Effect
    Ripple r;
    r.x = pt.fX;
    r.y = pt.fY;
    buttonRipples_.push_back(r);

    dismiss(); // Dismiss the hub to trigger the new project flow.
    return;
  }
}

void ZenithHubComponent::mouseUp(const juce::MouseEvent &e) {}

} // namespace zenith
