/*
  ==============================================================================

    PlatformWindowUtils_Linux.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "../../../ui/framework/PlatformWindowUtils.h"
#include <include/gpu/ganesh/gl/GrGLInterface.h>
#include <include/core/SkRefCnt.h>
#include <include/gpu/ganesh/gl/GrGLInterface.h>
#include <include/gpu/ganesh/gl/GrGLAssembleInterface.h>
#include <juce_opengl/juce_opengl.h>
#include "../../../engine/ZenithLogger.h"

#ifdef __linux__
namespace zenith {

sk_sp<const GrGLInterface> PlatformWindowUtils::createNativeGLInterface(juce::OpenGLContext& context) {
    auto interface = GrGLMakeNativeInterface();
    
    if (interface == nullptr) {
        ZENITH_LOG_INFO("PlatformWindowUtils: GrGLMakeNativeInterface failed, using assembled fallback");
        interface = GrGLMakeAssembledInterface(
            &context, [](void* /*ctx*/, const char* name) -> GrGLFuncPtr {
                return (GrGLFuncPtr) juce::OpenGLHelpers::getExtensionFunction(name);
            });
    } else {
        ZENITH_LOG_INFO("PlatformWindowUtils: GrGLMakeNativeInterface succeeded");
    }
    
    return interface;
}

} // namespace zenith
#endif
