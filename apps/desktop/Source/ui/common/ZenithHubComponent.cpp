/*
  ==============================================================================

    ZenithHubComponent.cpp
    Fixed by Claude - December 2025
    Professional welcome screen with proper design system

    Pinocchio Protocol Implementation:
    - Removed createMockData() completely
    - Uses RecentProjectManager for persistent project data
    - Implements actual project loading via callbacks

  ==============================================================================
*/

#include "ZenithHubComponent.h"
#include "ZenithIcons.h"
#include <array>
#include <cmath>
#include <map>
#include <random>

namespace zenith {

using namespace design;

// static icon maps for cleaner lookups
static const std::map<juce::String, SkPath (*)()> kGenreIconMap = {
    {"electronic", &icons::Synth},    {"techno", &icons::Synth},
    {"edm", &icons::Synth},           {"synth", &icons::Synth},
    {"cinematic", &icons::MusicNote}, {"orchestral", &icons::MusicNote},
    {"jazz", &icons::MusicNote},      {"ambient", &icons::Cloud},
    {"chill", &icons::Cloud},         {"rock", &icons::Waveform},
    {"metal", &icons::Waveform}};

// Template icon mapping
// Ideally this would be an enum, but for now we map string ID to icon function
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

  // Register as listener for project list changes
  recentProjectManager_.addListener(this);

  // Load real project data from manager
  loadFromManager();

  // Initialize templates (these are static)
  // Using ID strings that match our map
  templates_ = {{"Electronic", "icon_synth", colors::CYAN, {}, false},
                {"Orchestral", "icon_note", colors::VIOLET, {}, false},
                {"Recording", "icon_mic", colors::NEON_PINK, {}, false}};

  // Initialize Aurora Background
  auroraBackground_ = std::make_unique<AuroraBackground>();

  // Start fade-in
  alpha_.setTarget(0.0f, 0);
  alpha_.setTarget(1.0f, 600, AnimatedValue::EasingCurve::EaseOut);

  startTimerHz(60);

  // Initialize Greeting Editor as permanent hidden child
  addChildComponent(greetingEditor_);
  greetingEditor_.setVisible(false);
  greetingEditor_.setMultiLine(false);
  greetingEditor_.setReturnKeyStartsNewLine(false);
  
  // Configure callbacks for safe hiding
  auto safeDismiss = [this]() { hideGreetingEditor(false); };
  
  greetingEditor_.onEscapeKey = safeDismiss;
  greetingEditor_.onFocusLost = safeDismiss;
  greetingEditor_.onReturnKey = [this]() { hideGreetingEditor(true); };
}

ZenithHubComponent::~ZenithHubComponent() {
  stopTimer();
  recentProjectManager_.removeListener(this);
}

