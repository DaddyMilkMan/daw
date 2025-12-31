/*
  ==============================================================================

    ZenithHubComponent.cpp
    Clean implementation with robust layout and proper font caching.

  ==============================================================================
*/

#include "ZenithHubComponent.h"
#include "../design-system/ZenithLayout.h"
#include "ZenithIcons.h"
#include "../../engine/ZenithLogger.h"
#include "../design-system/ZenithTypography.h"
#include <array>
#include <cmath>
#include <map>
#include <random>
#include <algorithm>

namespace zenith {

using namespace design;

// Custom LookAndFeel for pill-shaped text editor
class PillTextEditorLookAndFeel : public juce::LookAndFeel_V4 {
public:
  void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override {
    if (textEditor.isEnabled()) {
      if (textEditor.hasKeyboardFocus(true)) {
        g.setColour(juce::Colours::transparentWhite);
      } else {
        g.setColour(juce::Colours::transparentWhite);
      }
    } else {
      g.setColour(juce::Colours::transparentWhite);
    }
    
    // Draw rounded rectangle for pill shape
    g.drawRoundedRectangle(0, 0, width, height, 14.0f, 1.0f);
  }
  
  void fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override {
    g.setColour(textEditor.findColour(juce::TextEditor::backgroundColourId));
    g.fillRoundedRectangle(0, 0, width, height, 14.0f);
  }
};

static PillTextEditorLookAndFeel pillTextEditorLookAndFeel;

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
  if (auto* auth = AuthenticationService::getInstance()) {
      auth->addListener(this);
  }
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

  // Initialize Greeting Editor as pure-Skia pill text box
  greetingEditor_ = std::make_unique<SkiaTextEditor>("greetingEditor");
  greetingEditor_->setPillStyle(true);
  greetingEditor_->setPillCornerRadius(14.0f);
  greetingEditor_->setFont(design::getSkFont(18.0f, design::FontWeight::Regular));
  greetingEditor_->setMultiLine(false);
  greetingEditor_->setVisible(false);
  addChildComponent(greetingEditor_.get());

  auto safeDismiss = [this]() { hideGreetingEditor(false); };
  greetingEditor_->onEscapeKey = safeDismiss;
  greetingEditor_->onFocusLost = safeDismiss;
  greetingEditor_->onReturnKey = [this]() { hideGreetingEditor(true); };

  // Initialize Aurora Background
  auroraBackground_ = std::make_unique<AuroraBackground>();

  // FIX: Start fully visible (1.0) instead of transparent (0.0)
  // The animation from 0.0 wasn't completing before rendering, causing blank screen
  alpha_.set(1.0f);

  // Restart timer - critical for Hub interactivity and animations
  startTimerHz(60);
}

ZenithHubComponent::~ZenithHubComponent() {
  stopTimer();
  recentProjectManager_.removeListener(this);
  if (auto* auth = AuthenticationService::getInstance()) {
      auth->removeListener(this);
  }
}

void ZenithHubComponent::mouseExit(const juce::MouseEvent &e) {
  SkiaComponent::mouseExit(e);
  isNewProjectHovered_ = false;
  isProfileIconHovered_ = false;
  isGreetingHovered_ = false;
  for (auto &p : recentProjects_)
    p.isHovered = false;
  for (auto &t : templates_)
    t.isHovered = false;
  repaint();
}

void ZenithHubComponent::authStateChanged(bool isLoggedIn, const AuthUser& user) {
    juce::ignoreUnused(isLoggedIn, user);
    repaint();
}

