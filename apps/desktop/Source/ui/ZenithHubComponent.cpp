/*
  ==============================================================================

    ZenithHubComponent.cpp
    Fixed by Claude - December 2025
    Professional welcome screen with proper design system

  ==============================================================================
*/

#include "ZenithHubComponent.h"
#include "skia/ZenithIcons.h"
#include <array>

namespace zenith {

using namespace design;

ZenithHubComponent::ZenithHubComponent(std::function<void()> onDismiss)
    : onDismiss_(std::move(onDismiss)) {
  setWantsKeyboardFocus(true);
  createMockData();

  // Start fade-in
  alpha_.setTarget(0.0f, 0);
  alpha_.setTarget(1.0f, 600, AnimatedValue::EasingCurve::EaseOut);

  startTimerHz(60);
}

ZenithHubComponent::~ZenithHubComponent() { stopTimer(); }

void ZenithHubComponent::createMockData() {
  // Mock Recent Projects with better color choices
  recentProjects_ = {
      {"Cyberpunk City", "2 hours ago", "Electronic", colors::CYAN, {}, false},
      {"Orchestral Suite No. 1", "Yesterday", "Cinematic", colors::VIOLET, {}, false},
      {"Late Night Jazz", "3 days ago", "Jazz", colors::NEON_PINK, {}, false},
      {"Techno Bunker", "1 week ago", "Techno", colors::NEON_GREEN, {}, false},
      {"Ambient Dreams", "2 weeks ago", "Ambient", colors::BLUE, {}, false},
      {"Rock Anthem", "1 month ago", "Rock", colors::AMBER, {}, false}
  };

  // Mock Templates
  templates_ = {
      {"Electronic", "🎹", colors::CYAN, {}, false},
      {"Orchestral", "🎻", colors::VIOLET, {}, false},
      {"Recording", "🎤", colors::NEON_PINK, {}, false}
  };
}

void ZenithHubComponent::resized() { updateLayout(); }

void ZenithHubComponent::updateLayout() {
  auto bounds = getLocalBounds().toFloat();
  float w = bounds.getWidth();
  float h = bounds.getHeight();

  // Main Glass Card in Center (slightly larger)
  float cardW = std::min(1100.0f, w * 0.9f);
  float cardH = std::min(750.0f, h * 0.85f);

  float cardX = (w - cardW) * 0.5f;
  float cardY = (h - cardH) * 0.5f;

  mainCardBounds_ = SkRect::MakeXYWH(cardX, cardY, cardW, cardH);

  // Internal Layout
  float padding = 40.0f;
  float colOneW = (cardW - (padding * 3)) * 0.63f;
  float colTwoW = (cardW - (padding * 3)) * 0.37f;

  // Recent Projects Area
  recentArea_ = SkRect::MakeXYWH(
      cardX + padding,
      cardY + padding + 80.0f, // More header space
      colOneW,
      cardH - (padding * 2) - 80.0f
  );

  // Sidebar
  float sidebarX = cardX + padding + colOneW + padding;

  // Account (Top Right)
  float accountH = 160.0f;
  accountArea_ = SkRect::MakeXYWH(
      sidebarX,
      cardY + padding + 80.0f,
      colTwoW,
      accountH
  );

  // New Project Button (Solid, professional)
  float buttonH = 60.0f;
  newProjectButtonBounds_ = SkRect::MakeXYWH(
      sidebarX,
      accountArea_.bottom() + padding,
      colTwoW,
      buttonH
  );

  // Templates
  templatesArea_ = SkRect::MakeXYWH(
      sidebarX,
      newProjectButtonBounds_.bottom() + padding,
      colTwoW,
      recentArea_.bottom() - (newProjectButtonBounds_.bottom() + padding)
  );

  // Update Recent Project Cards Layout (Grid)
  float gridW = recentArea_.width();
  float cardGap = 16.0f;
  float pCardW = (gridW - cardGap) / 2.0f;
  float pCardH = 110.0f;

  for (size_t i = 0; i < recentProjects_.size(); ++i) {
    int row = (int)i / 2;
    int col = (int)i % 2;

    float px = recentArea_.fLeft + (col * (pCardW + cardGap));
    float py = recentArea_.fTop + (row * (pCardH + cardGap));

    recentProjects_[i].bounds = SkRect::MakeXYWH(px, py, pCardW, pCardH);
  }

  // Update Template Cards (Larger with icons)
  float tCardH = 90.0f;
  for (size_t i = 0; i < templates_.size(); ++i) {
    float tx = templatesArea_.fLeft;
    float ty = templatesArea_.fTop + 50.0f + (i * (tCardH + cardGap));
    templates_[i].bounds = SkRect::MakeXYWH(tx, ty, templatesArea_.width(), tCardH);
  }

  // Profile Button
  profileBounds_ = SkRect::MakeXYWH(
      accountArea_.fLeft,
      accountArea_.fTop + 50.0f,
      accountArea_.width(),
      90.0f
  );
}

void ZenithHubComponent::timerCallback() {
  animationTime_ += 0.016f;
  alpha_.update(16.0f);

  if (alpha_.isAnimating()) {
    repaint();
  }

  // Subtle background animation
  repaint();
}

void ZenithHubComponent::show() {
  setVisible(true);
  alpha_.setTarget(1.0f, 600, AnimatedValue::EasingCurve::EaseOut);
}

void ZenithHubComponent::dismiss() {
  alpha_.setTarget(0.0f, 400, AnimatedValue::EasingCurve::EaseIn);
  if (onDismiss_) {
    onDismiss_();
  }
}

void ZenithHubComponent::drawSkia(SkCanvas *canvas) {
  float opacity = alpha_.getCurrentValue();
  if (opacity <= 0.001f)
    return;

  canvas->saveLayerAlpha(nullptr, (U8CPU)(opacity * 255));

  drawBackground(canvas);

  // Main Glass Card with stronger shadow
  SkPaint shadowPaint;
  shadowPaint.setColor(SkColorSetARGB(80, 0, 0, 0));
  shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 30.0f));
  shadowPaint.setAntiAlias(true);
  
  SkRRect shadowRRect = SkRRect::MakeRectXY(
      mainCardBounds_.makeOutset(5.0f, 5.0f), 16.0f, 16.0f
  );
  canvas->drawRRect(shadowRRect, shadowPaint);

  GlassmorphicPanel::draw(canvas, mainCardBounds_, GlassmorphicPanel::Style::Floating);

  // Header with proper hierarchy
  {
    SkFont titleFont = design::getSkFont(48.0f, design::FontWeight::Bold);
    SkPaint titlePaint;
    titlePaint.setColor(colors::TEXT_PRIMARY);
    titlePaint.setAntiAlias(true);

    float headerX = mainCardBounds_.fLeft + 40;
    float headerY = mainCardBounds_.fTop + 60;

    canvas->drawString("Zenith Hub", headerX, headerY, titleFont, titlePaint);

    // Subtitle with proper sizing
    SkFont subFont = design::getSkFont(18.0f, design::FontWeight::Regular);
    SkPaint subPaint;
    subPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.6f));
    subPaint.setAntiAlias(true);
    
    canvas->drawString("Welcome back, User", headerX, headerY + 32, subFont, subPaint);
  }

  drawRecentProjects(canvas);
  drawAccount(canvas);
  drawNewProjectButton(canvas);
  drawTemplates(canvas);

  canvas->restore();
}

