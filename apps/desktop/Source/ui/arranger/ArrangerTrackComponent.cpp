// UI Polish applied: Synced Mute/Solo with ProjectState
#include "ArrangerTrackComponent.h"
#include "TakeFolderComponent.h"
#include "ZenithDesignSystem.h"
#include <core/SkBlurTypes.h>
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkPoint.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>


#include "ZenithDesignSystem.h"
#include "../design-system/ZenithTheme.h"
#include <core/SkBlurTypes.h>

static constexpr float HEADER_WIDTH = 260.0f;

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
    juce::Colour bg00 = ZenithTheme::Colors::bg_00;
    juce::Colour bg01 = ZenithTheme::Colors::bg_01;
    SkColor hdrGradColors[3] = {
        SkColorSetRGB(bg01.getRed(), bg01.getGreen(), bg01.getBlue()), // Top
        SkColorSetRGB(25, 25, 30), // Middle
        SkColorSetRGB(bg00.getRed(), bg00.getGreen(), bg00.getBlue())  // Bottom
    };
    float hdrPositions[3] = {0.0f, 0.3f, 1.0f};
    trackBgPaint.setShader(SkGradientShader::MakeLinear(
        hdrGradPts, hdrGradColors, hdrPositions, 3, SkTileMode::kClamp));
    canvas->drawRect(headerRect, trackBgPaint);
  }

  // Top edge highlight
  SkPaint topHighlight;
  juce::Colour border = ZenithTheme::Colors::border_subtle;
  topHighlight.setColor(SkColorSetARGB(border.getAlpha(), border.getRed(), border.getGreen(), border.getBlue()));
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
    juce::Colour badgeBg = ZenithTheme::Colors::bg_03;
    badgePaint.setColor(SkColorSetARGB(badgeBg.getAlpha(), badgeBg.getRed(), badgeBg.getGreen(), badgeBg.getBlue()));
    SkRect badgeRect = SkRect::MakeXYWH(spacing::SM, 8, 24, 18);
    canvas->drawRRect(SkRRect::MakeRectXY(badgeRect, 4, 4), badgePaint);

    SkPaint numPaint;
    numPaint.setAntiAlias(true);
    juce::Colour numCol = ZenithTheme::Colors::text_secondary;
    numPaint.setColor(SkColorSetARGB(numCol.getAlpha(), numCol.getRed(), numCol.getGreen(), numCol.getBlue()));
    canvas->drawString(juce::String(trackIndex_ + 1).toStdString().c_str(),
                       spacing::SM + 6, 21, smallFont, numPaint);
  }

  // Track Name
  {
    SkPaint shadowPaint;
    shadowPaint.setAntiAlias(true);
    shadowPaint.setColor(SkColorSetARGB(80, 0, 0, 0));
    canvas->drawString(trackName_.toStdString().c_str(), spacing::MD + 24 + 1,
                       23.0f + 1, nameFont, shadowPaint);

    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    juce::Colour textCol = ZenithTheme::Colors::text_primary;
    textPaint.setColor(SkColorSetARGB(textCol.getAlpha(), textCol.getRed(), textCol.getGreen(), textCol.getBlue()));
    canvas->drawString(trackName_.toStdString().c_str(), spacing::MD + 24,
                       23.0f, nameFont, textPaint);
  }

  // Controls (Mute/Solo/Rec)
  drawControls(canvas, spacing::MD, 42.0f);

  // E. Right Border for Header
  {
    SkPaint dividerPaint;
    SkPoint divPts[2] = {{HEADER_WIDTH - 1, 0},
                         {HEADER_WIDTH - 1, trackHeight}};
    juce::Colour divColor = ZenithTheme::Colors::border_subtle;
    SkColor divColors[3] = {SkColorSetARGB(60, divColor.getRed(), divColor.getGreen(), divColor.getBlue()),
                            SkColorSetARGB(30, divColor.getRed(), divColor.getGreen(), divColor.getBlue()),
                            SkColorSetARGB(10, divColor.getRed(), divColor.getGreen(), divColor.getBlue())};
    float divPos[3] = {0.0f, 0.2f, 1.0f};
    dividerPaint.setShader(SkGradientShader::MakeLinear(
        divPts, divColors, divPos, 3, SkTileMode::kClamp));
    canvas->drawLine(HEADER_WIDTH - 0.5f, 0, HEADER_WIDTH - 0.5f, trackHeight,
                     dividerPaint);

    SkPaint shadowLine;
    shadowLine.setColor(SkColorSetARGB(40, 0, 0, 0));
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
    juce::Colour alt = ZenithTheme::Colors::bg_04; // Using bg_04 as very subtle highlight
    altRowPaint.setColor(SkColorSetARGB(8, alt.getRed(), alt.getGreen(), alt.getBlue()));
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
  SkFont smallFont =
      typography::getSkFont(typography::FONT_XS, FontWeight::Bold);

  auto drawBtn = [&](int index, const char *label, bool active,
                     SkColor activeColor) {
    float bx = startX + (index * btnGap);
    SkRect btnRect = SkRect::MakeXYWH(bx, btnY, btnSize, btnSize);
    SkRRect btnRRect = SkRRect::MakeRectXY(btnRect, 6.0f, 6.0f);

    bool isHovered = (hoveredButtonIndex_ == index);

    // Shadow
    SkPaint shadowPaint;
    shadowPaint.setAntiAlias(true);
    shadowPaint.setColor(SkColorSetARGB(40, 0, 0, 0));
    shadowPaint.setMaskFilter(
        SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 2.0f));
    SkRect shadowRect = btnRect;
    shadowRect.offset(0, 1);
    canvas->drawRRect(SkRRect::MakeRectXY(shadowRect, 6.0f, 6.0f), shadowPaint);

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

    // Label
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(active ? SK_ColorWHITE : colors::TEXT_SECONDARY);
    float textW =
        smallFont.measureText(label, strlen(label), SkTextEncoding::kUTF8);
    canvas->drawString(label, bx + (btnSize - textW) * 0.5f, btnY + 15,
                       smallFont, textPaint);
  };

  drawBtn(0, "M", isMuted_, SkColorSetRGB(ZenithTheme::Colors::warning.getRed(), ZenithTheme::Colors::warning.getGreen(), ZenithTheme::Colors::warning.getBlue()));
  drawBtn(1, "S", isSoloed_, SkColorSetRGB(ZenithTheme::Colors::info.getRed(), ZenithTheme::Colors::info.getGreen(), ZenithTheme::Colors::info.getBlue()));
  drawBtn(2, "R", isRecordArmed_, SkColorSetRGB(ZenithTheme::Colors::error.getRed(), ZenithTheme::Colors::error.getGreen(), ZenithTheme::Colors::error.getBlue()));
  drawBtn(3, "I", isInputMonitoring_, SkColorSetRGB(ZenithTheme::Colors::success.getRed(), ZenithTheme::Colors::success.getGreen(), ZenithTheme::Colors::success.getBlue())); // Input Monitor
}

void ArrangerTrackComponent::drawSections(SkCanvas *canvas,
                                          const SkRect &bounds) {
  using namespace design;

  // Background
  {
    SkPoint bgPts[2] = {{0, 0}, {0, (float)bounds.height()}};
    juce::Colour bg01 = ZenithTheme::Colors::bg_01;
    juce::Colour bg02 = ZenithTheme::Colors::bg_02;
    SkColor bgColors[2] = {SkColorSetRGB(bg01.getRed(), bg01.getGreen(), bg01.getBlue()),
                           SkColorSetRGB(bg02.getRed(), bg02.getGreen(), bg02.getBlue())};
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
    // Generic Track Headers
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
    float btnY = 42.0f;
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
