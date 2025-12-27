/**
 * @file MenuBar.cpp
 * @brief Custom Skia-based Menu Bar implementation - Neon Noir Edition
 * 
 * Premium glassmorphic menu bar with animated hover states, vector icons,
 * and neon glow effects.
 */

#include "MenuBar.h"
#include "../controls/CollabPanel.h"
#include "../design-system/ZenithTheme.h"

namespace zenith {

ZenithMenuBar::ZenithMenuBar() {
  setOpaque(false); // Allow glassmorphism transparency

  // Initialize menu items with icons
  items_ = {
    {"File", {}, {}, icons::File()},
    {"Edit", {}, {}, icons::Edit()},
    {"View", {}, {}, icons::ViewToggle()},
    {"Help", {}, {}, icons::Info()}
  };

  // Initialize cached fonts
  updateCachedPaints();
}

void ZenithMenuBar::updateCachedPaints() {
  // Menu item font - medium weight for readability
  menuFont_ = design::getSkFont(14.0f, design::FontWeight::Medium);
  
  // Small font for secondary elements
  smallFont_ = design::getSkFont(12.0f, design::FontWeight::Regular);
}

void ZenithMenuBar::visibilityChanged() {
  if (isVisible() && getPeer() != nullptr && !isTimerRunning()) {
    if (juce::MessageManager::getInstanceWithoutCreating() != nullptr) startTimerHz(60); // 60fps for smooth animations
  } else if (!isVisible() && isTimerRunning()) {
    stopTimer();
  }
}

void ZenithMenuBar::timerCallback() {
  SkiaComponent::timerCallback();

  float dt = 1.0f / 60.0f;
  bool needsRepaint = false;

  // Update all item animations
  for (auto &item : items_) {
    item.state.update(dt);
    if (item.state.isAnimating()) {
      needsRepaint = true;
    }
  }

  // Update collab button animation
  collabState_.update(dt);
  if (collabState_.isAnimating()) {
    needsRepaint = true;
  }

  if (needsRepaint) {
    repaint();
  }
}

void ZenithMenuBar::paint(juce::Graphics &g) {
  SkiaComponent::paint(g);
}

void ZenithMenuBar::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds().toFloat();
  SkRect skBounds = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  // 1. Glassmorphic Background
  GlassmorphicPanel::draw(canvas, skBounds, GlassmorphicPanel::Style::Elevated);

  // 2. Bottom Border with Neon Glow
  SkRect borderRect = SkRect::MakeXYWH(0, skBounds.height() - 2.0f, 
                                        skBounds.width(), 2.0f);
  NeonGlow::drawGlow(canvas, borderRect, design::colors::CYAN, 
                     NeonGlow::Intensity::Subtle);

  // 3. Draw Menu Items
  for (size_t i = 0; i < items_.size(); ++i) {
    drawMenuItem(canvas, items_[i], static_cast<int>(i) == hoveredItemIndex_);
  }

  // 4. Draw Collab Button
  drawCollabButton(canvas);

  // 5. Subtle vertical divider before Collab button
  float dividerX = collabButtonBounds_.getX() - design::spacing::MD;
  SkPaint dividerPaint;
  dividerPaint.setColor(design::colors::BORDER_SUBTLE);
  dividerPaint.setAntiAlias(true);
  canvas->drawLine(dividerX, 6.0f, dividerX, getHeight() - 6.0f, dividerPaint);
}

