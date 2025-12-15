/*
  ==============================================================================

    ModulationMatrixView.cpp
    Created: 2025-12-09
    Enhanced: 2025-12-12
    Author:  Zenith DAW

    Animated Modulation Routing Visualization Implementation.

    Features bezier curves, particle animations, neon glows, and
    real-time modulation value visualization.

  ==============================================================================
*/

#include "ModulationMatrixView.h"
#include "Engine.h"
#include "../../engine/Track.h"
#include <algorithm>
#include <cmath>
#include <core/SkBlurTypes.h>
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkFont.h>
#include <core/SkMaskFilter.h>
#include <core/SkPaint.h>
#include <core/SkPath.h>
#include <core/SkRRect.h>
#include <effects/SkDashPathEffect.h>
#include <effects/SkGradientShader.h>

namespace zenith {

//==============================================================================
// Constructor / Destructor
//==============================================================================

ModulationMatrixView::ModulationMatrixView() : rng_(std::random_device{}()) {
  setOpaque(true);
  startTimerHz(60); // 60 FPS for smooth animation
}

ModulationMatrixView::~ModulationMatrixView() { stopTimer(); }

void ModulationMatrixView::setEngine(Engine *engine) {
  engine_ = engine;
  refreshMatrix();
}

//==============================================================================
// Timer Callback - Animation Update
//==============================================================================

void ModulationMatrixView::timerCallback() {
  const float deltaTime = 1.0f / 60.0f;
  globalAnimTime_ += deltaTime;

  // Update particle animations
  updateParticles(deltaTime);

  // Update real-time modulation values from engine
  updateModulationValues();

  markDirty();
}

void ModulationMatrixView::updateParticles(float deltaTime) {
  for (auto &conn : connections_) {
    // Sync particle speed to modulation source rate if LFO
    float baseSpeed = 0.3f + std::abs(conn.amount) * 0.5f;

    // Update existing particles
    for (auto &p : conn.particles) {
      p.position += p.speed * deltaTime * 2.0f;

      // Fade out near end
      if (p.position > 0.85f) {
        p.alpha = (1.0f - p.position) / 0.15f;
      }

      // Pulse size
      p.size =
          3.0f + std::sin(globalAnimTime_ * 8.0f + p.position * 10.0f) * 1.5f;
    }

    // Remove dead particles
    conn.particles.erase(
        std::remove_if(conn.particles.begin(), conn.particles.end(),
                       [](const ModulationConnection::Particle &p) {
                         return p.position > 1.0f;
                       }),
        conn.particles.end());

    // Spawn new particles (rate based on mod amount)
    float spawnRate = 0.1f + std::abs(conn.amount) * 0.2f;
    if (conn.particles.size() < 8 &&
        std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_) < spawnRate) {
      ModulationConnection::Particle p;
      p.position = 0.0f;
      p.speed =
          baseSpeed * std::uniform_real_distribution<float>(0.8f, 1.2f)(rng_);
      p.size = std::uniform_real_distribution<float>(3.0f, 5.0f)(rng_);
      p.alpha = 1.0f;
      conn.particles.push_back(p);
    }

    // Update animation phase for dash animation
    conn.animationPhase += deltaTime * (0.5f + std::abs(conn.amount));
    if (conn.animationPhase > 1.0f)
      conn.animationPhase -= 1.0f;
  }
}

void ModulationMatrixView::updateModulationValues() {
  if (!engine_)
    return;

  // Update LFO values
  for (auto &src : sourceNodes_) {
    if (src.id.startsWith("sys:lfo:")) {
      int lfoIndex = src.id.getTrailingIntValue();
      if (lfoIndex >= 0 && lfoIndex < 4) {
        // src.currentValue = engine_->getGlobalLFO(lfoIndex).getValue();
      }
    } else if (src.id.startsWith("sys:macro:")) {
      int macroIndex = src.id.getTrailingIntValue();
      if (macroIndex >= 0 && macroIndex < 8) {
        // src.currentValue = engine_->getMacro(macroIndex).getValue();
      }
    }
    // Other sources like velocity would be updated from MIDI input
  }
}

//==============================================================================
// Main Drawing
//==============================================================================

