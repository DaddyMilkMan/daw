#include "ArrangerTrackComponent.h"
#include "TakeFolderComponent.h"
#include "../design-system/ColorBridge.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ZenithIcons.h"
// #include "../design-system/ZenithTheme.h" // Deprecated access
#include "../controls/SkiaButton.h"
#include "../controls/SkiaAlertWindow.h"
#include "../controls/SkiaPopupMenu.h"
#include "../controls/ContextMenuManager.h"
#include <core/SkBlurTypes.h>
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkPoint.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>

#ifdef kNormal_SkBlurStyle
#undef kNormal_SkBlurStyle
#endif

static constexpr float HEADER_WIDTH = 240.0f; // Aligned with design::spacing::trackHeaderWidth

namespace zenith {

ArrangerTrackComponent::ArrangerTrackComponent(ProjectState &ps,
                                               ArrangerGridUtils &gridUtils,
                                               TrackType type)
    : projectState(ps), gridUtils_(gridUtils), type_(type) {

  if (type_ == TrackType::Section) {
    rebuildSections();
  }
}

ArrangerTrackComponent::~ArrangerTrackComponent() = default;

void ArrangerTrackComponent::setMuted(bool m) {
  if (isMuted_ != m) {
    isMuted_ = m;
    repaint();
  }
}

void ArrangerTrackComponent::setSoloed(bool s) {
  if (isSoloed_ != s) {
    isSoloed_ = s;
    repaint();
  }
}

void ArrangerTrackComponent::setRecordArmed(bool r) {
  if (isRecordArmed_ != r) {
    isRecordArmed_ = r;
    repaint();
  }
}

void ArrangerTrackComponent::setInputMonitor(bool i) {
  if (isInputMonitoring_ != i) {
    isInputMonitoring_ = i;
    repaint();
  }
}

void ArrangerTrackComponent::setViewContext(double pixelsPerBeat,
                                            double viewStartBeats) {
  if (std::abs(pixelsPerBeat - pixelsPerBeat_) > 0.001 ||
      std::abs(viewStartBeats - viewStartBeats_) > 0.001) {
    pixelsPerBeat_ = pixelsPerBeat;
    viewStartBeats_ = viewStartBeats;
    repaint();
  }
}

void ArrangerTrackComponent::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());

  if (type_ == TrackType::Section) {
    drawSections(canvas, rect);
  } else {
    // Generic Track (Audio/Midi)
    drawTrackBackground(canvas, rect);
    drawTrackHeader(canvas, rect);
  }
}

