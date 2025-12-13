#include "../../include/ui/MacroToolbar.h"
#include "../../include/ui/ArrangerComponent.h" // For context if needed

// Skia Includes
#ifdef ZENITH_USE_SKIA
#include "skia/ZenithDesignSystem.h"
#include <core/SkCanvas.h>
#include <core/SkColor.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <effects/SkRuntimeEffect.h>
#endif

namespace zenith {

MacroToolbar::MacroToolbar(Engine &engine, ProjectState &projectState)
    : engine_(engine), projectState_(projectState) {
  setAlwaysOnTop(true);

  // Initial state
  currentOpacity_ = 0.0f; // Start hidden
  targetOpacity_ = 0.2f;  // Idle state (semi-visible)

  rebuildButtons();
  startTimerHz(60); // Animation loop (SkiaComponent has virtual timerCallback)
}

MacroToolbar::~MacroToolbar() = default;

void MacroToolbar::rebuildButtons() {
  macros_.clear();
  buttons_.clear();

  // 1. Heal Splits
  macros_.push_back({"Heal Splits", "Merge selected adjacent clips", "🧩",
                     [this](Engine &, ProjectState &) { healSplits(); }});

  // 2. Instant Freeze
  macros_.push_back({"Instant Freeze", "Bounce selected track to audio", "❄️",
                     [this](Engine &, ProjectState &) { instantFreeze(); }});

  // 3. Color by Track
  macros_.push_back({"Color by Track", "Reset clip colors to track default",
                     "🎨",
                     [this](Engine &, ProjectState &) { colorByTrack(); }});

  // Create buttons
  for (size_t i = 0; i < macros_.size(); ++i) {
    auto &macro = macros_[i];
    // ZenithButton constructor: text, onClick
    auto *btn = new ZenithButton(
        macro.name, [this, i]() { executeMacro(static_cast<int>(i)); });

    btn->setIconText(macro.iconChar);
    btn->setButtonStyle(ZenithButton::Style::Ghost); // Glassy look
    btn->setButtonSize(ZenithButton::Size::Medium);

    // Note: Tooltip functionality not yet implemented in ZenithButton
    // TODO: Add tooltip support to ZenithButton via juce::TooltipClient

    addAndMakeVisible(btn);
    buttons_.add(btn);
  }
}

void MacroToolbar::resized() {
  auto area = getLocalBounds().toFloat();

  float x = kPillPadding;
  float y = (area.getHeight() - 32.0f) /
            2.0f; // Centered vertically (Medium button is 32)

  for (auto *btn : buttons_) {
    // Measure text width or use fixed
    float btnWidth = 120.0f;
    btn->setBounds((int)x, (int)y, (int)btnWidth, 32);
    x += btnWidth + kButtonSpacing;
  }
}

void MacroToolbar::paint(juce::Graphics &g) {
  // Minimal fallback if Skia not enabled
  g.setOpacity(currentOpacity_);
  g.setColour(juce::Colours::black.withAlpha(0.5f));
  g.fillRoundedRectangle(getLocalBounds().toFloat(), 24.0f);
}

void MacroToolbar::drawSkia(SkCanvas *canvas) {
#ifdef ZENITH_USE_SKIA
  using namespace zenith::design;

  if (currentOpacity_ <= 0.01f)
    return;

  auto bounds = getLocalBounds();
  SkRect rect = SkRect::MakeWH(bounds.getWidth(), bounds.getHeight());
  SkRRect rrect;
  rrect.setRectXY(rect, 24.0f, 24.0f); // Fully rounded caps

  // Backdrop "Blur" simulation (Dark Glass)
  SkPaint fillPaint;
  // Set alpha manually
  SkColor baseColor = colors::BG_DARKEST;
  SkColor fillColor =
      SkColorSetA(baseColor, static_cast<U8CPU>(200 * currentOpacity_));

  fillPaint.setColor(fillColor);
  fillPaint.setStyle(SkPaint::kFill_Style);
  canvas->drawRRect(rrect, fillPaint);

  // Border / Shine
  SkPaint borderPaint;
  SkColor borderColor =
      SkColorSetA(colors::CYAN, static_cast<U8CPU>(100 * currentOpacity_));

  borderPaint.setColor(borderColor);
  borderPaint.setStyle(SkPaint::kStroke_Style);
  borderPaint.setStrokeWidth(1.0f);
  canvas->drawRRect(rrect, borderPaint);
#endif
}

void MacroToolbar::mouseEnter(const juce::MouseEvent &) {
  targetOpacity_ = 1.0f;
  isHovered_ = true;
}

void MacroToolbar::mouseExit(const juce::MouseEvent &) {
  targetOpacity_ = 0.2f;
  isHovered_ = false;
}

void MacroToolbar::mouseMove(const juce::MouseEvent &) {
  if (!isHovered_) {
    targetOpacity_ = 1.0f;
    isHovered_ = true;
  }
}

void MacroToolbar::checkProximity(juce::Point<float> mousePosInParent) {
  // Calculate distance to this component in parent coordinates.
  // getBounds() returns bounds in parent.
  auto bounds = getBounds().toFloat();
  float dist = 0.0f;
  if (bounds.contains(mousePosInParent)) {
    dist = 0.0f;
  } else {
    // Calculate distance to rectangle
    float dx = juce::jmax(bounds.getX() - mousePosInParent.x, 0.0f,
                          mousePosInParent.x - bounds.getRight());
    float dy = juce::jmax(bounds.getY() - mousePosInParent.y, 0.0f,
                          mousePosInParent.y - bounds.getBottom());
    dist = std::sqrt(dx * dx + dy * dy);
  }

  if (dist < 100.0f) {
    // Near
    targetOpacity_ = 1.0f - (dist / 100.0f); // Fade out as we go away
    if (targetOpacity_ < 0.2f)
      targetOpacity_ = 0.2f;
  } else {
    targetOpacity_ = 0.0f; // Completely hidden if far away
  }
}

void MacroToolbar::timerCallback() {
  // Smooth opacity transition
  if (std::abs(targetOpacity_ - currentOpacity_) > 0.01f) {
    currentOpacity_ += (targetOpacity_ - currentOpacity_) * 0.1f;
    setAlpha(currentOpacity_); // JUCE alpha for children
    repaint();
  }
}

void MacroToolbar::executeMacro(int index) {
  if (index >= 0 && index < (int)macros_.size()) {
    const auto &macro = macros_[index];
    DBG("MacroToolbar: Executing " << macro.name);
    if (macro.action) {
      macro.action(engine_, projectState_);
    }
  }
}

void MacroToolbar::healSplits() {
  if (!getSelectedClipIds)
    return;
  auto clipIds = getSelectedClipIds();
  if (clipIds.size() < 2)
    return;

  projectState_.getUndoManager().beginNewTransaction("Macro: Heal Splits");
  DBG("MacroToolbar: Heal Splits triggered for " << clipIds.size() << " clips");
  // Implementation placeholder logic
}

void MacroToolbar::instantFreeze() {
  if (!getSelectedTrackId)
    return;
  auto trackId = getSelectedTrackId();
  if (trackId.isEmpty())
    return;

  // Find track index
  int trackIndex = -1;
  int index = 0;
  auto tracks =
      projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
  for (auto track : tracks) {
    if (track[ProjectState::PROP_ID].toString() == trackId) {
      trackIndex = index;
      break;
    }
    index++;
  }

  if (trackIndex >= 0) {
    DBG("MacroToolbar: Freezing track " << trackIndex);
    engine_.freezeTrack(trackIndex);
  }
}

void MacroToolbar::colorByTrack() {
  if (!getSelectedClipIds)
    return;
  auto clipIds = getSelectedClipIds();

  projectState_.getUndoManager().beginNewTransaction("Macro: Color by Track");

  for (const auto &id : clipIds) {
    auto [trackV, clipV] = projectState_.findClip(id);
    if (clipV.isValid()) {
      clipV.removeProperty(ProjectState::PROP_MANUALLY_COLORED,
                           &projectState_.getUndoManager());
      clipV.removeProperty("color", &projectState_.getUndoManager());
    }
  }
}

} // namespace zenith