void ModulationMatrixView::drawSkia(SkCanvas *canvas) {
  if (!canvas)
    return;

  canvas->save();

  // Apply zoom and pan
  canvas->translate(viewOffset_.x, viewOffset_.y);
  canvas->scale(zoomLevel_, zoomLevel_);

  drawBackground(canvas);
  drawGrid(canvas);
  drawConnections(canvas);
  drawSourceNodes(canvas);
  drawDestNodes(canvas);

  if (isDraggingConnection_) {
    drawDragPreview(canvas);
  }

  if (selectedConnection_ != nullptr) {
    drawAmountEditor(canvas);
  }

  canvas->restore();

  // Draw title (not affected by pan/zoom)
  SkPaint titlePaint;
  titlePaint.setColor(design::colors::TEXT_PRIMARY);
  titlePaint.setAntiAlias(true);

  auto font = design::typography::getDisplayFont(20.0f, FontWeight::Bold);
  canvas->drawString("Modulation Matrix", 20, 35, font, titlePaint);

  // Draw subtitle
  SkPaint subtitlePaint;
  subtitlePaint.setColor(design::colors::TEXT_SECONDARY);
  subtitlePaint.setAntiAlias(true);
  auto smallFont = design::typography::getSkFont(12.0f);
  canvas->drawString("Drag from source to destination to create routes", 20, 52,
                     smallFont, subtitlePaint);
}

void ModulationMatrixView::drawBackground(SkCanvas *canvas) {
  // Deep dark background with subtle gradient
  SkPaint bgPaint;

  SkPoint pts[] = {{0, 0}, {0, static_cast<float>(getHeight())}};
  SkColor colors[] = {design::colors::BG_DARKEST,
                      design::darken(design::colors::BG_DARKEST, 0.2f)};

  bgPaint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2,
                                                 SkTileMode::kClamp));
  canvas->drawRect(SkRect::MakeWH(static_cast<float>(getWidth()) / zoomLevel_,
                                  static_cast<float>(getHeight()) / zoomLevel_),
                   bgPaint);
}

void ModulationMatrixView::drawGrid(SkCanvas *canvas) {
  SkPaint gridPaint;
  gridPaint.setColor(gridColor_);
  gridPaint.setAntiAlias(true);
  gridPaint.setStrokeWidth(0.5f);
  gridPaint.setStyle(SkPaint::kStroke_Style);

  float w = static_cast<float>(getWidth()) / zoomLevel_;
  float h = static_cast<float>(getHeight()) / zoomLevel_;

  // Vertical lines
  for (float x = 0; x < w; x += gridSpacing_) {
    canvas->drawLine(x, 0, x, h, gridPaint);
  }

  // Horizontal lines
  for (float y = 0; y < h; y += gridSpacing_) {
    canvas->drawLine(0, y, w, y, gridPaint);
  }
}

//==============================================================================
// Node Drawing
//==============================================================================

void ModulationMatrixView::drawSourceNodes(SkCanvas *canvas) {
  // Draw column header
  SkPaint headerPaint;
  headerPaint.setColor(design::colors::TEXT_SECONDARY);
  headerPaint.setAntiAlias(true);
  auto headerFont = design::typography::getSkFont(14.0f, FontWeight::SemiBold);
  canvas->drawString("SOURCES", kSourceColumnX - 25, kHeaderHeight - 10,
                     headerFont, headerPaint);

  for (auto &node : sourceNodes_) {
    drawNode(canvas, node.position, node.radius, node.color, node.displayName,
             true, node.isHovered, node.isSelected, node.currentValue);
  }
}

void ModulationMatrixView::drawDestNodes(SkCanvas *canvas) {
  // Draw column header
  SkPaint headerPaint;
  headerPaint.setColor(design::colors::TEXT_SECONDARY);
  headerPaint.setAntiAlias(true);
  auto headerFont = design::typography::getSkFont(14.0f, FontWeight::SemiBold);
  canvas->drawString("DESTINATIONS", kDestColumnX - 40, kHeaderHeight - 10,
                     headerFont, headerPaint);

  for (auto &node : destNodes_) {
    drawNode(canvas, node.position, node.radius, node.color, node.displayName,
             false, node.isHovered, node.isSelected, node.currentValue);
  }
}