bool ZenithHubComponent::hitTest(int x, int y) {
  juce::ignoreUnused(x, y);
  return true;
#if 0
  float fx = (float)x;
  float fy = (float)y;

  if (mainCardBounds_.contains(fx, fy))
    return true;
#endif

  // Always allow profile icon interaction (handles rounding edges)
  if (!profileIconBounds_.isEmpty() && profileIconBounds_.contains(fx, fy))
    return true;

  // FIX: Use menu animation progress instead of boolean flag
  // This prevents flickering during open/close transition by keeping
  // consistent hit test behavior until animation is complete
  float menuProgress = menuSpring_.getCurrent();
  if (menuProgress > 0.01f && !profileIconBounds_.isEmpty()) {
     float menuWidth = 220.0f;
     bool isLoggedIn = false;
     if (auto* auth = AuthenticationService::getInstance()) {
         isLoggedIn = auth->isLoggedIn();
     }
     float menuHeight = isLoggedIn ? 240.0f : 120.0f; 
     SkRect menuBounds = SkRect::MakeXYWH(
          profileIconBounds_.right() - menuWidth,
          profileIconBounds_.bottom() + 12.0f,
          menuWidth, menuHeight
     );
     // Add slop
     if (menuBounds.makeOutset(2.0f, 2.0f).contains(fx, fy))
       return true;
  }
  
  return false;
}