void ZenithMenuBar::drawMenuItem(SkCanvas *canvas, const MenuItem &item,
                                  bool isHovered) {
  SkRect rect = SkRect::MakeXYWH(
      static_cast<float>(item.bounds.getX()),
      static_cast<float>(item.bounds.getY()),
      static_cast<float>(item.bounds.getWidth()),
      static_cast<float>(item.bounds.getHeight()));

  float hoverAmount = item.state.hoverAmount;
  float pressAmount = item.state.pressAmount;

  // Draw hover overlay with animation
  if (hoverAmount > 0.01f) {
    InteractionHelper::drawHoverOverlay(canvas, rect, hoverAmount,
                                        design::dimensions::RADIUS_SM);
  }

  // Draw pressed overlay
  if (pressAmount > 0.01f) {
    InteractionHelper::drawPressedOverlay(canvas, rect, pressAmount,
                                          design::dimensions::RADIUS_SM);
  }

  // Icon positioning - left side of item
  float iconSize = 16.0f;
  float iconX = rect.left() + design::spacing::SM;
  float iconY = rect.centerY();

  // Icon color: tertiary at rest, cyan on hover
  SkColor iconColor = design::interpolateColor(
      design::colors::TEXT_TERTIARY,
      design::colors::CYAN,
      hoverAmount);

  icons::IconStyle iconStyle;
  iconStyle.color = iconColor;
  iconStyle.strokeWidth = icons::STROKE_LIGHT;
  
  // Add subtle glow on hover
  if (hoverAmount > 0.1f) {
    iconStyle.glowRadius = design::effects::GLOW_SUBTLE * hoverAmount;
    iconStyle.glowColor = design::colors::CYAN;
  }

  // Draw icon centered vertically
  SkRect iconBounds = SkRect::MakeXYWH(iconX, iconY - iconSize / 2.0f, 
                                        iconSize, iconSize);
  icons::drawIconCentered(canvas, item.icon, iconBounds, iconSize, iconStyle);

  // Text positioning - after icon
  float textX = iconX + iconSize + design::spacing::XS;
  float textY = rect.centerY() + 5.0f; // Approximate vertical center

  // Text color: secondary at rest, primary on hover
  SkColor textColor = design::interpolateColor(
      design::colors::TEXT_SECONDARY,
      design::colors::TEXT_PRIMARY,
      hoverAmount);

  // Draw text with optional glow on hover
  if (hoverAmount > 0.3f) {
    NeonGlow::drawTextGlow(canvas, item.name.toStdString().c_str(),
                           textX, textY, menuFont_, design::colors::CYAN,
                           NeonGlow::Intensity::Subtle);
  }

  SkPaint textPaint;
  textPaint.setColor(textColor);
  textPaint.setAntiAlias(true);
  canvas->drawString(item.name.toStdString().c_str(), textX, textY,
                     menuFont_, textPaint);
}

void ZenithMenuBar::drawCollabButton(SkCanvas *canvas) {
  SkRect rect = SkRect::MakeXYWH(
      static_cast<float>(collabButtonBounds_.getX()),
      static_cast<float>(collabButtonBounds_.getY()),
      static_cast<float>(collabButtonBounds_.getWidth()),
      static_cast<float>(collabButtonBounds_.getHeight()));

  float hoverAmount = collabState_.hoverAmount;

  // Glassmorphic button with accent
  if (hoverAmount > 0.01f) {
    GlassmorphicPanel::Options opts;
    opts.style = GlassmorphicPanel::Style::Elevated;
    opts.cornerRadius = design::dimensions::RADIUS_SM;
    opts.accentColor = design::withAlpha(design::colors::MAGENTA, 
                                          0.3f * hoverAmount);
    opts.glowIntensity = hoverAmount;
    GlassmorphicPanel::drawWithOptions(canvas, rect, opts);
  } else {
    GlassmorphicPanel::draw(canvas, rect, GlassmorphicPanel::Style::Subtle);
  }

  // Icon
  float iconSize = 16.0f;
  SkColor iconColor = design::interpolateColor(
      design::colors::TEXT_SECONDARY,
      design::colors::MAGENTA,
      hoverAmount);

  icons::IconStyle iconStyle;
  iconStyle.color = iconColor;
  iconStyle.strokeWidth = icons::STROKE_REGULAR;
  if (hoverAmount > 0.1f) {
    iconStyle.glowRadius = design::effects::GLOW_SUBTLE * hoverAmount;
    iconStyle.glowColor = design::colors::MAGENTA;
  }

  SkRect iconBounds = SkRect::MakeXYWH(
      rect.left() + design::spacing::SM,
      rect.centerY() - iconSize / 2.0f,
      iconSize, iconSize);
  icons::drawIconCentered(canvas, icons::Users(), iconBounds, iconSize, iconStyle);

  // Text
  float textX = iconBounds.right() + design::spacing::XS;
  float textY = rect.centerY() + 5.0f;

  SkColor textColor = design::interpolateColor(
      design::colors::TEXT_SECONDARY,
      design::colors::TEXT_PRIMARY,
      hoverAmount);

  SkPaint textPaint;
  textPaint.setColor(textColor);
  textPaint.setAntiAlias(true);
  canvas->drawString("Collab", textX, textY, smallFont_, textPaint);
}