void ModulationMatrixView::drawNode(SkCanvas *canvas,
                                    const juce::Point<float> &pos, float radius,
                                    SkColor color, const juce::String &label,
                                    bool isSource, bool isHovered,
                                    bool isSelected, float value) {
  // Glow effect for active/hovered nodes
  float glowIntensity = isSelected ? 1.0f : (isHovered ? 0.7f : 0.3f);
  if (isSource && std::abs(value) > 0.01f) {
    glowIntensity = std::max(glowIntensity, std::abs(value) * 0.8f);
  }

  drawNodeGlow(canvas, pos, radius * 1.5f, color, glowIntensity);

  // Node background
  SkPaint bgPaint;
  bgPaint.setAntiAlias(true);
  bgPaint.setColor(design::colors::BG_MEDIUM);
  canvas->drawCircle(pos.x, pos.y, radius, bgPaint);

  // Node border with glow
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(isHovered ? 3.0f : 2.0f);
  borderPaint.setColor(color);
  canvas->drawCircle(pos.x, pos.y, radius - 1.0f, borderPaint);

  // Value indicator (arc for sources, fill for destinations)
  if (isSource) {
    // Draw modulation value as arc
    float normalizedValue = (value + 1.0f) * 0.5f; // -1..1 -> 0..1
    float startAngle = -90.0f;                     // Top
    float sweepAngle = (value) * 180.0f;           // -180 to 180

    SkPaint arcPaint;
    arcPaint.setAntiAlias(true);
    arcPaint.setStyle(SkPaint::kStroke_Style);
    arcPaint.setStrokeWidth(4.0f);
    arcPaint.setStrokeCap(SkPaint::kRound_Cap);
    arcPaint.setColor(color);

    SkRect arcRect =
        SkRect::MakeXYWH(pos.x - radius * 0.6f, pos.y - radius * 0.6f,
                         radius * 1.2f, radius * 1.2f);

    SkPath arcPath;
    arcPath.addArc(arcRect, startAngle, sweepAngle);
    canvas->drawPath(arcPath, arcPaint);
  } else {
    // Destination: vertical bar indicator
    float barHeight = radius * 1.2f;
    float barY = pos.y + barHeight * 0.5f - barHeight * value;

    SkPaint barPaint;
    barPaint.setAntiAlias(true);
    barPaint.setColor(design::withAlpha(color, 0.6f));

    SkRect barRect = SkRect::MakeXYWH(
        pos.x - 3, pos.y + barHeight * 0.5f - barHeight * value, 6,
        barHeight * value);
    canvas->drawRoundRect(barRect, 2, 2, barPaint);
  }

  // Label
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(design::colors::TEXT_PRIMARY);

  auto font = design::typography::getSkFont(10.0f, FontWeight::Medium);

  // Draw label below node
  float labelWidth = font.measureText(label.toRawUTF8(), label.length(),
                                      SkTextEncoding::kUTF8);
  canvas->drawString(label.toRawUTF8(), pos.x - labelWidth / 2,
                     pos.y + radius + 14, font, textPaint);
}

void ModulationMatrixView::drawNodeGlow(SkCanvas *canvas,
                                        const juce::Point<float> &pos,
                                        float radius, SkColor color,
                                        float intensity) {
  if (intensity < 0.01f)
    return;

  SkPaint glowPaint;
  glowPaint.setAntiAlias(true);
  glowPaint.setColor(design::withAlpha(color, intensity * 0.4f));
  glowPaint.setMaskFilter(
      SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, radius * 0.3f));

  canvas->drawCircle(pos.x, pos.y, radius, glowPaint);
}

//==============================================================================
// Connection Drawing
//==============================================================================

void ModulationMatrixView::drawConnections(SkCanvas *canvas) {
  for (auto &conn : connections_) {
    drawConnection(canvas, conn);
  }
}