void ZenithHubComponent::drawBackground(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // FIXED: Subtle radial gradient instead of loud animated blobs
  SkPoint center = SkPoint::Make(bounds.getWidth() * 0.5f, bounds.getHeight() * 0.5f);
  
  SkColor gradientColors[] = {
      colors::BG_DARKEST,
      SkColorSetARGB(255, 8, 8, 12)  // Slightly lighter at edges
  };
  
  SkScalar positions[] = {0.0f, 1.0f};
  
  SkPaint bgPaint;
  bgPaint.setShader(
      SkGradientShader::MakeRadial(
          center,
          bounds.getWidth() * 0.8f,
          gradientColors,
          positions,
          2,
          SkTileMode::kClamp
      )
  );
  
  canvas->drawRect(rect, bgPaint);

  // FIXED: Very subtle moving accent (not overwhelming)
  float t = animationTime_ * 0.3f;
  SkPaint accentPaint;
  accentPaint.setAntiAlias(true);
  accentPaint.setBlendMode(SkBlendMode::kScreen);
  
  // Single subtle blob
  float blobX = bounds.getWidth() * 0.7f + std::sin(t) * 80;
  float blobY = bounds.getHeight() * 0.3f + std::cos(t * 0.7f) * 60;
  
  accentPaint.setColor(withAlpha(colors::BLUE, 0.06f));
  accentPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 120.0f));
  canvas->drawCircle(blobX, blobY, 350.0f, accentPaint);
}

