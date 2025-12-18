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
  greetingEditor_ = std::make_unique<juce::TextEditor>();
  addChildComponent(greetingEditor_.get());
  greetingEditor_->setVisible(false);
  greetingEditor_->setMultiLine(false);
  greetingEditor_->setReturnKeyStartsNewLine(false);
  greetingEditor_->setSelectAllWhenFocused(true);

  auto safeDismiss = [this]() { hideGreetingEditor(false); };
  greetingEditor_->onEscapeKey = safeDismiss;
  greetingEditor_->onFocusLost = safeDismiss;
  greetingEditor_->onReturnKey = [this]() { hideGreetingEditor(true); };

  // Initialize Aurora Background
  auroraBackground_ = std::make_unique<AuroraBackground>();

  alpha_.setTarget(0.0f, 0);
  alpha_.setTarget(1.0f, 600, AnimatedValue::EasingCurve::EaseOut);

  startTimerHz(60);
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
  for (auto &p : recentProjects_) p.isHovered = false;
  for (auto &t : templates_) t.isHovered = false;
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
    if (recentProjects_.size() >= 6) break;
  }

  if (isVisible()) {
    updateLayout();
    repaint();
  }
}

SkColor ZenithHubComponent::getAccentColorForGenre(const juce::String &genre) {
  juce::String g = genre.toLowerCase();
  if (g.contains("electronic") || g.contains("edm") || g.contains("synth")) return colors::CYAN;
  if (g.contains("orchestral") || g.contains("cinematic") || g.contains("score")) return colors::VIOLET;
  if (g.contains("jazz") || g.contains("swing")) return colors::NEON_PINK;
  if (g.contains("techno") || g.contains("house") || g.contains("dance")) return colors::NEON_GREEN;
  if (g.contains("ambient") || g.contains("chill")) return colors::BLUE;
  if (g.contains("rock") || g.contains("metal")) return colors::AMBER;
  if (g.contains("hip") || g.contains("rap") || g.contains("trap")) return colors::MAGENTA;
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

  float cardW = std::min(1100.0f, w * 0.9f);
  float cardH = std::min(750.0f, h * 0.85f);
  float cardX = (w - cardW) * 0.5f;
  float cardY = (h - cardH) * 0.5f;

  mainCardBounds_ = SkRect::MakeXYWH(cardX, cardY, cardW, cardH);

<<<<<<< HEAD
  // Internal Layout
  float padding = 40.0f; // Generous padding
  float gridGap = 40.0f; // Requested 40px grid gap

  // Define Content Area (Main Card minus padding and header)
=======
  float padding = 40.0f; 
  float gridGap = 40.0f; 
>>>>>>> origin/master
  float headerHeight = 140.0f;
  juce::Rectangle<float> contentRect(
      cardX + padding, 
      cardY + padding + headerHeight,
      cardW - (padding * 2), 
      cardH - (padding * 2) - headerHeight);

<<<<<<< HEAD
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
=======
  juce::Rectangle<float> contentRect(
      cardX + padding, 
      cardY + padding + headerHeight,
      cardW - (padding * 2), 
      cardH - (padding * 2) - headerHeight);

  auto mainColumns = ZenithLayout::begin()
      .withFloatBounds(contentRect)
      .withGap(gridGap)
      .addFlexItem(juce::FlexItem().withFlex(0.6f)) 
      .addFlexItem(juce::FlexItem().withFlex(0.4f)) 
      .layout(juce::FlexBox::Direction::row);

  if (mainColumns.size() >= 2) {
      recentArea_ = SkRect::MakeXYWH(mainColumns[0].getX(), mainColumns[0].getY(), 
                                     mainColumns[0].getWidth(), mainColumns[0].getHeight());
      
      auto sidebarRows = ZenithLayout::begin()
          .withFloatBounds(mainColumns[1])
>>>>>>> origin/master
          .withGap(padding)
          .addFlexItem(juce::FlexItem().withHeight(100.0f))
          .addFlexItem(juce::FlexItem().withHeight(60.0f))
          .addFlexItem(juce::FlexItem().withFlex(1.0f))
          .layout(juce::FlexBox::Direction::column);
          
      if (sidebarRows.size() >= 3) {
<<<<<<< HEAD
          auto a = sidebarRows[0];
          accountArea_ = SkRect::MakeXYWH(a.getX(), a.getY(), a.getWidth(), a.getHeight());
          
          auto b = sidebarRows[1];
          newProjectButtonBounds_ = SkRect::MakeXYWH(b.getX(), b.getY(), b.getWidth(), b.getHeight());
          
          auto t = sidebarRows[2];
          templatesArea_ = SkRect::MakeXYWH(t.getX(), t.getY(), t.getWidth(), t.getHeight());

          // Profile Button - SAFE calculation inside valid sidebar check
          profileBounds_ =
            SkRect::MakeXYWH(accountArea_.fLeft, accountArea_.fTop + 50.0f,
                   accountArea_.width(), 90.0f);
      } else {
        accountArea_.setEmpty();
        newProjectButtonBounds_.setEmpty();
        templatesArea_.setEmpty();
        profileBounds_.setEmpty();
      }
  } else {
    // Reset layout on failure
    recentArea_.setEmpty();
    accountArea_.setEmpty();
    newProjectButtonBounds_.setEmpty();
    templatesArea_.setEmpty();
    profileBounds_.setEmpty();
  }
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

      // Profile Button - SAFE calculation inside valid sidebar check
      profileBounds_ =
        SkRect::MakeXYWH(accountArea_.fLeft, accountArea_.fTop + 50.0f,
               accountArea_.width(), 90.0f);
  } else {
    accountArea_.setEmpty();
    newProjectButtonBounds_.setEmpty();
    templatesArea_.setEmpty();
    profileBounds_.setEmpty();
  }

  // Update Recent Project Cards Layout (Grid)
  if (!recentArea_.isEmpty()) {
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
  }

  // Update Template Cards
  if (!templatesArea_.isEmpty()) {
    float tCardH = 90.0f;
    float cardGap = 16.0f;
    for (size_t i = 0; i < templates_.size(); ++i) {
      float tx = templatesArea_.fLeft;
      float ty = templatesArea_.fTop + 50.0f + (i * (tCardH + cardGap));
      templates_[i].bounds = SkRect::MakeXYWH(tx, ty, templatesArea_.width(), tCardH);
    }
  }

  if (greetingEditor_.isVisible()) {
      showGreetingEditor(); // Re-layout editor
=======
          accountArea_ = SkRect::MakeXYWH(sidebarRows[0].getX(), sidebarRows[0].getY(), 
                                          sidebarRows[0].getWidth(), sidebarRows[0].getHeight());
          newProjectButtonBounds_ = SkRect::MakeXYWH(sidebarRows[1].getX(), sidebarRows[1].getY(), 
                                                     sidebarRows[1].getWidth(), sidebarRows[1].getHeight());
          templatesArea_ = SkRect::MakeXYWH(sidebarRows[2].getX(), sidebarRows[2].getY(), 
                                            sidebarRows[2].getWidth(), sidebarRows[2].getHeight());
      }
  }

  profileBounds_ = SkRect::MakeXYWH(accountArea_.fLeft, accountArea_.fTop + 50.0f,
                                    accountArea_.width(), 90.0f);

  float cardGap = 16.0f;
  float pCardW = (recentArea_.width() - cardGap) / 2.0f;
  float pCardH = 110.0f;

  for (size_t i = 0; i < recentProjects_.size(); ++i) {
    int row = (int)i / 2;
    int col = (int)i % 2;
    float px = recentArea_.fLeft + (col * (pCardW + cardGap));
    float py = recentArea_.fTop + (row * (pCardH + cardGap));
    recentProjects_[i].bounds = SkRect::MakeXYWH(px, py, pCardW, pCardH);
  }

  float tCardH = 90.0f;
  for (size_t i = 0; i < templates_.size(); ++i) {
    float tx = templatesArea_.fLeft;
    float ty = templatesArea_.fTop + 50.0f + (i * (tCardH + cardGap));
    templates_[i].bounds = SkRect::MakeXYWH(tx, ty, templatesArea_.width(), tCardH);
  }

  if (greetingEditor_.isVisible()) {
      showGreetingEditor(); 
>>>>>>> origin/master
  }
}