void ModulationMatrixView::drawConnection(SkCanvas *canvas,
                                          ModulationConnection &conn) {
  if (conn.path.isEmpty())
    return;

  SkColor color = conn.getColor();
  float thickness = conn.getThickness();

  // Outer glow
  SkPaint glowPaint;
  glowPaint.setAntiAlias(true);
  glowPaint.setStyle(SkPaint::kStroke_Style);
  glowPaint.setStrokeWidth(thickness + 6.0f);
  glowPaint.setColor(design::withAlpha(color, 0.15f));
  glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 4.0f));
  canvas->drawPath(conn.path, glowPaint);

  // Main connection line with animated dash
  SkPaint linePaint;
  linePaint.setAntiAlias(true);
  linePaint.setStyle(SkPaint::kStroke_Style);
  linePaint.setStrokeWidth(thickness);
  linePaint.setStrokeCap(SkPaint::kRound_Cap);
  linePaint.setColor(color);

  // Animated dash effect
  float dashLength = 12.0f;
  float gapLength = 6.0f;
  float dashOffset = conn.animationPhase * (dashLength + gapLength);

  SkScalar intervals[] = {dashLength, gapLength};
  linePaint.setPathEffect(
      SkDashPathEffect::Make(SkSpan<const SkScalar>(intervals, 2), dashOffset));

  canvas->drawPath(conn.path, linePaint);

  // Draw particles
  drawParticles(canvas, conn);

  // Bipolar indicator at midpoint
  drawBipolarIndicator(canvas, conn);

  // Hover/select highlight
  if (conn.isHovered || conn.isSelected) {
    SkPaint highlightPaint;
    highlightPaint.setAntiAlias(true);
    highlightPaint.setStyle(SkPaint::kStroke_Style);
    highlightPaint.setStrokeWidth(thickness + 2.0f);
    highlightPaint.setColor(
        design::withAlpha(design::colors::TEXT_PRIMARY, 0.3f));
    canvas->drawPath(conn.path, highlightPaint);
  }
}

void ModulationMatrixView::drawParticles(SkCanvas *canvas,
                                         ModulationConnection &conn) {
  if (conn.pathLength < 1.0f)
    return;

  SkPathMeasure measure(conn.path, false);
  SkColor particleColor = conn.getColor();

  for (const auto &particle : conn.particles) {
    float distance = particle.position * conn.pathLength;

    SkPoint pos;
    SkVector tangent;
    if (measure.getPosTan(distance, &pos, &tangent)) {
      // Particle glow
      SkPaint glowPaint;
      glowPaint.setAntiAlias(true);
      glowPaint.setColor(
          design::withAlpha(particleColor, particle.alpha * 0.5f));
      glowPaint.setMaskFilter(
          SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, particle.size));
      canvas->drawCircle(pos.fX, pos.fY, particle.size * 1.5f, glowPaint);

      // Particle core
      SkPaint corePaint;
      corePaint.setAntiAlias(true);
      corePaint.setColor(
          design::withAlpha(SK_ColorWHITE, particle.alpha * 0.9f));
      canvas->drawCircle(pos.fX, pos.fY, particle.size * 0.4f, corePaint);
    }
  }
}

void ModulationMatrixView::drawBipolarIndicator(
    SkCanvas *canvas, const ModulationConnection &conn) {
  // Find midpoint of path
  SkPathMeasure measure(conn.path, false);
  float midDist = measure.getLength() * 0.5f;

  SkPoint midPos;
  SkVector tangent;
  if (!measure.getPosTan(midDist, &midPos, &tangent))
    return;

  // Background pill
  float pillWidth = 44.0f;
  float pillHeight = 20.0f;
  SkRect pillRect =
      SkRect::MakeXYWH(midPos.fX - pillWidth / 2, midPos.fY - pillHeight / 2,
                       pillWidth, pillHeight);

  SkPaint pillBgPaint;
  pillBgPaint.setAntiAlias(true);
  pillBgPaint.setColor(design::colors::BG_DARKER);
  canvas->drawRoundRect(pillRect, pillHeight / 2, pillHeight / 2, pillBgPaint);

  // Border
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setColor(conn.getColor());
  canvas->drawRoundRect(pillRect, pillHeight / 2, pillHeight / 2, borderPaint);

  // Amount text
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(design::colors::TEXT_PRIMARY);

  auto font = design::typography::getMonoFont(10.0f, FontWeight::Medium);

  juce::String amountStr = juce::String(conn.amount * 100.0f, 0) + "%";

  float textWidth = font.measureText(amountStr.toRawUTF8(), amountStr.length(),
                                     SkTextEncoding::kUTF8);
  canvas->drawString(amountStr.toRawUTF8(), midPos.fX - textWidth / 2,
                     midPos.fY + 4, font, textPaint);
}

