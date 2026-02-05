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

#pragma once
#include <juce_core/juce_core.h>

namespace zenith {

// Forward declarations
class Engine;
class ProjectState;
class CommandAPI;


class TransportCommands {
public:
  TransportCommands(Engine &engine, ProjectState &projectState,
                    CommandAPI &api);

  juce::var play(const juce::var &params);
  juce::var stop(const juce::var &params);
  juce::var record(const juce::var &params);
  juce::var rewind(const juce::var &params);
  juce::var setLoop(const juce::var &params);
  juce::var setTempo(const juce::var &params);
  juce::var setTimeSignature(const juce::var &params);
  juce::var addTempoChange(const juce::var &params);
  juce::var getTempoMap(const juce::var &params);

private:
  Engine &engine;
  ProjectState &projectState;
  CommandAPI &api;
};

} // namespace zenith