void ZenithHubComponent::timerCallback() {
  animationTime_ += 0.016f;
  alpha_.update(16.0f);
  if (alpha_.isAnimating() || auroraBackground_) repaint();
}

void ZenithHubComponent::show() {
  setVisible(true);
  alpha_.setTarget(1.0f, 600, AnimatedValue::EasingCurve::EaseOut);
}

void ZenithHubComponent::dismiss() {
  alpha_.setTarget(0.0f, 400, AnimatedValue::EasingCurve::EaseIn);
  if (onDismiss_) onDismiss_();
}

void ZenithHubComponent::drawSkia(SkCanvas *canvas) {
  float opacity = alpha_.getCurrentValue();
  if (opacity <= 0.001f) return;

  canvas->saveLayerAlpha(nullptr, (U8CPU)(opacity * 255));
  drawBackground(canvas);

  SkPaint shadowPaint;
  shadowPaint.setColor(SkColorSetARGB(80, 0, 0, 0));
  shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 30.0f));
  shadowPaint.setAntiAlias(true);
  SkRRect shadowRRect = SkRRect::MakeRectXY(mainCardBounds_.makeOutset(5.0f, 5.0f), 16.0f, 16.0f);
  canvas->drawRRect(shadowRRect, shadowPaint);

  GlassmorphicPanel::draw(canvas, mainCardBounds_, GlassmorphicPanel::Style::Floating);