void ModulationMatrixView::drawDragPreview(SkCanvas *canvas) {
  // Find the source node position
  juce::Point<float> startPos;
  for (const auto &node : sourceNodes_) {
    if (node.id == dragSourceId_) {
      startPos = node.position;
      break;
    }
  }

  // Draw bezier from source to current mouse position
  SkPath previewPath;
  previewPath.moveTo(startPos.x + kNodeRadius, startPos.y);

  float ctrlOffset = std::abs(dragCurrentPos_.x - startPos.x) * 0.5f;
  previewPath.cubicTo(startPos.x + kNodeRadius + ctrlOffset, startPos.y,
                      dragCurrentPos_.x - ctrlOffset, dragCurrentPos_.y,
                      dragCurrentPos_.x, dragCurrentPos_.y);

  // Animated dash
  SkPaint previewPaint;
  previewPaint.setAntiAlias(true);
  previewPaint.setStyle(SkPaint::kStroke_Style);
  previewPaint.setStrokeWidth(2.0f);
  previewPaint.setStrokeCap(SkPaint::kRound_Cap);
  previewPaint.setColor(design::colors::CYAN);

  SkScalar dash[] = {8.0f, 4.0f};
  float offset = globalAnimTime_ * 30.0f;
  previewPaint.setPathEffect(SkDashPathEffect::Make(
      SkSpan<const SkScalar>(dash, 2), std::fmod(offset, 12.0f)));

  canvas->drawPath(previewPath, previewPaint);

  // Glowing cursor at end
  SkPaint cursorPaint;
  cursorPaint.setAntiAlias(true);
  cursorPaint.setColor(design::colors::CYAN);
  cursorPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 6.0f));
  canvas->drawCircle(dragCurrentPos_.x, dragCurrentPos_.y, 8.0f, cursorPaint);

  cursorPaint.setMaskFilter(nullptr);
  cursorPaint.setColor(SK_ColorWHITE);
  canvas->drawCircle(dragCurrentPos_.x, dragCurrentPos_.y, 4.0f, cursorPaint);
}

void ModulationMatrixView::drawAmountEditor(SkCanvas *canvas) {
  if (!selectedConnection_)
    return;

  // Find connection midpoint for editor placement
  SkPathMeasure measure(selectedConnection_->path, false);
  SkPoint midPos;
  measure.getPosTan(measure.getLength() * 0.5f, &midPos, nullptr);

  // Draw larger editor panel
  float panelWidth = 120.0f;
  float panelHeight = 60.0f;
  SkRect panelRect = SkRect::MakeXYWH(midPos.fX - panelWidth / 2,
                                      midPos.fY + 20, panelWidth, panelHeight);

  // Panel background with glass effect
  SkPaint panelPaint;
  panelPaint.setAntiAlias(true);
  panelPaint.setColor(design::colors::BG_DARK);
  canvas->drawRoundRect(panelRect, 8, 8, panelPaint);

  // Border
  SkPaint borderPaint;
  borderPaint.setAntiAlias(true);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  borderPaint.setColor(selectedConnection_->getColor());
  canvas->drawRoundRect(panelRect, 8, 8, borderPaint);

  // Title
  SkPaint textPaint;
  textPaint.setAntiAlias(true);
  textPaint.setColor(design::colors::TEXT_SECONDARY);
  auto smallFont = design::typography::getSkFont(10.0f);
  canvas->drawString("AMOUNT", panelRect.centerX() - 22, panelRect.fTop + 16,
                     smallFont, textPaint);

  // Value display
  textPaint.setColor(design::colors::TEXT_PRIMARY);
  auto valueFont = design::typography::getMonoFont(18.0f, FontWeight::Bold);

  juce::String valueStr =
      juce::String(selectedConnection_->amount * 100.0f, 0) + "%";
  float valueWidth = valueFont.measureText(
      valueStr.toRawUTF8(), valueStr.length(), SkTextEncoding::kUTF8);
  canvas->drawString(valueStr.toRawUTF8(), panelRect.centerX() - valueWidth / 2,
                     panelRect.fTop + 40, valueFont, textPaint);

  // Hint
  textPaint.setColor(design::colors::TEXT_TERTIARY);
  auto hintFont = design::typography::getSkFont(9.0f);
  canvas->drawString("Drag to adjust", panelRect.centerX() - 30,
                     panelRect.fBottom - 6, hintFont, textPaint);
}

