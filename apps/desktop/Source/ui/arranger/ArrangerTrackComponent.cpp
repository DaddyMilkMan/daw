/**
 * @file ArrangerTrackComponent.cpp
 * @brief Implementation of Arranger Tracks (Generic and Section)
 */

#include "ArrangerTrackComponent.h"
#include "ZenithDesignSystem.h"
#include <core/SkBlurTypes.h>
#include <core/SkCanvas.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>

namespace zenith {

static constexpr float HEADER_WIDTH = 260.0f;

ArrangerTrackComponent::ArrangerTrackComponent(ProjectState &ps, TrackType type)
    : projectState(ps), type_(type) {
  
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
        drawTrackBackground(canvas, rect);
        drawTrackHeader(canvas, rect);
    }
}

void ArrangerTrackComponent::drawTrackHeader(SkCanvas* canvas, const SkRect& bounds) {
    using namespace design;
    
    float y = 0;
    float trackHeight = bounds.height();
    SkRect headerRect = SkRect::MakeXYWH(0, 0, dimensions::ARRANGER_HEADER_WIDTH, trackHeight);

    SkPaint trackBgPaint;
    trackBgPaint.setStyle(SkPaint::kFill_Style);

    // A. Track Header Background - PREMIUM GLASSMORPHIC GRADIENT
    {
        SkPoint hdrGradPts[2] = {{0, y}, {0, y + trackHeight}};
        SkColor hdrGradColors[3] = {
            SkColorSetRGB(35, 45, 55), // Top - Cyan tint
            SkColorSetRGB(25, 25, 30), // Middle
            SkColorSetRGB(18, 18, 22)  // Bottom - darkest
        };
        float hdrPositions[3] = {0.0f, 0.3f, 1.0f};
        trackBgPaint.setShader(SkGradientShader::MakeLinear(
            hdrGradPts, hdrGradColors, hdrPositions, 3, SkTileMode::kClamp));
        canvas->drawRect(headerRect, trackBgPaint);
    }

    // Top edge highlight
    SkPaint topHighlight;
    topHighlight.setColor(SkColorSetARGB(20, 255, 255, 255));
    topHighlight.setStrokeWidth(1.0f);
    canvas->drawLine(0, 0.5f, dimensions::ARRANGER_HEADER_WIDTH, 0.5f, topHighlight);

    // D. Header Content
    SkFont nameFont = typography::getSkFont(typography::FONT_MD, FontWeight::Medium);
    SkFont smallFont = typography::getSkFont(typography::FONT_XS, FontWeight::Regular);

    // Track Number Badge
    {
        SkPaint badgePaint;
        badgePaint.setAntiAlias(true);
        badgePaint.setColor(SkColorSetARGB(40, 255, 255, 255));
        SkRect badgeRect = SkRect::MakeXYWH(spacing::SM, 8, 24, 18);
        canvas->drawRRect(SkRRect::MakeRectXY(badgeRect, 4, 4), badgePaint);

        SkPaint numPaint;
        numPaint.setAntiAlias(true);
        numPaint.setColor(colors::TEXT_SECONDARY);
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
        textPaint.setColor(colors::TEXT_PRIMARY);
        canvas->drawString(trackName_.toStdString().c_str(), spacing::MD + 24,
                           23.0f, nameFont, textPaint);
    }

    // Controls (Mute/Solo/Rec)
    drawControls(canvas, spacing::MD, 42.0f);

    // E. Right Border for Header
    {
        SkPaint dividerPaint;
        SkPoint divPts[2] = {{dimensions::ARRANGER_HEADER_WIDTH - 1, 0}, {dimensions::ARRANGER_HEADER_WIDTH - 1, trackHeight}};
        SkColor divColors[3] = {
            SkColorSetARGB(60, 255, 255, 255),
            SkColorSetARGB(30, 255, 255, 255),
            SkColorSetARGB(10, 255, 255, 255)
        };
        float divPos[3] = {0.0f, 0.2f, 1.0f};
        dividerPaint.setShader(SkGradientShader::MakeLinear(
            divPts, divColors, divPos, 3, SkTileMode::kClamp));
        canvas->drawLine(dimensions::ARRANGER_HEADER_WIDTH - 0.5f, 0, dimensions::ARRANGER_HEADER_WIDTH - 0.5f, trackHeight, dividerPaint);

        SkPaint shadowLine;
        shadowLine.setColor(SkColorSetARGB(40, 0, 0, 0));
        canvas->drawLine(dimensions::ARRANGER_HEADER_WIDTH + 0.5f, 0, dimensions::ARRANGER_HEADER_WIDTH + 0.5f, trackHeight, shadowLine);
    }
}

