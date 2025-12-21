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

  textPaint_.setAntiAlias(true);
  textPaint_.setColor(colors::TEXT_PRIMARY);
  
  subPaint_.setAntiAlias(true);
  subPaint_.setColor(withAlpha(colors::TEXT_PRIMARY, 0.6f));

  // Initialize Aurora Background
  auroraBackground_ = std::make_unique<AuroraBackground>();

  // Start fade-in
  alpha_.setTarget(0.0f, 0);
  alpha_.setTarget(1.0f, 600, AnimatedValue::EasingCurve::EaseOut);

  startTimerHz(60);

  // Initialize Greeting Editor
  addChildComponent(greetingEditor_);
  greetingEditor_.setVisible(false);
  greetingEditor_.setMultiLine(false);
  greetingEditor_.setReturnKeyStartsNewLine(false);

  // Configure callbacks for safe hiding
  greetingEditor_.onReturnKey = [this]() {
    greetingText_ = greetingEditor_.getText();
    greetingEditor_.setVisible(false);
    repaint();
  };

  greetingEditor_.onEscapeKey = [this]() { greetingEditor_.setVisible(false); };
  greetingEditor_.onFocusLost = [this]() { greetingEditor_.setVisible(false); };

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

  mainCardBounds_ = SkRect::MakeXYWH(cardX, cardY, cardW, cardH);

  // Internal Layout
  float padding = 40.0f; // Generous padding
  float gridGap = 40.0f; // Requested 40px grid gap
  float headerHeight = 140.0f;

  // Define Content Area (Main Card minus padding and header)
  juce::Rectangle<float> contentRect(
      cardX + padding, 
      cardY + padding + headerHeight,
      cardW - (padding * 2), 
      cardH - (padding * 2) - headerHeight);

  // 1. Layout Left (Recent) vs Right (Sidebar) using ZenithLayout
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
      // sidebarX/Rect used for sub-layout
      juce::Rectangle<float> sidebarRect = s; 

      // 2. Layout Sidebar (Account, Button, Templates)
      // Account: Fixed 100px
      // Button: Fixed 60px
      // Templates: Flex 1 (remaining)
      auto sidebarRows = ZenithLayout::begin()
          .withFloatBounds(sidebarRect)
          .withGap(padding) // Matches original vertical logic
          .addFlexItem(juce::FlexItem().withHeight(100.0f))                                   // Account
          .addFlexItem(juce::FlexItem().withHeight(60.0f))                                    // New Project Btn
          .addFlexItem(juce::FlexItem().withFlex(1.0f))                                       // Templates
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
  
  // Profile Button (Manual sub-positioning within account area remains ok as it's specific rendering)
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

  // Update Template Cards (Larger with icons)
  float tCardH = 90.0f;
  for (size_t i = 0; i < templates_.size(); ++i) {
    float tx = templatesArea_.fLeft;
    float ty = templatesArea_.fTop + 50.0f + (i * (tCardH + cardGap));
    templates_[i].bounds =
        SkRect::MakeXYWH(tx, ty, templatesArea_.width(), tCardH);
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
    float headerX = mainCardBounds_.fLeft + 40;
    float headerY = mainCardBounds_.fTop + 60;
    
    // Header with helper
    SkRect headerRect = SkRect::MakeXYWH(headerX, headerY - 48.0f, 300.0f, 48.0f);
    drawText(canvas, "Zenith Hub", headerRect, titleFont_, textPaint_, false);

    // Subtitle
    SkString greeting(greetingText_.toRawUTF8());
    SkRect bounds;
    subFont_.measureText(greeting.c_str(), greeting.size(), SkTextEncoding::kUTF8, &bounds);
    
    float subX = headerX;
    float subY = headerY + 32;
    
    // Store bounds for interaction
    greetingTextBounds_ = SkRect::MakeXYWH(subX, subY - bounds.height(), bounds.width(), bounds.height() + 4);
    
    // Draw using helper with precise baseline alignment
    SkRect helperBounds = SkRect::MakeXYWH(subX, subY - bounds.height(), bounds.width(), bounds.height());
    drawText(canvas, greetingText_, helperBounds, subFont_, subPaint_, false);

    // Draw Edit Icon using the icon system
    constexpr float kGreetingIconSize = 16.0f;
    greetingEditIconBounds_ = SkRect::MakeXYWH(subX + bounds.width() + 10, subY - 14, kGreetingIconSize, kGreetingIconSize);
    
    icons::IconStyle iconStyle;
    iconStyle.color = (isGreetingHovered_) ? colors::CYAN : withAlpha(colors::TEXT_SECONDARY, 0.5f);
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
  canvas->drawString("Recent Projects", recentArea_.fLeft,
                     recentArea_.fTop - 20, headerFont_, textPaint_);

  // Empty state check
  if (recentProjects_.empty()) {
    canvas->drawString("No recent projects yet.", recentArea_.fLeft,
                       recentArea_.fTop + 30, cardTitleFont_, textPaint_);

    canvas->drawString("Click 'New Project' to get started!", recentArea_.fLeft,
                       recentArea_.fTop + 55, statusFont_, textPaint_);
    return;
  }

  for (const auto &proj : recentProjects_) {
    SkRRect rrect = SkRRect::MakeRectXY(proj.bounds, 12.0f, 12.0f);

    // Card background with better depth
    bool isSelected = (selectedSection_ == Section::RecentProjects && (int)i == selectedIndex_);
    bool active = proj.isHovered || isSelected;

    SkPaint cardPaint;
    cardPaint.setColor(active ? withAlpha(colors::BG_LIGHT, 0.15f)
                              : withAlpha(colors::BG_LIGHT, 0.08f));
    cardPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, cardPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(isSelected ? 2.0f : 1.0f);
    borderPaint.setColor(active
                             ? withAlpha(proj.accent, 0.6f)
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
    SkRect pathBounds = iconPath.getBounds();
    if (!pathBounds.isEmpty()) {
      float iconSize = 40.0f; // Fits nicely in 86x86
      float scale =
          iconSize / std::max(pathBounds.width(), pathBounds.height());

      SkMatrix matrix;
      matrix.reset();
      matrix.postTranslate(-pathBounds.centerX(), -pathBounds.centerY());
      matrix.postScale(scale, scale);
      matrix.postTranslate(thumbRect.centerX(), thumbRect.centerY());

      SkPaint iconPaint;
      iconPaint.setColor(withAlpha(proj.accent, 0.8f));
      iconPaint.setAntiAlias(true);

      SkPath scaledPath;
      iconPath.transform(matrix, &scaledPath);
      canvas->drawPath(scaledPath, iconPaint);
    } else {
        // Fallback for empty path
        SkPaint iconPaint;
        iconPaint.setColor(withAlpha(proj.accent, 0.8f));
        iconPaint.setAntiAlias(true);
        canvas->drawCircle(thumbRect.centerX(), thumbRect.centerY(), 12, iconPaint);
    }

    // Text content
    float textX = thumbRect.right() + 16;

    SkPaint cardTextPaint = textPaint_;
    cardTextPaint.setColor(colors::TEXT_PRIMARY);
    canvas->drawString(proj.name.toStdString().c_str(), textX,
                       proj.bounds.fTop + 35, cardTitleFont_, cardTextPaint);

    cardTextPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.55f));
    canvas->drawString(proj.date.toStdString().c_str(), textX,
                       proj.bounds.fTop + 60, cardDateFont_, cardTextPaint);

    // Genre badge
    if (proj.genre.isNotEmpty()) {
      SkPaint genrePaint = textPaint_;
      genrePaint.setColor(proj.accent);
      canvas->drawString(proj.genre.toStdString().c_str(), textX,
                         proj.bounds.fTop + 82, cardGenreFont_, genrePaint);
    }
  }
}