void ZenithHubComponent::loadFromManager() {
  std::lock_guard<std::mutex> lock(projectsMutex_);
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

  // DEBUG: Always print this to confirm function is called
  ZENITH_LOG_INFO(juce::String::formatted("[ZenithHub] updateLayout() called, bounds: %.1f x %.1f", w, h));

  // Ensure the component bounds are valid
  if (w < 400 || h < 300) {
    ZENITH_LOG_INFO("[ZenithHub] Early return! Bounds too small.");
    return;
  }

  float cardW = std::min(1100.0f, w * 0.9f);
  float cardH = std::min(750.0f, h * 0.85f);
  
  // Ensure card dimensions are reasonable
  cardW = std::max(600.0f, cardW);
  cardH = std::max(400.0f, cardH);
  
  float cardX = (w - cardW) * 0.5f;
  float cardY = (h - cardH) * 0.5f;
  
  // Clamp card position with edge margin to ensure it never touches screen edges
  constexpr float kEdgeMargin = 20.0f;
  cardX = std::clamp(cardX, kEdgeMargin, std::max(kEdgeMargin, w - cardW - kEdgeMargin));
  cardY = std::clamp(cardY, kEdgeMargin, std::max(kEdgeMargin, h - cardH - kEdgeMargin));

  mainCardBounds_ = SkRect::MakeXYWH(cardX, cardY, cardW, cardH);

  // Position Sign In button in top right of main card
  // Profile icon (small circular avatar, top-right of card)
  constexpr float kProfileIconSize = 36.0f;
  constexpr float kProfileIconMargin = 40.0f;
  
  profileIconBounds_ = SkRect::MakeXYWH(
    cardX + cardW - kProfileIconSize - kProfileIconMargin,
    cardY + kProfileIconMargin,
    kProfileIconSize,
    kProfileIconSize
  );

  float padding = 40.0f; // Generous padding
  float gridGap = 40.0f; // Requested 40px grid gap

  float headerHeight = 140.0f;
  juce::Rectangle<float> contentRect(
      cardX + padding, cardY + padding + headerHeight, cardW - (padding * 2),
      cardH - (padding * 2) - headerHeight);

  // 1. Layout Left (Recent) vs Right (Sidebar) using ZenithLayout (Agent 4)
  auto mainColumns =
      ZenithLayout::begin()
          .withFloatBounds(contentRect)
          .withGap(gridGap)
          .addFlexItem(juce::FlexItem().withFlex(0.6f)) // Recent Area (60%)
          .addFlexItem(juce::FlexItem().withFlex(0.4f)) // Sidebar (40%)
          .layout(juce::FlexBox::Direction::row);

  if (mainColumns.size() >= 2) {
    auto r = mainColumns[0];
    recentArea_ = SkRect::MakeXYWH(r.getX(), r.getY(), r.getWidth(), r.getHeight());

    // Slice Recent Area: Header vs Grid
    float kHeaderHeight = 32.0f;
    float kHeaderGap = 16.0f;
    
    recentHeaderBounds_ = SkRect::MakeXYWH(recentArea_.fLeft, recentArea_.fTop, recentArea_.width(), kHeaderHeight);
    recentGridBounds_ = SkRect::MakeXYWH(recentArea_.fLeft, recentArea_.fTop + kHeaderHeight + kHeaderGap,
                                         recentArea_.width(),
                                         recentArea_.height() - kHeaderHeight - kHeaderGap);


    auto s = mainColumns[1];
    juce::Rectangle<float> sidebarRect = s;

    // DEBUG: Log sidebar rect
    fprintf(stderr, "[ZenithHub] sidebarRect: %.1f,%.1f %.1fx%.1f\n", 
            sidebarRect.getX(), sidebarRect.getY(), sidebarRect.getWidth(), sidebarRect.getHeight());

    // Sidebar layout: New Project Button -> Templates (Collaborations REMOVED)
    auto sidebarRows = ZenithLayout::begin()
                           .withFloatBounds(sidebarRect)
                           .withGap(24.0f)
                           .addFlexItem(juce::FlexItem().withHeight(60.0f))   // New Project button
                           .addFlexItem(juce::FlexItem().withFlex(1.0f))      // Templates (gets all remaining space)
                           .layout(juce::FlexBox::Direction::column);

    if (sidebarRows.size() >= 2) {
      // Row 0: New Project Button
      auto b = sidebarRows[0];
      newProjectButtonBounds_ =
          SkRect::MakeXYWH(b.getX(), b.getY(), b.getWidth(), b.getHeight());

      // Row 1: Templates/Quick Start area (now gets more space without Collaborations)
      auto t = sidebarRows[1];
      templatesArea_ = SkRect::MakeXYWH(t.getX(), t.getY(), t.getWidth(), t.getHeight());
      
      quickStartHeaderBounds_ = SkRect::MakeXYWH(templatesArea_.fLeft, templatesArea_.fTop, templatesArea_.width(), kHeaderHeight);
      
      templatesContentBounds_ = SkRect::MakeXYWH(templatesArea_.fLeft, templatesArea_.fTop + kHeaderHeight + kHeaderGap,
                                         templatesArea_.width(),
                                         templatesArea_.height() - kHeaderHeight - kHeaderGap);
      
      // Clear removed Collaborations bounds
      accountArea_.setEmpty();
      accountHeaderBounds_.setEmpty();
      accountContentBounds_.setEmpty();

                                         
    } else {
      fprintf(stderr, "[ZenithHub] ERROR: sidebarRows.size() < 2, setting empty bounds!\\n");
      newProjectButtonBounds_.setEmpty();
      templatesArea_.setEmpty();
      quickStartHeaderBounds_.setEmpty();
      templatesContentBounds_.setEmpty();
      accountArea_.setEmpty();
      accountHeaderBounds_.setEmpty();
      accountContentBounds_.setEmpty();

    }
  } else {
    // Reset layout on failure
    recentArea_.setEmpty();
    recentHeaderBounds_.setEmpty();
    recentGridBounds_.setEmpty();
    
    accountArea_.setEmpty();
    accountHeaderBounds_.setEmpty();
    accountContentBounds_.setEmpty();
    
    newProjectButtonBounds_.setEmpty();
    
    templatesArea_.setEmpty();
    quickStartHeaderBounds_.setEmpty();
    templatesContentBounds_.setEmpty();
    

  }

  // Update Recent Project Cards Layout (Grid)
  if (!recentGridBounds_.isEmpty()) {
    std::lock_guard<std::mutex> lock(projectsMutex_);
    float gridW = recentGridBounds_.width();
    float cardGap = 16.0f;
    float pCardW = (gridW - cardGap) / 2.0f;
    float pCardH = 110.0f;

    for (size_t i = 0; i < recentProjects_.size(); ++i) {
      int row = (int)i / 2;
      int col = (int)i % 2;

      float px = recentGridBounds_.fLeft + (col * (pCardW + cardGap));
      float py = recentGridBounds_.fTop + (row * (pCardH + cardGap));

      recentProjects_[i].bounds = SkRect::MakeXYWH(px, py, pCardW, pCardH);
    }
  }

  // Update Template Cards (Larger with icons)
  if (!templatesContentBounds_.isEmpty()) {
    float tCardH = 80.0f;  // Slightly smaller for better fit
    float cardGap = 12.0f;
    
    for (size_t i = 0; i < templates_.size(); ++i) {
      float tx = templatesContentBounds_.fLeft;
      float ty = templatesContentBounds_.fTop + (i * (tCardH + cardGap));
      templates_[i].bounds =
          SkRect::MakeXYWH(tx, ty, templatesContentBounds_.width(), tCardH);
    }
  }

  if (greetingEditor_ && greetingEditor_->isVisible()) {
    showGreetingEditor(); // Re-layout editor
  }
  ZENITH_LOG_INFO("[ZenithHub] updateLayout() COMPLETE");
}