void ZenithHubComponent::drawRecentProjects(SkCanvas *canvas) {
  SkFont headerFont = design::getSkFont(22.0f, design::FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  canvas->drawString("Recent Projects", recentArea_.fLeft, recentArea_.fTop - 20, headerFont, textPaint);

  // Empty state check
  if (recentProjects_.empty()) {
    SkFont emptyFont = design::getSkFont(16.0f, design::FontWeight::Regular);
    textPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.35f));
    
    canvas->drawString("No recent projects yet.", recentArea_.fLeft, recentArea_.fTop + 30, emptyFont, textPaint);
    
    SkFont hintFont = design::getSkFont(14.0f, design::FontWeight::Regular);
    canvas->drawString("Click 'New Project' to get started!", recentArea_.fLeft, recentArea_.fTop + 55, hintFont, textPaint);
    return;
  }

  for (const auto &proj : recentProjects_) {
    SkRRect rrect = SkRRect::MakeRectXY(proj.bounds, 12.0f, 12.0f);

    // Card background with better depth
    SkPaint cardPaint;
    cardPaint.setColor(proj.isHovered ? withAlpha(colors::BG_LIGHT, 0.15f) : withAlpha(colors::BG_LIGHT, 0.08f));
    cardPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, cardPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(proj.isHovered ? withAlpha(proj.accent, 0.4f) : withAlpha(colors::TEXT_PRIMARY, 0.06f));
    borderPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, borderPaint);

    // Thumbnail with accent color
    SkRect thumbRect = SkRect::MakeXYWH(proj.bounds.fLeft + 12, proj.bounds.fTop + 12, 86, 86);
    SkRRect thumbRRect = SkRRect::MakeRectXY(thumbRect, 8.0f, 8.0f);
    
    SkPaint thumbPaint;
    thumbPaint.setColor(withAlpha(proj.accent, 0.25f));
    thumbPaint.setAntiAlias(true);
    canvas->drawRRect(thumbRRect, thumbPaint);

    // Text content
    float textX = thumbRect.right() + 16;
    
    SkFont titleFont = design::getSkFont(16.0f, design::FontWeight::Bold);
    textPaint.setColor(colors::TEXT_PRIMARY);
    canvas->drawString(proj.name.toStdString().c_str(), textX, proj.bounds.fTop + 35, titleFont, textPaint);

    SkFont dateFont = design::getSkFont(13.0f, design::FontWeight::Regular);
    textPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.55f));
    canvas->drawString(proj.date.toStdString().c_str(), textX, proj.bounds.fTop + 60, dateFont, textPaint);

    // Genre badge
    SkFont genreFont = design::getSkFont(12.0f, design::FontWeight::Medium);
    textPaint.setColor(proj.accent);
    canvas->drawString(proj.genre.toStdString().c_str(), textX, proj.bounds.fTop + 82, genreFont, textPaint);
  }
}

