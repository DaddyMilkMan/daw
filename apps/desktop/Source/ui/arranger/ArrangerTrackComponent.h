/**
 * @file ArrangerTrackComponent.h
 * @brief Component for managing individual tracks (Audio, MIDI, Group, Section)
 */

#pragma once

#include "SkiaComponent.h"
#include "ProjectState.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

struct ArrangementSection {
  juce::String name;
  double startBeats;
  double lengthBeats;
  juce::Colour color;
};

class ArrangerTrackComponent : public SkiaComponent {
public:
  enum class TrackType { Audio, Midi, Group, Master, Section };

  ArrangerTrackComponent(ProjectState &ps, TrackType type = TrackType::Audio);
  ~ArrangerTrackComponent() override;

  void drawSkia(SkCanvas *canvas) override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;

  // Set the view parameters for rendering
  void setViewContext(double pixelsPerBeat, double viewStartBeats);
  void setVisibleRange(double startBeats, double endBeats);

  // Track Data Setters
  void setTrackId(const juce::String& id) { trackId_ = id; }
  void setTrackName(const juce::String& name) { trackName_ = name; repaint(); }
  void setTrackIndex(int index) { trackIndex_ = index; repaint(); }
  
  void setMuted(bool m);
  void setSoloed(bool s);
  void setRecordArmed(bool r);
  
  void setAccentColor(juce::Colour c) { accentColor_ = c; repaint(); }

  // Command to re-order sections (The "Magic" of this feature)
  void moveSection(int index, double newStartBeats);

  // State Access
  const ArrangementSection *getHoveredSection() const;
  const ArrangementSection *getDraggingSection() const;

private:
  ProjectState &projectState;
  TrackType type_;
  
  // Track State
  juce::String trackId_;
  juce::String trackName_ = "Track";
  int trackIndex_ = 0;
  juce::Colour accentColor_ = juce::Colours::cyan;
  
  bool isMuted_ = false;
  bool isSoloed_ = false;
  bool isRecordArmed_ = false;

  // Interaction State
  bool isHovered_ = false;
  int hoveredButtonIndex_ = -1; // 0=Mute, 1=Solo, 2=Rec
  
  // Section Specific State
  std::vector<ArrangementSection> sections_; // Cache

  // View State
  double pixelsPerBeat_ = 50.0;
  double viewStartBeats_ = 0.0;

  // Section Interaction
  int draggingSectionIndex_ = -1;
  double dragStartBeats_ = 0.0;
  double initialSectionStart_ = 0.0;

  void rebuildSections(); // Pull from ProjectState
  
  // Helpers
  void drawTrackHeader(SkCanvas* canvas, const SkRect& bounds);
  void drawTrackBackground(SkCanvas* canvas, const SkRect& bounds);
  void drawControls(SkCanvas* canvas, float x, float y);
  void drawSections(SkCanvas* canvas, const SkRect& bounds);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerTrackComponent)
};

} // namespace zenith
