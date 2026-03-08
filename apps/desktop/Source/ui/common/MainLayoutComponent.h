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
class WorkspaceCenterComponent;
class SampleEditorComponent;
class MidiEditorContainer;
class RightSidePanel;
class BrowserPanel;
class CommandAPI;

class MainLayoutComponent : public SkiaComponent {
public:
  MainLayoutComponent(Engine &engine, ProjectState &state, CommandAPI &api);
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

  ResizablePanelContainer* getRootContainer() { return panelContainer_.get(); }

private:
  Engine &engine_;
  ProjectState &projectState_;
  CommandAPI &api_;

  std::unique_ptr<BrowserModel> browserModel_;
  std::unique_ptr<ResizablePanelContainer> panelContainer_;
  WorkspaceCenterComponent *workspaceShell_ = nullptr;
  BrowserPanel *browserPanel_ = nullptr;
  RightSidePanel *rightSidePanel_ = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainLayoutComponent)
};

} // namespace zenith
