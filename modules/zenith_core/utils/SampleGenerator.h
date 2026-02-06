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
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <functional>

namespace zenith {

class SampleGenerator {
public:
  /**
   * Checks for missing sample files referenced by example maps

   * and generates simple placeholders if they are missing.
   *
   * @param threadPool Optional thread pool to run generation asynchronously.
   *                   If nullptr, runs synchronously.
   * @param onComplete Optional callback when generation finishes.
   */
  static void generateMissingSamples(juce::ThreadPool* threadPool = nullptr, std::function<void()> onComplete = nullptr);
};

} // namespace zenith
