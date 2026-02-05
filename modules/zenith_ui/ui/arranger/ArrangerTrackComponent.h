/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

 * @file ArrangerTrackComponent.h
 * @brief Component for managing individual tracks (Audio, MIDI, Group, Section)
 */


#include "TakeFolderComponent.h"
#include <vector>
#include "../controls/ZenithSlider.h"
#include "../controls/ZenithKnob.h"
#include "../controls/SkiaButton.h" 
#include "../design-system/ZenithTheme.h"

namespace zenith {

struct ArrangementSection {
  juce::String id;
  juce::String name;
  double startBeats;
  double lengthBeats;
  juce::Colour color;
};

class ArrangerGridUtils;

class ArrangerTrackComponent : public SkiaComponent {
public:
  enum class TrackType { Audio, Midi, Group, Master, Section };

  ArrangerTrackComponent(ProjectState &ps, ArrangerGridUtils &gridUtils,
                         TrackType type = TrackType::Audio);
  ~ArrangerTrackComponent() override;

  void drawSkia(SkCanvas *canvas) override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void mouseDoubleClick(const juce::MouseEvent &e) override;

  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;

  // Set the view parameters for rendering
  void setViewContext(double pixelsPerBeat, double viewStartBeats);
  void setVisibleRange(double startBeats, double endBeats);
  
  void updateTakeFolders(); // Rebuilds take folder components

  // Track Data Setters
  void setTrackId(const juce::String &id) { trackId_ = id; }
  void setTrackName(const juce::String &name) {
    trackName_ = name;
    repaint();
  }
  void setTrackIndex(int index) {
    trackIndex_ = index;
    repaint();
  }

  void setMuted(bool m);
  void setSoloed(bool s);
  void setRecordArmed(bool r);
  void setInputMonitor(bool i);

  void setAccentColor(juce::Colour c) {
    accentColor_ = c;
    repaint();
  }

  // Command to re-order sections (The "Magic" of this feature)
  void moveSection(int index, double newStartBeats);

  // State Access
  const ArrangementSection *getHoveredSection() const;
  const ArrangementSection *getDraggingSection() const;

  std::function<void(const juce::String&)> onFreeze;
  std::function<void(const juce::String&)> onUnfreeze;
  std::function<void(const juce::String&)> onSeparateStems;
  std::function<void(const juce::String&, const juce::String&)> onAutomationLaneRequested;
  std::function<void(const juce::String&)> onHideAllAutomation;

private:
  ProjectState &projectState;
  ArrangerGridUtils &gridUtils_;
  TrackType type_;

  // Track State
  juce::String trackId_;
  juce::String trackName_ = "Track";
  int trackIndex_ = 0;
  juce::Colour accentColor_ = ZenithTheme::Colors::accent_primary;

  bool isMuted_ = false;
  bool isSoloed_ = false;
  bool isRecordArmed_ = false;
  bool isInputMonitoring_ = false;

  // Interaction State
  bool isHovered_ = false;
  float hoverIntensity_ = 0.0f; // 0.0 to 1.0 for animation

  int hoveredButtonIndex_ = -1; // 0=Mute, 1=Solo, 2=Rec
  
  // Controls
  std::unique_ptr<ZenithSlider> volSlider;
  std::unique_ptr<ZenithKnob> panKnob;

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
  void drawTrackHeader(SkCanvas *canvas, const SkRect &bounds);
  void drawTrackBackground(SkCanvas *canvas, const SkRect &bounds);
  void drawControls(SkCanvas *canvas, float x, float y);
  void drawSections(SkCanvas *canvas, const SkRect &bounds);
  
  // Take Folders
  std::vector<std::unique_ptr<TakeFolderComponent>> takeFolders_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangerTrackComponent)
};

} // namespace zenith
