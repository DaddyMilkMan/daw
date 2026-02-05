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
#include "../engine/Clip.h"
#include "../engine/Engine.h"
#include "../engine/ProjectState.h"
#include "../engine/Track.h"
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace zenith {


inline juce::var createSuccessResponse(const juce::var &result = juce::var()) {
  juce::DynamicObject::Ptr response = new juce::DynamicObject();
  response->setProperty("success", true);
  if (!result.isVoid())
    response->setProperty("result", result);
  return juce::var(response.get());
}

inline juce::var createErrorResponse(const juce::String &errorMessage) {
  juce::DynamicObject::Ptr response = new juce::DynamicObject();
  response->setProperty("success", false);
  response->setProperty("error", errorMessage);
  return juce::var(response.get());
}

inline Track *findTrackById(Engine &engine, const juce::String &trackId) {
  for (auto &track : engine.tracks()) {
    if (track && track->getTrackId() == trackId)
      return track.get();
  }
  return nullptr;
}

inline Clip *findClipById(ProjectState &state, Track *track,
                          const juce::String &clipId) {
  if (track == nullptr)
    return nullptr;

  juce::String trackId = track->getTrackId();
  auto trackTree = state.getTrack(trackId);
  if (!trackTree.isValid())
    return nullptr;

  auto clipsList = trackTree.getChildWithName(ProjectState::ID_CLIPS);
  int index = 0;
  for (const auto &child : clipsList) {
    if (child.getProperty(ProjectState::PROP_ID).toString() == clipId) {
      return track->getClip(index);
    }
    index++;
  }
  return nullptr;
}

} // namespace zenith
