/**
 * @file MenuBar.h
 * @brief Custom Skia-based Menu Bar for Zenith DAW
 */

#pragma once

#include "../ui/skia/SkiaComponent.h"
#include "../ui/skia/ZenithDesignSystem.h"
#include <JuceHeader.h>

namespace zenith {

class ZenithMenuBar : public SkiaComponent {
public:
  ZenithMenuBar();
  ~ZenithMenuBar() override = default;

  // Configuration Callbacks
  std::function<void()> onSaveProject;
  std::function<void()> onSaveProjectAs;
  std::function<void()> onImportAudio;
  std::function<void()> onUndo;
  std::function<void()> onRedo;

  void paint(juce::Graphics &g) override;
  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseMove(const juce::MouseEvent &e) override;
  void mouseExit(const juce::MouseEvent &e) override;

private:
  struct MenuItem {
    juce::String name;
    juce::Rectangle<int> bounds;
    bool isHovered = false;
  };

  std::vector<MenuItem> items;
  int hoveredItemIndex = -1;

  void showFileMenu();
  void showEditMenu();
  void showViewMenu();
  void showHelpMenu();

  void updateLayout();

  juce::TextButton collabButton;
  std::unique_ptr<juce::CallOutBox> collabCallout;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZenithMenuBar)
};

} // namespace zenith