<<<<<<< HEAD
  // Header with proper hierarchy
  textPaint_.setColor(colors::TEXT_PRIMARY);

  float headerX = mainCardBounds_.fLeft + 40;
  float headerY = mainCardBounds_.fTop + 60;

  // Use drawText helper for title as per code review
  // We construct a specific bounds for the title or just use the whole card
  // width relative to headerX
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

  // Agent 5: Use Helper
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
=======
  {
    float headerX = mainCardBounds_.fLeft + 40;
    float headerY = mainCardBounds_.fTop + 60;
    
    SkRect titleRect = SkRect::MakeXYWH(headerX, headerY - 48.0f, 400.0f, 48.0f);
    drawText(canvas, "Zenith Hub", titleRect, titleFont_, mainTextPaint_, false);

    SkString greeting(greetingText_.toRawUTF8());
    SkRect bounds;
    subFont_.measureText(greeting.c_str(), greeting.size(), SkTextEncoding::kUTF8, &bounds);
    
    float subX = headerX;
    float subY = headerY + 32;
    greetingTextBounds_ = SkRect::MakeXYWH(subX, subY - bounds.height(), bounds.width(), bounds.height() + 4);
    
    SkRect subRect = SkRect::MakeXYWH(subX, subY - 20, bounds.width() + 10, 24);
    drawText(canvas, greetingText_, subRect, subFont_, subTextPaint_, false);

    constexpr float kGreetingIconSize = 16.0f;
    greetingEditIconBounds_ = SkRect::MakeXYWH(subX + bounds.width() + 10, subY - 14, kGreetingIconSize, kGreetingIconSize);
    
    icons::IconStyle iconStyle;
    iconStyle.color = isGreetingHovered_ ? colors::CYAN : withAlpha(colors::TEXT_SECONDARY, 0.5f);
    iconStyle.strokeWidth = icons::STROKE_THIN;
    icons::drawIconCentered(canvas, icons::Edit(), greetingEditIconBounds_, kGreetingIconSize, iconStyle);
  }
>>>>>>> origin/master

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
  SkPaint bgPaint;
  bgPaint.setColor(colors::BG_DARKEST);
  canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), bgPaint);
}

void ZenithHubComponent::drawRecentProjects(SkCanvas *canvas) {
<<<<<<< HEAD
  textPaint_.setColor(colors::TEXT_PRIMARY);
  canvas->drawString("Recent Projects", recentArea_.fLeft,
                     recentArea_.fTop - 20, headerFont_, textPaint_);

  // Empty state check
  if (recentProjects_.empty()) {
    SkPaint emptyStatePaint = textPaint_;
    emptyStatePaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.35f));

    SkFont bodyFont = design::getSkFont(16.0f, design::FontWeight::Regular);
    canvas->drawString("No recent projects yet.", recentArea_.fLeft,
                       recentArea_.fTop + 30, bodyFont_, emptyStatePaint);

    canvas->drawString("Click 'New Project' to get started!", recentArea_.fLeft,
                       recentArea_.fTop + 55, statusFont_, emptyStatePaint);