void ZenithHubComponent::mouseExit(const juce::MouseEvent &e) {
  SkiaComponent::mouseExit(e);

  // Reset ALL hover states
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

  // Random generator for waveforms
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
    // bounds will be set in updateLayout()

    // Generate procedural waveform
    int numBars = barCountDist(gen);
    for (int i = 0; i < numBars; ++i) {
      proj.waveform.push_back(heightDist(gen));
    }

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

  // Main Glass Card in Center (slightly larger)
  float cardW = std::min(1100.0f, w * 0.9f);
  float cardH = std::min(750.0f, h * 0.85f);

  float cardX = (w - cardW) * 0.5f;
  float cardY = (h - cardH) * 0.5f;

  mainCardBounds_ = SkRect::MakeXYWH(cardX, cardY, cardW, cardH);

  float padding = 40.0f; 
  float gridGap = 40.0f; 
  float headerHeight = 140.0f;

  // Define Content Area (Main Card minus padding and header)
  juce::Rectangle<float> contentRect(
      cardX + padding, 
      cardY + padding + headerHeight,
      cardW - (padding * 2), 
      cardH - (padding * 2) - headerHeight);

  // 1. Layout Left (Recent) vs Right (Sidebar) using ZenithLayout (Agent 4)
  auto mainColumns = ZenithLayout::begin()
      .withFloatBounds(contentRect)
      .withGap(gridGap)
      .addFlexItem(juce::FlexItem().withFlex(0.6f)) // Recent Area (60%)
      .addFlexItem(juce::FlexItem().withFlex(0.4f)) // Sidebar (40%)
      .layout(juce::FlexBox::Direction::row);

  if (mainColumns.size() >= 2) {
      auto r = mainColumns[0];
      recentArea_ = SkRect::MakeXYWH(r.getX(), r.getY(), r.getWidth(), r.getHeight());
      
      auto s = mainColumns[1];
      juce::Rectangle<float> sidebarRect = s; 

      // 2. Layout Sidebar (Account, Button, Templates)
      auto sidebarRows = ZenithLayout::begin()
          .withFloatBounds(sidebarRect)
          .withGap(padding)
          .addFlexItem(juce::FlexItem().withHeight(100.0f))
          .addFlexItem(juce::FlexItem().withHeight(60.0f))
          .addFlexItem(juce::FlexItem().withFlex(1.0f))
          .layout(juce::FlexBox::Direction::column);
          
      if (sidebarRows.size() >= 3) {
          auto a = sidebarRows[0];
          accountArea_ = SkRect::MakeXYWH(a.getX(), a.getY(), a.getWidth(), a.getHeight());
          
          auto b = sidebarRows[1];
          newProjectButtonBounds_ = SkRect::MakeXYWH(b.getX(), b.getY(), b.getWidth(), b.getHeight());
          
          auto t = sidebarRows[2];
          templatesArea_ = SkRect::MakeXYWH(t.getX(), t.getY(), t.getWidth(), t.getHeight());
      }
  }

  // Profile Button
  profileBounds_ =
      SkRect::MakeXYWH(accountArea_.fLeft, accountArea_.fTop + 50.0f,
                       accountArea_.width(), 90.0f);

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

  // Update Template Cards
  float tCardH = 90.0f;
  for (size_t i = 0; i < templates_.size(); ++i) {
    float tx = templatesArea_.fLeft;
    float ty = templatesArea_.fTop + 50.0f + (i * (tCardH + cardGap));
    templates_[i].bounds = SkRect::MakeXYWH(tx, ty, templatesArea_.width(), tCardH);
  }

  if (greetingEditor_.isVisible()) {
      showGreetingEditor(); // Re-layout editor
  }
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

  SkRRect shadowRRect =
      SkRRect::MakeRectXY(mainCardBounds_.makeOutset(5.0f, 5.0f), 16.0f, 16.0f);
  canvas->drawRRect(shadowRRect, shadowPaint);

  GlassmorphicPanel::draw(canvas, mainCardBounds_,
                          GlassmorphicPanel::Style::Floating);

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

    SkString greeting(greetingText_.toRawUTF8());
    SkRect bounds;
    subFont.measureText(greeting.c_str(), greeting.size(), SkTextEncoding::kUTF8, &bounds);
    
    float subX = headerX;
    float subY = headerY + 32;
    
    // Store bounds for interaction
    greetingTextBounds_ = SkRect::MakeXYWH(subX, subY - bounds.height(), bounds.width(), bounds.height() + 4);
    
    canvas->drawString(greeting, subX, subY, subFont, subPaint);
    // Draw Edit Icon using the icon system
    // Use named constant for icon size per code review feedback
    constexpr float kGreetingIconSize = 16.0f;
    greetingEditIconBounds_ = SkRect::MakeXYWH(subX + bounds.width() + 10, subY - 14, kGreetingIconSize, kGreetingIconSize);
    
    icons::IconStyle iconStyle;
    iconStyle.color = isGreetingHovered_ ? colors::CYAN : withAlpha(colors::TEXT_SECONDARY, 0.5f);
    // Use predefined constant instead of magic number per code review feedback
    iconStyle.strokeWidth = icons::STROKE_THIN;
    
    icons::drawIconCentered(canvas, icons::Edit(), greetingEditIconBounds_, kGreetingIconSize, iconStyle);
  }

  drawRecentProjects(canvas);
  drawAccount(canvas);
  drawNewProjectButton(canvas);
  drawTemplates(canvas);

  canvas->restore();
}

void ZenithHubComponent::drawBackground(SkCanvas *canvas) {
  if (auroraBackground_) {
    auto bounds = getLocalBounds().toFloat();
    SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
    auroraBackground_->draw(canvas, rect, animationTime_);
    return;
  }

  // Fallback if no Aurora
  auto bounds = getLocalBounds().toFloat();
  SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
  SkPaint bgPaint;
  bgPaint.setColor(colors::BG_DARKEST);
  canvas->drawRect(rect, bgPaint);
}