void ZenithHubComponent::timerCallback() {
  animationTime_ += 0.016f;
  alpha_.update(16.0f);
  
  // Profile Menu Animation
  if (isProfileMenuOpen_) {
      // Entrance: Fast & Smooth (Critical damping)
      // Damping 1.0 = No bounce.
      menuSpring_.update(0.45f, 1.0f);
      menuSpring_.setTarget(1.0f);
  } else {
      // Exit: Instant Snap (Quick Exit)
      menuSpring_.update(0.6f, 0.5f); 
      menuSpring_.setTarget(0.0f);
  }
  
  // FIX: Only repaint when truly needed to avoid flicker.
  // Check if any animations are actively running.
  bool needsRepaint = alpha_.isAnimating() || !menuSpring_.isResting();
  
  // Also check if we need to keep the aurora background animated
  if (auroraBackground_ && isVisible()) {
      needsRepaint = true;
  }
  
  if (needsRepaint) {
    repaint();
  }
}


void ZenithHubComponent::show() {
  ZENITH_LOG_INFO("ZenithHubComponent::show() called. Forcing visibility and layout.");
  setVisible(true);
  resized(); // Force layout update now that we are visible
  alpha_.setTarget(1.0f, 600, ::zenith::animation::Easing::EaseOut);
}

void ZenithHubComponent::dismiss() {
  alpha_.setTarget(0.0f, 400, ::zenith::animation::Easing::EaseIn);
  if (onDismiss_)
    onDismiss_();
}

void ZenithHubComponent::drawSkia(SkCanvas *canvas) {
  float opacity = alpha_.get();
  
  // FIX: Lazy layout if bounds are missing but size is valid
  if (mainCardBounds_.isEmpty() && getWidth() >= 400 && getHeight() >= 300) {
      updateLayout();
  }

  static int hubFrameCount = 0;
  if (hubFrameCount++ % 300 == 0) {
      ZENITH_LOG_INFO("[ZenithHub] drawSkia - Opacity: " + juce::String(opacity) + 
                      ", CardBounds: " + juce::String(mainCardBounds_.isEmpty() ? "EMPTY" : "VALID"));
  }

  if (opacity <= 0.001f)
    return;

  // Skip drawing if layout hasn't been computed yet
  if (mainCardBounds_.isEmpty()) {
    // Just draw the background while waiting for proper layout
    ZENITH_LOG_INFO("[ZenithHub] Render SKIP: mainCardBounds is empty. Opacity=" + juce::String(opacity));
    drawBackground(canvas);
    return;
  }

  canvas->saveLayerAlpha(nullptr, (U8CPU)(opacity * 255));
  drawBackground(canvas);

  GlassmorphicPanel::draw(canvas, mainCardBounds_,
                          GlassmorphicPanel::Style::Elevated);

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
  
  juce::String currentGreeting = greetingText_;
  if (auto* auth = AuthenticationService::getInstance()) {
      if (auth->isLoggedIn()) {
          auto user = auth->getCurrentUser();
          currentGreeting = "Welcome back, " + (user.displayName.isNotEmpty() ? user.displayName : user.email);
      }
  }
  
  drawText(canvas, currentGreeting, helperBounds, subFont_, subPaint_, false);

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

  drawRecentProjects(canvas);
  drawNewProjectButton(canvas);
  drawTemplates(canvas);
  drawProfileIcon(canvas);  // Draw icon on top
  drawProfileMenu(canvas);  // Draw menu on top of everything


  canvas->restore();
}

