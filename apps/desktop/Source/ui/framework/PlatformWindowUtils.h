/*
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