//==============================================================================
// Matrix Building
//==============================================================================

void ModulationMatrixView::refreshMatrix() {
  buildSourceNodes();
  buildDestNodes();
  buildConnections();
  layoutNodes();
  updateConnectionPaths();
  repaint();
}

void ModulationMatrixView::buildSourceNodes() {
  sourceNodes_.clear();

  // 1. Build Source Nodes (Left Side)
  constexpr int kNumLFOs = 4;
  constexpr int kNumMacros = 8;
  constexpr int kTrackLimit = 4;
  constexpr int kPluginLimit = 2;
  constexpr int kParamLimit = 4;

  // Global LFOs
  for (int i = 0; i < kNumLFOs; ++i) {
    ModulationSourceNode node;
    node.id = "sys:lfo:" + juce::String(i);
    node.displayName = "LFO " + juce::String(i + 1);
    node.type = ModulationSourceNode::Type::LFO;
    node.color = design::colors::CYAN;
    sourceNodes_.push_back(node);
  }

  // Macros
  for (int i = 0; i < kNumMacros; ++i) {
    ModulationSourceNode node;
    node.id = "sys:macro:" + juce::String(i);
    if (engine_) {
      node.displayName = engine_->getMacro(i).getName();
    } else {
      node.displayName = "Macro " + juce::String(i + 1);
    }
    node.type = ModulationSourceNode::Type::Macro;
    node.color = design::colors::NEON_GREEN;
    sourceNodes_.push_back(node);
  }

  // Standard modulation sources
  ModulationSourceNode velocity;
  velocity.id = "sys:velocity";
  velocity.displayName = "Velocity";
  velocity.type = ModulationSourceNode::Type::Velocity;
  velocity.color = design::colors::MAGENTA;
  sourceNodes_.push_back(velocity);

  ModulationSourceNode modWheel;
  modWheel.id = "sys:modwheel";
  modWheel.displayName = "Mod Wheel";
  modWheel.type = ModulationSourceNode::Type::ModWheel;
  modWheel.color = design::colors::VIOLET;
  sourceNodes_.push_back(modWheel);

  ModulationSourceNode aftertouch;
  aftertouch.id = "sys:aftertouch";
  aftertouch.displayName = "Aftertouch";
  aftertouch.type = ModulationSourceNode::Type::Aftertouch;
  aftertouch.color = design::colors::AMBER;
  sourceNodes_.push_back(aftertouch);

  // Track envelopes (if engine available)
  if (engine_) {
    const auto &tracks = engine_->tracks();
    for (size_t i = 0; i < tracks.size() && i < kTrackLimit; ++i) {
      if (tracks[i]) {
        ModulationSourceNode node;
        node.id = tracks[i]->getTrackId() + ":env";
        node.displayName = tracks[i]->getName() + " Env";
        node.type = ModulationSourceNode::Type::Envelope;
        node.color = design::colors::BLUE;
        sourceNodes_.push_back(node);
      }
    }
  }
}

