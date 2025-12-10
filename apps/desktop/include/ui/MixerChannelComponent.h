/**
 * @file MixerChannelComponent.h
 * @brief Mixer channel strip UI component
 */

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../../Source/ui/skia/SkiaButton.h"
#include "../../Source/ui/skia/SkiaComponent.h"
#include "../../Source/ui/skia/SkiaKnob.h"
#include "../../Source/ui/skia/SkiaSlider.h"
#include "../../Source/ui/skia/ZenithDesignSystem.h"

namespace zenith {
class Track;

class MixerChannelComponent : public zenith::SkiaComponent,
                              public juce::ChangeListener {
public:
  explicit MixerChannelComponent(zenith::Track *track);
  ~MixerChannelComponent() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  // ChangeListener
  void changeListenerCallback(juce::ChangeBroadcaster *source) override;

  zenith::Track *getTrack() const { return track_; }
  void updateFromTrack();

private:
  void timerCallback() override;

  void onFaderChanged();
  void onPanChanged();
  void onMuteClicked();
  void onSoloClicked();

  zenith::Track *track_;

  // juce::Label nameLabel_; // Removed in favor of Skia drawing

  zenith::SkiaSlider faderSlider_;
  zenith::SkiaKnob panKnob_;
  zenith::SkiaButton muteButton_;
  zenith::SkiaButton soloButton_;

  class LevelMeter : public zenith::SkiaComponent {
  public:
    LevelMeter();
    ~LevelMeter() override;
    void drawSkia(SkCanvas *canvas) override;
    void setLevel(float level);
    void timerCallback() override;

  private:
    std::atomic<float> targetLevel_{0.0f};
    float currentLevel_{0.0f};
    float peakLevel_{0.0f};
    int peakHoldCounter_{0};
  };

  LevelMeter meter_;
  bool updatingControls_ = false;

  // Cached Layout (Critique #2: Allocations in Paint Loop)
  std::string cachedName_;
  float cachedNameWidth_ = 0.0f;
  float cachedNameX_ = 0.0f;
  float cachedNameY_ = 0.0f;
  void updateNameCache(); // Helper

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannelComponent)
};

} // namespace zenith
