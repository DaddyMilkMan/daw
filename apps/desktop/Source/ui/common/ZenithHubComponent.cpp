/*
  ==============================================================================

    ZenithHubComponent.cpp
    Clean implementation with robust layout and proper font caching.

  ==============================================================================
*/

#include "ZenithHubComponent.h"
#include "../design-system/ZenithLayout.h"
#include "ZenithIcons.h"
#include <array>
#include <cmath>
#include <map>
#include <random>

namespace zenith {

using namespace design;

static const std::map<juce::String, SkPath (*)()> kGenreIconMap = {
    {"electronic", &icons::Synth},    {"techno", &icons::Synth},
    {"edm", &icons::Synth},           {"synth", &icons::Synth},
    {"cinematic", &icons::MusicNote}, {"orchestral", &icons::MusicNote},
    {"jazz", &icons::MusicNote},      {"ambient", &icons::Cloud},
    {"chill", &icons::Cloud},         {"rock", &icons::Waveform},
    {"metal", &icons::Waveform}};

static const std::map<juce::String, SkPath (*)()> kTemplateIconMap = {
    {"icon_synth", &icons::Synth},
    {"icon_note", &icons::MusicNote},
    {"icon_mic", &icons::Microphone}};

ZenithHubComponent::ZenithHubComponent(
    RecentProjectManager &recentProjectManager,
    LoadProjectCallback onLoadProject, NewProjectCallback onNewProject,
    std::function<void()> onDismiss)
    : recentProjectManager_(recentProjectManager),
      onLoadProject_(std::move(onLoadProject)),
      onNewProject_(std::move(onNewProject)), onDismiss_(std::move(onDismiss)) {
  setWantsKeyboardFocus(true);

  recentProjectManager_.addListener(this);
  loadFromManager();

  templates_ = {{"Electronic", "icon_synth", colors::CYAN, {}, false},
                {"Orchestral", "icon_note", colors::VIOLET, {}, false},
                {"Recording", "icon_mic", colors::NEON_PINK, {}, false}};

  // Initialize Cached Fonts (A+ Enhancement)
  titleFont_ = design::getSkFont(48.0f, design::FontWeight::Bold);
  subFont_ = design::getSkFont(18.0f, design::FontWeight::Regular);
  headerFont_ = design::getSkFont(22.0f, design::FontWeight::Bold);
  cardTitleFont_ = design::getSkFont(16.0f, design::FontWeight::Bold);
  cardDateFont_ = design::getSkFont(13.0f, design::FontWeight::Regular);
  cardGenreFont_ = design::getSkFont(12.0f, design::FontWeight::Medium);
  buttonFont_ = design::getSkFont(20.0f, design::FontWeight::Bold);
  statusFont_ = design::getSkFont(14.0f, design::FontWeight::Regular);
  templateFont_ = design::getSkFont(18.0f, design::FontWeight::Bold);
  profileFont_ = design::getSkFont(17.0f, design::FontWeight::Bold);
  bodyFont_ = design::getSkFont(16.0f, design::FontWeight::Regular);

  textPaint_.setAntiAlias(true);
  textPaint_.setColor(colors::TEXT_PRIMARY);

  subPaint_.setAntiAlias(true);
  subPaint_.setColor(withAlpha(colors::TEXT_PRIMARY, 0.6f));

  // Initialize Greeting Editor as permanent hidden child
  addChildComponent(&greetingEditor_);
  greetingEditor_.setVisible(false);
  greetingEditor_.setMultiLine(false);
  greetingEditor_.setReturnKeyStartsNewLine(false);
  greetingEditor_.setSelectAllWhenFocused(true);

  auto safeDismiss = [this]() { hideGreetingEditor(false); };
  greetingEditor_.onEscapeKey = safeDismiss;
  greetingEditor_.onFocusLost = safeDismiss;
  greetingEditor_.onReturnKey = [this]() { hideGreetingEditor(true); };

  // Initialize Aurora Background
  auroraBackground_ = std::make_unique<AuroraBackground>();

  // Initialize Springs
  curtainAlpha_.setCurrent(0.0f); // Start invisible, fade in
  curtainAlpha_.setTarget(1.0f);  // Target visible

  // VBlank Animation Loop
  vBlankAttachment_ = std::make_unique<juce::VBlankAttachment>(this, [this]() {
    bool working = false;

    animationTime_ += 0.016f; // Approx 60fps increment

    // 1. Curtain Animation
    working |= curtainY_.update(0.1f, 0.85f);
    working |= curtainAlpha_.update(0.05f, 0.9f);

    // 2. Tilt Physics (Mouse Parallax)
    // Calculate target tilt based on mouse position
    auto mouse = getMouseXYRelative();
    float cx = getWidth() * 0.5f;
    float cy = getHeight() * 0.5f;

    // Only tilt if mouse is inside
    if (contains(mouse)) {
      float tx = (mouse.x - cx) / cx; // -1 to 1
      float ty = (mouse.y - cy) / cy; // -1 to 1
      tiltX_.setTarget(ty * 5.0f);    // Max 5 degrees tilt
      tiltY_.setTarget(tx * -5.0f);
    } else {
      tiltX_.setTarget(0.0f);
      tiltY_.setTarget(0.0f);
    }

    working |= tiltX_.update(0.05f, 0.92f); // Smooth, heavy feel
    working |= tiltY_.update(0.05f, 0.92f);

    // 3. Item Animations
    for (auto &p : recentProjects_) {
      p.scaleSpring.setTarget(p.isHovered ? 1.05f : 1.0f);
      working |= p.scaleSpring.update(0.2f, 0.75f); // Bouncy
    }
    for (auto &t : templates_) {
      t.scaleSpring.setTarget(t.isHovered ? 1.05f : 1.0f);
      working |= t.scaleSpring.update(0.2f, 0.75f);
    }

    // Check for Dismiss Completion
    if (curtainAlpha_.getTarget() < 0.1f && curtainAlpha_.isResting()) {
      if (onDismiss_) {
        // Ensure we don't call this repeatedly
        auto callback = onDismiss_;
        onDismiss_ = nullptr;
        callback();
      }
    }

    // Repaint if valid
    if (working || auroraBackground_) {
      repaint();
    }
  });
}

ZenithHubComponent::~ZenithHubComponent() {
  stopTimer();
  recentProjectManager_.removeListener(this);
}

void ZenithHubComponent::mouseExit(const juce::MouseEvent &e) {
  SkiaComponent::mouseExit(e);
  isNewProjectHovered_ = false;
  isProfileHovered_ = false;
  isGreetingHovered_ = false;
  for (auto &p : recentProjects_)
    p.isHovered = false;
  for (auto &t : templates_)
    t.isHovered = false;
  repaint();
}

bool ZenithHubComponent::hitTest(int x, int y) {
  return mainCardBounds_.contains((float)x, (float)y);
}

void ZenithHubComponent::loadFromManager() {
  recentProjects_.clear();
  auto projects = recentProjectManager_.getRecentProjects(true);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> barCountDist(20, 30);
  std::uniform_real_distribution<float> heightDist(0.2f, 1.0f);

  for (const auto &entry : projects) {
    RecentProject proj;
    proj.name = entry.name;
    proj.date = entry.getRelativeTimeString();
    proj.genre = entry.genre;
    proj.path = entry.path;
    proj.accent = getAccentColorForGenre(entry.genre);
    proj.isHovered = false;

    int numBars = barCountDist(gen);
    for (int i = 0; i < numBars; ++i) {
      proj.waveform.push_back(heightDist(gen));
    }

    recentProjects_.push_back(proj);
    if (recentProjects_.size() >= 6)
      break;
  }

  if (isVisible()) {
    updateLayout();
    repaint();
  }
}

SkColor ZenithHubComponent::getAccentColorForGenre(const juce::String &genre) {
  juce::String g = genre.toLowerCase();
  if (g.contains("electronic") || g.contains("edm") || g.contains("synth"))
    return colors::CYAN;
  if (g.contains("orchestral") || g.contains("cinematic") ||
      g.contains("score"))
    return colors::VIOLET;
  if (g.contains("jazz") || g.contains("swing"))
    return colors::NEON_PINK;
  if (g.contains("techno") || g.contains("house") || g.contains("dance"))
    return colors::NEON_GREEN;
  if (g.contains("ambient") || g.contains("chill"))
    return colors::BLUE;
  if (g.contains("rock") || g.contains("metal"))
    return colors::AMBER;
  if (g.contains("hip") || g.contains("rap") || g.contains("trap"))
    return colors::MAGENTA;
  return colors::CYAN;
}

void ZenithHubComponent::refreshProjects() { loadFromManager(); }

void ZenithHubComponent::recentProjectsChanged() {
  juce::MessageManager::callAsync([this]() { loadFromManager(); });
}

void ZenithHubComponent::resized() { updateLayout(); }

void ZenithHubComponent::updateLayout() {
  auto bounds = getLocalBounds().toFloat();
  float w = bounds.getWidth();
  float h = bounds.getHeight();

  // 1. Center Floating Card Calculation
  float cardW = std::min(1150.0f, w * 0.92f);
  float cardH = std::min(800.0f, h * 0.88f);
  float cardX = (w - cardW) * 0.5f;
  float cardY = (h - cardH) * 0.5f;
  mainCardBounds_ = SkRect::MakeXYWH(cardX, cardY, cardW, cardH);

  float padding = 48.0f; // Premium spacing
  float sectionGap = 32.0f;

  // 2. Main Content Area (below Hub title)
  float headerAreaHeight = 120.0f;
  juce::Rectangle<float> contentRect(cardX + padding, cardY + headerAreaHeight,
                                     cardW - (padding * 2),
                                     cardH - padding - headerAreaHeight);

  // 3. Main Split: Recent Projects (Left) vs Sidebar (Right)
  auto mainColumns =
      ZenithLayout::begin()
          .withFloatBounds(contentRect)
          .withGap(48.0f)
          .withAlign(juce::FlexBox::AlignItems::stretch)
          .addFlexItem(juce::FlexItem().withFlex(0.62f)) // Projects
          .addFlexItem(juce::FlexItem().withFlex(0.38f)) // Sidebar
          .layout(juce::FlexBox::Direction::row);

  if (mainColumns.size() >= 2) {
    // Recent Projects Area
    auto r = mainColumns[0];
    recentArea_ =
        SkRect::MakeXYWH(r.getX(), r.getY(), r.getWidth(), r.getHeight());

    // Sidebar Area
    auto s = mainColumns[1];

    // 4. Sidebar Internal Layout: Collaborations, New Project, Quick Start
    auto sidebarSections =
        ZenithLayout::begin()
            .withFloatBounds(s)
            .withGap(sectionGap)
            .withAlign(juce::FlexBox::AlignItems::stretch)
            .addFlexItem(
                juce::FlexItem().withHeight(150.0f)) // Collaborations Section
            .addFlexItem(
                juce::FlexItem().withHeight(60.0f))       // New Project Button
            .addFlexItem(juce::FlexItem().withFlex(1.0f)) // Quick Start Section
            .layout(juce::FlexBox::Direction::column);

    if (sidebarSections.size() >= 3) {
      accountArea_ = SkRect::MakeXYWH(
          sidebarSections[0].getX(), sidebarSections[0].getY(),
          sidebarSections[0].getWidth(), sidebarSections[0].getHeight());

      newProjectButtonBounds_ = SkRect::MakeXYWH(
          sidebarSections[1].getX(), sidebarSections[1].getY(),
          sidebarSections[1].getWidth(), sidebarSections[1].getHeight());

      templatesArea_ = SkRect::MakeXYWH(
          sidebarSections[2].getX(), sidebarSections[2].getY(),
          sidebarSections[2].getWidth(), sidebarSections[2].getHeight());

      // Profile Button (within accountArea)
      profileBounds_ =
          SkRect::MakeXYWH(accountArea_.fLeft, accountArea_.fTop + 40.0f,
                           accountArea_.width(), 90.0f);
    }
  }

  // 5. Grid Layout for Recent Projects
  if (!recentArea_.isEmpty()) {
    float headerOffset = 40.0f; // Space for "Recent Projects" text
    float gridTop = recentArea_.fTop + headerOffset;
    float gridH = recentArea_.height() - headerOffset;

    float cardGap = 16.0f;
    float pCardW = (recentArea_.width() - cardGap) / 2.0f;
    float pCardH = 110.0f;

    for (size_t i = 0; i < recentProjects_.size(); ++i) {
      int row = (int)i / 2;
      int col = (int)i % 2;
      float px = recentArea_.fLeft + (col * (pCardW + cardGap));
      float py = gridTop + (row * (pCardH + cardGap));
      recentProjects_[i].bounds = SkRect::MakeXYWH(px, py, pCardW, pCardH);
    }
  }

  // 6. List Layout for Templates
  if (!templatesArea_.isEmpty()) {
    float headerOffset = 45.0f; // Space for "Quick Start" text
    float listTop = templatesArea_.fTop + headerOffset;
    float tCardH = 80.0f;
    float cardGap = 12.0f;

    for (size_t i = 0; i < templates_.size(); ++i) {
      float ty = listTop + (i * (tCardH + cardGap));
      templates_[i].bounds = SkRect::MakeXYWH(templatesArea_.fLeft, ty,
                                              templatesArea_.width(), tCardH);
    }
  }

  if (greetingEditor_.isVisible()) {
    showGreetingEditor();
  }
}

void ZenithHubComponent::timerCallback() {
  // Deprecated in favor of VBlankAttachment
}

void ZenithHubComponent::show() {
  setVisible(true);
  curtainY_.setCurrent(getHeight()); // Start from bottom? or just fade in?
  // Actually, "Curtain Lift" implies it lifts UP to reveal. So it starts at 0
  // (covering) and moves to -Height (revealing). But wait, the Hub IS the
  // curtain. So on STARTup, it is ON screen (0). On DISMISS, it moves UP
  // (-Height).

  curtainY_.setCurrent(0.0f);
  curtainY_.setTarget(0.0f);

  curtainAlpha_.setCurrent(0.0f);
  curtainAlpha_.setTarget(1.0f);
}

void ZenithHubComponent::dismiss() {
  // "Curtain Lift": Slide UP and Fade OUT
  curtainY_.setTarget(-(float)getHeight());
  curtainAlpha_.setTarget(0.0f);
}

void ZenithHubComponent::drawSkia(SkCanvas *canvas) {
  float opacity = curtainAlpha_.getCurrent();
  float yOffset = curtainY_.getCurrent();

  if (opacity <= 0.001f && yOffset < -getHeight() + 1.0f)
    return;

  canvas->save();

  // Apply Curtain Lift Transform
  canvas->translate(0.0f, yOffset);

  // Restore opacity fading
  // Note: saveLayerAlpha handles the opacity for the whole group
  canvas->saveLayerAlpha(nullptr, (U8CPU)(opacity * 255));

  drawBackground(canvas);

  // Apply Parallax Tilt to the card
  canvas->save();
  float tiltX = tiltX_.getCurrent();
  float tiltY = tiltY_.getCurrent();

  // Simple 2D skew/translate simulation of 3D tilt
  // Real 3D would require SkMatrix44 but standard translate is safer
  // specifically for UI
  SkMatrix m;
  m.setIdentity();
  // Pivot around center
  float cx = mainCardBounds_.centerX();
  float cy = mainCardBounds_.centerY();
  m.preTranslate(-cx, -cy);
  m.postSkew(tiltY * 0.0005f, tiltX * 0.0005f);          // Micro-skew
  m.postTranslate(cx + tiltY * 2.0f, cy + tiltX * 2.0f); // Micro-translate
  canvas->concat(m);

  GlassmorphicPanel::draw(canvas, mainCardBounds_,
                          GlassmorphicPanel::Style::Floating);

  // Restore Matrix (End Parallax)
  canvas->restore();

  // Header with proper hierarchy
  textPaint_.setColor(colors::TEXT_PRIMARY);

  float headerX = mainCardBounds_.fLeft + 40;
  float headerY = mainCardBounds_.fTop + 60;

  // Use drawText helper for title as per code review
  SkRect titleBounds =
      SkRect::MakeXYWH(headerX, headerY - 50, mainCardBounds_.width() - 80, 60);
  drawText(canvas, "Zenith Hub", titleBounds, titleFont_, textPaint_, false);

  // Subtitle with proper sizing
  subPaint_.setColor(withAlpha(colors::TEXT_PRIMARY, 0.6f));

  // Subtitle
  SkString greeting(greetingText_.toRawUTF8());
  SkRect bounds;
  subFont_.measureText(greeting.c_str(), greeting.size(), SkTextEncoding::kUTF8,
                       &bounds);

  float subX = headerX;
  float subY = headerY + 32;

  // Store bounds for interaction
  greetingTextBounds_ = SkRect::MakeXYWH(subX, subY - bounds.height(),
                                         bounds.width(), bounds.height() + 4);

  SkRect helperBounds = SkRect::MakeXYWH(subX, subY - bounds.height(),
                                         bounds.width(), bounds.height());
  drawText(canvas, greetingText_, helperBounds, subFont_, subPaint_, false);

  // Use predefined constant instead of magic number per code review feedback
  constexpr float kGreetingIconSize = 16.0f;

  // Calculate icon bounds
  greetingEditIconBounds_ =
      SkRect::MakeXYWH(subX + bounds.width() + 10, subY - 14, kGreetingIconSize,
                       kGreetingIconSize);

  icons::IconStyle iconStyle;
  iconStyle.color = isGreetingHovered_
                        ? colors::CYAN
                        : withAlpha(colors::TEXT_SECONDARY, 0.5f);
  iconStyle.strokeWidth = icons::STROKE_THIN;

  icons::drawIconCentered(canvas, icons::Edit(), greetingEditIconBounds_,
                          kGreetingIconSize, iconStyle);

  drawProjectList(canvas); // Renamed for clarity and direct opacity handling
  drawAccount(canvas);
  drawNewProjectButton(canvas);
  drawTemplates(canvas);

  canvas->restore(); // Restore opacity layer
  canvas->restore(); // Restore translation
}

void ZenithHubComponent::drawBackground(SkCanvas *canvas) {
  if (auroraBackground_) {
    auto bounds = getLocalBounds().toFloat();
    SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
    auroraBackground_->draw(canvas, rect, animationTime_);
    return;
  }
  SkPaint bgPaint;
  bgPaint.setColor(colors::BG_DARKEST);
  canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), bgPaint);
}

