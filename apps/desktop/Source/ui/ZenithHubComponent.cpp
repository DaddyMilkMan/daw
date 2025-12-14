/*
  ==============================================================================

    ZenithHubComponent.cpp
    Created: 2025-12-13
    Author:  Zenith DAW Team

    Pinocchio Protocol Implementation:
    - Removed createMockData() completely
    - Uses RecentProjectManager for persistent project data
    - Implements actual project loading via callbacks

  ==============================================================================
*/

#include "ZenithHubComponent.h"
#include "skia/ZenithIcons.h"
#include <array>

namespace zenith {

using namespace design;

ZenithHubComponent::ZenithHubComponent(
    RecentProjectManager &recentProjectManager,
    LoadProjectCallback onLoadProject, NewProjectCallback onNewProject,
    std::function<void()> onDismiss)
    : recentProjectManager_(recentProjectManager),
      onLoadProject_(std::move(onLoadProject)),
      onNewProject_(std::move(onNewProject)), onDismiss_(std::move(onDismiss)) {
  setWantsKeyboardFocus(true);

  // Register as listener for project list changes
  recentProjectManager_.addListener(this);

  // Load real project data from manager
  loadFromManager();

  // Initialize templates (these are static)
  templates_ = {{"Electronic", "🎹", colors::CYAN, {}, false},
                {"Orchestral", "🎻", colors::VIOLET, {}, false},
                {"Recording", "🎤", colors::NEON_PINK, {}, false}};

  // Start fade-in
  alpha_.setTarget(0.0f, 0); // Start invisible
  alpha_.setTarget(1.0f, 600, AnimatedValue::EasingCurve::EaseOut); // Fade in

  startTimerHz(60);
}

ZenithHubComponent::~ZenithHubComponent() {
  stopTimer();
  recentProjectManager_.removeListener(this);
}

void ZenithHubComponent::loadFromManager() {
  recentProjects_.clear();

  auto projects = recentProjectManager_.getRecentProjects(true);

  for (const auto &entry : projects) {
    RecentProject proj;
    proj.name = entry.name;
    proj.date = entry.getRelativeTimeString();
    proj.genre = entry.genre;
    proj.path = entry.path;
    proj.accent = getAccentColorForGenre(entry.genre);
    proj.isHovered = false;
    // bounds will be set in updateLayout()

    recentProjects_.push_back(proj);

    // Only show first 6 projects in the grid
    if (recentProjects_.size() >= 6)
      break;
  }

  DBG("ZenithHubComponent: Loaded " + juce::String(recentProjects_.size()) +
      " recent projects from manager");

  // Trigger layout update if visible
  if (isVisible()) {
    updateLayout();
    repaint();
  }
}

SkColor ZenithHubComponent::getAccentColorForGenre(const juce::String &genre) {
  // Map genre strings to accent colors
  juce::String g = genre.toLowerCase();

  if (g.contains("electronic") || g.contains("edm") || g.contains("synth")) {
    return colors::CYAN;
  } else if (g.contains("orchestral") || g.contains("cinematic") ||
             g.contains("score")) {
    return colors::VIOLET;
  } else if (g.contains("jazz") || g.contains("swing")) {
    return colors::NEON_PINK;
  } else if (g.contains("techno") || g.contains("house") ||
             g.contains("dance")) {
    return colors::NEON_GREEN;
  } else if (g.contains("ambient") || g.contains("chill")) {
    return colors::BLUE;
  } else if (g.contains("rock") || g.contains("metal")) {
    return colors::AMBER;
  } else if (g.contains("hip") || g.contains("rap") || g.contains("trap")) {
    return colors::MAGENTA;
  } else {
    // Default accent color
    return colors::CYAN;
  }
}

void ZenithHubComponent::refreshProjects() { loadFromManager(); }

void ZenithHubComponent::recentProjectsChanged() {
  // Called when RecentProjectManager updates
  juce::MessageManager::callAsync([this]() { loadFromManager(); });
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
  float padding = 32.0f;
  float colOneW = (cardW - (padding * 3)) * 0.66f; // 2/3rds for Recent
  float colTwoW = (cardW - (padding * 3)) * 0.34f; // 1/3rd for Sidebar

  // Recent Projects Area
  recentArea_ = SkRect::MakeXYWH(cardX + padding,
                                 cardY + padding + 60.0f, // Header space
                                 colOneW, cardH - (padding * 2) - 60.0f);

  // Sidebar (Account + Templates)
  float sidebarX = cardX + padding + colOneW + padding;

  // Account (Top Right)
  float accountH = 150.0f;
  accountArea_ =
      SkRect::MakeXYWH(sidebarX, cardY + padding + 60.0f, colTwoW, accountH);

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
  float cardGap = 16.0f;
  float pCardW = (gridW - cardGap) / 2.0f; // 2 columns
  float pCardH = 100.0f;

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

  // Profile Button
  profileBounds_ =
      SkRect::MakeXYWH(accountArea_.fLeft, accountArea_.fTop + 40.0f,
                       accountArea_.width(), 80.0f);
}

void ZenithHubComponent::timerCallback() {
  animationTime_ += 0.016f; // ~60fps
  alpha_.update(16.0f);

  if (alpha_.isAnimating()) {
    repaint();
  }

  // Animate glowing background
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

  // Save layer for global opacity
  canvas->saveLayerAlpha(nullptr, (U8CPU)(opacity * 255));

  drawBackground(canvas);

  // Main Glass Card
  GlassmorphicPanel::draw(canvas, mainCardBounds_,
                          GlassmorphicPanel::Style::Floating);

  // Header
  {
    SkFont titleFont = design::getSkFont(42.0f, design::FontWeight::Bold);
    SkPaint paint;
    paint.setColor(colors::TEXT_PRIMARY);
    paint.setAntiAlias(true);

    SkRect titleBounds =
        SkRect::MakeXYWH(mainCardBounds_.fLeft + 32, mainCardBounds_.fTop + 32,
                         mainCardBounds_.width(), 50);
    canvas->drawString("Zenith Hub", titleBounds.fLeft, titleBounds.bottom(),
                       titleFont, paint);

    SkFont subFont = design::getSkFont(16.0f, design::FontWeight::Regular);
    paint.setColor(colors::TEXT_SECONDARY);
    canvas->drawString("Welcome back, User", titleBounds.fLeft + 240,
                       titleBounds.bottom(), subFont, paint);
  }

  drawRecentProjects(canvas);
  drawAccount(canvas);
  drawNewProjectButton(canvas);
  drawTemplates(canvas);

  canvas->restore();
}

void ZenithHubComponent::drawBackground(SkCanvas *canvas) {
  // Animated Mesh Gradient Logic
  auto bounds = getLocalBounds().toFloat();
  SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  SkPaint bgPaint;

  // Base dark background
  bgPaint.setColor(colors::BG_DARKEST);
  canvas->drawRect(rect, bgPaint);

  // Moving blobs
  float t = animationTime_;

  struct Blob {
    float x, y, r;
    SkColor c;
  };
  Blob blobs[] = {
      {0.2f * bounds.getWidth() + sin(t * 0.5f) * 100,
       0.3f * bounds.getHeight() + cos(t * 0.3f) * 100, 400.0f, colors::VIOLET},
      {0.8f * bounds.getWidth() - cos(t * 0.4f) * 100,
       0.7f * bounds.getHeight() + sin(t * 0.6f) * 100, 500.0f, colors::CYAN},
      {0.5f * bounds.getWidth() + sin(t * 0.7f) * 50,
       0.5f * bounds.getHeight() + cos(t * 0.8f) * 50, 300.0f,
       colors::NEON_PINK}};

  SkPaint blobPaint;
  blobPaint.setAntiAlias(true);
  blobPaint.setBlendMode(SkBlendMode::kScreen); // Blend nicely

  for (const auto &b : blobs) {
    blobPaint.setColor(withAlpha(b.c, 0.15f));
    blobPaint.setMaskFilter(
        SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 100.0f));
    canvas->drawCircle(b.x, b.y, b.r, blobPaint);
  }
}

