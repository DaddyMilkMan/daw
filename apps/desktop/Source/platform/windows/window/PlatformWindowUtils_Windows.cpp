/*
  ==============================================================================

    PlatformWindowUtils_Windows.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "../../../ui/framework/PlatformWindowUtils.h"

#ifdef _WIN32
#include <include/gpu/gl/GrGLAssembleInterface.h>

namespace zenith {

sk_sp<const GrGLInterface> PlatformWindowUtils::createNativeGLInterface(juce::OpenGLContext& context) {
    return GrGLMakeNativeInterface();
}

} // namespace zenith
#endif