void ZenithHubComponent::drawTemplates(SkCanvas *canvas) {
  canvas->drawString("Quick Start", templatesArea_.fLeft,
                     templatesArea_.fTop - 20, headerFont_, textPaint_);

  for (const auto &tmpl : templates_) {
    SkRRect rrect = SkRRect::MakeRectXY(tmpl.bounds, 12.0f, 12.0f);

    // Card background
    bool isSelected = (selectedSection_ == Section::Templates && (int)i == selectedIndex_);
    bool active = tmpl.isHovered || isSelected;

    SkPaint cardPaint;
    cardPaint.setColor(active ? withAlpha(tmpl.color, 0.15f)
                              : withAlpha(colors::BG_LIGHT, 0.08f));
    cardPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, cardPaint);

    // Border
    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(isSelected ? 2.0f : 1.0f);
    borderPaint.setColor(active
                             ? withAlpha(tmpl.color, 0.8f)
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
    SkRect pathBounds = iconPath.getBounds();
    if (!pathBounds.isEmpty()) {
      float scale =
          (iconSize * 0.5f) / std::max(pathBounds.width(), pathBounds.height());

      SkMatrix matrix;
      // Center the path at (0,0) then scale, then translate to destination
      matrix.reset();
      matrix.postTranslate(-pathBounds.centerX(), -pathBounds.centerY());
      matrix.postScale(scale, scale);
      matrix.postTranslate(iconBounds.centerX(), iconBounds.centerY());

      SkPaint iconPaint;
      iconPaint.setColor(tmpl.color);
      iconPaint.setAntiAlias(true);

      SkPath scaledPath;
      iconPath.transform(matrix, &scaledPath);
      canvas->drawPath(scaledPath, iconPaint);
    } else {
      SkPaint iconPaint;
      iconPaint.setColor(tmpl.color);
      iconPaint.setAntiAlias(true);
      canvas->drawCircle(iconBounds.centerX(), iconBounds.centerY(), 12,
                         iconPaint);
    }

    // Text
    canvas->drawString(tmpl.name.toStdString().c_str(), iconBounds.right() + 16,
                       tmpl.bounds.centerY() + 6, templateFont_, textPaint_);
  }
}