=======
  drawText(canvas, "Recent Projects", SkRect::MakeXYWH(recentArea_.fLeft, recentArea_.fTop - 40, 300, 30), 
           headerFont_, mainTextPaint_, false);

  if (recentProjects_.empty()) {
    drawText(canvas, "No recent projects yet.", SkRect::MakeXYWH(recentArea_.fLeft, recentArea_.fTop + 10, 300, 20), 
             cardDateFont_, subTextPaint_, false);
>>>>>>> origin/master
    return;
  }

  for (size_t i = 0; i < recentProjects_.size(); ++i) {
    const auto &proj = recentProjects_[i];
    bool isSelected = (this->selectedSection_ == ZenithHubComponent::Section::RecentProjects && (int)i == selectedIndex_);
    bool active = proj.isHovered || isSelected;

    SkRRect rrect = SkRRect::MakeRectXY(proj.bounds, 12.0f, 12.0f);
    SkPaint cardPaint;
    cardPaint.setColor(active ? withAlpha(colors::BG_LIGHT, 0.15f) : withAlpha(colors::BG_LIGHT, 0.08f));
    cardPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, cardPaint);

    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(isSelected ? 2.0f : 1.0f);
    borderPaint.setColor(active ? withAlpha(proj.accent, 0.6f) : withAlpha(colors::TEXT_PRIMARY, 0.06f));
    borderPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, borderPaint);

    SkRect thumbRect = SkRect::MakeXYWH(proj.bounds.fLeft + 12, proj.bounds.fTop + 12, 86, 86);
    SkPaint thumbPaint;
    thumbPaint.setColor(withAlpha(proj.accent, 0.25f));
    thumbPaint.setAntiAlias(true);
    canvas->drawRRect(SkRRect::MakeRectXY(thumbRect, 8.0f, 8.0f), thumbPaint);

    // Draw genre-specific icon
    SkPath iconPath = icons::Project(); // Default
    auto it = kGenreIconMap.find(proj.genre.toLowerCase());
    if (it != kGenreIconMap.end()) {
      iconPath = it->second();
    }

<<<<<<< HEAD
    // Draw icon centered in thumbnail
    SkRect pathBounds = iconPath.getBounds();

    SkPaint iconPaint;
    iconPaint.setColor(withAlpha(proj.accent, 0.8f));
    iconPaint.setAntiAlias(true);

    if (!pathBounds.isEmpty()) {
      float iconSize = 40.0f; // Fits nicely in 86x86
      float scale =
          iconSize / std::max(pathBounds.width(), pathBounds.height());

      SkMatrix matrix;
      matrix.reset();
      matrix.postTranslate(-pathBounds.centerX(), -pathBounds.centerY());
      matrix.postScale(scale, scale);
      matrix.postTranslate(thumbRect.centerX(), thumbRect.centerY());

      SkPath scaledPath;
      iconPath.transform(matrix, &scaledPath);
      canvas->drawPath(scaledPath, iconPaint);
    } else {
      // Fallback for empty path
      canvas->drawCircle(thumbRect.centerX(), thumbRect.centerY(), 12,
                         iconPaint);
    }
=======
    SkRect pathBounds = iconPath.getBounds();
    SkPaint iconPaint;
    iconPaint.setColor(withAlpha(proj.accent, 0.8f));
    iconPaint.setAntiAlias(true);
>>>>>>> origin/master

    if (!pathBounds.isEmpty()) {
      float iconSize = 40.0f;
      float scale = iconSize / std::max(pathBounds.width(), pathBounds.height());

<<<<<<< HEAD
    SkPaint cardTextPaint = textPaint_;
    cardTextPaint.setColor(colors::TEXT_PRIMARY);
    canvas->drawString(proj.name.toStdString().c_str(), textX,
                       proj.bounds.fTop + 35, cardTitleFont_, cardTextPaint);

    cardTextPaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.55f));
    canvas->drawString(proj.date.toStdString().c_str(), textX,
                       proj.bounds.fTop + 60, cardDateFont_, cardTextPaint);

    // Genre badge
    if (proj.genre.isNotEmpty()) {
      cardTextPaint.setColor(proj.accent);
      canvas->drawString(proj.genre.toStdString().c_str(), textX,
                         proj.bounds.fTop + 82, cardGenreFont_, cardTextPaint);
=======
      SkMatrix matrix;
      matrix.reset();
      matrix.postTranslate(-pathBounds.centerX(), -pathBounds.centerY());
      matrix.postScale(scale, scale);
      matrix.postTranslate(thumbRect.centerX(), thumbRect.centerY());

      SkPath scaledPath;
      iconPath.transform(matrix, &scaledPath);
      canvas->drawPath(scaledPath, iconPaint);
    } else {
      canvas->drawCircle(thumbRect.centerX(), thumbRect.centerY(), 12, iconPaint);
>>>>>>> origin/master
    }

    float textX = thumbRect.right() + 16;
    drawText(canvas, proj.name, SkRect::MakeXYWH(textX, proj.bounds.fTop + 20, proj.bounds.width() - 110, 24), 
             cardTitleFont_, mainTextPaint_, false);
    drawText(canvas, proj.date, SkRect::MakeXYWH(textX, proj.bounds.fTop + 45, proj.bounds.width() - 110, 20), 
             cardDateFont_, subTextPaint_, false);
    drawText(canvas, proj.genre, SkRect::MakeXYWH(textX, proj.bounds.fTop + 65, proj.bounds.width() - 110, 20), 
             cardGenreFont_, mainTextPaint_, false);
  }
}