void ArrangerTrackComponent::drawTrackHeader(SkCanvas *canvas,
                                             const SkRect &bounds) {
  using namespace design;

  float y = 0;
  float trackHeight = bounds.height();
  SkRect headerRect = SkRect::MakeXYWH(0, 0, HEADER_WIDTH, trackHeight);

  SkPaint trackBgPaint;
  trackBgPaint.setStyle(SkPaint::kFill_Style);

  // A. Track Header Background - PREMIUM GLASSMORPHIC GRADIENT (Distinct Cyan Tint)
  {
    SkPoint hdrGradPts[2] = {{0, y}, {0, y + trackHeight}};
    SkColor hdrGradColors[3] = {
        design::colors::BG_01, // Top
        design::withAlpha(design::colors::BG_DARKER, 0.8f), // Middle
        design::colors::BG_00  // Bottom
    };
    float hdrPositions[3] = {0.0f, 0.3f, 1.0f};
    trackBgPaint.setShader(SkGradientShader::MakeLinear(
        hdrGradPts, hdrGradColors, hdrPositions, 3, SkTileMode::kClamp));
    canvas->drawRect(headerRect, trackBgPaint);
  }

  // Top edge highlight
  SkPaint topHighlight;
  topHighlight.setColor(design::colors::BORDER_SUBTLE);
  topHighlight.setStrokeWidth(1.0f);
  canvas->drawLine(0, 0.5f, HEADER_WIDTH, 0.5f, topHighlight);

  // D. Header Content
  SkFont nameFont =
      typography::getSkFont(typography::FONT_MD, FontWeight::Medium);
  SkFont smallFont =
      typography::getSkFont(typography::FONT_XS, FontWeight::Regular);

  // Track Number Badge
  {
    SkPaint badgePaint;
    badgePaint.setAntiAlias(true);
    badgePaint.setColor(design::colors::BG_03);
    SkRect badgeRect = SkRect::MakeXYWH(spacing::SM, spacing::SM, spacing::LG, 18);
    canvas->drawRRect(SkRRect::MakeRectXY(badgeRect, dimensions::RADIUS_SM, dimensions::RADIUS_SM), badgePaint);

    SkPaint numPaint;
    numPaint.setAntiAlias(true);
    numPaint.setColor(design::colors::TEXT_SECONDARY);
    canvas->drawString(juce::String(trackIndex_ + 1).toStdString().c_str(),
                       spacing::SM + 6, 21, smallFont, numPaint);
  }

  // Track Name
  {
    SkPaint shadowPaint;
    shadowPaint.setAntiAlias(true);
    shadowPaint.setColor(design::withAlpha(design::colors::BG_DARKEST, 0.3f));
    canvas->drawString(trackName_.toStdString().c_str(), spacing::MD + 24 + 1,
                       bounds.centerY() + 7.0f, nameFont, shadowPaint);

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(design::colors::TEXT_PRIMARY);
    canvas->drawString(trackName_.toStdString().c_str(), spacing::MD + 24,
                       bounds.centerY() + 6.0f, nameFont, textPaint);
  }

  // Controls (Mute/Solo/Rec) - centered vertically
  drawControls(canvas, spacing::MD, bounds.centerY() - 11.0f);

  // E. Right Border for Header
  {
    SkPaint dividerPaint;
    SkPoint divPts[2] = {{HEADER_WIDTH - 1, 0},
                         {HEADER_WIDTH - 1, trackHeight}};
    SkColor divBase = design::colors::BORDER_SUBTLE;
    SkColor divColors[3] = {design::withAlpha(divBase, 0.25f),
                            design::withAlpha(divBase, 0.12f),
                            design::withAlpha(divBase, 0.04f)};
    float divPos[3] = {0.0f, 0.2f, 1.0f};
    dividerPaint.setShader(SkGradientShader::MakeLinear(
        divPts, divColors, divPos, 3, SkTileMode::kClamp));
    canvas->drawLine(HEADER_WIDTH - 0.5f, 0, HEADER_WIDTH - 0.5f, trackHeight,
                     dividerPaint);
 
    SkPaint shadowLine;
    shadowLine.setColor(design::withAlpha(design::colors::BG_DARKEST, 0.15f));
    canvas->drawLine(HEADER_WIDTH + 0.5f, 0, HEADER_WIDTH + 0.5f, trackHeight,
                     shadowLine);
  }
}

void ArrangerTrackComponent::drawTrackBackground(SkCanvas *canvas,
                                                 const SkRect &bounds) {
  using namespace design;

  // Alternating row tint
  if (trackIndex_ % 2 == 1) {
    SkPaint altRowPaint;
    altRowPaint.setColor(design::withAlpha(design::colors::BG_04, 0.03f));
    canvas->drawRect(SkRect::MakeXYWH(HEADER_WIDTH, 0,
                                      bounds.width() - HEADER_WIDTH,
                                      bounds.height()),
                     altRowPaint);
  }

  // Separator
  SkPaint sepPaint;
  SkPoint sepPts[2] = {{0, 0}, {bounds.width(), 0}};
  SkColor sepColors[3] = {SkColorSetARGB(60, 255, 255, 255),
                          SkColorSetARGB(30, 255, 255, 255),
                          SkColorSetARGB(10, 255, 255, 255)};
  float sepPos[3] = {0.0f, 0.3f, 1.0f};
  sepPaint.setShader(SkGradientShader::MakeLinear(sepPts, sepColors, sepPos, 3,
                                                  SkTileMode::kClamp));
  canvas->drawLine(0, bounds.height() - 0.5f, bounds.width(),
                   bounds.height() - 0.5f, sepPaint);
}

