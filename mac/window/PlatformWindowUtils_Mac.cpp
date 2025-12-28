/*
  ==============================================================================

    PlatformWindowUtils_Mac.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "ui/framework/PlatformWindowUtils.h"

#ifdef __APPLE__
namespace zenith {

sk_sp<const GrGLInterface> PlatformWindowUtils::createNativeGLInterface(juce::OpenGLContext& context) {
    return GrGLMakeNativeInterface();
}

} // namespace zenith
#endif