void ZenithHubComponent::drawTemplates(SkCanvas *canvas) {
  SkFont headerFont = design::getSkFont(22.0f, design::FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  canvas->drawString("Quick Start", templatesArea_.fLeft, templatesArea_.fTop - 20, headerFont, textPaint);

  for (const auto &tmpl : templates_) {
    SkRRect rrect = SkRRect::MakeRectXY(tmpl.bounds, 12.0f, 12.0f);

    // Card background
    SkPaint cardPaint;
    cardPaint.setColor(tmpl.isHovered ? withAlpha(tmpl.color, 0.15f) : withAlpha(colors::BG_LIGHT, 0.08f));
    cardPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, cardPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(tmpl.isHovered ? withAlpha(tmpl.color, 0.6f) : withAlpha(colors::TEXT_PRIMARY, 0.06f));
    borderPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, borderPaint);

    // FIXED: Larger icon with proper background
    float iconSize = 48.0f;
    SkRect iconBounds = SkRect::MakeXYWH(
        tmpl.bounds.fLeft + 20,
        tmpl.bounds.centerY() - iconSize * 0.5f,
        iconSize,
        iconSize
    );
    
    SkPaint iconBgPaint;
    iconBgPaint.setColor(withAlpha(tmpl.color, 0.2f));
    iconBgPaint.setAntiAlias(true);
    canvas->drawRoundRect(iconBounds, 8.0f, 8.0f, iconBgPaint);

    // Icon color
    SkPaint iconPaint;
    iconPaint.setColor(tmpl.color);
    iconPaint.setAntiAlias(true);
    canvas->drawCircle(iconBounds.centerX(), iconBounds.centerY(), 12, iconPaint);

    // Text
    SkFont nameFont = design::getSkFont(18.0f, design::FontWeight::Bold);
    textPaint.setColor(colors::TEXT_PRIMARY);
    canvas->drawString(tmpl.name.toStdString().c_str(), iconBounds.right() + 16, tmpl.bounds.centerY() + 6, nameFont, textPaint);
  }
}

void ZenithHubComponent::drawAccount(SkCanvas *canvas) {
  SkFont headerFont = design::getSkFont(22.0f, design::FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  canvas->drawString("Collaborations", accountArea_.fLeft, accountArea_.fTop - 20, headerFont, textPaint);

  // Profile card
  SkRRect rrect = SkRRect::MakeRectXY(profileBounds_, 12.0f, 12.0f);

  SkPaint bgPaint;
  bgPaint.setColor(isProfileHovered_ ? withAlpha(colors::BG_LIGHT, 0.15f) : withAlpha(colors::BG_LIGHT, 0.08f));
  bgPaint.setAntiAlias(true);
  canvas->drawRRect(rrect, bgPaint);

  // Border
  SkPaint borderPaint;
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.06f));
  borderPaint.setAntiAlias(true);
  canvas->drawRRect(rrect, borderPaint);

  // Avatar (larger)
  float avatarSize = 48.0f;
  SkPaint avatarPaint;
  avatarPaint.setColor(colors::AMBER);
  avatarPaint.setAntiAlias(true);
  canvas->drawCircle(profileBounds_.fLeft + 30 + avatarSize * 0.5f, profileBounds_.centerY(), avatarSize * 0.5f, avatarPaint);

  // Name
  textPaint.setColor(colors::TEXT_PRIMARY);
  SkFont nameFont = design::getSkFont(17.0f, design::FontWeight::Bold);
  canvas->drawString("SoundDesigner99", profileBounds_.fLeft + 90, profileBounds_.centerY() - 6, nameFont, textPaint);

  // FIXED: Proper semantic color for online status
  SkFont statusFont = design::getSkFont(14.0f, design::FontWeight::Regular);
  SkPaint statusPaint;
  statusPaint.setColor(SkColorSetARGB(255, 16, 185, 129)); // colors::success equivalent
  statusPaint.setAntiAlias(true);
  canvas->drawString("● Online", profileBounds_.fLeft + 90, profileBounds_.centerY() + 18, statusFont, statusPaint);
}