void ArrangerTrackComponent::drawControls(SkCanvas *canvas, float startX,
                                          float btnY) {
  using namespace design;

  float btnSize = 22.0f;
  float btnGap = 28.0f;

  auto drawBtn = [&](int index, const SkPath &icon, bool active,
                     SkColor activeColor) {
    float bx = startX + (index * btnGap);
    SkRect btnRect = SkRect::MakeXYWH(bx, btnY, btnSize, btnSize);
    SkRRect btnRRect = SkRRect::MakeRectXY(btnRect, dimensions::RADIUS_SM, dimensions::RADIUS_SM);

    bool isHovered = (hoveredButtonIndex_ == index);

    // Shadow
    SkPaint shadowPaint;
    shadowPaint.setAntiAlias(true);
    shadowPaint.setColor(SkColorSetARGB(40, 0, 0, 0));
    shadowPaint.setMaskFilter(
        SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 2.0f));
    SkRect shadowRect = btnRect;
    shadowRect.offset(0, 1);
    canvas->drawRRect(SkRRect::MakeRectXY(shadowRect, dimensions::RADIUS_SM, dimensions::RADIUS_SM), shadowPaint);

    if (active) {
      // Active state
      SkPaint glowPaint;
      glowPaint.setAntiAlias(true);
      glowPaint.setColor(withAlpha(activeColor, 0.5f));
      glowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
      canvas->drawRRect(btnRRect, glowPaint);

      SkPaint btnPaint;
      btnPaint.setAntiAlias(true);
      btnPaint.setColor(activeColor);
      canvas->drawRRect(btnRRect, btnPaint);
    } else {
      // Inactive state
      SkPaint btnPaint;
      btnPaint.setAntiAlias(true);
      btnPaint.setColor(isHovered ? SkColorSetARGB(30, 255, 255, 255)
                                  : SkColorSetARGB(15, 255, 255, 255));
      canvas->drawRRect(btnRRect, btnPaint);

      SkPaint border;
      border.setStyle(SkPaint::kStroke_Style);
      border.setColor(SkColorSetARGB(30, 255, 255, 255));
      canvas->drawRRect(btnRRect, border);
    }

    // Draw icon centered
    icons::IconStyle style;
    style.color = active ? SK_ColorWHITE : colors::TEXT_SECONDARY;
    style.filled = active; 
    style.strokeWidth = icons::STROKE_REGULAR;
    
    // Brighten on hover
    if (!active && isHovered) {
        style.color = SK_ColorWHITE;
    }

    icons::drawIconCentered(canvas, icon, btnRect, btnSize * 0.6f, style);
  };

  drawBtn(0, icons::Mute(), isMuted_, design::colors::WARNING);
  drawBtn(1, icons::Solo(), isSoloed_, design::colors::INFO);
  drawBtn(2, icons::Arm(), isRecordArmed_, design::colors::DANGER);
  drawBtn(3, icons::Eye(), isInputMonitoring_, design::colors::SUCCESS); // Input Monitor (Eye)
}

void ArrangerTrackComponent::drawSections(SkCanvas *canvas,
                                          const SkRect &bounds) {
  using namespace design;

  // Background
  {
    SkPoint bgPts[2] = {{0, 0}, {0, (float)bounds.height()}};
    SkColor bgColors[2] = {design::colors::BG_01,
                           design::colors::BG_02};
    SkPaint bgPaint;
    bgPaint.setShader(SkGradientShader::MakeLinear(bgPts, bgColors, nullptr, 2,
                                                   SkTileMode::kClamp));
    canvas->drawRect(bounds, bgPaint);
  }

  SkFont font =
      typography::getSkFont(typography::FONT_XS, FontWeight::SemiBold);

  for (size_t idx = 0; idx < sections_.size(); ++idx) {
    const auto &section = sections_[idx];
    double relStart = section.startBeats - viewStartBeats_;
    float x = static_cast<float>(relStart * pixelsPerBeat_);
    float w = static_cast<float>(section.lengthBeats * pixelsPerBeat_);

    if (x + w < 0 || x > bounds.width())
      continue;

    float padding = 3.0f;
    SkRect rect =
        SkRect::MakeXYWH(x + 2, padding, w - 4, bounds.height() - padding * 2);
    if (rect.width() < 8)
      continue;

    SkRRect rrect = SkRRect::MakeRectXY(rect, 6.0f, 6.0f);
    SkColor c =
        SkColorSetARGB(255, section.color.getRed(), section.color.getGreen(),
                       section.color.getBlue());
    bool isDragging = (draggingSectionIndex_ == static_cast<int>(idx));

    // Pill Fill
    SkPaint fillPaint;
    fillPaint.setAntiAlias(true);
    SkPoint pts[2] = {{rect.left(), rect.top()}, {rect.left(), rect.bottom()}};
    SkColor gradColors[3] = {
        withAlpha(lighten(c, 0.2f), isDragging ? 0.9f : 0.7f),
        withAlpha(c, isDragging ? 0.7f : 0.5f),
        withAlpha(darken(c, 0.2f), isDragging ? 0.6f : 0.4f)};
    float positions[3] = {0.0f, 0.4f, 1.0f};
    fillPaint.setShader(SkGradientShader::MakeLinear(pts, gradColors, positions,
                                                     3, SkTileMode::kClamp));
    canvas->drawRRect(rrect, fillPaint);

    // Label
    if (w > 30) {
      SkPaint textPaint;
      textPaint.setColor(SK_ColorWHITE);
      textPaint.setAntiAlias(true);
      juce::String label = section.name;
      canvas->drawSimpleText(label.toRawUTF8(), label.length(),
                             SkTextEncoding::kUTF8, x + 8,
                             rect.centerY() + 4.0f, font, textPaint);
    }
  }
}