void ZenithHubComponent::drawTemplates(SkCanvas *canvas) {
  drawText(canvas, "Quick Start", SkRect::MakeXYWH(templatesArea_.fLeft, templatesArea_.fTop - 40, 200, 30), 
           headerFont_, mainTextPaint_, false);

  for (size_t i = 0; i < templates_.size(); ++i) {
    const auto &tmpl = templates_[i];
    bool isSelected = (this->selectedSection_ == ZenithHubComponent::Section::Templates && (int)i == selectedIndex_);
    bool active = tmpl.isHovered || isSelected;

<<<<<<< HEAD
  for (size_t i = 0; i < templates_.size(); ++i) {
    const auto &tmpl = templates_[i];

=======
>>>>>>> origin/master
    SkRRect rrect = SkRRect::MakeRectXY(tmpl.bounds, 12.0f, 12.0f);
    SkPaint cardPaint;
    cardPaint.setColor(active ? withAlpha(tmpl.color, 0.15f) : withAlpha(colors::BG_LIGHT, 0.08f));
    cardPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, cardPaint);

    float iconSize = 48.0f;
    SkRect iconBounds = SkRect::MakeXYWH(tmpl.bounds.fLeft + 20, tmpl.bounds.centerY() - iconSize * 0.5f, iconSize, iconSize);
    SkPaint iconBgPaint;
    iconBgPaint.setColor(withAlpha(tmpl.color, 0.2f));
    iconBgPaint.setAntiAlias(true);
    canvas->drawRoundRect(iconBounds, 8.0f, 8.0f, iconBgPaint);

    SkPaint iconPaint;
    iconPaint.setColor(tmpl.color);
    iconPaint.setAntiAlias(true);
    
    // Draw template-specific icon
    SkPath iconPath = icons::Template(); // fallback
    auto it = kTemplateIconMap.find(tmpl.icon);
    if (it != kTemplateIconMap.end()) {
      iconPath = it->second();
    }

    icons::IconStyle iconStyle;
    iconStyle.color = tmpl.color;
    iconStyle.strokeWidth = icons::STROKE_REGULAR;
    icons::drawIconCentered(canvas, iconPath, iconBounds, iconBounds.width() * 0.6f, iconStyle);

<<<<<<< HEAD
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
    // std::map/enum for logic, not necessarily to implement the path drawing
    // if it wasn't there.
    canvas->drawCircle(iconBounds.centerX(), iconBounds.centerY(), 12,
                       iconPaint);

    // Text
    SkFont nameFont = design::getSkFont(18.0f, design::FontWeight::Bold);
    textPaint.setColor(colors::TEXT_PRIMARY);
    canvas->drawString(tmpl.name.toStdString().c_str(), iconBounds.right() + 16,
                       tmpl.bounds.centerY() + 6, nameFont, textPaint);
=======
    drawText(canvas, tmpl.name, SkRect::MakeXYWH(iconBounds.right() + 16, tmpl.bounds.centerY() - 12, 200, 24), 
             templateFont_, mainTextPaint_, false);
>>>>>>> origin/master
  }
}

void ZenithHubComponent::drawAccount(SkCanvas *canvas) {
  drawText(canvas, "Collaborations", SkRect::MakeXYWH(accountArea_.fLeft, accountArea_.fTop - 40, 200, 30), 
           headerFont_, mainTextPaint_, false);

  SkRRect rrect = SkRRect::MakeRectXY(profileBounds_, 12.0f, 12.0f);
  SkPaint bgPaint;
  bgPaint.setColor(isProfileHovered_ ? withAlpha(colors::BG_LIGHT, 0.15f) : withAlpha(colors::BG_LIGHT, 0.08f));
  bgPaint.setAntiAlias(true);
  canvas->drawRRect(rrect, bgPaint);

  float avatarSize = 48.0f;
  SkPaint avatarPaint;
  avatarPaint.setColor(colors::AMBER);
  avatarPaint.setAntiAlias(true);
  canvas->drawCircle(profileBounds_.fLeft + 30 + avatarSize * 0.5f, profileBounds_.centerY(), avatarSize * 0.5f, avatarPaint);

  drawText(canvas, "SoundDesigner99", SkRect::MakeXYWH(profileBounds_.fLeft + 90, profileBounds_.centerY() - 18, 200, 24), 
           profileFont_, mainTextPaint_, false);

  SkPaint statusPaint;
  statusPaint.setColor(SkColorSetARGB(255, 16, 185, 129)); 
  statusPaint.setAntiAlias(true);
  drawText(canvas, "● Online", SkRect::MakeXYWH(profileBounds_.fLeft + 90, profileBounds_.centerY() + 4, 100, 20), 
           statusFont_, statusPaint, false);
}

