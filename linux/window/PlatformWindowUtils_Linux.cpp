/*
  ==============================================================================

    PlatformWindowUtils_Linux.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "ui/framework/PlatformWindowUtils.h"
#include <gpu/ganesh/gl/GrGLAssembleInterface.h>
#include "engine/ZenithLogger.h"

#ifdef __linux__
namespace zenith {

sk_sp<const GrGLInterface> PlatformWindowUtils::createNativeGLInterface(juce::OpenGLContext& context) {
    auto interface = GrGLMakeNativeInterface();
    
    if (interface == nullptr) {
        ZENITH_LOG_WARNING("PlatformWindowUtils: Native GL interface creation failed, trying fallback...");
        // Fallback for Linux GL drivers
        interface = GrGLMakeAssembledInterface(
            &context, [](void* ctx, const char* name) -> GrGLFuncPtr {
                juce::ignoreUnused(ctx);
                return (GrGLFuncPtr) juce::OpenGLHelpers::getExtensionFunction(name);
            });
            
        if (interface) {
             ZENITH_LOG_INFO("PlatformWindowUtils: Assembled GL interface created successfully.");
        } else {
             ZENITH_LOG_ERROR("PlatformWindowUtils: Fallback GL interface creation FAILED.");
        }
    } else {
        ZENITH_LOG_INFO("PlatformWindowUtils: Native GL interface created successfully.");
    }
    
    return interface;
}

} // namespace zenith
#endif