void ZenithMenuBar::resized() { 
  updateLayout(); 
}

void ZenithMenuBar::updateLayout() {
  int x = static_cast<int>(design::spacing::MD);
  int itemHeight = getHeight() - 8; // 4px padding top and bottom
  int itemY = 4;

  // Calculate item widths based on content
  for (auto &item : items_) {
    // Icon (16) + spacing (4) + text (~40-60) + padding (16)
    int itemWidth = 16 + 4 + 50 + 16; // ~86px per item
    item.bounds = juce::Rectangle<int>(x, itemY, itemWidth, itemHeight);
    x += itemWidth + static_cast<int>(design::spacing::XS);
  }

  // Collab button on right side
  int collabWidth = 90;
  int collabHeight = itemHeight;
  collabButtonBounds_ = juce::Rectangle<int>(
      getWidth() - collabWidth - static_cast<int>(design::spacing::MD),
      itemY, collabWidth, collabHeight);
}

void ZenithMenuBar::mouseMove(const juce::MouseEvent &e) {
  int prevHover = hoveredItemIndex_;
  hoveredItemIndex_ = -1;

  // Check menu items
  for (size_t i = 0; i < items_.size(); ++i) {
    bool isHovered = items_[i].bounds.contains(e.getPosition());
    items_[i].state.isHovered = isHovered;
    if (isHovered) {
      hoveredItemIndex_ = static_cast<int>(i);
    }
  }

  // Check collab button
  collabState_.isHovered = collabButtonBounds_.contains(e.getPosition());

  if (prevHover != hoveredItemIndex_) {
    repaint();
  }
}

void ZenithMenuBar::mouseExit(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  hoveredItemIndex_ = -1;
  
  for (auto &item : items_) {
    item.state.isHovered = false;
  }
  collabState_.isHovered = false;
  
  repaint();
}

void ZenithMenuBar::mouseDown(const juce::MouseEvent &e) {
  // Set pressed states
  for (size_t i = 0; i < items_.size(); ++i) {
    items_[i].state.isPressed = items_[i].bounds.contains(e.getPosition());
  }
  collabState_.isPressed = collabButtonBounds_.contains(e.getPosition());

  // Handle menu item clicks
  if (hoveredItemIndex_ >= 0 && hoveredItemIndex_ < static_cast<int>(items_.size())) {
    const auto &item = items_[hoveredItemIndex_];
    if (item.name == "File")
      showFileMenu();
    else if (item.name == "Edit")
      showEditMenu();
    else if (item.name == "View")
      showViewMenu();
    else if (item.name == "Help")
      showHelpMenu();
  }

  // Handle collab button click
  if (collabButtonBounds_.contains(e.getPosition())) {
    auto *content = new CollabPanel();
    collabCallout_.reset(new juce::CallOutBox(
        *content, collabButtonBounds_.translated(getScreenX(), getScreenY()), 
        nullptr));
    collabCallout_->setVisible(true);
  }
}

void ZenithMenuBar::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  
  for (auto &item : items_) {
    item.state.isPressed = false;
  }
  collabState_.isPressed = false;
}

