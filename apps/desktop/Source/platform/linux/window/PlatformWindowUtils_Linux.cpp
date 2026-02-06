/*
  ==============================================================================

    PlatformWindowUtils_Linux.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "PlatformWindowUtils.h"
#include <core/SkRefCnt.h>
#include <gpu/ganesh/gl/GrGLAssembleInterface.h>
#include <gpu/ganesh/gl/GrGLInterface.h>
#include <juce_opengl/juce_opengl.h>
#include "ZenithLogger.h"

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xatom.h>

// Motif WM hints for borderless windows
struct MWMHints {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
};

#define MWM_HINTS_DECORATIONS (1L << 1)

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

void PlatformWindowUtils::removeWindowDecorations(juce::Component* window) {
    if (window == nullptr) return;
    
    auto* peer = window->getPeer();
    if (peer == nullptr) {
        ZENITH_LOG_INFO("PlatformWindowUtils::removeWindowDecorations - No peer yet, deferring");
        return;
    }
    
    // Get the native window handle (X11 Window ID)
    void* nativeHandle = peer->getNativeHandle();
    if (nativeHandle == nullptr) {
        ZENITH_LOG_INFO("PlatformWindowUtils::removeWindowDecorations - No native handle");
        return;
    }
    
    Window xWindow = reinterpret_cast<Window>(nativeHandle);
    Display* display = XOpenDisplay(nullptr);
    
    if (display == nullptr) {
        ZENITH_LOG_INFO("PlatformWindowUtils::removeWindowDecorations - Cannot open X11 display");
        return;
    }
    
    // Set Motif WM hints to remove decorations
    Atom motifHintsAtom = XInternAtom(display, "_MOTIF_WM_HINTS", False);
    
    MWMHints hints;
    hints.flags = MWM_HINTS_DECORATIONS;
    hints.functions = 0;
    hints.decorations = 0;  // No decorations
    hints.input_mode = 0;
    hints.status = 0;
    
    XChangeProperty(display, xWindow, motifHintsAtom, motifHintsAtom, 32,
                    PropModeReplace, reinterpret_cast<unsigned char*>(&hints), 5);
    
    XFlush(display);
    XCloseDisplay(display);
    
    ZENITH_LOG_INFO("PlatformWindowUtils::removeWindowDecorations - Removed X11 window decorations");
}

void PlatformWindowUtils::setTrueFullscreen(juce::Component* window, bool enable) {
    if (window == nullptr) return;
    
    auto* peer = window->getPeer();
    if (peer == nullptr) return;
    
    void* nativeHandle = peer->getNativeHandle();
    if (nativeHandle == nullptr) return;
    
    Window xWindow = reinterpret_cast<Window>(nativeHandle);
    Display* display = XOpenDisplay(nullptr);
    
    if (display == nullptr) return;
    
    Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);
    Atom wmFullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    
    XEvent event;
    memset(&event, 0, sizeof(event));
    event.type = ClientMessage;
    event.xclient.window = xWindow;
    event.xclient.message_type = wmState;
    event.xclient.format = 32;
    event.xclient.data.l[0] = enable ? 1 : 0;  // _NET_WM_STATE_ADD or _NET_WM_STATE_REMOVE
    event.xclient.data.l[1] = wmFullscreen;
    event.xclient.data.l[2] = 0;
    event.xclient.data.l[3] = 1;  // Source indicator: normal application
    
    XSendEvent(display, DefaultRootWindow(display), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
    
    XFlush(display);
    XCloseDisplay(display);
    
    ZENITH_LOG_INFO(juce::String::formatted("PlatformWindowUtils::setTrueFullscreen - %s", 
                    enable ? "enabled" : "disabled"));
}

} // namespace zenith
#endif

