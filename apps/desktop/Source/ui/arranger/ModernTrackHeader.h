/**
 * @file ModernTrackHeader.h
 * @brief Professional track header with modern design system
 * @author Fixed by Claude - December 2025
 */

#include "../framework/SkiaComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zenith {

/**
 * @class ModernTrackHeader
 * @brief Track header component with proper visual hierarchy and interactions
 */
class ModernTrackHeader : public SkiaComponent {
public:
  ModernTrackHeader(int trackIndex = 0);
  ~ModernTrackHeader() override = default;

  //==========================================================================
  // SkiaComponent Overrides
  //==========================================================================
  void drawSkia(SkCanvas *canvas) override;
  void paint(juce::Graphics& g) override { SkiaComponent::paint(g); }
  void resized() override;
  void mouseEnter(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

  //==========================================================================
  // Public Interface
  //==========================================================================
  void setTrackName(const juce::String &name);
  juce::String getTrackName() const;

  void setTrackColor(const juce::Colour &color);
  juce::Colour getTrackColor() const { return trackColor_; }

  void setMuted(bool shouldBeMuted);
  bool isMuted() const { return isMuted_; }

  void setSoloed(bool shouldBeSoloed);
  bool isSoloed() const { return isSoloed_; }

  void setArmed(bool shouldBeArmed);
  bool isArmed() const { return isArmed_; }

  void setSelected(bool shouldBeSelected);
  bool isSelected() const { return isSelected_; }

  //==========================================================================
  // Callbacks
  //==========================================================================
  std::function<void()> onNameChanged;
  std::function<void(bool)> onMuteToggled;
  std::function<void(bool)> onSoloToggled;
  std::function<void(bool)> onArmToggled;
  std::function<void()> onHeaderClicked;

private:
  //==========================================================================
  // UI Components
  //==========================================================================
  class TrackButton : public juce::Button {
  public:
    TrackButton(const juce::String &buttonText);
    void paintButton(juce::Graphics &g, bool isHighlighted,
                     bool isDown) override;

    enum class Type { Mute, Solo, Arm };
    void setButtonType(Type type);

  private:
    Type type_ = Type::Mute;
  };

  juce::Label nameLabel_;
  TrackButton muteButton_{"M"};
  TrackButton soloButton_{"S"};
  TrackButton armButton_{"R"};

  //==========================================================================
  // State
  //==========================================================================
  juce::Colour trackColor_;
  int trackIndex_;
  bool isMuted_ = false;
  bool isSoloed_ = false;
  bool isArmed_ = false;
  bool isSelected_ = false;
  bool isHovered_ = false;

  //==========================================================================
  // Animation
  //==========================================================================
  float hoverProgress_ = 0.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernTrackHeader)
};

} // namespace zenith