void ZenithHubComponent::drawProjectList(SkCanvas *canvas) {
  textPaint_.setColor(colors::TEXT_PRIMARY);
  drawText(canvas, "Recent Projects",
           SkRect::MakeXYWH(recentArea_.fLeft, recentArea_.fTop + 8, 300, 30),
           headerFont_, textPaint_, false);

  // Empty state check
  if (recentProjects_.empty()) {
    SkPaint emptyStatePaint = textPaint_;
    emptyStatePaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.35f));
    drawText(
        canvas, "No recent projects yet.",
        SkRect::MakeXYWH(recentArea_.fLeft, recentArea_.fTop + 10, 300, 20),
        bodyFont_, emptyStatePaint, false);
    return;
  }

  for (size_t i = 0; i < recentProjects_.size(); ++i) {
    const auto &proj = recentProjects_[i];
    bool isSelected = (selectedSection_ == SelectionSection::Recent &&
                       (int)i == selectedIndex_);

    bool active = proj.isHovered || isSelected;

    SkRRect rrect = SkRRect::MakeRectXY(proj.bounds, 12.0f, 12.0f);
    SkPaint cardPaint;
    cardPaint.setColor(active ? withAlpha(colors::BG_LIGHT, 0.15f)
                              : withAlpha(colors::BG_LIGHT, 0.08f));
    cardPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, cardPaint);

    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(isSelected ? 2.0f : 1.0f);
    borderPaint.setColor(active ? withAlpha(proj.accent, 0.6f)
                                : withAlpha(colors::TEXT_PRIMARY, 0.06f));

    borderPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, borderPaint);

    SkRect thumbRect =
        SkRect::MakeXYWH(proj.bounds.fLeft + 12, proj.bounds.fTop + 12, 86, 86);
    SkPaint thumbPaint;
    thumbPaint.setColor(withAlpha(proj.accent, 0.25f));
    thumbPaint.setAntiAlias(true);
    canvas->drawRRect(SkRRect::MakeRectXY(thumbRect, 8.0f, 8.0f), thumbPaint);

    // PREMIUM: Draw genre-specific vector icon
    SkPath iconPath = icons::Project(); // Default
    auto it = kGenreIconMap.find(proj.genre.toLowerCase());
    if (it != kGenreIconMap.end()) {
      iconPath = it->second();
    }

    icons::IconStyle iconStyle;
    iconStyle.color = active ? proj.accent : withAlpha(proj.accent, 0.6f);
    iconStyle.strokeWidth = icons::STROKE_REGULAR;
    if (active) {
      iconStyle.glowRadius = 4.0f;
      iconStyle.glowColor = withAlpha(proj.accent, 0.4f);
    }

    icons::drawIconCentered(canvas, iconPath, thumbRect, 40.0f, iconStyle);

    float textX = thumbRect.right() + 16;
    drawText(canvas, proj.name,
             SkRect::MakeXYWH(textX, proj.bounds.fTop + 20,
                              proj.bounds.width() - 110, 24),
             cardTitleFont_, textPaint_, false);
    drawText(canvas, proj.date,
             SkRect::MakeXYWH(textX, proj.bounds.fTop + 45,
                              proj.bounds.width() - 110, 20),
             cardDateFont_, subPaint_, false);

    if (proj.genre.isNotEmpty()) {
      SkPaint genrePaint = textPaint_;
      genrePaint.setColor(proj.accent);
      drawText(canvas, proj.genre,
               SkRect::MakeXYWH(textX, proj.bounds.fTop + 65,
                                proj.bounds.width() - 110, 20),
               cardGenreFont_, genrePaint, false);
    }
  }
}