void ModulationMatrixView::buildDestNodes() {
  destNodes_.clear();

  // Common synth destinations (always available)
  const struct {
    const char *id;
    const char *name;
    ModulationDestNode::Type type;
    SkColor color;
  } commonDests[] = {
      {"synth:filter:cutoff", "Filter Cutoff",
       ModulationDestNode::Type::FilterCutoff, 0xFFFF6B35},
      {"synth:filter:resonance", "Filter Reso",
       ModulationDestNode::Type::FilterResonance, 0xFFFF8C42},
      {"synth:osc:pitch", "Osc Pitch", ModulationDestNode::Type::OscPitch,
       0xFF4ECDC4},
      {"synth:osc:detune", "Osc Detune", ModulationDestNode::Type::OscDetune,
       0xFF45B7D1},
      {"synth:osc:mix", "Osc Mix", ModulationDestNode::Type::OscMix,
       0xFF96CEB4},
      {"synth:amp:gain", "Amp Gain", ModulationDestNode::Type::AmpGain,
       0xFFFECEAB},
      {"synth:pan", "Pan", ModulationDestNode::Type::Pan, 0xFFDDA0DD}};

  for (const auto &dest : commonDests) {
    ModulationDestNode node;
    node.id = dest.id;
    node.displayName = dest.name;
    node.type = dest.type;
    node.color = dest.color;
    destNodes_.push_back(node);
  }

  // Active track FX parameters (just a few for demo)
  if (engine_) {
    int count = 0;
    for (const auto &track : engine_->tracks()) {
      if (count++ > 2)
        break; // Limit for demo
      if (track) {
        ModulationDestNode node;
        node.id = track->getTrackId() + ":vol";
        node.displayName = track->getName() + " Vol";
        node.type = ModulationDestNode::Type::FXParameter;
        node.color = design::colors::TEXT_SECONDARY;
        destNodes_.push_back(node);
      }
    }
  }
}

void ModulationMatrixView::buildConnections() {
  connections_.clear();

  // Create some default connections for demo
  if (sourceNodes_.size() > 0 && destNodes_.size() > 0) {
    createConnection(sourceNodes_[0].id, destNodes_[0].id); // LFO1 -> Cutoff
    connections_.back().amount = 0.5f;

    if (sourceNodes_.size() > 2 && destNodes_.size() > 3) {
      createConnection(sourceNodes_[2].id,
                       destNodes_[3].id); // Velocity -> Phase
      connections_.back().amount = -0.3f;
    }
  }
}

void ModulationMatrixView::layoutNodes() {
  size_t count = 0;
  for (auto &node : sourceNodes_) {
    node.position = {kSourceColumnX, kHeaderHeight + 50.0f + count * 60.0f};
    count++;
  }

  count = 0;
  for (auto &node : destNodes_) {
    node.position = {kDestColumnX, kHeaderHeight + 50.0f + count * 60.0f};
    count++;
  }
}

void ModulationMatrixView::updateConnectionPaths() {
  for (auto &conn : connections_) {
    SkPoint start = {0, 0};
    SkPoint end = {0, 0};

    // Find source position
    for (const auto &node : sourceNodes_) {
      if (node.id == conn.sourceId) {
        start = {node.position.x + kNodeRadius, node.position.y};
        break;
      }
    }

    // Find dest position
    for (const auto &node : destNodes_) {
      if (node.id == conn.destId) {
        end = {node.position.x - kNodeRadius, node.position.y};
        break;
      }
    }

    // Build bezier path
    conn.path.reset();
    conn.path.moveTo(start);

    float controlDist = std::abs(end.fX - start.fX) * 0.5f;
    conn.path.cubicTo(start.fX + controlDist, start.fY, end.fX - controlDist,
                      end.fY, end.fX, end.fY);

    // Calculate length for particles
    SkPathMeasure measure(conn.path, false);
    conn.pathLength = measure.getLength();
  }
}

//==============================================================================
// Interaction
//==============================================================================

void ModulationMatrixView::mouseDown(const juce::MouseEvent &e) {
  auto pos = e.position.toFloat();
  // Reverse transforms if using zoom... but local bounds should handle it if
  // view is set transform. For now assume interaction in untransformed space or
  // apply inverse.

  // Simplified hit testing
  if (auto *source = hitTestSource(pos)) {
    isDraggingConnection_ = true;
    dragSourceId_ = source->id;
    dragCurrentPos_ = pos;
  } else if (auto *conn = hitTestConnection(pos)) {
    selectedConnection_ = conn;
    repaint();
  } else {
    selectedConnection_ = nullptr;
    isPanning_ = true;
    lastPanPos_ = pos;
    repaint();
  }
}

void ModulationMatrixView::mouseDrag(const juce::MouseEvent &e) {
  auto pos = e.position.toFloat();

  if (isDraggingConnection_) {
    dragCurrentPos_ = pos;
    repaint();
  } else if (isPanning_) {
    viewOffset_ += (pos - lastPanPos_);
    lastPanPos_ = pos;
    repaint();
  } else if (selectedConnection_) {
    // Modify amount by vertical drag
    float delta = (lastPanPos_.y - pos.y) * 0.01f;
    updateConnectionAmount(selectedConnection_, delta);
    lastPanPos_ = pos;
  }
}

