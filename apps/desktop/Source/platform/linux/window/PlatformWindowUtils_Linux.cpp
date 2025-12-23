/*
  ==============================================================================

    PlatformWindowUtils_Linux.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "../../../ui/framework/PlatformWindowUtils.h"
#include <include/gpu/gl/GrGLAssembleInterface.h>

#ifdef __linux__
namespace zenith {

sk_sp<const GrGLInterface> PlatformWindowUtils::createNativeGLInterface(juce::OpenGLContext& context) {
    auto interface = GrGLMakeNativeInterface();
    
    if (interface == nullptr) {
        // Fallback for Linux GL drivers
        interface = GrGLMakeAssembledInterface(
            &context, [](void* ctx, const char* name) -> GrGLFuncPtr {
                return (GrGLFuncPtr) static_cast<juce::OpenGLContext*>(ctx)->getOpenGLProcAddress(name);
            });
    }
    
    return interface;
}

} // namespace zenith
#endif