void ZenithHubComponent::drawTemplates(SkCanvas *canvas) {
  drawText(
      canvas, "Quick Start",
      SkRect::MakeXYWH(templatesArea_.fLeft, templatesArea_.fTop + 8, 200, 30),
      headerFont_, textPaint_, false);

  for (size_t i = 0; i < templates_.size(); ++i) {
    const auto &tmpl = templates_[i];
    bool isSelected = (this->selectedSection_ == SelectionSection::Templates &&
                       (int)i == selectedIndex_);

    bool active = tmpl.isHovered || isSelected;

    SkRRect rrect = SkRRect::MakeRectXY(tmpl.bounds, 12.0f, 12.0f);
    SkPaint cardPaint;
    cardPaint.setColor(active ? withAlpha(tmpl.color, 0.15f)
                              : withAlpha(colors::BG_LIGHT, 0.08f));
    cardPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, cardPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(isSelected ? 2.0f : 1.0f);
    borderPaint.setColor(active ? withAlpha(tmpl.color, 0.6f)
                                : withAlpha(colors::TEXT_PRIMARY, 0.06f));
    borderPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, borderPaint);

    float iconSize = 48.0f;
    SkRect iconBounds = SkRect::MakeXYWH(
        tmpl.bounds.fLeft + 20, tmpl.bounds.centerY() - iconSize * 0.5f,
        iconSize, iconSize);
    SkPaint iconBgPaint;
    iconBgPaint.setColor(withAlpha(tmpl.color, 0.2f));
    iconBgPaint.setAntiAlias(true);
    canvas->drawRoundRect(iconBounds, 8.0f, 8.0f, iconBgPaint);

    // PREMIUM: Draw template-specific vector icon with glow
    SkPath iconPath = icons::Template(); // fallback
    auto it = kTemplateIconMap.find(tmpl.icon);
    if (it != kTemplateIconMap.end()) {
      iconPath = it->second();
    }

    icons::IconStyle iconStyle;
    iconStyle.color = tmpl.color;
    iconStyle.strokeWidth = active ? icons::STROKE_BOLD : icons::STROKE_REGULAR;
    if (active) {
      iconStyle.glowRadius = 8.0f;
      iconStyle.glowColor = withAlpha(tmpl.color, 0.5f);
    }
    icons::drawIconCentered(canvas, iconPath, iconBounds,
                            iconBounds.width() * 0.6f, iconStyle);

    drawText(canvas, tmpl.name,
             SkRect::MakeXYWH(iconBounds.right() + 16,
                              tmpl.bounds.centerY() - 12, 200, 24),
             templateFont_, textPaint_, false);
  }
}

