/*
  ==============================================================================

    MainLayoutComponent.h
    Created: 2025-11-28
    Author:  Dr. Aris Vokos + Leo Rossi + Isabella Moretti

  ==============================================================================
*/

#pragma once

#include "../framework/SkiaComponent.h"
#include <JuceHeader.h>

namespace zenith {

class Engine;
class Engine;
class ProjectState;
class CommandAPI;
class BrowserModel;

class ResizablePanelContainer;
class RemoteCursorOverlay;
class SampleEditorComponent;
class ViewSwitcher;
class MidiEditorContainer;

class MainLayoutComponent : public SkiaComponent {
public:
  MainLayoutComponent(Engine &engine, ProjectState &state, CommandAPI &api);
  ~MainLayoutComponent() override;


  void resized() override;
  void drawSkia(SkCanvas *canvas) override;

  void toggleView();
  void toggleBrowser();
  void toggleSampleEditor();

  bool isSessionView() const;
  bool isBrowserVisible() const;
  bool isSampleEditorVisible() const;
  bool isMidiEditorVisible() const;

  SampleEditorComponent *getSampleEditor();
  MidiEditorContainer *getMidiEditor();
  
  ResizablePanelContainer* getRootContainer() { return panelContainer_.get(); }

private:
  Engine &engine_;
  ProjectState &projectState_;
  CommandAPI &api_;


  std::unique_ptr<BrowserModel> browserModel_;
  std::unique_ptr<ResizablePanelContainer> panelContainer_;

  // Raw pointers to managed components (owned by containers)
  ResizablePanelContainer *centerContainer_ = nullptr;
  ViewSwitcher *viewSwitcher_ = nullptr;
  ViewSwitcher *editorSwitcher_ = nullptr;
  SampleEditorComponent *sampleEditor_ = nullptr;
  MidiEditorContainer *midiEditor_ = nullptr;

  std::unique_ptr<RemoteCursorOverlay> cursorOverlay_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainLayoutComponent)
};

} // namespace zenith