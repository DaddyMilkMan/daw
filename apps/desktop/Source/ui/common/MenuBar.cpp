/**
 * @file MenuBar.cpp
 * @brief Custom Skia-based Menu Bar implementation
 *
 * Premium matte-black menu bar with animated hover states, vector icons,
 * and restrained accent lighting.
 */

#include "MenuBar.h"
#include "../design-system/ZenithTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

ZenithMenuBar::ZenithMenuBar() {
  setOpaque(false); // Allow layered Skia composition

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

  // 1. Solid Dark Background (Sharp edges, no rounding)
  // FIX: Removed to prevent double-drawing/flickering. TitleBarComponent already draws the background.
  /*
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(design::colors::BG_DARK); // Solid dark background for visibility
  canvas->drawRect(skBounds, bgPaint);
  */
  
  // 2. Subtle bottom border
  // FIX: Removed redundant border
  /*
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setColor(SkColorSetA(design::colors::CYAN, 80));
  canvas->drawLine(0, skBounds.height() - 1, skBounds.width(), skBounds.height() - 1, borderPaint);
  */

  // 3. Draw Menu Items
  for (size_t i = 0; i < items_.size(); ++i) {
    drawMenuItem(canvas, items_[i], static_cast<int>(i) == hoveredItemIndex_);
    
    // Draw green update indicator next to Help (last item)
    // FIX: Removed as per user feedback ("random green dot")
    /*
    if (updateAvailable_ && i == items_.size() - 1) {
        float dotX = items_[i].bounds.getRight() + design::spacing::SM;
        float dotY = skBounds.centerY();
        float dotRadius = 4.0f;
        
        SkPaint dotPaint;
        dotPaint.setAntiAlias(true);
        dotPaint.setColor(design::colors::NEON_GREEN);
        canvas->drawCircle(dotX, dotY, dotRadius, dotPaint);
    }
    */
  }
  
  // Collab button removed - cleaner interface
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
  repaint();
}

void ZenithMenuBar::mouseDown(const juce::MouseEvent &e) {
  // Set pressed states
  for (size_t i = 0; i < items_.size(); ++i) {
    items_[i].state.isPressed = items_[i].bounds.contains(e.getPosition());
  }
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

}

void ZenithMenuBar::mouseUp(const juce::MouseEvent &e) {
  juce::ignoreUnused(e);
  
  for (auto &item : items_) {
    item.state.isPressed = false;
  }
}

void ZenithMenuBar::showFileMenu() {
  // Create menu if it doesn't exist
  if (!fileMenu_) {
    fileMenu_ = std::make_unique<SkiaPopupMenu>();

    fileMenu_->addItemComplete(1, "New Project", icons::Plus(), "⌘N",
      onNewProject != nullptr, false, false,
      [this]() { if (onNewProject) onNewProject(); });

    fileMenu_->addItemComplete(2, "Open Project...", icons::Folder(), "⌘O",
      onOpenProject != nullptr, false, false,
      [this]() { if (onOpenProject) onOpenProject(); });

    fileMenu_->addSeparator();

    fileMenu_->addItemComplete(3, "Save Project", icons::Save(), "⌘S",
      onSaveProject != nullptr, false, false,
      [this]() { if (onSaveProject) onSaveProject(); });

    fileMenu_->addItemComplete(4, "Save Project As...", icons::Upload(), "⇧⌘S",
      onSaveProjectAs != nullptr, false, false,
      [this]() { if (onSaveProjectAs) onSaveProjectAs(); });

    fileMenu_->addSeparator();

    fileMenu_->addItemComplete(5, "Import Audio...", icons::Upload(), "⌘I",
      onImportAudio != nullptr, false, false,
      [this]() { if (onImportAudio) onImportAudio(); });

    fileMenu_->addItemComplete(6, "Export Audio...", icons::Download(), "⌘E",
      onExportAudio != nullptr, false, false,
      [this]() { if (onExportAudio) onExportAudio(); });

    fileMenu_->addSeparator();

    fileMenu_->addItemComplete(7, "Quit", icons::Close(), "⌘Q",
      true, false, false,
      []() { juce::JUCEApplication::getInstance()->systemRequestedQuit(); });

    // Add to desktop so it can be shown as a popup
    fileMenu_->addToDesktop(juce::ComponentPeer::windowIsTemporary);
  }

  // Position directly below the File menu item
  auto screenPos = localPointToGlobal(juce::Point<int>(
    items_[0].bounds.getX(),
    items_[0].bounds.getBottom()
  ));

  fileMenu_->showAt(screenPos);
}