void ZenithHubComponent::drawAccount(SkCanvas *canvas) {
  drawText(canvas, "Collaborations",
           SkRect::MakeXYWH(accountArea_.fLeft, accountArea_.fTop + 8, 200, 30),
           headerFont_, textPaint_, false);

  SkRRect rrect = SkRRect::MakeRectXY(profileBounds_, 12.0f, 12.0f);
  SkPaint bgPaint;
  bgPaint.setColor(isProfileHovered_ ? withAlpha(colors::BG_LIGHT, 0.15f)
                                     : withAlpha(colors::BG_LIGHT, 0.08f));
  bgPaint.setAntiAlias(true);
  canvas->drawRRect(rrect, bgPaint);

  float avatarSize = 48.0f;
  SkPaint avatarPaint;
  avatarPaint.setColor(colors::AMBER);
  avatarPaint.setAntiAlias(true);
  float centerY = profileBounds_.fTop + profileBounds_.height() * 0.5f;
  canvas->drawCircle(profileBounds_.fLeft + 30 + avatarSize * 0.5f, centerY,
                     avatarSize * 0.5f, avatarPaint);

  drawText(canvas, "SoundDesigner99",
           SkRect::MakeXYWH(profileBounds_.fLeft + 90, centerY - 18, 200, 24),
           profileFont_, textPaint_, false);

  SkPaint onlineStatusPaint;
  onlineStatusPaint.setColor(SkColorSetARGB(255, 16, 185, 129));
  onlineStatusPaint.setAntiAlias(true);
  drawText(canvas, "[*] Online",
           SkRect::MakeXYWH(
               profileBounds_.fLeft + 90,
               (profileBounds_.fTop + profileBounds_.height() * 0.5f) + 4, 100,
               20),
           statusFont_, onlineStatusPaint, false);
}

