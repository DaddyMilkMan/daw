/*
  ==============================================================================

    PlatformWindowUtils.h
    Created: 2025-12-22

    Interface for platform-specific window and GL utility functions.

  ==============================================================================
*/

#pragma once

#include <include/gpu/ganesh/gl/GrGLInterface.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace zenith {

class PlatformWindowUtils {
public:
  /**
   * Creates a Skia GL interface for the current native context.
   */
  static sk_sp<const GrGLInterface>
  createNativeGLInterface(juce::OpenGLContext &context);
};

} // namespace zenith