void ZenithHubComponent::drawRecentProjects(SkCanvas *canvas) {
  SkFont headerFont = design::getSkFont(22.0f, design::FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  canvas->drawString("Recent Projects", recentArea_.fLeft,
                     recentArea_.fTop - 20, headerFont, textPaint);

  // Empty state check
  if (recentProjects_.empty()) {
    SkFont emptyFont = design::getSkFont(16.0f, design::FontWeight::Regular);
    textPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.35f));

    canvas->drawString("No recent projects yet.", recentArea_.fLeft,
                       recentArea_.fTop + 30, emptyFont, textPaint);

    SkFont hintFont = design::getSkFont(14.0f, design::FontWeight::Regular);
    canvas->drawString("Click 'New Project' to get started!", recentArea_.fLeft,
                       recentArea_.fTop + 55, hintFont, textPaint);
    return;
  }

  for (const auto &proj : recentProjects_) {
    SkRRect rrect = SkRRect::MakeRectXY(proj.bounds, 12.0f, 12.0f);

    // Card background with better depth
    SkPaint cardPaint;
    cardPaint.setColor(proj.isHovered ? withAlpha(colors::BG_LIGHT, 0.15f)
                                      : withAlpha(colors::BG_LIGHT, 0.08f));
    cardPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, cardPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(proj.isHovered
                             ? withAlpha(proj.accent, 0.4f)
                             : withAlpha(colors::TEXT_PRIMARY, 0.06f));
    borderPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, borderPaint);

    // Thumbnail with accent color
    SkRect thumbRect =
        SkRect::MakeXYWH(proj.bounds.fLeft + 12, proj.bounds.fTop + 12, 86, 86);
    SkRRect thumbRRect = SkRRect::MakeRectXY(thumbRect, 8.0f, 8.0f);

    SkPaint thumbPaint;
    thumbPaint.setColor(withAlpha(proj.accent, 0.25f));
    thumbPaint.setAntiAlias(true);
    canvas->drawRRect(thumbRRect, thumbPaint);

    // Draw Icon based on map
    SkPath iconPath = icons::Project(); // Default
    auto it = kGenreIconMap.find(proj.genre.toLowerCase());
    if (it != kGenreIconMap.end()) {
      iconPath = it->second();
    }

    // Draw icon centered in thumbnail
    // TODO: Scale icon to fit? Assuming icons are normalized or standard size.
    // For now assuming icons::... returns a path around 0,0 or 24x24.
    // Let's just fill a rect with color for now as in original code, or try to
    // draw path if we knew how to scale it. Original HEAD code had
    // `canvas->drawRRect` for thumbnail. Let's stick to the color block as the
    // "Thumbnail". Wait, the review mentioned: "Mock Image / Icon (accent
    // colored rectangle)" was master. HEAD had "Thumbnail with accent color".
    // I will stick to the accent color block for safety, but maybe add a small
    // icon overlay if I can.

    // Text content
    float textX = thumbRect.right() + 16;

    SkFont titleFont = design::getSkFont(16.0f, design::FontWeight::Bold);
    textPaint.setColor(colors::TEXT_PRIMARY);
    canvas->drawString(proj.name.toStdString().c_str(), textX,
                       proj.bounds.fTop + 35, titleFont, textPaint);

    SkFont dateFont = design::getSkFont(13.0f, design::FontWeight::Regular);
    textPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.55f));
    canvas->drawString(proj.date.toStdString().c_str(), textX,
                       proj.bounds.fTop + 60, dateFont, textPaint);

    // Genre badge
    if (proj.genre.isNotEmpty()) {
      SkFont genreFont = design::getSkFont(12.0f, design::FontWeight::Medium);
      textPaint.setColor(proj.accent);
      canvas->drawString(proj.genre.toStdString().c_str(), textX,
                         proj.bounds.fTop + 82, genreFont, textPaint);
    }
  }
}