void ArrangerTrackComponent::mouseDown(const juce::MouseEvent &e) {
  if (type_ == TrackType::Section) {
    double clickBeats = (e.position.x / pixelsPerBeat_) + viewStartBeats_;
    draggingSectionIndex_ = -1;
    for (size_t i = 0; i < sections_.size(); ++i) {
      if (clickBeats >= sections_[i].startBeats &&
          clickBeats < sections_[i].startBeats + sections_[i].lengthBeats) {
        draggingSectionIndex_ = static_cast<int>(i);
        dragStartBeats_ = clickBeats;
        initialSectionStart_ = sections_[i].startBeats;
        repaint();
        return;
      }
    }
  } else {
    // Right-click context menu for track header
    if (e.mods.isRightButtonDown() && e.position.x < HEADER_WIDTH) {
      auto menu = ContextMenuManager::createMenu();
      juce::String currentTrackId = trackId_;
      juce::String currentTrackName = trackName_;
      
      // Track operations section
      menu->addSectionHeader("Track");
      
      menu->addItem(1, "Rename...", true, false,
          [this, currentTrackId, currentTrackName]() {
              auto* dialog = new SkiaAlertWindow(
                  "Rename Track",
                  "Enter a new name for the track:",
                  SkiaAlertWindow::IconType::NoIcon);

              dialog->addTextEditor("name", currentTrackName, "Track Name");
              dialog->addButton("Rename", SkiaAlertWindow::Result::Button1, SkiaButton::Style::Primary);
              dialog->addButton("Cancel", SkiaAlertWindow::Result::Cancelled, SkiaButton::Style::Secondary);

              // Find parent to add to
              auto* parent = getParentComponent();
              while (parent != nullptr && parent->getParentComponent() != nullptr) {
                  parent = parent->getParentComponent();
              }

              if (parent != nullptr) {
                  int w = 400;
                  int h = 200;
                  dialog->setBounds((parent->getWidth() - w) / 2, (parent->getHeight() - h) / 2, w, h);
                  parent->addAndMakeVisible(dialog);
                  
                  dialog->showAsync([this, currentTrackId, dialog](SkiaAlertWindow::Result result) {
                      if (result == SkiaAlertWindow::Result::Button1) {
                          juce::String newName = dialog->getTextEditorContents("name");
                          if (newName.isNotEmpty()) {
                              projectState.renameTrack(currentTrackId, newName, "Rename Track");
                          }
                      }
                      delete dialog;
                  });
              } else {
                  delete dialog;
              }
          });
      
      menu->addItemWithShortcut(2, "Duplicate Track", "Ctrl+Shift+D", true,
          [this, currentTrackId]() {
              projectState.duplicateTrack(currentTrackId, "Duplicate Track");
          });
      
      menu->addItem(3, "Insert Track Above", true, false,
          [this, currentTrackId]() {
              projectState.insertTrackAbove(currentTrackId, "Audio", "Insert Track Above");
          });
      
      menu->addItem(4, "Insert Track Below", true, false,
          [this, currentTrackId]() {
              projectState.insertTrackBelow(currentTrackId, "Audio", "Insert Track Below");
          });
      
      menu->addSeparator();
      
      // Processing section
      menu->addSectionHeader("Processing");
      
      bool isFrozen = projectState.findTrack(currentTrackId).getProperty("frozen", false);

      menu->addItem(10, "Freeze Track", !isFrozen, false,
          [this, currentTrackId]() {
              if (onFreeze) onFreeze(currentTrackId);
          });
      
      menu->addItem(11, "Unfreeze Track", isFrozen, false,
          [this, currentTrackId]() {
              if (onUnfreeze) onUnfreeze(currentTrackId);
          });
      
      if (type_ == TrackType::Audio) {
          menu->addItem(12, "Separate Stems (AI)", !isFrozen, false,
              [this, currentTrackId]() {
                  if (onSeparateStems) onSeparateStems(currentTrackId);
              });
      }
      
      menu->addSeparator();
      
      // Automation section
      menu->addSectionHeader("Automation");
      
      auto automationMenu = ContextMenuManager::createMenu();
      automationMenu->addItem(100, "Volume", true, false, [this, currentTrackId]() {
          if (onAutomationLaneRequested) onAutomationLaneRequested(currentTrackId, "volume");
      });
      automationMenu->addItem(101, "Pan", true, false, [this, currentTrackId]() {
          if (onAutomationLaneRequested) onAutomationLaneRequested(currentTrackId, "pan");
      });
      automationMenu->addItem(102, "Mute", true, false, [this, currentTrackId]() {
          if (onAutomationLaneRequested) onAutomationLaneRequested(currentTrackId, "mute");
      });
      menu->addSubMenu("Add Automation Lane", std::move(automationMenu));
      
      menu->addItem(20, "Hide All Automation", true, false,
          [this, currentTrackId]() {
              if (onHideAllAutomation) onHideAllAutomation(currentTrackId);
          });
      
      menu->addSeparator();
      
      // Appearance section
      menu->addSectionHeader("Appearance");
      
      // Color submenu
      auto colorMenu = ContextMenuManager::createMenu();
      colorMenu->addItem(200, "Red", true, false, [this, currentTrackId]() {
          projectState.setTrackColor(currentTrackId, juce::Colour(0xFFFF4444), "Set Track Color");
      });
      colorMenu->addItem(201, "Orange", true, false, [this, currentTrackId]() {
          projectState.setTrackColor(currentTrackId, juce::Colour(0xFFFF8844), "Set Track Color");
      });
      colorMenu->addItem(202, "Yellow", true, false, [this, currentTrackId]() {
          projectState.setTrackColor(currentTrackId, juce::Colour(0xFFFFDD44), "Set Track Color");
      });
      colorMenu->addItem(203, "Green", true, false, [this, currentTrackId]() {
          projectState.setTrackColor(currentTrackId, juce::Colour(0xFF44FF88), "Set Track Color");
      });
      colorMenu->addItem(204, "Cyan", true, false, [this, currentTrackId]() {
          projectState.setTrackColor(currentTrackId, juce::Colour(0xFF44DDFF), "Set Track Color");
      });
      colorMenu->addItem(205, "Blue", true, false, [this, currentTrackId]() {
          projectState.setTrackColor(currentTrackId, juce::Colour(0xFF4488FF), "Set Track Color");
      });
      colorMenu->addItem(206, "Purple", true, false, [this, currentTrackId]() {
          projectState.setTrackColor(currentTrackId, juce::Colour(0xFF8844FF), "Set Track Color");
      });
      colorMenu->addItem(207, "Pink", true, false, [this, currentTrackId]() {
          projectState.setTrackColor(currentTrackId, juce::Colour(0xFFFF44AA), "Set Track Color");
      });
      menu->addSubMenu("Set Color", std::move(colorMenu));
      
      menu->addSeparator();
      
      // Danger zone
      menu->addItemComplete(99, "Delete Track", SkPath(), "", true, false, true,
          [this, currentTrackId]() {
              projectState.removeTrack(currentTrackId);
          });
      
      // Show menu
      ContextMenuManager::getInstance().showMenuAt(
          std::move(menu), this,
          static_cast<int>(e.position.x),
          static_cast<int>(e.position.y));
      return;
    }
    
    // Left-click on buttons (existing logic)
    if (e.position.x < HEADER_WIDTH && hoveredButtonIndex_ >= 0) {
      // Button clicked
      if (hoveredButtonIndex_ == 0) {
        bool newMute = !isMuted_;
        setMuted(newMute);
        if (trackId_.isNotEmpty()) {
          projectState.setTrackMute(trackId_, newMute, "Toggle Mute");
        }
      } else if (hoveredButtonIndex_ == 1) {
        bool newSolo = !isSoloed_;
        setSoloed(newSolo);
        if (trackId_.isNotEmpty()) {
          projectState.setTrackSolo(trackId_, newSolo, "Toggle Solo");
        }
      } else if (hoveredButtonIndex_ == 2) {
        bool newArmed = !isRecordArmed_;
        setRecordArmed(newArmed);
        if (trackId_.isNotEmpty()) {
          projectState.setTrackArmed(trackId_, newArmed, "Toggle Record Arm");
        }
      } else if (hoveredButtonIndex_ == 3) {
        bool newMonitor = !isInputMonitoring_;
        setInputMonitor(newMonitor);
        if (trackId_.isNotEmpty()) {
          projectState.setTrackInputMonitor(trackId_, newMonitor,
                                            "Toggle Input Monitor");
        }
      }
    }
  }
}

