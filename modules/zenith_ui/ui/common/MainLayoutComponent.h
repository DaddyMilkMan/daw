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