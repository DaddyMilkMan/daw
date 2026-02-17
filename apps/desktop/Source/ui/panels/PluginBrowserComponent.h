/**
 * @file PluginBrowserComponent.h
 * @brief Pure Skia plugin browser for selecting/loading audio plugins
 */

#pragma once

#include "../controls/SkiaAlertWindow.h"
#include "../controls/SkiaListBox.h"
#include "../controls/SkiaTextEditor.h"
#include "../controls/ZenithButton.h"
#include "../controls/SkiaLabel.h"
#include "../framework/SkiaComponent.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

namespace zenith {

class Engine;
class Track;

class PluginBrowserComponent : public SkiaComponent {
public:
  explicit PluginBrowserComponent(Engine &engine);
  ~PluginBrowserComponent() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  void setTargetTrack(Track *track);
  int getSelectedPluginIndex() const;
  bool loadSelectedPlugin();
  void refresh();

private:
  class PluginListModel : public SkiaListBox::Model {
  public:
    explicit PluginListModel(PluginBrowserComponent &owner) : owner_(owner) {}
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, SkCanvas &canvas, int width, int height,
                          bool rowIsSelected) override;
    void listBoxItemDoubleClicked(int rowNumber,
                                  const juce::MouseEvent &e) override;

  private:
    PluginBrowserComponent &owner_;
  };

  void updateFilteredList();
  void loadPluginAtIndex(int index);
  void setStatus(const juce::String &text);
  void cycleTargetTrack(int direction);
  void showWarning(const juce::String &title, const juce::String &message);
  void dismissAlert();

  Engine &engine_;
  Track *targetTrack_ = nullptr;

  std::unique_ptr<SkiaLabel> titleLabel_;
  std::unique_ptr<SkiaLabel> searchLabel_;
  std::unique_ptr<SkiaTextEditor> searchBox_;
  std::unique_ptr<SkiaListBox> pluginList_;
  std::unique_ptr<SkiaLabel> trackLabel_;
  std::unique_ptr<ZenithButton> trackPrevButton_;
  std::unique_ptr<ZenithButton> trackNextButton_;
  std::unique_ptr<ZenithButton> loadButton_;
  std::unique_ptr<SkiaLabel> statusLabel_;

  std::unique_ptr<SkiaAlertWindow> activeAlert_;
  PluginListModel listModel_;

  juce::Array<juce::PluginDescription> filteredPlugins_;
  juce::String currentFilter_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowserComponent)
};

class PluginBrowserWindow : public juce::DocumentWindow {
public:
  explicit PluginBrowserWindow(Engine &engine);
  ~PluginBrowserWindow() override;

  void closeButtonPressed() override;
  PluginBrowserComponent *getBrowserComponent();

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowserWindow)
};

} // namespace zenith