void ZenithHubComponent::drawAccount(SkCanvas *canvas) {
  canvas->drawString("Collaborations", accountArea_.fLeft,
                     accountArea_.fTop - 20, headerFont_, textPaint_);

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
  canvas->drawString("SoundDesigner99", profileBounds_.fLeft + 90,
                     profileBounds_.centerY() - 6, profileFont_, textPaint_);

  // FIXED: Proper semantic color for online status
  SkPaint statusPaintr; // Note: statusPaint was local in original code
  statusPaintr.setColor(SkColorSetARGB(255, 16, 185, 129)); 
  statusPaintr.setAntiAlias(true);
  canvas->drawString("● Online", profileBounds_.fLeft + 90,
                     profileBounds_.centerY() + 18, statusFont_, statusPaintr);
}

void ZenithHubComponent::drawNewProjectButton(SkCanvas *canvas) {
  SkRRect rrect = SkRRect::MakeRectXY(newProjectButtonBounds_, 12.0f, 12.0f);

  // FIXED: Solid professional blue with proper states
  bool isSelected = (selectedSection_ == Section::NewProject);
  bool active = isNewProjectHovered_ || isSelected;

  SkColor buttonColor =
      active
          ? SkColorSetARGB(255, 96, 165, 250)  // Hover: lighter blue
          : SkColorSetARGB(255, 59, 130, 246); // Default: professional blue

  SkPaint btnPaint;
  btnPaint.setColor(buttonColor);
  btnPaint.setAntiAlias(true);

  // Shadow on hover
  if (active) {
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
  SkPaint btnTextPaint;
  btnTextPaint.setColor(SK_ColorWHITE);
  btnTextPaint.setAntiAlias(true);

  SkString text("New Project");
  SkRect textBounds;
  buttonFont_.measureText(text.c_str(), text.size(), SkTextEncoding::kUTF8,
                      &textBounds);

  float tx = newProjectButtonBounds_.centerX() - (textBounds.width() / 2.0f);
  float ty =
      newProjectButtonBounds_.centerY() + (textBounds.height() / 2.0f) - 4.0f;

  canvas->drawString(text, tx, ty, buttonFont_, btnTextPaint);
}

void ZenithHubComponent::mouseMove(const juce::MouseEvent &e) {
  SkPoint pt = {(float)e.x, (float)e.y};
  bool needsUpdate = false;

  if (selectedSection_ != Section::None) {
      selectedSection_ = Section::None;
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
  if (greetingEditor_->isVisible()) return;

  greetingEditor_->setText(greetingText_);
  greetingEditor_->setJustification(juce::Justification::left);
  // Use a standard JUCE font that matches size approx
  greetingEditor_->setFont(juce::Font(18.0f)); 
 
  
  // Named constants for TextEditor sizing
  constexpr int kEditorWidthPadding = 60;
  constexpr int kEditorHeight = 24;
  
  // Calculate bounds (convert from SkRect to JUCE Rectangle)
  juce::Rectangle<int> bounds(
      (int)greetingTextBounds_.left(), 
      (int)greetingTextBounds_.top() + (int)greetingTextBounds_.height() / 2, 
      (int)(greetingTextBounds_.width() + kEditorWidthPadding), 
      kEditorHeight);
      
  greetingEditor_->setBounds(bounds);
  greetingEditor_->setVisible(true);
  greetingEditor_->selectAll();
  greetingEditor_->grabKeyboardFocus();
}

void ZenithHubComponent::keyPressed(const juce::KeyPress &key) {
  if (key == juce::KeyPress::returnKey) {
    triggerSelection();
    return;
  }

  int dx = 0;
  int dy = 0;
  if (key.isKeyCode(juce::KeyPress::upKey)) dy = -1;
  else if (key.isKeyCode(juce::KeyPress::downKey)) dy = 1;
  else if (key.isKeyCode(juce::KeyPress::leftKey)) dx = -1;
  else if (key.isKeyCode(juce::KeyPress::rightKey)) dx = 1;

  if (dx != 0 || dy != 0) {
    moveSelection(dx, dy);
  }
}

void ZenithHubComponent::moveSelection(int dx, int dy) {
  if (selectedSection_ == Section::None) {
    selectedSection_ = Section::RecentProjects;
    selectedIndex_ = 0;
    repaint();
    return;
  }

  if (selectedSection_ == Section::RecentProjects) {
    int row = selectedIndex_ / 2;
    int col = selectedIndex_ % 2;
    
    if (dx == 1 && col == 1) {
       // Move to Sidebar
       selectedSection_ = Section::NewProject; // Default to button
       selectedIndex_ = -1;
    } else if (dx == -1 && col == 0) {
       // Stay
    } else {
       // Navigation within grid
       int newRow = row + dy;
       int newCol = col + dx;
       int newIdx = (newRow * 2) + newCol;
       
       if (newIdx >= 0 && newIdx < (int)recentProjects_.size()) {
           selectedIndex_ = newIdx;
       }
    }
  } else if (selectedSection_ == Section::NewProject) {
      if (dy == 1 && !templates_.empty()) {
          selectedSection_ = Section::Templates;
          selectedIndex_ = 0;
      } else if (dy == -1) {
          // Account not selectable, stay
      }
      
      if (dx == -1) {
          selectedSection_ = Section::RecentProjects;
          selectedIndex_ = std::min((int)recentProjects_.size() - 1, 1);
      }
  } else if (selectedSection_ == Section::Templates) {
      if (dy == -1 && selectedIndex_ == 0) {
         selectedSection_ = Section::NewProject;
         selectedIndex_ = -1;
      } else if (dx == -1) {
         selectedSection_ = Section::RecentProjects;
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
    if (selectedSection_ == Section::RecentProjects && selectedIndex_ >= 0 && selectedIndex_ < recentProjects_.size()) {
        if (onLoadProject_) onLoadProject_(recentProjects_[selectedIndex_].path);
        dismiss();
    } else if (selectedSection_ == Section::NewProject) {
        if (onNewProject_) onNewProject_();
        dismiss();
    } else if (selectedSection_ == Section::Templates && selectedIndex_ >= 0 && selectedIndex_ < templates_.size()) {
        if (onNewProject_) onNewProject_();
        dismiss();
    }
}

void ZenithHubComponent::drawText(SkCanvas* canvas, const juce::String& text, const SkRect& bounds, 
                const SkFont& font, const SkPaint& paint, bool centerVertical) {
    SkString skText(text.toRawUTF8());
    SkRect textBounds;
    font.measureText(skText.c_str(), skText.size(), SkTextEncoding::kUTF8, &textBounds);
    
    float x = bounds.fLeft;
    float y = bounds.fTop + textBounds.height(); // Default roughly top aligned
    
    if (centerVertical) {
        y = bounds.centerY() + (textBounds.height() * 0.5f) - textBounds.fBottom;
    } else {
        // Find cap height or just use height
        y = bounds.fBottom;
    }
    
    canvas->drawString(skText, x, y, font, paint);
}


} // namespace zenith