void ZenithHubComponent::drawBackground(SkCanvas *canvas) {
    // Background is now handled by MainComponent to ensure a unified "Aurora" flow.
    // Drawing it here would cause "bleed" or misaligned patterns.
    // We only fill with black if absolutely necessary for clip areas, 
    // but here we want pure transparency.
    // SkPaint p;
    // p.setColor(SK_ColorTRANSPARENT);
    // canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), p);
}

void ZenithHubComponent::drawRecentProjects(SkCanvas *canvas) {
  std::lock_guard<std::mutex> lock(projectsMutex_);
  textPaint_.setColor(colors::TEXT_PRIMARY);
  drawText(canvas, "Recent Projects", recentHeaderBounds_, headerFont_, textPaint_, true);

  // Empty state check
  if (recentProjects_.empty()) {
    SkPaint emptyStatePaint = textPaint_;
    emptyStatePaint.setColor(withAlpha(colors::TEXT_PRIMARY, 0.35f));
    drawText(
        canvas, "No recent projects yet.",
        SkRect::MakeXYWH(recentGridBounds_.fLeft, recentGridBounds_.fTop, 300, 20),
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
  // Skip if bounds not calculated
  if (templatesArea_.isEmpty() || quickStartHeaderBounds_.isEmpty()) {
    return;
  }

  // Draw "Quick Start" header inside its explicit bounds
  drawText(
      canvas, "Quick Start",
      quickStartHeaderBounds_,
      headerFont_, textPaint_, true);

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



void ZenithHubComponent::drawNewProjectButton(SkCanvas *canvas) {
  // Skip if bounds are empty
  if (newProjectButtonBounds_.isEmpty()) {
    return;
  }

  bool isSelected = (this->selectedSection_ == SelectionSection::New);
  bool active = isNewProjectHovered_ || isSelected;

  // PREMIUM: Subtle glassmorphism button (reduced intensity per user feedback)
  GlassmorphicPanel::Options opts;
  opts.style = active ? GlassmorphicPanel::Style::Elevated
                      : GlassmorphicPanel::Style::Subtle;
  // Reduce intensity per user feedback
  opts.accentColor = active ? withAlpha(colors::BLUE, 0.4f) : 0x00000000;
  opts.cornerRadius = 12.0f;
  opts.glowIntensity = active ? 0.05f : 0.0f;  // Extremely subtle glow
  opts.drawTopHighlight = false; // Remove sharp bevel highlight

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

void ZenithHubComponent::drawProfileIcon(SkCanvas *canvas) {
  // Skip if bounds are empty
  if (profileIconBounds_.isEmpty()) {
    return;
  }

  bool active = isProfileIconHovered_;
  float centerX = profileIconBounds_.centerX();
  float centerY = profileIconBounds_.centerY();
  float radius = profileIconBounds_.width() * 0.5f;

  // Modern "Glow" style (No hard borders/pill shape)
   if (active) {
       SkPaint glowPaint;
       glowPaint.setAntiAlias(true);
       glowPaint.setColor(withAlpha(colors::CYAN, 0.2f)); // Subtle fill
       // detailed soft blur
       glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
       canvas->drawCircle(centerX, centerY, radius, glowPaint);
   }
   // No stroke border (Intense borders removed)


  // Draw user icon inside (or initials when logged in)
  icons::IconStyle iconStyle;
  iconStyle.color = active ? colors::CYAN : colors::TEXT_SECONDARY;
  iconStyle.strokeWidth = icons::STROKE_REGULAR;
  icons::drawIconCentered(canvas, icons::Users(), profileIconBounds_,
                          radius * 1.1f, iconStyle);
}

void ZenithHubComponent::mouseMove(const juce::MouseEvent &e) {
  static int moveLogCount = 0;
  if (moveLogCount++ % 100 == 0) {
      ZENITH_LOG_INFO("[ZenithHub] mouseMove: " + juce::String(e.x) + ", " + juce::String(e.y));
  }
  SkPoint pt = {(float)e.x, (float)e.y};
  bool needsUpdate = false;

  if (selectedSection_ != SelectionSection::None) {
    selectedSection_ = SelectionSection::None;
    selectedIndex_ = -1;
    needsUpdate = true;
  }

  // Check if hovering over profile menu to block background items
  bool menuHover = false;
  if (isProfileMenuOpen_) {
    float menuWidth = 220.0f;
    bool isLoggedIn = false;
    if (auto* auth = AuthenticationService::getInstance()) {
        isLoggedIn = auth->isLoggedIn();
    }
    float menuHeight = isLoggedIn ? 240.0f : 120.0f; 
    SkRect menuBounds = SkRect::MakeXYWH(
        profileIconBounds_.right() - menuWidth,
        profileIconBounds_.bottom() + 12.0f,
        menuWidth, menuHeight
    );
    // Add margin to safely block background interactions near menu
    if (menuBounds.makeOutset(10.0f, 10.0f).contains(pt.fX, pt.fY)) {
        menuHover = true;
    }
  }

  // OPTIMIZATION: Early-exit bounding box check for recent projects
  // Only perform inner contains check if point is within the recentGridBounds_
  bool inRecentArea = !menuHover && recentGridBounds_.contains(pt.fX, pt.fY);
  for (auto &proj : recentProjects_) {
    bool h = inRecentArea && proj.bounds.contains(pt.fX, pt.fY);
    if (h != proj.isHovered) {
      proj.isHovered = h;
      needsUpdate = true;
      // ZENITH_LOG_INFO("Hover Update: Recent Project " + proj.name);
    }
  }

  // Check template hover
  for (auto &tmpl : templates_) {
    bool h = !menuHover && tmpl.bounds.contains(pt.fX, pt.fY);
    if (h != tmpl.isHovered) {
      tmpl.isHovered = h;
      needsUpdate = true;
      // ZENITH_LOG_INFO("Hover Update: Template " + tmpl.title);
    }
  }



  bool nph = !menuHover && newProjectButtonBounds_.contains(pt.fX, pt.fY);
  if (nph != isNewProjectHovered_) {
    isNewProjectHovered_ = nph;
    needsUpdate = true;
  }

  bool pih = profileIconBounds_.contains(pt.fX, pt.fY);
  if (pih != isProfileIconHovered_) {
    isProfileIconHovered_ = pih;
    needsUpdate = true;
  }

  bool gh = !menuHover && (greetingTextBounds_.contains(pt.fX, pt.fY) ||
            greetingEditIconBounds_.contains(pt.fX, pt.fY));
  if (gh != isGreetingHovered_) {
    isGreetingHovered_ = gh;
    needsUpdate = true;
  }

  if (needsUpdate)
    repaint();
}

void ZenithHubComponent::mouseDown(const juce::MouseEvent &e) {
  ZENITH_LOG_INFO("[ZenithHub] mouseDown: " + juce::String(e.x) + ", " + juce::String(e.y));
  SkPoint pt = {(float)e.x, (float)e.y};
  
  // Checking specific elements instead of generic mainCardBounds_
  // to support elements extending outside (like popout menu)

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

  // Handle Profile Menu Interactions
  if (isProfileMenuOpen_) {
    float menuWidth = 220.0f;
    bool isLoggedIn = false;
    if (auto* auth = AuthenticationService::getInstance()) {
        isLoggedIn = auth->isLoggedIn();
    }
    float menuHeight = isLoggedIn ? 240.0f : 120.0f; 
    SkRect menuBounds = SkRect::MakeXYWH(
        profileIconBounds_.right() - menuWidth,
        profileIconBounds_.bottom() + 12.0f,
        menuWidth, menuHeight
    );
    
    if (menuBounds.contains(pt.fX, pt.fY)) {
       auto* auth = AuthenticationService::getInstance();
       if (!isLoggedIn) {
          float itemY = menuBounds.fTop + 16.0f;
          
          // Sign In (First Item)
          if (pt.fY >= itemY && pt.fY < itemY + 40.0f) {
              if (auth) {
                  auth->loginWithWeb([](bool success, juce::String error) {
                      if (!success && error.isNotEmpty()) {
                          juce::NativeMessageBox::showMessageBoxAsync(
                              juce::MessageBoxIconType::WarningIcon,
                              "Login Failed",
                              "Could not sign in: " + error
                          );
                      }
                  });
              }
              isProfileMenuOpen_ = false;
              repaint();
          }
          // Create Account (Second Item)
          else if (pt.fY >= itemY + 40.0f && pt.fY < itemY + 80.0f) {
              if (auth) {
                  auth->signupWithWeb([](bool success, juce::String error) {
                      if (!success && error.isNotEmpty()) {
                          juce::NativeMessageBox::showMessageBoxAsync(
                              juce::MessageBoxIconType::WarningIcon,
                              "Signup Failed",
                              "Could not create account: " + error
                          );
                      }
                  });
              }
              isProfileMenuOpen_ = false;
              repaint();
          }
       } else {
          // Sign Out (Bottom area)
          if (pt.fY > menuBounds.fBottom - 60.0f) {
              if (auth) auth->logout();
              isProfileMenuOpen_ = false;
              repaint();
          }
       }
       return; // Consume click
    } else if (!profileIconBounds_.contains(pt.fX, pt.fY)) {
        // Click outside menu AND icon -> Close menu
        isProfileMenuOpen_ = false;
        repaint();
        return;
    }
  }

  if (profileIconBounds_.contains(pt.fX, pt.fY)) {
    // Toggle profile menu
    isProfileMenuOpen_ = !isProfileMenuOpen_;
    repaint();
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
  if (!greetingEditor_ || greetingEditor_->isVisible())
    return;

  greetingEditor_->setText(greetingText_);

  // Named constants for TextEditor sizing
  constexpr int kEditorWidthPadding = 60;
  constexpr int kEditorHeight = 32; // Slightly taller for pill appearance

  if (greetingTextBounds_.isEmpty()) {
    return;
  }

  juce::Rectangle<int> bounds(
      (int)greetingTextBounds_.left(),
      (int)greetingTextBounds_.top() - 4,
      (int)(greetingTextBounds_.width() + kEditorWidthPadding), kEditorHeight);

  greetingEditor_->setBounds(bounds);
  greetingEditor_->setVisible(true);
  greetingEditor_->grabKeyboardFocus();
}

void ZenithHubComponent::hideGreetingEditor(bool save) {
  if (!greetingEditor_)
    return;
    
  if (save) {
    greetingText_ = greetingEditor_->getText();
  }
  greetingEditor_->setVisible(false);
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
// Text Rendering Helper - Fixed vertical alignment and added truncation
void ZenithHubComponent::drawText(SkCanvas *canvas, const juce::String &text,
                                  const SkRect &bounds, const SkFont &font,
                                  const SkPaint &paint, bool centerVertical) {
  SkString skText(text.toRawUTF8());
  SkScalar width = font.measureText(skText.c_str(), skText.size(), SkTextEncoding::kUTF8);
  
  // Truncate if text is wider than bounds
  if (width > bounds.width()) {
    SkString ellipsis("...");
    SkScalar ellipsisWidth = font.measureText(ellipsis.c_str(), ellipsis.size(), SkTextEncoding::kUTF8);
    
    // If even ellipsis doesn't fit, just draw nothing or ellipsis
    if (ellipsisWidth > bounds.width()) {
      skText = ellipsis; 
    } else {
      // Linear reduction - simple and effective for short titles
      juce::String jStr = text;
      
      while (jStr.length() > 0 && width + ellipsisWidth > bounds.width()) {
        jStr = jStr.substring(0, jStr.length() - 1);
        skText = SkString(jStr.toRawUTF8());
        width = font.measureText(skText.c_str(), skText.size(), SkTextEncoding::kUTF8);
      }
      skText.append("...");
    }
  }

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

void ZenithHubComponent::drawProfileMenu(SkCanvas* canvas) {
  float rawProgress = menuSpring_.getCurrent();
  if (rawProgress <= 0.001f) return;

  // Menu dimensions
  float menuWidth = 220.0f;
  bool isLoggedIn = false;
  AuthUser currentUser;
  if (auto* auth = AuthenticationService::getInstance()) {
      isLoggedIn = auth->isLoggedIn();
      currentUser = auth->getCurrentUser();
  }
  float menuHeight = isLoggedIn ? 240.0f : 120.0f; 
  
  // Anchor to bottom-right of profile icon
  float anchorX = profileIconBounds_.right();
  float anchorY = profileIconBounds_.bottom() + 12.0f;

  SkRect menuBounds = SkRect::MakeXYWH(
      anchorX - menuWidth,
      anchorY,
      menuWidth,
      menuHeight
  );

  // FIX: Don't use saveLayerAlpha - causes flicker with parent layer
  // Instead, apply alpha directly to colors
  float alpha = juce::jlimit(0.0f, 1.0f, rawProgress);
  
  // Only save/restore for clipping, not alpha
  canvas->save();

  // FIX: Disable backdrop blur for menu - it causes flicker with constant repaints
  // Use solid background instead for stable rendering
  GlassmorphicPanel::Options opts;
  opts.style = GlassmorphicPanel::Style::Floating;
  opts.cornerRadius = 16.0f;
  opts.drawShadow = alpha > 0.5f;
  opts.glowIntensity = alpha;
  opts.useBackdropBlur = false; // KEY FIX: Disable backdrop blur to stop flicker
  
  GlassmorphicPanel::drawWithOptions(canvas, menuBounds, opts);

  // Content - apply alpha to text colors directly
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(design::withAlpha(colors::TEXT_PRIMARY, alpha));

  float itemHeight = 40.0f;
  float currentY = menuBounds.fTop + 16.0f;
  float contentLeft = menuBounds.fLeft + 16.0f;

  auto drawMenuItem = [&](const char* text, bool isDestructive = false) {
      SkRect itemBounds = SkRect::MakeXYWH(contentLeft, currentY, menuWidth - 32, itemHeight);
      
      SkPaint itemTextPaint = textPaint;
      if (isDestructive) {
        itemTextPaint.setColor(design::withAlpha(colors::RED, alpha));
      }
      
      drawText(canvas, text, itemBounds, buttonFont_, itemTextPaint, false);
      currentY += itemHeight;
  };

  if (isLoggedIn) {
      SkPaint subPaint = textPaint;
      subPaint.setColor(design::withAlpha(colors::TEXT_SECONDARY, 0.7f * alpha));
      
      juce::String userName = currentUser.displayName.isNotEmpty() ? currentUser.displayName : currentUser.email;
      if (userName.isEmpty()) userName = "User";
      
      drawText(canvas, "Signed in as " + userName, SkRect::MakeXYWH(contentLeft, currentY, menuWidth, 20), subFont_, subPaint, false);
      currentY += 24.0f;

      SkPaint divPaint;
      divPaint.setColor(design::withAlpha(colors::TEXT_SECONDARY, 0.1f * alpha));
      canvas->drawLine(menuBounds.fLeft, currentY, menuBounds.fRight, currentY, divPaint);
      currentY += 12.0f;

      drawMenuItem("My Account");
      drawMenuItem("Friends");
      drawMenuItem("Zenith Settings");
      
      currentY += 8.0f;
      drawMenuItem("Sign Out", true);
  } else {
      drawMenuItem("Sign In");
      drawMenuItem("Create Account");
  }

  canvas->restore();
}

} // namespace zenith
