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
#include "../../../include/Engine.h"
#include "../../engine/Track.h"
#include <algorithm>
#include <cmath>
#include <core/SkBlurTypes.h>
#include <core/SkMaskFilter.h>
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
      if (lfoIndex >= 0 && lfoIndex < zenith::kNumGlobalLFOs) {
        src.currentValue = engine_->getGlobalLFO(lfoIndex).getValue();
      }
    } else if (src.id.startsWith("sys:macro:")) {
      int macroIndex = src.id.getTrailingIntValue();
      if (macroIndex >= 0 && macroIndex < Engine::getNumMacros()) {
        src.currentValue = engine_->getMacro(macroIndex).getValue();
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

  char amountStr[16];
  snprintf(amountStr, sizeof(amountStr), "%+.0f%%", conn.amount * 100.0f);

  float textWidth =
      font.measureText(amountStr, strlen(amountStr), SkTextEncoding::kUTF8);
  canvas->drawString(amountStr, midPos.fX - textWidth / 2, midPos.fY + 4, font,
                     textPaint);
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

  char valueStr[16];
  snprintf(valueStr, sizeof(valueStr), "%+.0f%%",
           selectedConnection_->amount * 100.0f);
  float valueWidth =
      valueFont.measureText(valueStr, strlen(valueStr), SkTextEncoding::kUTF8);
  canvas->drawString(valueStr, panelRect.centerX() - valueWidth / 2,
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

  // Global LFOs
  for (int i = 0; i < zenith::kNumGlobalLFOs; ++i) {
    ModulationSourceNode node;
    node.id = "sys:lfo:" + juce::String(i);
    node.displayName = "LFO " + juce::String(i + 1);
    node.type = ModulationSourceNode::Type::LFO;
    node.color = design::colors::CYAN;
    sourceNodes_.push_back(node);
  }

  // Macros
  for (int i = 0; i < Engine::getNumMacros(); ++i) {
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
    for (size_t i = 0; i < tracks.size() && i < 4; ++i) {
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

  // Plugin parameters from tracks
  if (engine_) {
    const auto &tracks = engine_->tracks();
    for (size_t ti = 0; ti < tracks.size() && ti < 4; ++ti) {
      if (!tracks[ti])
        continue;

      int numPlugins = tracks[ti]->getNumPlugins();
      for (int pi = 0; pi < numPlugins && pi < 2; ++pi) {
        auto *plugin = tracks[ti]->getPlugin(pi);
        if (!plugin)
          continue;

        auto params = plugin->getParameters();
        // Take first 4 params from each plugin
        for (int paramIdx = 0; paramIdx < std::min(4, params.size());
             ++paramIdx) {
          auto *param = params[paramIdx];
          if (!param)
            continue;

          ModulationDestNode node;
          node.id = tracks[ti]->getTrackId() + ":fx" + juce::String(pi) + ":p" +
                    juce::String(paramIdx);
          node.displayName = param->getName(16);
          if (node.displayName.isEmpty()) {
            node.displayName = "Param " + juce::String(paramIdx + 1);
          }
          node.type = ModulationDestNode::Type::FXParameter;
          node.color = design::colors::MAGENTA;
          node.trackId = tracks[ti]->getTrackId();
          node.pluginIndex = pi;
          node.paramIndex = paramIdx;
          destNodes_.push_back(node);
        }
      }
    }
  }
}

void ModulationMatrixView::buildConnections() {
  connections_.clear();

  if (!engine_)
    return;

  // Query existing modulation connections from routing graph
  // For each source-dest pair, check if a modulation connection exists
  for (const auto &src : sourceNodes_) {
    for (const auto &dest : destNodes_) {
      // Check routing graph for modulation connection
      auto connections = engine_->getRoutingGraph().getConnectionsFrom(src.id);
      for (const auto &conn : connections) {
        if (conn.destId == dest.id) {
          ModulationConnection modConn;
          modConn.sourceId = src.id;
          modConn.destId = dest.id;
          modConn.amount = conn.gain; // Using gain as modulation amount
          connections_.push_back(modConn);
          break;
        }
      }
    }
  }
}

void ModulationMatrixView::layoutNodes() {
  float startY = kHeaderHeight + 40.0f;

  // Layout source nodes (left column)
  for (size_t i = 0; i < sourceNodes_.size(); ++i) {
    sourceNodes_[i].position = {kSourceColumnX, startY + i * kNodeSpacingY};
    sourceNodes_[i].radius = kNodeRadius;
  }

  // Layout destination nodes (right column)
  for (size_t i = 0; i < destNodes_.size(); ++i) {
    destNodes_[i].position = {kDestColumnX, startY + i * kNodeSpacingY};
    destNodes_[i].radius = kNodeRadius;
  }
}

void ModulationMatrixView::updateConnectionPaths() {
  for (auto &conn : connections_) {
    // Find source and dest positions
    juce::Point<float> srcPos, destPos;
    SkColor srcColor = design::colors::CYAN;

    for (const auto &node : sourceNodes_) {
      if (node.id == conn.sourceId) {
        srcPos = node.position;
        srcColor = node.color;
        break;
      }
    }

    for (const auto &node : destNodes_) {
      if (node.id == conn.destId) {
        destPos = node.position;
        break;
      }
    }

    // Build bezier path
    conn.path.reset();
    conn.path.moveTo(srcPos.x + kNodeRadius, srcPos.y);

    float ctrlOffset = std::abs(destPos.x - srcPos.x) * 0.4f;
    conn.path.cubicTo(srcPos.x + kNodeRadius + ctrlOffset, srcPos.y,
                      destPos.x - kNodeRadius - ctrlOffset, destPos.y,
                      destPos.x - kNodeRadius, destPos.y);

    // Calculate path length for particle animation
    SkPathMeasure measure(conn.path, false);
    conn.pathLength = measure.getLength();
  }
}

//==============================================================================
// Mouse Interaction
//==============================================================================

void ModulationMatrixView::mouseDown(const juce::MouseEvent &e) {
  auto pos = screenToWorld({static_cast<float>(e.x), static_cast<float>(e.y)});

  if (e.mods.isMiddleButtonDown()) {
    // Start panning
    isPanning_ = true;
    lastPanPos_ = {static_cast<float>(e.x), static_cast<float>(e.y)};
    return;
  }

  if (e.mods.isRightButtonDown()) {
    // Right-click: delete connection or show context menu
    auto *conn = hitTestConnection(pos);
    if (conn) {
      deleteConnection(conn);
      return;
    }
  }

  // Check for source node hit (start drag)
  auto *source = hitTestSource(pos);
  if (source) {
    isDraggingConnection_ = true;
    dragSourceId_ = source->id;
    dragCurrentPos_ = pos;
    source->isSelected = true;
    selectedConnection_ = nullptr;
    return;
  }

  // Check for connection hit (select for editing)
  auto *conn = hitTestConnection(pos);
  if (conn) {
    if (selectedConnection_)
      selectedConnection_->isSelected = false;
    conn->isSelected = true;
    selectedConnection_ = conn;
    return;
  }

  // Deselect
  if (selectedConnection_) {
    selectedConnection_->isSelected = false;
    selectedConnection_ = nullptr;
  }
}

void ModulationMatrixView::mouseUp(const juce::MouseEvent &e) {
  if (isPanning_) {
    isPanning_ = false;
    return;
  }

  if (isDraggingConnection_) {
    auto pos =
        screenToWorld({static_cast<float>(e.x), static_cast<float>(e.y)});

    // Check if dropped on a destination
    auto *dest = hitTestDest(pos);
    if (dest) {
      createConnection(dragSourceId_, dest->id);
    }

    // Clear drag state
    isDraggingConnection_ = false;
    dragSourceId_ = "";

    // Deselect source
    for (auto &src : sourceNodes_) {
      src.isSelected = false;
    }
  }
}

void ModulationMatrixView::mouseDrag(const juce::MouseEvent &e) {
  if (isPanning_) {
    viewOffset_.x += e.x - lastPanPos_.x;
    viewOffset_.y += e.y - lastPanPos_.y;
    lastPanPos_ = {static_cast<float>(e.x), static_cast<float>(e.y)};
    repaint();
    return;
  }

  if (isDraggingConnection_) {
    dragCurrentPos_ =
        screenToWorld({static_cast<float>(e.x), static_cast<float>(e.y)});
    repaint();
    return;
  }

  // Adjust connection amount with drag
  if (selectedConnection_ && e.mods.isLeftButtonDown()) {
    float delta = -e.getDistanceFromDragStartY() * 0.005f;
    updateConnectionAmount(selectedConnection_, delta);
    repaint();
  }
}

void ModulationMatrixView::mouseMove(const juce::MouseEvent &e) {
  auto pos = screenToWorld({static_cast<float>(e.x), static_cast<float>(e.y)});

  // Update hover states
  hoveredSource_ = hitTestSource(pos);
  hoveredDest_ = hitTestDest(pos);
  hoveredConnection_ = hitTestConnection(pos);

  for (auto &src : sourceNodes_) {
    src.isHovered = (&src == hoveredSource_);
  }

  for (auto &dest : destNodes_) {
    dest.isHovered = (&dest == hoveredDest_);
  }

  for (auto &conn : connections_) {
    conn.isHovered = (&conn == hoveredConnection_);
  }

  repaint();
}

void ModulationMatrixView::mouseDoubleClick(const juce::MouseEvent &e) {
  auto pos = screenToWorld({static_cast<float>(e.x), static_cast<float>(e.y)});

  // Double-click on connection to reset amount
  auto *conn = hitTestConnection(pos);
  if (conn) {
    conn->amount = 0.0f;
    // Update in engine
    if (engine_) {
      engine_->getRoutingGraph().connect(conn->sourceId, conn->destId, 0.0f);
    }
    repaint();
  }
}

void ModulationMatrixView::mouseWheelMove(
    const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) {
  // Zoom
  float zoomDelta = wheel.deltaY * 0.1f;
  float oldZoom = zoomLevel_;
  zoomLevel_ = juce::jlimit(0.5f, 2.0f, zoomLevel_ + zoomDelta);

  // Zoom towards mouse position
  if (zoomLevel_ != oldZoom) {
    float scale = zoomLevel_ / oldZoom;
    viewOffset_.x = e.x - (e.x - viewOffset_.x) * scale;
    viewOffset_.y = e.y - (e.y - viewOffset_.y) * scale;
  }

  repaint();
}

//==============================================================================
// Hit Testing
//==============================================================================

ModulationSourceNode *
ModulationMatrixView::hitTestSource(const juce::Point<float> &pos) {
  for (auto &node : sourceNodes_) {
    float dist = std::sqrt(std::pow(pos.x - node.position.x, 2) +
                           std::pow(pos.y - node.position.y, 2));
    if (dist <= node.radius) {
      return &node;
    }
  }
  return nullptr;
}

ModulationDestNode *
ModulationMatrixView::hitTestDest(const juce::Point<float> &pos) {
  for (auto &node : destNodes_) {
    float dist = std::sqrt(std::pow(pos.x - node.position.x, 2) +
                           std::pow(pos.y - node.position.y, 2));
    if (dist <= node.radius) {
      return &node;
    }
  }
  return nullptr;
}

ModulationConnection *
ModulationMatrixView::hitTestConnection(const juce::Point<float> &pos) {
  const float hitThreshold = 8.0f;

  for (auto &conn : connections_) {
    if (conn.path.isEmpty())
      continue;

    // Sample points along the path and check distance
    SkPathMeasure measure(conn.path, false);
    float length = measure.getLength();

    for (float dist = 0; dist < length; dist += 10.0f) {
      SkPoint pathPos;
      if (measure.getPosTan(dist, &pathPos, nullptr)) {
        float d = std::sqrt(std::pow(pos.x - pathPos.fX, 2) +
                            std::pow(pos.y - pathPos.fY, 2));
        if (d < hitThreshold) {
          return &conn;
        }
      }
    }
  }
  return nullptr;
}

//==============================================================================
// Connection Management
//==============================================================================

void ModulationMatrixView::createConnection(const juce::String &sourceId,
                                            const juce::String &destId) {
  // Check if connection already exists
  for (auto &conn : connections_) {
    if (conn.sourceId == sourceId && conn.destId == destId) {
      // Select existing connection
      selectedConnection_ = &conn;
      conn.isSelected = true;
      return;
    }
  }

  // Create new connection
  ModulationConnection conn;
  conn.sourceId = sourceId;
  conn.destId = destId;
  conn.amount = 1.0f; // Default to 100%
  connections_.push_back(conn);

  // Update in engine
  if (engine_) {
    engine_->getRoutingGraph().connect(sourceId, destId, conn.amount);
  }

  updateConnectionPaths();

  // Select the new connection
  selectedConnection_ = &connections_.back();
  selectedConnection_->isSelected = true;
}

void ModulationMatrixView::deleteConnection(ModulationConnection *conn) {
  if (!conn)
    return;

  // Remove from engine
  if (engine_) {
    engine_->getRoutingGraph().disconnect(conn->sourceId, conn->destId);
  }

  // Remove from our list
  connections_.erase(std::remove_if(connections_.begin(), connections_.end(),
                                    [conn](const ModulationConnection &c) {
                                      return c.sourceId == conn->sourceId &&
                                             c.destId == conn->destId;
                                    }),
                     connections_.end());

  if (selectedConnection_ == conn) {
    selectedConnection_ = nullptr;
  }
  if (hoveredConnection_ == conn) {
    hoveredConnection_ = nullptr;
  }

  repaint();
}

void ModulationMatrixView::updateConnectionAmount(ModulationConnection *conn,
                                                  float delta) {
  if (!conn)
    return;

  conn->amount = juce::jlimit(-1.0f, 1.0f, conn->amount + delta);

  // Update in engine
  if (engine_) {
    engine_->getRoutingGraph().connect(conn->sourceId, conn->destId,
                                       conn->amount);
  }
}

//==============================================================================
// Coordinate Transforms
//==============================================================================

juce::Point<float>
ModulationMatrixView::screenToWorld(const juce::Point<float> &screen) const {
  return {(screen.x - viewOffset_.x) / zoomLevel_,
          (screen.y - viewOffset_.y) / zoomLevel_};
}

juce::Point<float>
ModulationMatrixView::worldToScreen(const juce::Point<float> &world) const {
  return {world.x * zoomLevel_ + viewOffset_.x,
          world.y * zoomLevel_ + viewOffset_.y};
}

//==============================================================================
// AI Vision Support
//==============================================================================

std::vector<SkiaComponent::AIElementInfo>
ModulationMatrixView::getInspectableElements() {
  std::vector<AIElementInfo> elements;

  // Source nodes
  for (const auto &node : sourceNodes_) {
    auto screenPos = worldToScreen(node.position);
    AIElementInfo info;
    info.bounds =
        SkRect::MakeXYWH(screenPos.x - node.radius, screenPos.y - node.radius,
                         node.radius * 2, node.radius * 2);
    info.type = "mod_source";
    info.parameterId = node.id;
    info.currentValue = node.currentValue;
    elements.push_back(info);
  }

  // Destination nodes
  for (const auto &node : destNodes_) {
    auto screenPos = worldToScreen(node.position);
    AIElementInfo info;
    info.bounds =
        SkRect::MakeXYWH(screenPos.x - node.radius, screenPos.y - node.radius,
                         node.radius * 2, node.radius * 2);
    info.type = "mod_dest";
    info.parameterId = node.id;
    info.currentValue = node.currentValue;
    elements.push_back(info);
  }

  // Connections
  for (const auto &conn : connections_) {
    SkPathMeasure measure(conn.path, false);
    SkPoint midPos;
    measure.getPosTan(measure.getLength() * 0.5f, &midPos, nullptr);

    auto screenPos = worldToScreen({midPos.fX, midPos.fY});

    AIElementInfo info;
    info.bounds = SkRect::MakeXYWH(screenPos.x - 20, screenPos.y - 10, 40, 20);
    info.type = "mod_connection";
    info.parameterId = conn.sourceId + "->" + conn.destId;
    info.currentValue = conn.amount;
    elements.push_back(info);
  }

  return elements;
}

} // namespace zenith
