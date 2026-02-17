#pragma once
#include "../instruments/ZenithPresetManager.h"
#include "../framework/SkiaComponent.h"
#include "../controls/SkiaListBox.h"
#include "../controls/SkiaAlertWindow.h"
#include "../controls/ZenithButton.h"
#include <functional>

class PresetBrowserComponent : public zenith::SkiaComponent,
                               public zenith::SkiaListBox::Model {
public:
  PresetBrowserComponent();
  ~PresetBrowserComponent() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  // SkiaListBox::Model overrides
  int getNumRows() override;
  void paintListBoxItem(int rowNumber, SkCanvas &canvas, int width, int height,
                        bool rowIsSelected) override;
  // void selectedRowsChanged(int lastRowSelected) override; // SkiaListBox
  // doesn't have this virtual, uses callbacks
  void listBoxItemClicked(int row, const juce::MouseEvent &e) override;

  void refreshPresets();
  void setInstrumentId(const juce::String &instrumentId);

  using LoadCallback = std::function<void(const zenith::Preset &)>;
  using CaptureCallback = std::function<zenith::Preset()>;

  void setLoadPresetCallback(LoadCallback cb) { loadCallback = cb; }
  void setCaptureStateCallback(CaptureCallback cb) { captureCallback = cb; }

private:
  LoadCallback loadCallback;
  CaptureCallback captureCallback;
  zenith::SkiaListBox presetList;
  std::vector<zenith::PresetMetadata> presets;
  juce::String currentInstrumentId = "ZenithPolySynth"; // Default

  zenith::ZenithButton loadButton{"Load"};
  zenith::ZenithButton saveButton{"Save"};
  zenith::ZenithButton deleteButton{"Delete"};
  zenith::ZenithButton refreshButton{"Refresh"};
  std::unique_ptr<zenith::SkiaAlertWindow> savePresetDialog_;

  void loadSelectedPreset();
  void saveCurrentPreset();
  void deleteSelectedPreset();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserComponent)
};