void ZenithMenuBar::showEditMenu() {
  // Create menu if it doesn't exist
  if (!editMenu_) {
    editMenu_ = std::make_unique<SkiaPopupMenu>();

    editMenu_->addItemComplete(1, "Undo", icons::Undo(), "⌘Z",
      onUndo != nullptr, false, false,
      [this]() { if (onUndo) onUndo(); });

    editMenu_->addItemComplete(2, "Redo", icons::Redo(), "⇧⌘Z",
      onRedo != nullptr, false, false,
      [this]() { if (onRedo) onRedo(); });

    editMenu_->addSeparator();

    editMenu_->addItemComplete(3, "Cut", icons::Cut(), "⌘X",
      onCut != nullptr, false, false,
      [this]() { if (onCut) onCut(); });

    editMenu_->addItemComplete(4, "Copy", icons::Copy(), "⌘C",
      onCopy != nullptr, false, false,
      [this]() { if (onCopy) onCopy(); });

    editMenu_->addItemComplete(5, "Paste", icons::Paste(), "⌘V",
      onPaste != nullptr, false, false,
      [this]() { if (onPaste) onPaste(); });

    editMenu_->addItemComplete(6, "Delete", icons::Delete(), "⌫",
      onDelete != nullptr, false, false,
      [this]() { if (onDelete) onDelete(); });

    editMenu_->addSeparator();

    editMenu_->addItemComplete(7, "Select All", icons::Plus(), "⌘A",
      onSelectAll != nullptr, false, false,
      [this]() { if (onSelectAll) onSelectAll(); });

    // Add to desktop so it can be shown as a popup
    editMenu_->addToDesktop(juce::ComponentPeer::windowIsTemporary);
  }

  // Position directly below the Edit menu item
  auto screenPos = localPointToGlobal(juce::Point<int>(
    items_[1].bounds.getX(),
    items_[1].bounds.getBottom()
  ));

  editMenu_->showAt(screenPos);
}

void ZenithMenuBar::showViewMenu() {
  // Create menu if it doesn't exist
  if (!viewMenu_) {
    viewMenu_ = std::make_unique<SkiaPopupMenu>();

    viewMenu_->addItemComplete(1, "Toggle Session/Arranger", icons::ViewToggle(), "Tab",
      onToggleView != nullptr, false, false,
      [this]() { if (onToggleView) onToggleView(); });

    viewMenu_->addItemComplete(2, "Toggle Mixer", icons::Settings(), "M",
      onToggleMixer != nullptr, false, false,
      [this]() { if (onToggleMixer) onToggleMixer(); });

    viewMenu_->addItemComplete(3, "Toggle Browser", icons::Search(), "B",
      onToggleBrowser != nullptr, false, false,
      [this]() { if (onToggleBrowser) onToggleBrowser(); });

    viewMenu_->addSeparator();

    viewMenu_->addItemComplete(4, "Zoom In", icons::ZoomIn(), "⌘+",
      onZoomIn != nullptr, false, false,
      [this]() { if (onZoomIn) onZoomIn(); });

    viewMenu_->addItemComplete(5, "Zoom Out", icons::ZoomOut(), "⌘-",
      onZoomOut != nullptr, false, false,
      [this]() { if (onZoomOut) onZoomOut(); });

    viewMenu_->addItemComplete(6, "Fit to Window", icons::ViewToggle(), "⌘0",
      onFitToWindow != nullptr, false, false,
      [this]() { if (onFitToWindow) onFitToWindow(); });

    // Add to desktop so it can be shown as a popup
    viewMenu_->addToDesktop(juce::ComponentPeer::windowIsTemporary);
  }

  // Position directly below the View menu item
  auto screenPos = localPointToGlobal(juce::Point<int>(
    items_[2].bounds.getX(),
    items_[2].bounds.getBottom()
  ));

  viewMenu_->showAt(screenPos);
}

void ZenithMenuBar::showHelpMenu() {
  // Create menu if it doesn't exist
  if (!helpMenu_) {
    helpMenu_ = std::make_unique<SkiaPopupMenu>();

    helpMenu_->addItemComplete(1, "Getting Started", icons::Sparkles(), "",
      onGettingStarted != nullptr, false, false,
      [this]() { if (onGettingStarted) onGettingStarted(); });

    helpMenu_->addItemComplete(2, "Keyboard Shortcuts", icons::Edit(), "?",
      onKeyboardShortcuts != nullptr, false, false,
      [this]() { if (onKeyboardShortcuts) onKeyboardShortcuts(); });

    helpMenu_->addSeparator();

    helpMenu_->addItemComplete(3, "Documentation", icons::File(), "",
      onDocumentation != nullptr, false, false,
      [this]() { if (onDocumentation) onDocumentation(); });

    helpMenu_->addItemComplete(4, "Report a Bug", icons::Send(), "",
      onReportBug != nullptr, false, false,
      [this]() { if (onReportBug) onReportBug(); });

    helpMenu_->addSeparator();

    helpMenu_->addItemComplete(5, "About Zenith DAW...", icons::Info(), "",
      onAbout != nullptr, false, false,
      [this]() { if (onAbout) onAbout(); });

    // Add to desktop so it can be shown as a popup
    helpMenu_->addToDesktop(juce::ComponentPeer::windowIsTemporary);
  }

  // Position directly below the Help menu item
  auto screenPos = localPointToGlobal(juce::Point<int>(
    items_[3].bounds.getX(),
    items_[3].bounds.getBottom()
  ));

  helpMenu_->showAt(screenPos);
}

} // namespace zenith