void ZenithMenuBar::showFileMenu() {
  juce::PopupMenu menu;
  menu.addItem(1, "New Project", true, false);
  menu.addItem(2, "Open Project...", true, false);
  menu.addSeparator();
  menu.addItem(3, "Save Project", true, false);
  menu.addItem(4, "Save Project As...", true, false);
  menu.addSeparator();
  menu.addItem(5, "Import Audio...", true, false);
  menu.addItem(6, "Export Audio...", true, false);
  menu.addSeparator();
  menu.addItem(7, "Quit", true, false);

  auto screenBounds = items_[0].bounds.translated(getScreenX(), 
                                                   getScreenY() + getHeight());
  menu.showMenuAsync(
      juce::PopupMenu::Options()
          .withTargetComponent(this)
          .withTargetScreenArea(screenBounds),
      [this](int result) {
        switch (result) {
          case 1: if (onNewProject) onNewProject(); break;
          case 2: if (onOpenProject) onOpenProject(); break;
          case 3: if (onSaveProject) onSaveProject(); break;
          case 4: if (onSaveProjectAs) onSaveProjectAs(); break;
          case 5: if (onImportAudio) onImportAudio(); break;
          case 6: if (onExportAudio) onExportAudio(); break;
          case 7: juce::JUCEApplication::getInstance()->systemRequestedQuit(); break;
        }
      });
}

void ZenithMenuBar::showEditMenu() {
  juce::PopupMenu menu;
  menu.addItem(1, "Undo", true, false);
  menu.addItem(2, "Redo", true, false);
  menu.addSeparator();
  menu.addItem(3, "Cut", true, false);
  menu.addItem(4, "Copy", true, false);
  menu.addItem(5, "Paste", true, false);
  menu.addItem(6, "Delete", true, false);
  menu.addSeparator();
  menu.addItem(7, "Select All", true, false);

  auto screenBounds = items_[1].bounds.translated(getScreenX(), 
                                                   getScreenY() + getHeight());
  menu.showMenuAsync(
      juce::PopupMenu::Options()
          .withTargetComponent(this)
          .withTargetScreenArea(screenBounds),
      [this](int result) {
        if (result == 1 && onUndo) onUndo();
        else if (result == 2 && onRedo) onRedo();
      });
}

void ZenithMenuBar::showViewMenu() {
  juce::PopupMenu menu;
  menu.addItem(1, "Toggle Session/Arranger (Tab)", true, false);
  menu.addItem(2, "Toggle Mixer", true, false);
  menu.addItem(3, "Toggle Browser", true, false);
  menu.addSeparator();
  menu.addItem(4, "Zoom In", true, false);
  menu.addItem(5, "Zoom Out", true, false);
  menu.addItem(6, "Fit to Window", true, false);

  auto screenBounds = items_[2].bounds.translated(getScreenX(), 
                                                   getScreenY() + getHeight());
  menu.showMenuAsync(
      juce::PopupMenu::Options()
          .withTargetComponent(this)
          .withTargetScreenArea(screenBounds),
      [this](int result) {
        if (result == 2 && onToggleMixer) onToggleMixer();
        else if (result == 3 && onToggleBrowser) onToggleBrowser();
      });
}

void ZenithMenuBar::showHelpMenu() {
  juce::PopupMenu menu;
  menu.addItem(1, "Getting Started", true, false);
  menu.addItem(2, "Keyboard Shortcuts", true, false);
  menu.addSeparator();
  menu.addItem(3, "Documentation", true, false);
  menu.addItem(4, "Report a Bug", true, false);
  menu.addSeparator();
  menu.addItem(5, "About Zenith DAW...", true, false);

  auto screenBounds = items_[3].bounds.translated(getScreenX(), 
                                                   getScreenY() + getHeight());
  menu.showMenuAsync(
      juce::PopupMenu::Options()
          .withTargetComponent(this)
          .withTargetScreenArea(screenBounds));
}

} // namespace zenith