void ArrangerTrackComponent::drawTrackBackground(SkCanvas *canvas,
                                                 const SkRect &bounds) {
  using namespace design;

  // Alternating row tint
  if (trackIndex_ % 2 == 1) {
    SkPaint altRowPaint;
    altRowPaint.setColor(SkColorSetARGB(8, 255, 255, 255));
    canvas->drawRect(SkRect::MakeXYWH(dimensions::ARRANGER_HEADER_WIDTH, 0,
                                      bounds.width() -
                                          dimensions::ARRANGER_HEADER_WIDTH,
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
  sepPaint.setShader(SkGradientShader::MakeLinear(
      sepPts, sepColors, sepPos, 3, SkTileMode::kClamp));
  canvas->drawLine(0, bounds.height() - 0.5f, bounds.width(),
                   bounds.height() - 0.5f, sepPaint);
}

void ArrangerTrackComponent::drawControls(SkCanvas* canvas, float startX, float btnY) {
    using namespace design;
    
    float btnSize = 22.0f;
    float btnGap = 28.0f;
    SkFont smallFont = typography::getSkFont(typography::FONT_XS, FontWeight::Bold);

    auto drawBtn = [&](int index, const char* label, bool active, SkColor activeColor) {
        float bx = startX + (index * btnGap);
        SkRect btnRect = SkRect::MakeXYWH(bx, btnY, btnSize, btnSize);
        SkRRect btnRRect = SkRRect::MakeRectXY(btnRect, 6.0f, 6.0f);
        
        bool isHovered = (hoveredButtonIndex_ == index);

        // Shadow
        SkPaint shadowPaint;
        shadowPaint.setAntiAlias(true);
        shadowPaint.setColor(SkColorSetARGB(40, 0, 0, 0));
        shadowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 2.0f));
        SkRect shadowRect = btnRect; shadowRect.offset(0, 1);
        canvas->drawRRect(SkRRect::MakeRectXY(shadowRect, 6.0f, 6.0f), shadowPaint);

        if (active) {
             // Active state
             SkPaint glowPaint;
             glowPaint.setAntiAlias(true);
             glowPaint.setColor(withAlpha(activeColor, 0.5f));
             glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
             canvas->drawRRect(btnRRect, glowPaint);

             SkPaint btnPaint;
             btnPaint.setAntiAlias(true);
             btnPaint.setColor(activeColor);
             canvas->drawRRect(btnRRect, btnPaint);
        } else {
             // Inactive state
             SkPaint btnPaint;
             btnPaint.setAntiAlias(true);
             btnPaint.setColor(isHovered ? SkColorSetARGB(30, 255, 255, 255) : SkColorSetARGB(15, 255, 255, 255));
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
        float textW = smallFont.measureText(label, strlen(label), SkTextEncoding::kUTF8);
        canvas->drawString(label, bx + (btnSize - textW) * 0.5f, btnY + 15, smallFont, textPaint);
    };

    drawBtn(0, "M", isMuted_, colors::AMBER);
    drawBtn(1, "S", isSoloed_, colors::NEON_CYAN);
    drawBtn(2, "R", isRecordArmed_, colors::NEON_RED);
}

void ArrangerTrackComponent::drawSections(SkCanvas* canvas, const SkRect& bounds) {
    using namespace design;

    // Background
    {
      SkPoint bgPts[2] = {{0, 0}, {0, (float)bounds.height()}};
      SkColor bgColors[2] = {SkColorSetRGB(28, 28, 35),
                             SkColorSetRGB(22, 22, 28)};
      SkPaint bgPaint;
      bgPaint.setShader(SkGradientShader::MakeLinear(bgPts, bgColors, nullptr, 2, SkTileMode::kClamp));
      canvas->drawRect(bounds, bgPaint);
    }
    
    SkFont font = typography::getSkFont(typography::FONT_XS, FontWeight::SemiBold);

    for (size_t idx = 0; idx < sections_.size(); ++idx) {
        const auto &section = sections_[idx];
        double relStart = section.startBeats - viewStartBeats_;
        float x = static_cast<float>(relStart * pixelsPerBeat_);
        float w = static_cast<float>(section.lengthBeats * pixelsPerBeat_);

        if (x + w < 0 || x > bounds.width()) continue;

        float padding = 3.0f;
        SkRect rect = SkRect::MakeXYWH(x + 2, padding, w - 4, bounds.height() - padding * 2);
        if (rect.width() < 8) continue;

        SkRRect rrect = SkRRect::MakeRectXY(rect, 6.0f, 6.0f);
        SkColor c = SkColorSetARGB(255, section.color.getRed(), section.color.getGreen(), section.color.getBlue());
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
        fillPaint.setShader(SkGradientShader::MakeLinear(pts, gradColors, positions, 3, SkTileMode::kClamp));
        canvas->drawRRect(rrect, fillPaint);
        
        // Label
        if (w > 30) {
             SkPaint textPaint;
             textPaint.setColor(SK_ColorWHITE);
             textPaint.setAntiAlias(true);
             juce::String label = section.name;
             canvas->drawSimpleText(label.toRawUTF8(), label.length(), SkTextEncoding::kUTF8, x + 8, rect.centerY() + 4.0f, font, textPaint);
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
              setMuted(!isMuted_);
              projectState.setTrackMute(trackId_, isMuted_);
          } else if (hoveredButtonIndex_ == 1) {
              setSoloed(!isSoloed_);
              projectState.setTrackSolo(trackId_, isSoloed_);
          } else if (hoveredButtonIndex_ == 2) {
            setRecordArmed(!isRecordArmed_);
            projectState.setTrackArmed(trackId_, isRecordArmed_);
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

void ArrangerTrackComponent::mouseDoubleClick(const juce::MouseEvent &e) {
}

void ArrangerTrackComponent::mouseMove(const juce::MouseEvent &e) {
    if (type_ != TrackType::Section && e.position.x < HEADER_WIDTH) {
        using namespace design;
        float startX = spacing::MD;
        float btnY = 42.0f;
        float btnSize = 22.0f;
        float btnGap = 28.0f;
        
        int oldHover = hoveredButtonIndex_;
        hoveredButtonIndex_ = -1;
        
        for (int i=0; i<3; ++i) {
            float bx = startX + (i * btnGap);
            if (e.position.x >= bx && e.position.x <= bx + btnSize &&
                e.position.y >= btnY && e.position.y <= btnY + btnSize) {
                hoveredButtonIndex_ = i;
                break;
            }
        }
        
        if (oldHover != hoveredButtonIndex_) repaint();
    }
}

void ArrangerTrackComponent::mouseExit(const juce::MouseEvent &e) {
    if (hoveredButtonIndex_ != -1) {
        hoveredButtonIndex_ = -1;
        repaint();
    }
}

void ArrangerTrackComponent::moveSection(int index, double newStartBeats) {
  if (index < 0 || index >= sections_.size()) return;
  auto &section = sections_[index];
  
  // Call ProjectState to move the section AND its content
  projectState.moveSectionContent(section.id, newStartBeats, "Move section content");
  
  // Refresh our cache
  rebuildSections();
  repaint();
}

void ArrangerTrackComponent::rebuildSections() {
    sections_.clear();
    auto sectionsNode = projectState.getSections();
    if (!sectionsNode.isValid()) return;

    for (auto s : sectionsNode) {
        ArrangementSection section;
        section.id = s.getProperty(ProjectState::PROP_ID).toString();
        section.name = s.getProperty(ProjectState::PROP_NAME).toString();
        section.startBeats = s.getProperty(ProjectState::PROP_START);
        section.lengthBeats = s.getProperty(ProjectState::PROP_LENGTH);
        section.color = juce::Colour::fromString(s.getProperty(ProjectState::PROP_COLOR).toString());
        sections_.push_back(section);
    }
}

void ArrangerTrackComponent::setVisibleRange(double startBeats, double endBeats) {
  juce::ignoreUnused(endBeats);
  viewStartBeats_ = startBeats;
  repaint();
}

const ArrangementSection *ArrangerTrackComponent::getHoveredSection() const {
  return nullptr;
}

const ArrangementSection *ArrangerTrackComponent::getDraggingSection() const {
  if (draggingSectionIndex_ >= 0 && draggingSectionIndex_ < (int)sections_.size()) {
    return &sections_[draggingSectionIndex_];
  }
  return nullptr;
}

} // namespace zenith
