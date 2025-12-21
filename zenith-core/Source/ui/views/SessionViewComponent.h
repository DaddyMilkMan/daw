#pragma once
#include "../skia/SkiaCanvasComponent.h"
#include "../skia/SkiaTheme.h"
#include <array>
#include <utility>

namespace zenith {

/**
 * @class SessionViewComponent
 * @brief Professional clip launcher grid with Skia GPU-accelerated rendering
 *
 * Features:
 * - 8x8 clip launcher grid (Ableton-style)
 * - Animated playback progress indicators
 * - Hover/selection states with smooth transitions
 * - Track headers with level meters
 * - Scene launch buttons
 */
class SessionViewComponent : public SkiaCanvasComponent, public juce::Timer {
public:
  SessionViewComponent();
  ~SessionViewComponent() override;

  // Interaction
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

  // The Skia Render Loop
#if ZENITH_ENABLE_SKIA
  void paintSkia(SkCanvas &canvas, const juce::Rectangle<int> &bounds) override;
#endif

  // Timer for animation
  void timerCallback() override;

private:
  //============================================================================
  // Data Model
  //============================================================================

  struct ClipSlot {
    bool hasClip = false;
    bool isPlaying = false;
    bool isRecording = false;
    bool isQueued = false;
    juce::String name;
    float playProgress = 0.0f;
    juce::uint32 color = 0xFF4DABF7; // Replaced SkColor with uint32
  };

  //============================================================================
  // Drawing Methods
  //============================================================================

  void initializeDemoData();

#if ZENITH_ENABLE_SKIA
  void drawBackground(SkCanvas &canvas, const juce::Rectangle<int> &bounds);
  void drawGridPanel(SkCanvas &canvas, const juce::Rectangle<int> &bounds,
                     float clipWidth, float clipHeight);
  void drawSceneHeaders(SkCanvas &canvas, const juce::Rectangle<int> &bounds,
                        float clipHeight);
  void drawTrackHeaders(SkCanvas &canvas, const juce::Rectangle<int> &bounds,
                        float clipWidth);
  void drawClipGrid(SkCanvas &canvas, const juce::Rectangle<int> &bounds,
                    float clipWidth, float clipHeight);
  void drawClipSlot(SkCanvas &canvas, const SkRect &rect, const ClipSlot &slot,
                    bool isHovered);
  void drawEmptySlot(SkCanvas &canvas, const SkRect &rect, bool isHovered);
  void drawWaveform(SkCanvas &canvas, const SkRect &rect, const ClipSlot &slot);
  void drawMasterSection(SkCanvas &canvas, const juce::Rectangle<int> &bounds);
  void drawGridLines(SkCanvas &canvas, const juce::Rectangle<int> &bounds);
#endif

  //============================================================================
  // Hit Testing
  //============================================================================

  std::pair<int, int> getSlotAtPosition(juce::Point<float> pos) const;

  //============================================================================
  // Constants
  //============================================================================

  static constexpr int NUM_TRACKS = 8;
  static constexpr int NUM_SCENES = 8;

  //============================================================================
  // State
  //============================================================================

  std::array<std::array<ClipSlot, NUM_SCENES>, NUM_TRACKS> grid;
  std::array<juce::String, NUM_TRACKS> trackNames;
  std::array<float, NUM_TRACKS> trackMeterValues;

  // Layout Metrics
  float trackHeaderHeight = 56.0f;
  float sceneHeaderWidth = 72.0f;
  float clipGap = 8.0f;

  // Hover state
  juce::Point<int> hoveredSlot = {-1, -1};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionViewComponent)
};

} // namespace zenith
