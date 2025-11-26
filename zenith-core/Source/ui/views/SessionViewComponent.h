#pragma once
#include "../skia/SkiaCanvasComponent.h"
#include "../skia/SkiaTheme.h"
#include <array>

namespace zenith {

class SessionViewComponent : public SkiaCanvasComponent, public juce::Timer {
public:
  SessionViewComponent();
  ~SessionViewComponent() override;

  // Interaction
  void mouseDown(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;

  // The Skia Render Loop
  void paintSkia(SkCanvas &canvas, const juce::Rectangle<int> &bounds) override;

  // Timer for animation
  void timerCallback() override;

private:
  // Data Model (Placeholder for your real Engine data)
  struct ClipSlot {
    bool hasClip = false;
    bool isPlaying = false;
    bool isRecording = false;
    juce::String name;
    float playProgress = 0.0f; // 0.0 to 1.0
    SkColor color;
  };

  // Drawing methods
  void drawSceneHeaders(SkCanvas &canvas, const juce::Rectangle<int> &bounds, float clipHeight);
  void drawTrackHeaders(SkCanvas &canvas, const juce::Rectangle<int> &bounds, float clipWidth);
  void drawClipGrid(SkCanvas &canvas, const juce::Rectangle<int> &bounds, float clipWidth, float clipHeight);
  void drawClipSlot(SkCanvas &canvas, const SkRect &rect, const ClipSlot &slot, bool isHovered);
  void drawEmptySlot(SkCanvas &canvas, const SkRect &rect, bool isHovered);
  void drawMasterSection(SkCanvas &canvas, const juce::Rectangle<int> &bounds);

  // 8 Tracks x 8 Scenes
  static constexpr int NUM_TRACKS = 8;
  static constexpr int NUM_SCENES = 8;
  std::array<std::array<ClipSlot, NUM_SCENES>, NUM_TRACKS> grid;

  // Layout Metrics
  float trackHeaderHeight = 40.0f;
  float sceneHeaderWidth = 60.0f;
  float clipGap = 6.0f;

  // Hover state
  juce::Point<int> hoveredSlot = {-1, -1}; // x=track, y=scene

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionViewComponent)
};
} // namespace zenith
