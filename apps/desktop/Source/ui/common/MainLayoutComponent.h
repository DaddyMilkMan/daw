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
class ProjectState;
class BrowserModel;
class ResizablePanelContainer;
class RemoteCursorOverlay;
class SampleEditorComponent;
class ViewSwitcher;
class MidiEditorContainer;
class RightSidePanel;
class CommandAPI;

class MainLayoutComponent : public SkiaComponent {
public:
  MainLayoutComponent(Engine &engine, CommandAPI &api, ProjectState &state);
  ~MainLayoutComponent() override;

  void resized() override;
  void drawSkia(SkCanvas *canvas) override;

  void toggleView();
  void toggleBrowser();
  void toggleSampleEditor();
  void toggleWingman();

  bool isSessionView() const;
  bool isBrowserVisible() const;
  bool isSampleEditorVisible() const;
  bool isMidiEditorVisible() const;

  SampleEditorComponent *getSampleEditor();
  MidiEditorContainer *getMidiEditor();

private:
  Engine &engine_;
  ProjectState &projectState_;

  std::unique_ptr<BrowserModel> browserModel_;
  std::unique_ptr<ResizablePanelContainer> panelContainer_;

  // NUKED - All center content components removed
  // Raw pointers to managed components (owned by containers)
  RightSidePanel *rightSidePanel_ = nullptr;

  // NUKED - RemoteCursorOverlay removed since no arranger exists

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainLayoutComponent)
};

} // namespace zenith