void ZenithHubComponent::drawNewProjectButton(SkCanvas *canvas) {
  bool isSelected = (this->selectedSection_ == ZenithHubComponent::Section::NewProject);
  bool active = isNewProjectHovered_ || isSelected;

  SkColor buttonColor = active ? SkColorSetARGB(255, 96, 165, 250) : SkColorSetARGB(255, 59, 130, 246);
  SkPaint btnPaint;
  btnPaint.setColor(buttonColor);
  btnPaint.setAntiAlias(true);

  if (active) {
    SkPaint shadowPaint;
    shadowPaint.setColor(withAlpha(buttonColor, 0.4f));
    shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 12.0f));
    shadowPaint.setAntiAlias(true);
    canvas->drawRRect(SkRRect::MakeRectXY(newProjectButtonBounds_.makeOutset(4.0f, 4.0f), 12.0f, 12.0f), shadowPaint);
  }

  canvas->drawRRect(SkRRect::MakeRectXY(newProjectButtonBounds_, 12.0f, 12.0f), btnPaint);
  drawText(canvas, "New Project", newProjectButtonBounds_, buttonFont_, mainTextPaint_, true);
}

void ZenithHubComponent::mouseMove(const juce::MouseEvent &e) {
  SkPoint pt = {(float)e.x, (float)e.y};
  bool needsUpdate = false;

  if (selectedSection_ != ZenithHubComponent::Section::None) {
      selectedSection_ = ZenithHubComponent::Section::None;
      selectedIndex_ = -1;
      needsUpdate = true;
  }

  for (auto &proj : recentProjects_) {
    bool h = proj.bounds.contains(pt.fX, pt.fY);
    if (h != proj.isHovered) { proj.isHovered = h; needsUpdate = true; }
  }

  for (auto &tmpl : templates_) {
    bool h = tmpl.bounds.contains(pt.fX, pt.fY);
    if (h != tmpl.isHovered) { tmpl.isHovered = h; needsUpdate = true; }
  }

  bool ph = profileBounds_.contains(pt.fX, pt.fY);
  if (ph != isProfileHovered_) { isProfileHovered_ = ph; needsUpdate = true; }

  bool nph = newProjectButtonBounds_.contains(pt.fX, pt.fY);
  if (nph != isNewProjectHovered_) { isNewProjectHovered_ = nph; needsUpdate = true; }

<<<<<<< HEAD
  bool gh = greetingTextBounds_.contains(pt.fX, pt.fY) ||
            greetingEditIconBounds_.contains(pt.fX, pt.fY);
  if (gh != isGreetingHovered_) {
    isGreetingHovered_ = gh;
    needsUpdate = true;
  }
=======
  bool gh = greetingTextBounds_.contains(pt.fX, pt.fY) || greetingEditIconBounds_.contains(pt.fX, pt.fY);
  if (gh != isGreetingHovered_) { isGreetingHovered_ = gh; needsUpdate = true; }
>>>>>>> origin/master

  if (needsUpdate) repaint();
}

