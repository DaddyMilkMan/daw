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

  ==============================================================================

    PlatformONNXUtils.h
    Created: 2025-12-23

    Platform-agnostic ONNX Runtime utilities interface

  ==============================================================================
*/



#pragma once

#ifdef ZENITH_USE_ONNX_RUNTIME
#include <juce_core/juce_core.h>
#include <memory>
#include <onnxruntime_cxx_api.h>

namespace zenith {

class PlatformONNXUtils {
public:
  /**
   * @brief Create ONNX Runtime session with platform-specific path handling
   * @param env ONNX Runtime environment
   * @param options Session options
   * @param modelPath Path to the ONNX model file
   * @return Unique pointer to created session
   * @note Windows uses wide-string paths, Unix uses standard strings
   */
  static std::unique_ptr<Ort::Session>
  createSession(Ort::Env &env, Ort::SessionOptions &options,
                const juce::File &modelPath);
};

} // namespace zenith
#endif