void ArrangerTrackComponent::mouseDrag(const juce::MouseEvent &e) {
  if (type_ == TrackType::Section && draggingSectionIndex_ >= 0) {
    double currentBeats = (e.position.x / pixelsPerBeat_) + viewStartBeats_;
    double delta = currentBeats - dragStartBeats_;
    sections_[draggingSectionIndex_].startBeats = initialSectionStart_ + delta;
    repaint();
  }
}

void ArrangerTrackComponent::mouseUp(const juce::MouseEvent &e) {
  if (type_ == TrackType::Section && draggingSectionIndex_ >= 0) {
    double finalStart = sections_[draggingSectionIndex_].startBeats;
    moveSection(draggingSectionIndex_, std::round(finalStart));
  }
  draggingSectionIndex_ = -1;
}

void ArrangerTrackComponent::mouseDoubleClick(const juce::MouseEvent &e) {}

void ArrangerTrackComponent::mouseMove(const juce::MouseEvent &e) {
  if (type_ != TrackType::Section && e.position.x < HEADER_WIDTH) {
    using namespace design;
    float startX = spacing::MD;
    float btnY = e.eventComponent->getLocalBounds().getCentreY() - 11.0f;
    float btnSize = 22.0f;
    float btnGap = 28.0f;

    int oldHover = hoveredButtonIndex_;
    hoveredButtonIndex_ = -1;

    for (int i = 0; i < 4; ++i) {
      float bx = startX + (i * btnGap);
      if (e.position.x >= bx && e.position.x <= bx + btnSize &&
          e.position.y >= btnY && e.position.y <= btnY + btnSize) {
        hoveredButtonIndex_ = i;
        break;
      }
    }

    if (oldHover != hoveredButtonIndex_)
      repaint();
  }
}