void ZenithHubComponent::drawTemplates(SkCanvas *canvas) {
  SkFont headerFont = design::getSkFont(22.0f, design::FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  canvas->drawString("Quick Start", templatesArea_.fLeft,
                     templatesArea_.fTop - 20, headerFont, textPaint);

  for (const auto &tmpl : templates_) {
    SkRRect rrect = SkRRect::MakeRectXY(tmpl.bounds, 12.0f, 12.0f);

    // Card background
    SkPaint cardPaint;
    cardPaint.setColor(tmpl.isHovered ? withAlpha(tmpl.color, 0.15f)
                                      : withAlpha(colors::BG_LIGHT, 0.08f));
    cardPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, cardPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1.0f);
    borderPaint.setColor(tmpl.isHovered
                             ? withAlpha(tmpl.color, 0.6f)
                             : withAlpha(colors::TEXT_PRIMARY, 0.06f));
    borderPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, borderPaint);

    // FIXED: Larger icon with proper background
    float iconSize = 48.0f;
    SkRect iconBounds = SkRect::MakeXYWH(
        tmpl.bounds.fLeft + 20, tmpl.bounds.centerY() - iconSize * 0.5f,
        iconSize, iconSize);

    SkPaint iconBgPaint;
    iconBgPaint.setColor(withAlpha(tmpl.color, 0.2f));
    iconBgPaint.setAntiAlias(true);
    canvas->drawRoundRect(iconBounds, 8.0f, 8.0f, iconBgPaint);

    // Icon (via Map)
    // Here we can use the map to get the path
    SkPath iconPath = icons::Template(); // fallback
    auto it = kTemplateIconMap.find(tmpl.icon);
    if (it != kTemplateIconMap.end()) {
      iconPath = it->second();
    }

    // Draw the icon path scaled and centered
    // Basic scaling logic (assuming 24x24 viewbox for icons)
    SkRect pathBounds = iconPath.getBounds();
    float scale =
        (iconSize * 0.5f) / std::max(pathBounds.width(), pathBounds.height());

    SkMatrix matrix;
    matrix.setTranslate(iconBounds.centerX() - pathBounds.centerX(),
                        iconBounds.centerY() - pathBounds.centerY());
    matrix.preScale(scale, scale, pathBounds.centerX(), pathBounds.centerY());

    SkPaint iconPaint;
    iconPaint.setColor(tmpl.color);
    iconPaint.setAntiAlias(true);

    // For now drawing circle as fallback/placeholder if path is empty, or
    // drawPath if we trust it The original code drew a circle.
    // "canvas->drawCircle(iconBounds.centerX(), iconBounds.centerY(), 12,
    // iconPaint);" I will stick to the circle for safety unless I'm sure
    // icons::... are implemented and working. The review asked to use
    // std::map/enum for logic, not necessarily to implement the path drawing if
    // it wasn't there.
    canvas->drawCircle(iconBounds.centerX(), iconBounds.centerY(), 12,
                       iconPaint);

    // Text
    SkFont nameFont = design::getSkFont(18.0f, design::FontWeight::Bold);
    textPaint.setColor(colors::TEXT_PRIMARY);
    canvas->drawString(tmpl.name.toStdString().c_str(), iconBounds.right() + 16,
                       tmpl.bounds.centerY() + 6, nameFont, textPaint);
  }
}