void ModulationMatrixView::mouseUp(const juce::MouseEvent &e) {
  auto pos = e.position.toFloat();

  if (isDraggingConnection_) {
    if (auto *dest = hitTestDest(pos)) {
      createConnection(dragSourceId_, dest->id);
      refreshMatrix(); // Rebuild paths
    }
    isDraggingConnection_ = false;
    repaint();
  }
  isPanning_ = false;
}

void ModulationMatrixView::mouseMove(const juce::MouseEvent &e) {
  auto pos = e.position.toFloat();

  auto *prevSource = hoveredSource_;
  auto *prevDest = hoveredDest_;
  auto *prevConn = hoveredConnection_;

  hoveredSource_ = hitTestSource(pos);
  hoveredDest_ = hitTestDest(pos);
  hoveredConnection_ = hitTestConnection(pos);

  // Update hover states
  for (auto &n : sourceNodes_)
    n.isHovered = (&n == hoveredSource_);
  for (auto &n : destNodes_)
    n.isHovered = (&n == hoveredDest_);
  for (auto &c : connections_)
    c.isHovered = (&c == hoveredConnection_);

  if (prevSource != hoveredSource_ || prevDest != hoveredDest_ ||
      prevConn != hoveredConnection_) {
    repaint();
  }
}

void ModulationMatrixView::mouseDoubleClick(const juce::MouseEvent &e) {
  // Reset connection on double click
  if (auto *conn = hitTestConnection(e.position.toFloat())) {
    conn->amount = 0.0f;
    repaint();
  }
}

void ModulationMatrixView::mouseWheelMove(
    const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) {
  zoomLevel_ += wheel.deltaY * 0.1f;
  zoomLevel_ = juce::jlimit(0.5f, 2.0f, zoomLevel_);
  repaint();
}

//==============================================================================
// Helpers
//==============================================================================

ModulationSourceNode *
ModulationMatrixView::hitTestSource(const juce::Point<float> &pos) {
  for (auto &node : sourceNodes_) {
    if (pos.getDistanceFrom(node.position) < kNodeRadius + 4.0f) {
      return &node;
    }
  }
  return nullptr;
}

ModulationDestNode *
ModulationMatrixView::hitTestDest(const juce::Point<float> &pos) {
  for (auto &node : destNodes_) {
    if (pos.getDistanceFrom(node.position) < kNodeRadius + 4.0f) {
      return &node;
    }
  }
  return nullptr;
}

ModulationConnection *
ModulationMatrixView::hitTestConnection(const juce::Point<float> &pos) {
  // Simple distance check to path midpoint for now
  for (auto &conn : connections_) {
    SkPathMeasure measure(conn.path, false);
    SkPoint mid;
    measure.getPosTan(measure.getLength() * 0.5f, &mid, nullptr);
    if (pos.getDistanceFrom({mid.fX, mid.fY}) < 20.0f) {
      return &conn;
    }
  }
  return nullptr;
}

void ModulationMatrixView::createConnection(const juce::String &sourceId,
                                            const juce::String &destId) {
  // Check if exists
  for (const auto &conn : connections_) {
    if (conn.sourceId == sourceId && conn.destId == destId)
      return;
  }

  ModulationConnection conn;
  conn.sourceId = sourceId;
  conn.destId = destId;
  conn.amount = 0.5f; // Default amount
  connections_.push_back(conn);
}

void ModulationMatrixView::deleteConnection(ModulationConnection *conn) {
  // TODO: remove from vector
}

void ModulationMatrixView::updateConnectionAmount(ModulationConnection *conn,
                                                  float delta) {
  if (conn) {
    conn->amount = juce::jlimit(-1.0f, 1.0f, conn->amount + delta);
    repaint();
  }
}

std::vector<AIElementInfo> ModulationMatrixView::getInspectableElements() {
  std::vector<AIElementInfo> elements;
  // TODO: Expose nodes and connections for AI access
  return elements;
}

} // namespace zenith