void ZenithHubComponent::drawNewProjectButton(SkCanvas *canvas) {
  bool isSelected = (this->selectedSection_ == SelectionSection::New);
  bool active = isNewProjectHovered_ || isSelected;

  // PREMIUM: Real glassmorphism button
  GlassmorphicPanel::Options opts;
  opts.style = active ? GlassmorphicPanel::Style::ActiveGlow
                      : GlassmorphicPanel::Style::Elevated;
  opts.accentColor = colors::BLUE;
  opts.cornerRadius = 12.0f;
  opts.glowIntensity = active ? 1.5f : 0.0f;

  GlassmorphicPanel::drawWithOptions(canvas, newProjectButtonBounds_, opts);

  // Draw Icon + Text
  float iconSize = 20.0f;
  SkRect iconBounds = SkRect::MakeXYWH(
      newProjectButtonBounds_.fLeft + 20,
      newProjectButtonBounds_.centerY() - iconSize * 0.5f, iconSize, iconSize);

  icons::IconStyle iconStyle;
  iconStyle.color = colors::TEXT_PRIMARY;
  iconStyle.strokeWidth = icons::STROKE_BOLD;
  icons::drawIconCentered(canvas, icons::Plus(), iconBounds, iconSize,
                          iconStyle);

  SkRect textBounds = newProjectButtonBounds_;
  textBounds.fLeft += 45;
  drawText(canvas, "New Project", textBounds, buttonFont_, textPaint_, true);
}