void ZenithHubComponent::drawNewProjectButton(SkCanvas *canvas) {
  SkRRect rrect = SkRRect::MakeRectXY(newProjectButtonBounds_, 12.0f, 12.0f);

  // FIXED: Solid professional blue with proper states
  SkColor buttonColor = isNewProjectHovered_ 
      ? SkColorSetARGB(255, 96, 165, 250)   // Hover: lighter blue
      : SkColorSetARGB(255, 59, 130, 246);  // Default: professional blue

  SkPaint btnPaint;
  btnPaint.setColor(buttonColor);
  btnPaint.setAntiAlias(true);

  // Shadow on hover
  if (isNewProjectHovered_) {
    SkPaint shadowPaint;
    shadowPaint.setColor(withAlpha(buttonColor, 0.4f));
    shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 12.0f));
    shadowPaint.setAntiAlias(true);
    canvas->drawRRect(rrect.makeOutset(2.0f, 2.0f), shadowPaint);
  }

  canvas->drawRRect(rrect, btnPaint);

  // Text
  SkFont btnFont = design::getSkFont(20.0f, design::FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(SK_ColorWHITE);
  textPaint.setAntiAlias(true);

  SkString text("New Project");
  SkRect textBounds;
  btnFont.measureText(text.c_str(), text.size(), SkTextEncoding::kUTF8, &textBounds);

  float tx = newProjectButtonBounds_.centerX() - (textBounds.width() / 2.0f);
  float ty = newProjectButtonBounds_.centerY() + (textBounds.height() / 2.0f) - 4.0f;

  canvas->drawString(text, tx, ty, btnFont, textPaint);
}

void ZenithHubComponent::mouseMove(const juce::MouseEvent &e) {
  SkPoint pt = {(float)e.x, (float)e.y};
  bool needsUpdate = false;

  for (auto &proj : recentProjects_) {
    bool h = proj.bounds.contains(pt.fX, pt.fY);
    if (h != proj.isHovered) {
      proj.isHovered = h;
      needsUpdate = true;
    }
  }

  for (auto &tmpl : templates_) {
    bool h = tmpl.bounds.contains(pt.fX, pt.fY);
    if (h != tmpl.isHovered) {
      tmpl.isHovered = h;
      needsUpdate = true;
    }
  }

  bool ph = profileBounds_.contains(pt.fX, pt.fY);
  if (ph != isProfileHovered_) {
    isProfileHovered_ = ph;
    needsUpdate = true;
  }

  bool nph = newProjectButtonBounds_.contains(pt.fX, pt.fY);
  if (nph != isNewProjectHovered_) {
    isNewProjectHovered_ = nph;
    needsUpdate = true;
  }

  if (needsUpdate)
    repaint();
}

void ZenithHubComponent::mouseDown(const juce::MouseEvent &e) {
  SkPoint pt = {(float)e.x, (float)e.y};

  for (auto &proj : recentProjects_) {
    if (proj.bounds.contains(pt.fX, pt.fY)) {
      dismiss();
      return;
    }
  }

  for (auto &tmpl : templates_) {
    if (tmpl.bounds.contains(pt.fX, pt.fY)) {
      dismiss();
      return;
    }
  }

  if (profileBounds_.contains(pt.fX, pt.fY)) {
    // Open profile settings
  }

  if (newProjectButtonBounds_.contains(pt.fX, pt.fY)) {
    dismiss();
    return;
  }
}

void ZenithHubComponent::mouseUp(const juce::MouseEvent &e) {}

} // namespace zenith
