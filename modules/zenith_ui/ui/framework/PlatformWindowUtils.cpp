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

#include "PlatformWindowUtils.h"
#include <ZenithLogger.h>

#ifdef ZENITH_USE_SKIA
#include <gpu/ganesh/gl/GrGLAssembleInterface.h>
#include <gpu/ganesh/gl/GrGLDirectContext.h>
#include <gpu/ganesh/gl/GrGLInterface.h>
#endif

// Linux/Unix specific includes for dlsym
#if JUCE_LINUX || (JUCE_MAC && !defined(__OBJC__))
#include <dlfcn.h>
#endif

namespace zenith {

#ifndef __linux__
sk_sp<const GrGLInterface>
PlatformWindowUtils::createNativeGLInterface(juce::OpenGLContext &context) {
  juce::ignoreUnused(context); // Might be used in future or other platforms
  ZENITH_LOG_INFO("PlatformWindowUtils: Attempting to create GL interface...");

#ifdef ZENITH_USE_SKIA

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
#else
             return nullptr;
#endif
      });

  if (assembledInterface != nullptr) {
    ZENITH_LOG_INFO(
        "PlatformWindowUtils: Success with GrGLMakeAssembledInterface()");
    return assembledInterface;
  }

  ZENITH_LOG_ERROR(
      "PlatformWindowUtils: CRITICAL - Failed to create ANY GL interface!");
  return nullptr;

#else
  ZENITH_LOG_ERROR("PlatformWindowUtils: ZENITH_USE_SKIA not defined!");
  return nullptr;
#endif
}
#endif

} // namespace zenith