void ZenithHubComponent::mouseMove(const juce::MouseEvent &e) {
  SkPoint pt = {(float)e.x, (float)e.y};
  bool needsUpdate = false;

  if (selectedSection_ != SelectionSection::None) {
    selectedSection_ = SelectionSection::None;
    selectedIndex_ = -1;
    needsUpdate = true;
  }

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

  bool gh = greetingTextBounds_.contains(pt.fX, pt.fY) ||
            greetingEditIconBounds_.contains(pt.fX, pt.fY);
  if (gh != isGreetingHovered_) {
    isGreetingHovered_ = gh;
    needsUpdate = true;
  }

  if (needsUpdate)
    repaint();
}

void ZenithHubComponent::mouseDown(const juce::MouseEvent &e) {
  SkPoint pt = {(float)e.x, (float)e.y};
  if (!mainCardBounds_.contains(pt.fX, pt.fY))
    return;

  for (const auto &proj : recentProjects_) {
    if (proj.bounds.contains(pt.fX, pt.fY)) {
      if (onLoadProject_ && proj.path.existsAsFile())
        onLoadProject_(proj.path);
      dismiss();
      return;
    }
  }

  for (const auto &tmpl : templates_) {
    if (tmpl.bounds.contains(pt.fX, pt.fY)) {
      if (onNewProject_)
        onNewProject_();
      dismiss();
      return;
    }
  }

  if (newProjectButtonBounds_.contains(pt.fX, pt.fY)) {
    if (onNewProject_)
      onNewProject_();
    dismiss();
    return;
  }

  // Greeting Edit
  if (greetingTextBounds_.contains(pt.fX, pt.fY) ||
      greetingEditIconBounds_.contains(pt.fX, pt.fY)) {
    showGreetingEditor();
    return;
  }
}