void ZenithHubComponent::mouseDown(const juce::MouseEvent &e) {
  SkPoint pt = {(float)e.x, (float)e.y};
  if (!mainCardBounds_.contains(pt.fX, pt.fY)) return;

  for (const auto &proj : recentProjects_) {
    if (proj.bounds.contains(pt.fX, pt.fY)) {
      if (onLoadProject_ && proj.path.existsAsFile()) onLoadProject_(proj.path);
      dismiss();
      return;
    }
  }

  for (const auto &tmpl : templates_) {
    if (tmpl.bounds.contains(pt.fX, pt.fY)) {
      if (onNewProject_) onNewProject_();
      dismiss();
      return;
    }
  }

  if (newProjectButtonBounds_.contains(pt.fX, pt.fY)) {
    if (onNewProject_) onNewProject_();
    dismiss();
    return;
  }

<<<<<<< HEAD
  // Greeting Edit
  if (greetingTextBounds_.contains(pt.fX, pt.fY) ||
      greetingEditIconBounds_.contains(pt.fX, pt.fY)) {
=======
  if (greetingTextBounds_.contains(pt.fX, pt.fY) || greetingEditIconBounds_.contains(pt.fX, pt.fY)) {
>>>>>>> origin/master
    showGreetingEditor();
    return;
  }
}

void ZenithHubComponent::mouseUp(const juce::MouseEvent &e) { juce::ignoreUnused(e); }

void ZenithHubComponent::showGreetingEditor() {
<<<<<<< HEAD
  if (greetingEditor_.isVisible())
    return;

  greetingEditor_.setText(greetingText_);
  greetingEditor_.setSelectAllWhenFocused(true);
  greetingEditor_.setJustification(juce::Justification::left);
  // Use a standard JUCE font that matches size approx
  greetingEditor_.setFont(juce::Font(18.0f));

  // Named constants for TextEditor sizing
  constexpr int kEditorWidthPadding = 60;
  constexpr int kEditorHeight = 24;

  // Calculate bounds (convert from SkRect to JUCE Rectangle)
  // Ensure we have valid bounds
  if (greetingTextBounds_.isEmpty()) {
    // Fallback if bounds not yet calculated
    return;
  }

  juce::Rectangle<int> bounds(
      (int)greetingTextBounds_.left(),
      (int)greetingTextBounds_.top() + (int)greetingTextBounds_.height() / 2,
      (int)(greetingTextBounds_.width() + kEditorWidthPadding), kEditorHeight);

  greetingEditor_.setBounds(bounds);
  greetingEditor_.setVisible(true);
  greetingEditor_.grabKeyboardFocus();
}

void ZenithHubComponent::hideGreetingEditor(bool save) {
  if (save) {
    greetingText_ = greetingEditor_.getText();
  }
  greetingEditor_.setVisible(false);
  
  // Ensure we repaint to show the updated text
  repaint();
}

// Agent 5: Keyboard Navigation - Refactored to if-else per compiler
// requirements
bool ZenithHubComponent::keyPressed(const juce::KeyPress &key) {
  const int code = key.getKeyCode();

  if (code == juce::KeyPress::returnKey) {
    triggerSelection();
    return true;
  }
  if (code == juce::KeyPress::upKey) {
    moveSelection(-2); // Primitive grid nav for now
    return true;
  }
  if (code == juce::KeyPress::downKey) {
    moveSelection(2);
    return true;
  }
  if (code == juce::KeyPress::leftKey) {
    moveSelection(-1);
    return true;
  }
  if (code == juce::KeyPress::rightKey) {
    moveSelection(1);
    return true;
  }
  return SkiaComponent::keyPressed(key);
}

// Agent 5: Selection logic - Fixed empty list bug per code review
void ZenithHubComponent::moveSelection(int delta) {
  // Fix: Handle empty projects list per code review
  if (recentProjects_.empty()) {
    selectedSection_ = SelectionSection::None;
    selectedIndex_ = -1;
  } else if (selectedSection_ == SelectionSection::None) {
    selectedSection_ = SelectionSection::Recent;
    selectedIndex_ = 0;
  } else {
    selectedIndex_ += delta;
    selectedIndex_ =
        juce::jlimit(0, (int)recentProjects_.size() - 1, selectedIndex_);
=======
  if (greetingEditor_.isVisible()) return;

  greetingEditor_.setText(greetingText_);
  greetingEditor_.setJustification(juce::Justification::left);
  greetingEditor_.setFont(juce::Font(18.0f));

  if (greetingTextBounds_.isEmpty()) return;

  juce::Rectangle<int> bounds(
      (int)greetingTextBounds_.left(),
      (int)greetingTextBounds_.top() + (int)greetingTextBounds_.height() / 2,
      (int)(greetingTextBounds_.width() + 60), 24);

  greetingEditor_.setBounds(bounds);
  greetingEditor_.setVisible(true);
  greetingEditor_.selectAll();
  greetingEditor_.grabKeyboardFocus();
}

void ZenithHubComponent::hideGreetingEditor(bool save) {
  if (save) greetingText_ = greetingEditor_.getText();
  greetingEditor_.setVisible(false);
  repaint();
}

bool ZenithHubComponent::keyPressed(const juce::KeyPress &key) {
  const int code = key.getKeyCode();
  if (code == juce::KeyPress::returnKey) { triggerSelection(); return true; }
  if (code == juce::KeyPress::upKey) { moveSelection(0, -1); return true; }
  if (code == juce::KeyPress::downKey) { moveSelection(0, 1); return true; }
  if (code == juce::KeyPress::leftKey) { moveSelection(-1, 0); return true; }
  if (code == juce::KeyPress::rightKey) { moveSelection(1, 0); return true; }
  return SkiaComponent::keyPressed(key);
}

void ZenithHubComponent::moveSelection(int dx, int dy) {
  if (selectedSection_ == ZenithHubComponent::Section::None) {
    selectedSection_ = ZenithHubComponent::Section::RecentProjects;
    selectedIndex_ = 0;
    repaint();
    return;
  }

  if (selectedSection_ == ZenithHubComponent::Section::RecentProjects) {
    int row = selectedIndex_ / 2;
    int col = selectedIndex_ % 2;
    if (dx == 1 && col == 1) { selectedSection_ = ZenithHubComponent::Section::NewProject; selectedIndex_ = -1; }
    else if (dx == -1 && col == 0) {}
    else {
      int newRow = row + dy;
      int newCol = col + dx;
      int newIdx = (newRow * 2) + newCol;
      if (newIdx >= 0 && newIdx < (int)recentProjects_.size()) selectedIndex_ = newIdx;
    }
  } else if (selectedSection_ == ZenithHubComponent::Section::NewProject) {
    if (dy == 1 && !templates_.empty()) { selectedSection_ = ZenithHubComponent::Section::Templates; selectedIndex_ = 0; }
    if (dx == -1) { selectedSection_ = ZenithHubComponent::Section::RecentProjects; selectedIndex_ = std::min((int)recentProjects_.size() - 1, 1); }
  } else if (selectedSection_ == ZenithHubComponent::Section::Templates) {
    if (dy == -1 && selectedIndex_ == 0) { selectedSection_ = ZenithHubComponent::Section::NewProject; selectedIndex_ = -1; }
    else if (dx == -1) { selectedSection_ = ZenithHubComponent::Section::RecentProjects; selectedIndex_ = std::min((int)recentProjects_.size() - 1, 5); }
    else {
      int newIdx = selectedIndex_ + dy;
      if (newIdx >= 0 && newIdx < (int)templates_.size()) selectedIndex_ = newIdx;
    }
>>>>>>> origin/master
  }
  repaint();
}

void ZenithHubComponent::triggerSelection() {
<<<<<<< HEAD
  if (selectedSection_ == SelectionSection::Recent && selectedIndex_ >= 0 &&
      selectedIndex_ < (int)recentProjects_.size()) {
    if (onLoadProject_) {
      onLoadProject_(recentProjects_[selectedIndex_].path);
    }
  }
}

// Agent 5: Text Rendering Helper
=======
  if (selectedSection_ == ZenithHubComponent::Section::RecentProjects && selectedIndex_ >= 0 && selectedIndex_ < (int)recentProjects_.size()) {
    if (onLoadProject_) onLoadProject_(recentProjects_[selectedIndex_].path);
  } else if (selectedSection_ == ZenithHubComponent::Section::NewProject || selectedSection_ == ZenithHubComponent::Section::Templates) {
    if (onNewProject_) onNewProject_();
    dismiss();
  }
}

>>>>>>> origin/master
void ZenithHubComponent::drawText(SkCanvas *canvas, const juce::String &text,
                                  const SkRect &bounds, const SkFont &font,
                                  const SkPaint &paint, bool centerVertical) {
  SkString skText(text.toRawUTF8());
  SkRect textBounds;
<<<<<<< HEAD
  font.measureText(skText.c_str(), skText.size(), SkTextEncoding::kUTF8,
                   &textBounds);

  float x = bounds.left();
  // Align text top to the top of the bounds
  float y = bounds.fTop - textBounds.fTop;

  if (centerVertical) {
    y = bounds.centerY() + (textBounds.height() * 0.5f);
  }

=======
  font.measureText(skText.c_str(), skText.size(), SkTextEncoding::kUTF8, &textBounds);
  float x = bounds.left();
  float y = bounds.fTop - textBounds.fTop;
  if (centerVertical) y = bounds.centerY() + (textBounds.height() * 0.5f) - textBounds.fBottom;
>>>>>>> origin/master
  canvas->drawString(skText, x, y, font, paint);
}

} // namespace zenith