void ArrangerTrackComponent::mouseExit(const juce::MouseEvent &e) {
  if (hoveredButtonIndex_ != -1) {
    hoveredButtonIndex_ = -1;
    repaint();
  }
}

void ArrangerTrackComponent::moveSection(int index, double newStartBeats) {
  if (index < 0 || index >= sections_.size())
    return;
  auto &section = sections_[index];

  // Call ProjectState to move the section AND its content
  projectState.moveSectionContent(section.id, newStartBeats,
                                  "Move section content");

  // Refresh our cache
  rebuildSections();
  repaint();
}

void ArrangerTrackComponent::rebuildSections() {
  sections_.clear();
  auto sectionsNode = projectState.getSections();
  if (!sectionsNode.isValid())
    return;

  for (auto s : sectionsNode) {
    ArrangementSection section;
    section.id = s.getProperty(ProjectState::PROP_ID).toString();
    section.name = s.getProperty(ProjectState::PROP_NAME).toString();
    section.startBeats = s.getProperty(ProjectState::PROP_START);
    section.lengthBeats = s.getProperty(ProjectState::PROP_LENGTH);
    section.color = juce::Colour::fromString(
        s.getProperty(ProjectState::PROP_COLOR).toString());
    sections_.push_back(section);
  }
}

void ArrangerTrackComponent::setVisibleRange(double startBeats,
                                             double endBeats) {
  juce::ignoreUnused(endBeats);
  viewStartBeats_ = startBeats;
  repaint();
}