void ZenithHubComponent::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
}

void ZenithHubComponent::showGreetingEditor() {
  if (greetingEditor_.isVisible())
    return;

  greetingEditor_.setText(greetingText_);
  greetingEditor_.setSelectAllWhenFocused(true);
  greetingEditor_.setJustification(juce::Justification::left);
  greetingEditor_.setFont(juce::Font(18.0f));

  // Named constants for TextEditor sizing
  constexpr int kEditorWidthPadding = 60;
  constexpr int kEditorHeight = 24;

  if (greetingTextBounds_.isEmpty()) {
    return;
  }

  juce::Rectangle<int> bounds(
      (int)greetingTextBounds_.left(),
      (int)greetingTextBounds_.top() + (int)greetingTextBounds_.height() / 2,
      (int)(greetingTextBounds_.width() + kEditorWidthPadding), kEditorHeight);

  greetingEditor_.setBounds(bounds);
  greetingEditor_.setVisible(true);
  greetingEditor_.selectAll();
  greetingEditor_.grabKeyboardFocus();
}

void ZenithHubComponent::hideGreetingEditor(bool save) {
  if (save) {
    greetingText_ = greetingEditor_.getText();
  }
  greetingEditor_.setVisible(false);
  repaint();
}

// Keyboard Navigation
bool ZenithHubComponent::keyPressed(const juce::KeyPress &key) {
  if (key.isKeyCode(juce::KeyPress::upKey)) {
    moveSelection(0, -1);
    return true;
  }
  if (key.isKeyCode(juce::KeyPress::downKey)) {
    moveSelection(0, 1);
    return true;
  }
  if (key.isKeyCode(juce::KeyPress::leftKey)) {
    moveSelection(-1, 0);
    return true;
  }
  if (key.isKeyCode(juce::KeyPress::rightKey)) {
    moveSelection(1, 0);
    return true;
  }
  if (key.isKeyCode(juce::KeyPress::returnKey)) {
    triggerSelection();
    return true;
  }
  if (key.isKeyCode(juce::KeyPress::escapeKey)) {
    dismiss();
    return true;
  }

  return SkiaComponent::keyPressed(key);
}