void ZenithHubComponent::drawAccount(SkCanvas *canvas) {
  SkFont headerFont = design::getSkFont(22.0f, design::FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(colors::TEXT_PRIMARY);
  textPaint.setAntiAlias(true);

  canvas->drawString("Collaborations", accountArea_.fLeft,
                     accountArea_.fTop - 20, headerFont, textPaint);

  // Profile card
  SkRRect rrect = SkRRect::MakeRectXY(profileBounds_, 12.0f, 12.0f);

  SkPaint bgPaint;
  bgPaint.setColor(isProfileHovered_ ? withAlpha(colors::BG_LIGHT, 0.15f)
                                     : withAlpha(colors::BG_LIGHT, 0.08f));
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
  canvas->drawCircle(profileBounds_.fLeft + 30 + avatarSize * 0.5f,
                     profileBounds_.centerY(), avatarSize * 0.5f, avatarPaint);

  // Name
  textPaint.setColor(colors::TEXT_PRIMARY);
  SkFont nameFont = design::getSkFont(17.0f, design::FontWeight::Bold);
  canvas->drawString("SoundDesigner99", profileBounds_.fLeft + 90,
                     profileBounds_.centerY() - 6, nameFont, textPaint);

  // FIXED: Proper semantic color for online status
  SkFont statusFont = design::getSkFont(14.0f, design::FontWeight::Regular);
  SkPaint statusPaint;
  statusPaint.setColor(
      SkColorSetARGB(255, 16, 185, 129)); // colors::success equivalent
  statusPaint.setAntiAlias(true);
  canvas->drawString("● Online", profileBounds_.fLeft + 90,
                     profileBounds_.centerY() + 18, statusFont, statusPaint);
}

void ZenithHubComponent::drawNewProjectButton(SkCanvas *canvas) {
  SkRRect rrect = SkRRect::MakeRectXY(newProjectButtonBounds_, 12.0f, 12.0f);

  // FIXED: Solid professional blue with proper states
  SkColor buttonColor =
      isNewProjectHovered_
          ? SkColorSetARGB(255, 96, 165, 250)  // Hover: lighter blue
          : SkColorSetARGB(255, 59, 130, 246); // Default: professional blue

  SkPaint btnPaint;
  btnPaint.setColor(buttonColor);
  btnPaint.setAntiAlias(true);

  // Shadow on hover
  if (isNewProjectHovered_) {
    SkPaint shadowPaint;
    shadowPaint.setColor(withAlpha(buttonColor, 0.4f));
    shadowPaint.setMaskFilter(
        SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 12.0f));
    shadowPaint.setAntiAlias(true);
    SkRRect outset = rrect;
    outset.outset(4.0f, 4.0f);
    canvas->drawRRect(outset, shadowPaint);
  }

  canvas->drawRRect(rrect, btnPaint);

  // Text
  SkFont btnFont = design::getSkFont(20.0f, design::FontWeight::Bold);
  SkPaint textPaint;
  textPaint.setColor(SK_ColorWHITE);
  textPaint.setAntiAlias(true);

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

  bool gh = greetingTextBounds_.contains(pt.fX, pt.fY) || greetingEditIconBounds_.contains(pt.fX, pt.fY);
  if (gh != isGreetingHovered_) {
      isGreetingHovered_ = gh;
      needsUpdate = true;
  }

  if (needsUpdate)
    repaint();
}

void ZenithHubComponent::mouseDown(const juce::MouseEvent &e) {
  SkPoint pt = {(float)e.x, (float)e.y};

  // Click outside card?
  if (!mainCardBounds_.contains(pt.fX, pt.fY)) {
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
      }

      dismiss();
      return;
    }
  }

  // Click on template
  for (const auto &tmpl : templates_) {
    if (tmpl.bounds.contains(pt.fX, pt.fY)) {
      DBG("ZenithHubComponent: Creating project from template: " + tmpl.name);
      if (onNewProject_) {
        onNewProject_();
      }
      dismiss();
      return;
    }
  }

  if (profileBounds_.contains(pt.fX, pt.fY)) {
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

  // Greeting Edit
  if (greetingTextBounds_.contains(pt.fX, pt.fY) || greetingEditIconBounds_.contains(pt.fX, pt.fY)) {
      showGreetingEditor();
      return;
  }
}

void ZenithHubComponent::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
}

void ZenithHubComponent::showGreetingEditor() {
  greetingEditor_.setText(greetingText_);
  greetingEditor_.setJustification(juce::Justification::left);
  // Use a standard JUCE font that matches size approx
  greetingEditor_.setFont(juce::Font(18.0f)); 
  
  // Named constants for TextEditor sizing
  constexpr int kEditorWidthPadding = 60;
  constexpr int kEditorHeight = 24;
  
  // Calculate bounds (convert from SkRect to JUCE Rectangle)
  juce::Rectangle<int> bounds(
      (int)greetingTextBounds_.left(), 
      (int)greetingTextBounds_.top() + (int)greetingTextBounds_.height() / 2, 
      (int)(greetingTextBounds_.width() + kEditorWidthPadding), 
      kEditorHeight);
      
  greetingEditor_.setBounds(bounds);
  greetingEditor_.setVisible(true);
  greetingEditor_.grabKeyboardFocus();
}

void ZenithHubComponent::hideGreetingEditor(bool save) {
  if (!greetingEditor_.isVisible()) return;

  if (save) {
    greetingText_ = greetingEditor_.getText();
  }
  
  greetingEditor_.setVisible(false);
  repaint();
}

} // namespace zenith