const ArrangementSection *ArrangerTrackComponent::getHoveredSection() const {
  return nullptr;
}

const ArrangementSection *ArrangerTrackComponent::getDraggingSection() const {
  if (draggingSectionIndex_ >= 0 &&
      draggingSectionIndex_ < (int)sections_.size()) {
    return &sections_[draggingSectionIndex_];
  }
  return nullptr;
}

void ArrangerTrackComponent::updateTakeFolders() {
  if (type_ == TrackType::Section) return;
  
  if (trackId_.isEmpty()) {
      takeFolders_.clear();
      return;
  }
  
  auto trackNode = projectState.findTrack(trackId_);
  if (!trackNode.isValid()) return;
  
  auto clipsNode = trackNode.getChildWithName(ProjectState::ID_CLIPS);
  if (!clipsNode.isValid()) {
      takeFolders_.clear();
      return;
  }
  
  // Reuse existing components if possible? 
  // For simplicity, we'll clear and rebuild for now, optimization later if needed.
  // Ideally we should sync: add new, remove stale, update existing.
  
  std::vector<juce::String> keptIds;
  
  // 1. Mark and Sweep / Sync approach
  // Iterate current components, see if they still exist in ValueTree
  for (auto it = takeFolders_.begin(); it != takeFolders_.end(); ) {
      juce::String id = (*it)->getValueTree()[ProjectState::PROP_ID].toString();
      auto folderNode = clipsNode.getChildWithProperty(ProjectState::PROP_ID, id);
      
      if (folderNode.isValid() && folderNode.hasType(ProjectState::ID_TAKE_FOLDER)) {
           // Exists, keep it
           (*it)->setZoomLevel(pixelsPerBeat_);
           (*it)->updateBounds(pixelsPerBeat_, 0 /* y */, 0 /* height handled by drawExpanded */);
           // Actually, TakeFolderComponent needs to know its track height context?
           // Currently logic is self-contained.
           keptIds.push_back(id);
           ++it;
      } else {
           // Removed
           removeChildComponent(it->get());
           it = takeFolders_.erase(it);
      }
  }
  
  // 2. Add new folders
  for (const auto& child : clipsNode) {
      if (child.hasType(ProjectState::ID_TAKE_FOLDER)) {
          juce::String id = child[ProjectState::PROP_ID].toString();
          bool found = false;
          for (const auto& existingId : keptIds) {
              if (existingId == id) { found = true; break; }
          }
          
          if (!found) {
              auto tf = std::make_unique<TakeFolderComponent>(projectState, gridUtils_, child);
              tf->setZoomLevel(pixelsPerBeat_);
              addAndMakeVisible(tf.get());
              takeFolders_.push_back(std::move(tf));
          }
      }
  }
  
  // 3. Update Layout
  // Arrange them vertically? No, they are timeline objects.
  // Their x/w is determined by start/length.
  // The Track Height might need to expand!
  // This is a layout complexity. For now, we will layout them inside the track bounds.
  // If track is not tall enough, they might clip.
  
  // For now, auto-collapse or something.
  for (auto& tf : takeFolders_) {
      double start = tf->getValueTree()[ProjectState::PROP_START];
      double len = tf->getValueTree()[ProjectState::PROP_LENGTH];
      
      // Update bounds geometry
      // We need to properly calculate x/w in pixels
      // Using helper?
      int x = static_cast<int>((start - viewStartBeats_) * pixelsPerBeat_) + (int)HEADER_WIDTH;
      int w = static_cast<int>(len * pixelsPerBeat_);
      int h = 80; // Default track height?
                   // If expanded, it needs more height.
                   
      tf->setBounds(x, 0, w, h);
  }
}

} // namespace zenith