void ZenithHubComponent::moveSelection(int dx, int dy) {
  if (selectedSection_ == SelectionSection::None) {
    selectedSection_ = SelectionSection::Recent;
    selectedIndex_ = 0;
    repaint();
    return;
  }

  if (selectedSection_ == SelectionSection::Recent) {
    int row = selectedIndex_ / 2;
    int col = selectedIndex_ % 2;

    if (dx == 1 && col == 1) {
      // Move to Sidebar
      selectedSection_ = SelectionSection::New;
      selectedIndex_ = -1;
    } else if (dx == -1 && col == 0) {
      // Stay
    } else {
      // Navigation within grid
      int newRow = row + dy;
      int newCol = col + dx;

      // Vertical wrap or limit? Let's limit for now to be predictable.
      if (newRow < 0)
        newRow = 0;
      if (newRow >= (int)(recentProjects_.size() + 1) / 2)
        newRow = (int)(recentProjects_.size() + 1) / 2 - 1;

      int newIdx = (newRow * 2) + newCol;
      if (newIdx >= 0 && newIdx < (int)recentProjects_.size()) {
        selectedIndex_ = newIdx;
      }
    }
  } else if (selectedSection_ == SelectionSection::New) {
    if (dy == 1 && !templates_.empty()) {
      selectedSection_ = SelectionSection::Templates;
      selectedIndex_ = 0;
    } else if (dx == -1) {
      selectedSection_ = SelectionSection::Recent;
      selectedIndex_ = std::min((int)recentProjects_.size() - 1, 1);
    }
  } else if (selectedSection_ == SelectionSection::Templates) {
    if (dy == -1 && selectedIndex_ == 0) {
      selectedSection_ = SelectionSection::New;
      selectedIndex_ = -1;
    } else if (dx == -1) {
      selectedSection_ = SelectionSection::Recent;
      selectedIndex_ = std::min((int)recentProjects_.size() - 1, 5);
    } else {
      int newIdx = selectedIndex_ + dy;
      if (newIdx >= 0 && newIdx < (int)templates_.size()) {
        selectedIndex_ = newIdx;
      }
    }
  }
  repaint();
}

void ZenithHubComponent::triggerSelection() {
  if (selectedSection_ == SelectionSection::Recent && selectedIndex_ >= 0 &&
      selectedIndex_ < (int)recentProjects_.size()) {
    if (onLoadProject_) {
      onLoadProject_(recentProjects_[selectedIndex_].path);
    }
    dismiss();
  } else if (selectedSection_ == SelectionSection::New ||
             selectedSection_ == SelectionSection::Templates) {
    if (onNewProject_)
      onNewProject_();
    dismiss();
  }
}

// Text Rendering Helper - Fixed vertical alignment per code review
void ZenithHubComponent::drawText(SkCanvas *canvas, const juce::String &text,
                                  const SkRect &bounds, const SkFont &font,
                                  const SkPaint &paint, bool centerVertical) {
  SkString skText(text.toRawUTF8());
  SkRect textBounds;
  font.measureText(skText.c_str(), skText.size(), SkTextEncoding::kUTF8,
                   &textBounds);

  float x = bounds.left();
  // Align text top to the top of the bounds (A+ alignment)
  float y = bounds.fTop - textBounds.fTop;

  if (centerVertical) {
    y = bounds.centerY() + (textBounds.height() * 0.5f) - textBounds.fBottom;
  }
  canvas->drawString(skText, x, y, font, paint);
}

} // namespace zenith
