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

    PlatformWindowUtils.h
    Created: 2025-12-22

    Interface for platform-specific window and GL utility functions.

  ==============================================================================
*/



#pragma once

#include <gpu/ganesh/gl/GrGLInterface.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>

namespace zenith {

class PlatformWindowUtils {
public:
  /**
   * Creates a Skia GL interface for the current native context.
   */
  static sk_sp<const GrGLInterface>
  createNativeGLInterface(juce::OpenGLContext &context);
  
  /**
   * Removes window decorations (title bar, borders) from a JUCE window.
   * On Linux, this uses X11 Motif WM hints to request an undecorated window.
   * On other platforms, this is typically handled by JUCE itself.
   * 
   * @param window The component window to make borderless
   */
  static void removeWindowDecorations(juce::Component* window);
  
  /**
   * Sets the window to true fullscreen mode (covers entire screen including taskbars).
   * 
   * @param window The component window to fullscreen
   * @param enable True to enable fullscreen, false to exit
   */
  static void setTrueFullscreen(juce::Component* window, bool enable);
};

} // namespace zenith