void ZenithHubComponent::drawRecentProjects(SkCanvas *canvas) {
  SkFont headerFont = design::getSkFont(20.0f, design::FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  canvas->drawString("Recent Projects", recentArea_.fLeft,
                     recentArea_.fTop - 15, headerFont, textPaint);

  // If no projects, show a helpful message
  if (recentProjects_.empty()) {
    SkFont emptyFont = design::getSkFont(16.0f, design::FontWeight::Regular);
    textPaint.setColor(colors::TEXT_SECONDARY);
    canvas->drawString("No recent projects yet.", recentArea_.fLeft + 20,
                       recentArea_.fTop + 40, emptyFont, textPaint);
    canvas->drawString("Click 'New Project' to get started!",
                       recentArea_.fLeft + 20, recentArea_.fTop + 65, emptyFont,
                       textPaint);
    return;
  }

  for (const auto &proj : recentProjects_) {
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

    // Mock Image / Icon (accent colored rectangle)
    SkRect imageRect =
        SkRect::MakeXYWH(proj.bounds.fLeft + 10, proj.bounds.fTop + 10, 80, 80);
    SkPaint imgPaint;
    imgPaint.setColor(withAlpha(proj.accent, 0.2f));
    canvas->drawRect(imageRect, imgPaint);

    // Text
    SkFont titleFont = design::getSkFont(16.0f, design::FontWeight::Medium);
    textPaint.setColor(colors::TEXT_PRIMARY);
    canvas->drawString(proj.name.toStdString().c_str(), imageRect.right() + 15,
                       proj.bounds.fTop + 30, titleFont, textPaint);

    SkFont subFont = design::getSkFont(14.0f, design::FontWeight::Regular);
    textPaint.setColor(colors::TEXT_SECONDARY);
    canvas->drawString(proj.date.toStdString().c_str(), imageRect.right() + 15,
                       proj.bounds.fTop + 55, subFont, textPaint);

    // Genre Badge
    if (proj.genre.isNotEmpty()) {
      SkPaint badgePaint;
      badgePaint.setColor(withAlpha(proj.accent, 0.3f));
      SkRect badgeRect = SkRect::MakeXYWH(imageRect.right() + 15,
                                          proj.bounds.fTop + 65, 80, 20);
      SkRRect badgeRRect = SkRRect::MakeRectXY(badgeRect, 4.0f, 4.0f);
      canvas->drawRRect(badgeRRect, badgePaint);

      SkFont badgeFont = design::getSkFont(11.0f, design::FontWeight::Medium);
      SkPaint badgeText;
      badgeText.setColor(colors::TEXT_PRIMARY);
      badgeText.setAntiAlias(true);
      canvas->drawString(proj.genre.toStdString().c_str(), badgeRect.fLeft + 6,
                         badgeRect.centerY() + 4, badgeFont, badgeText);
    }
  }
}

void ZenithHubComponent::drawTemplates(SkCanvas *canvas) {
  SkFont headerFont = design::getSkFont(20.0f, design::FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  canvas->drawString("Quick Start", templatesArea_.fLeft,
                     templatesArea_.fTop - 15, headerFont, textPaint);

  for (const auto &tmpl : templates_) {
    SkRRect rrect = SkRRect::MakeRectXY(tmpl.bounds, 8.0f, 8.0f);

    SkPaint cardPaint;
    cardPaint.setColor(tmpl.isHovered ? withAlpha(tmpl.color, 0.2f)
                                      : withAlpha(colors::BG_LIGHT, 0.05f));
    cardPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, cardPaint);

    // Icon (colored circle)
    SkPaint iconPaint;
    iconPaint.setColor(tmpl.color);
    canvas->drawCircle(tmpl.bounds.fLeft + 30, tmpl.bounds.centerY(), 15,
                       iconPaint);

    // Text
    textPaint.setColor(colors::TEXT_PRIMARY);
    SkFont nameFont = design::getSkFont(16.0f, design::FontWeight::Medium);
    canvas->drawString(tmpl.name.toStdString().c_str(), tmpl.bounds.fLeft + 60,
                       tmpl.bounds.centerY() + 6, nameFont, textPaint);

    if (tmpl.isHovered) {
      SkPaint borderPaint;
      borderPaint.setStyle(SkPaint::kStroke_Style);
      borderPaint.setStrokeWidth(1.0f);
      borderPaint.setColor(withAlpha(tmpl.color, 0.8f));
      canvas->drawRRect(rrect, borderPaint);
    }
  }
}

void ZenithHubComponent::drawAccount(SkCanvas *canvas) {
  SkFont headerFont = design::getSkFont(20.0f, design::FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  canvas->drawString("Collaborations", accountArea_.fLeft,
                     accountArea_.fTop - 15, headerFont, textPaint);

  // Profile Box
  SkRRect rrect = SkRRect::MakeRectXY(profileBounds_, 8.0f, 8.0f);

  SkPaint bgPaint;
  bgPaint.setColor(isProfileHovered_ ? withAlpha(colors::BLUE, 0.15f)
                                     : withAlpha(colors::BG_LIGHT, 0.05f));
  bgPaint.setAntiAlias(true);
  canvas->drawRRect(rrect, bgPaint);

  // Avatar
  SkPaint avatarPaint;
  avatarPaint.setColor(colors::AMBER);
  canvas->drawCircle(profileBounds_.fLeft + 40, profileBounds_.centerY(), 25,
                     avatarPaint);

  textPaint.setColor(colors::TEXT_PRIMARY);
  SkFont nameFont = design::getSkFont(16.0f, design::FontWeight::Medium);
  canvas->drawString("SoundDesigner99", profileBounds_.fLeft + 80,
                     profileBounds_.centerY() + -5, nameFont, textPaint);

  textPaint.setColor(colors::NEON_GREEN);
  SkFont statusFont = design::getSkFont(12.0f, design::FontWeight::Regular);
  canvas->drawString("● Online", profileBounds_.fLeft + 80,
                     profileBounds_.centerY() + 15, statusFont, textPaint);
}

void ZenithHubComponent::drawNewProjectButton(SkCanvas *canvas) {
  SkRRect rrect = SkRRect::MakeRectXY(newProjectButtonBounds_, 12.0f, 12.0f);

  // Neon Gradient Background
  SkPaint btnPaint;
  btnPaint.setAntiAlias(true);

  const std::array<SkPoint, 2> pts = {
      SkPoint::Make(newProjectButtonBounds_.fLeft,
                    newProjectButtonBounds_.fTop),
      SkPoint::Make(newProjectButtonBounds_.fRight,
                    newProjectButtonBounds_.fBottom)};

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

  // Text
  SkFont btnFont = design::getSkFont(24.0f, design::FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(SK_ColorWHITE);
  textPaint.setAntiAlias(true);

  // Center text
  SkString text("New Project");
  SkRect textBounds;
  btnFont.measureText(text.c_str(), text.size(), SkTextEncoding::kUTF8,
                      &textBounds);

  float tx = newProjectButtonBounds_.centerX() - (textBounds.width() / 2.0f);
  float ty =
      newProjectButtonBounds_.centerY() + (textBounds.height() / 2.0f) - 4.0f;

  canvas->drawString(text, tx, ty, btnFont, textPaint);
}

void ZenithHubComponent::mouseMove(const juce::MouseEvent &e) {
  SkPoint pt = {(float)e.x, (float)e.y};
  bool needsUpdate = false;

  // Check Recent Projects
  for (auto &proj : recentProjects_) {
    bool h = proj.bounds.contains(pt.fX, pt.fY);
    if (h != proj.isHovered) {
      proj.isHovered = h;
      needsUpdate = true;
    }
  }

  // Check Templates
  for (auto &tmpl : templates_) {
    bool h = tmpl.bounds.contains(pt.fX, pt.fY);
    if (h != tmpl.isHovered) {
      tmpl.isHovered = h;
      needsUpdate = true;
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

  if (needsUpdate)
    repaint();
}

void ZenithHubComponent::mouseDown(const juce::MouseEvent &e) {
  SkPoint pt = {(float)e.x, (float)e.y};

  // Click outside card?
  if (!mainCardBounds_.contains(pt.fX, pt.fY)) {
    // Maybe nothing, forcing user to pick something?
    return;
  }

  // Click on recent project - ACTUALLY LOAD IT
  for (const auto &proj : recentProjects_) {
    if (proj.bounds.contains(pt.fX, pt.fY)) {
      DBG("ZenithHubComponent: Loading project: " +
          proj.path.getFullPathName());

      if (onLoadProject_ && proj.path.existsAsFile()) {
        onLoadProject_(proj.path);
      } else if (!proj.path.existsAsFile()) {
        DBG("ZenithHubComponent: Project file no longer exists: " +
            proj.path.getFullPathName());
        // Optionally remove from recent list
        // recentProjectManager_.removeProject(proj.path);
        // loadFromManager();
      }

      dismiss();
      return;
    }
  }

  // Click on template - create new project from template
  for (const auto &tmpl : templates_) {
    if (tmpl.bounds.contains(pt.fX, pt.fY)) {
      DBG("ZenithHubComponent: Creating project from template: " + tmpl.name);

      // For now, templates just create a new project
      // In the future, this could load a template file
      if (onNewProject_) {
        onNewProject_();
      }

      dismiss();
      return;
    }
  }

  if (profileBounds_.contains(pt.fX, pt.fY)) {
    // Open profile settings (future feature)
    DBG("ZenithHubComponent: Profile clicked");
  }

  if (newProjectButtonBounds_.contains(pt.fX, pt.fY)) {
    DBG("ZenithHubComponent: New Project clicked");

    if (onNewProject_) {
      onNewProject_();
    }

    dismiss();
    return;
  }
}

void ZenithHubComponent::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
}

} // namespace zenith
