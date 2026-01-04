/*
  ==============================================================================

    PlatformWindowUtils.cpp
    Created: 2025-12-23
    Author:  Zenith DAW Team

    Implementation of platform-specific window and GL utility functions.

  ==============================================================================
*/

#include "PlatformWindowUtils.h"
#include "../../engine/ZenithLogger.h"

#include <include/gpu/ganesh/gl/GrGLAssembleInterface.h>
#include <include/gpu/ganesh/gl/GrGLDirectContext.h>
#include <include/gpu/ganesh/gl/GrGLInterface.h>

// Linux/Unix specific includes for dlsym
#if JUCE_LINUX || (JUCE_MAC && !defined(__OBJC__))
#include <dlfcn.h>

namespace zenith {

#ifndef __linux__
sk_sp<const GrGLInterface>
PlatformWindowUtils::createNativeGLInterface(juce::OpenGLContext &context) {
  juce::ignoreUnused(context); // Might be used in future or other platforms
  ZENITH_LOG_INFO("PlatformWindowUtils: Attempting to create GL interface...");


  // 1. Native Interface Attempt
  // GrGLMakeNativeInterface is a global function in modern Skia
  auto interface = GrGLMakeNativeInterface();

  if (interface != nullptr) {
    ZENITH_LOG_INFO(
        "PlatformWindowUtils: Success with GrGLMakeNativeInterface()");
    return interface;
  }

  ZENITH_LOG_WARNING("PlatformWindowUtils: GrGLMakeNativeInterface() returned "
                     "null. Attempting fallback assembly...");

  // 2. Assembled Interface (Fallback)
  // On Linux, we use dlsym to find GL symbols if the native interface fails.
  ZENITH_LOG_INFO(
      "PlatformWindowUtils: Attempting GrGLMakeAssembledInterface...");

  auto assembledInterface = GrGLMakeAssembledInterface(
      nullptr, [](void *ctx, const char *name) -> GrGLFuncPtr {
        juce::ignoreUnused(ctx);
#if JUCE_LINUX
        // Use dlsym with RTLD_DEFAULT to search global scope
        return (GrGLFuncPtr)dlsym(RTLD_DEFAULT, name);
#elif JUCE_WINDOWS
             // On Windows, wglGetProcAddress would be needed, or JUCE's internals.
             // But GrGLMakeNativeInterface usually works on Windows.
             return nullptr;
#elif JUCE_MAC
             return (GrGLFuncPtr)dlsym(RTLD_DEFAULT, name);
      });

  if (assembledInterface != nullptr) {
    ZENITH_LOG_INFO(
        "PlatformWindowUtils: Success with GrGLMakeAssembledInterface()");
    return assembledInterface;
  }

  ZENITH_LOG_ERROR(
      "PlatformWindowUtils: CRITICAL - Failed to create ANY GL interface!");
  return nullptr;


} // namespace zenith
