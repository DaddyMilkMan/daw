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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    SkiaMainWindowIntegration.cpp
    Created: 2025-11-28
    Author:  Zenith DAW Team

    Implementation of Skia-rendered main window base class.

  ==============================================================================

*/

#include "SkiaMainWindowIntegration.h"

#ifdef ZENITH_USE_SKIA
#include "../../engine/ZenithLogger.h"
#include "PlatformWindowUtils.h"
#include <cstring>
#include <gpu/ganesh/gl/GrGLDirectContext.h>
#include <juce_opengl/juce_opengl.h>

#endif

namespace zenith {

#ifdef ZENITH_USE_SKIA

// ============================================================================
// SkiaOpenGLRenderer Implementation
// ============================================================================

SkiaOpenGLRenderer::SkiaOpenGLRenderer(juce::Component *componentToAttach)
    : targetComponent_(componentToAttach) {
  ZENITH_LOG_INFO("SkiaOpenGLRenderer: Constructor called");
  
  if (targetComponent_) {
    try {
      ZENITH_LOG_INFO("SkiaOpenGLRenderer: Setting renderer...");
      openGLContext_.setRenderer(this);
      openGLContext_.setOpenGLVersionRequired(juce::OpenGLContext::openGL3_2);
      
      // CRITICAL: Disable continuous repainting to save battery/CPU.
      // We rely on triggerRepaint() to update the UI when needed.
      openGLContext_.setContinuousRepainting(false);
      
      // Check if component already has a peer (rare but possible)
      if (targetComponent_->isShowing() && targetComponent_->getPeer() != nullptr) {
        ZENITH_LOG_INFO("SkiaOpenGLRenderer: Component already has peer, scheduling deferred attachment...");
        scheduleAttachmentCheck();
      } else {
        ZENITH_LOG_INFO("SkiaOpenGLRenderer: Component has no peer yet, scheduling deferred attachment...");
        // Schedule periodic checks via MessageManager
        scheduleAttachmentCheck();
      }
      
      ZENITH_LOG_INFO("SkiaOpenGLRenderer: Constructor complete");
    } catch (const std::exception &e) {
      ZENITH_LOG_ERROR(
          std::string("SkiaOpenGLRenderer: Exception in constructor: ") +
          e.what());
    } 
    // Removed catch(...) to allow critical failures to surface
  } else {
    ZENITH_LOG_WARNING(
        "SkiaOpenGLRenderer: WARNING - targetComponent is null!");
  }
}

void SkiaOpenGLRenderer::scheduleAttachmentCheck() {
  if (!targetComponent_)
  {
      juce::Logger::writeToLog("CRITICAL: SkiaOpenGLRenderer::scheduleAttachmentCheck called with null targetComponent. Skia rendering will not be initialized.");
      return;
  }
  
  juce::Component* comp = targetComponent_;
  juce::OpenGLContext* ctx = &openGLContext_;
  
  ZENITH_LOG_INFO("SkiaOpenGLRenderer::scheduleAttachmentCheck called");
  
  juce::MessageManager::callAsync([this, comp, ctx]() {
    if (!comp || !ctx) {
      ZENITH_LOG_ERROR("scheduleAttachmentCheck: Component or context is null!");
      return;
    }
    
    // Debug logging reduced to avoid spam
    // std::cerr << "[ASYNC] Checking peer..." << std::endl;
    
    if (comp->getPeer() != nullptr && !ctx->isAttached()) {
      ZENITH_LOG_INFO("SkiaOpenGLRenderer: Async check found peer, attaching context...");
      attachContextNow();
    } else if (!ctx->isAttached()) {
      // Reschedule - either no peer yet or dimensions too small
      ZENITH_LOG_INFO("Conditions not met - rescheduling attachment check in 100ms");
      juce::Timer::callAfterDelay(100, [this]() {
        scheduleAttachmentCheck();
      });
    } else {
      ZENITH_LOG_INFO("Context already attached - no action needed");
    }
  });
}

void SkiaOpenGLRenderer::timerCallback() {
    // Zombie callback removed
}

void SkiaOpenGLRenderer::attachContextNow() {
  ZENITH_LOG_INFO("SkiaOpenGLRenderer::attachContextNow called");
  
  if (!targetComponent_) {
    ZENITH_LOG_ERROR("attachContextNow: targetComponent is null!");
    return;
  }
  
  if (openGLContext_.isAttached()) {
    ZENITH_LOG_INFO("attachContextNow: Context already attached, skipping");
    return;
  }
  
  ZENITH_LOG_INFO(juce::String::formatted(
    "attachContextNow: Component size=%dx%d, hasPeer=%s",
    targetComponent_->getWidth(), targetComponent_->getHeight(),
    targetComponent_->getPeer() != nullptr ? "YES" : "NO"
  ));
  
  try {
    ZENITH_LOG_INFO("Calling openGLContext_.attachTo()...");
    openGLContext_.attachTo(*targetComponent_);
    ZENITH_LOG_INFO("OpenGL context attached successfully!");
  } catch (const std::exception& e) {
    ZENITH_LOG_ERROR("SkiaOpenGLRenderer: Exception during attachTo: " + std::string(e.what()));
  }
  // Removed catch(...) as unexpected exceptions should crash/dump
}


SkiaOpenGLRenderer::~SkiaOpenGLRenderer() {
  surface_.reset();
  grContext_.reset();
  openGLContext_.detach();
}

void SkiaOpenGLRenderer::triggerRepaint() {
  if (openGLContext_.isAttached()) {
    openGLContext_.triggerRepaint();
  } else if (targetComponent_) {
    targetComponent_->repaint();
  }
}

void SkiaOpenGLRenderer::newOpenGLContextCreated() {
  ZENITH_LOG_INFO("========================================");
  ZENITH_LOG_INFO("SkiaOpenGLRenderer::newOpenGLContextCreated called");
  ZENITH_LOG_INFO("========================================");
  
  // Log OpenGL version
  const char* glVersion = (const char*)juce::gl::glGetString(juce::gl::GL_VERSION);
  ZENITH_LOG_INFO("OpenGL Version: " + juce::String(glVersion ? glVersion : "unknown"));

  try {
    ZENITH_LOG_INFO("Creating Skia GL interface...");
    auto glInterface = PlatformWindowUtils::createNativeGLInterface(openGLContext_);
    if (!glInterface) {
      ZENITH_LOG_ERROR("Failed to create GL interface!");
      return;
    }
    ZENITH_LOG_INFO("GL interface created successfully");
    
    ZENITH_LOG_INFO("Creating Skia GrDirectContext...");
    grContext_ = GrDirectContexts::MakeGL(glInterface);
    if (!grContext_) {
      ZENITH_LOG_ERROR("Failed to create GrDirectContext!");
      return;
    }
    ZENITH_LOG_INFO("GrDirectContext created successfully");

    contextInitialized_ = true;
    ZENITH_LOG_INFO("Skia context initialized successfully");
    
    ZENITH_LOG_INFO("Creating Skia surface...");
    recreateSurface();
  } catch (const std::exception &e) {
    ZENITH_LOG_ERROR(
        std::string(
            "SkiaOpenGLRenderer: Exception in newOpenGLContextCreated: ") +
        e.what());
  }
  
  ZENITH_LOG_INFO("========================================");
}

void SkiaOpenGLRenderer::renderOpenGL() {
  if (!openGLContext_.makeActive()) return;
  if (!contextInitialized_) return;

  if (grContext_ && grContext_->abandoned()) {
    surface_.reset();
    grContext_.reset();
    contextInitialized_ = false;
    newOpenGLContextCreated();
    if (!contextInitialized_) return;
  }

  const double scale = openGLContext_.getRenderingScale();
  int logicalWidth = safeWidth_.load();
  int logicalHeight = safeHeight_.load();

  if (logicalWidth <= 0 || logicalHeight <= 0) return;

  const int physicalWidth = juce::roundToInt(logicalWidth * scale);
  const int physicalHeight = juce::roundToInt(logicalHeight * scale);

  juce::gl::glViewport(0, 0, physicalWidth, physicalHeight);
  
  if (lastWidth_ != physicalWidth || lastHeight_ != physicalHeight) {
    recreateSurfaceWithSize(physicalWidth, physicalHeight);
    lastWidth_ = physicalWidth;
    lastHeight_ = physicalHeight;
  }

  if (surface_) {
    SkCanvas *canvas = surface_->getCanvas();
    if (canvas) {
      canvas->save();
      canvas->scale(static_cast<float>(scale), static_cast<float>(scale));
      canvas->clear(SkColorSetARGB(255, 20, 20, 25));
      drawSkiaContent(canvas);
      canvas->restore();
      grContext_->flushAndSubmit();
    }
  } else {
      juce::gl::glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
      juce::gl::glClear(juce::gl::GL_COLOR_BUFFER_BIT);
  }
}

void SkiaOpenGLRenderer::openGLContextClosing() {
  if (surface_) surface_.reset();
  if (grContext_) grContext_.reset();
  contextInitialized_ = false;
}

void SkiaOpenGLRenderer::recreateSurface() {
  if (!targetComponent_) return;
  const double scale = openGLContext_.getRenderingScale();
  int physicalWidth = juce::roundToInt(targetComponent_->getWidth() * scale);
  int physicalHeight = juce::roundToInt(targetComponent_->getHeight() * scale);
  recreateSurfaceWithSize(physicalWidth, physicalHeight);
}

void SkiaOpenGLRenderer::recreateSurfaceWithSize(int width, int height) {
  if (!grContext_ || width <= 0 || height <= 0) return;

  GLint currentFBO = 0;
  GLint samples = 0;
  GLint stencilBits = 0;
#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif
#ifndef GL_SAMPLES
#define GL_SAMPLES 0x80A9
#endif
#ifndef GL_STENCIL_BITS
#define GL_STENCIL_BITS 0x0D57
#endif

  juce::gl::glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
  juce::gl::glGetIntegerv(GL_SAMPLES, &samples);
  juce::gl::glGetIntegerv(GL_STENCIL_BITS, &stencilBits);

  if (samples < 0) samples = 0;
  if (stencilBits < 0) stencilBits = 0;

  GrGLFramebufferInfo framebufferInfo;
  framebufferInfo.fFBOID = (GrGLuint)currentFBO;
  framebufferInfo.fFormat = 0x8058; // GL_RGBA8

  auto backendRT = GrBackendRenderTargets::MakeGL(width, height, samples, stencilBits, framebufferInfo);

  surface_ = SkSurfaces::WrapBackendRenderTarget(
      grContext_.get(), backendRT, kBottomLeft_GrSurfaceOrigin,
      kRGBA_8888_SkColorType, SkColorSpace::MakeSRGB(), nullptr);
}

// ============================================================================
// SkiaMainWindowIntegration Implementation
// ============================================================================

SkiaMainWindowIntegration::SkiaMainWindowIntegration()
    : SkiaOpenGLRenderer(this) {
  setOpaque(true);

#if JUCE_WINDOWS
  d3d12Context_ = std::make_unique<SkiaD3D12Context>();
#endif
}

SkiaMainWindowIntegration::~SkiaMainWindowIntegration() {
#if JUCE_WINDOWS
  d3d12Context_.reset();
#endif
}

void SkiaMainWindowIntegration::paint(juce::Graphics &g) {
#if JUCE_WINDOWS
  // 1. Try D3D12 Path
  if (useD3D12_ && d3d12Context_) {
      if (!d3d12Context_->isInitialized()) {
          // Lazy init
          void* hwnd = getPeer() ? getPeer()->getNativeHandle() : nullptr;
          if (hwnd && getWidth() > 0 && getHeight() > 0) {
              if (d3d12Context_->initialize(hwnd, getWidth(), getHeight())) {
                  ZENITH_LOG_INFO("D3D12 Initialized!");
              } else {
                  ZENITH_LOG_ERROR("D3D12 Init Failed - Unknown error");
                  useD3D12_ = false; // Fallback
              }
          }
      }

      if (d3d12Context_->isInitialized()) {
          SkCanvas* canvas = d3d12Context_->beginFrame();
          if (canvas) {
              canvas->clear(SkColorSetARGB(255, 20, 20, 25));
              drawSkiaContent(canvas);
              d3d12Context_->endFrame();
              return; // Done
          }
      }
  }
#endif

  // 2. Try OpenGL Path
  if (openGLContext_.isAttached()) {
    return;
  }

  // 3. Fallback to Software Raster
  static int frameCount = 0;
  frameCount++;
  
  auto bounds = getLocalBounds();
  if (bounds.isEmpty()) return;

  const int width = bounds.getWidth();
  const int height = bounds.getHeight();

  // Create/Recreate Raster Surface if needed
  if (!softwareSurface_ || softwareSurface_->width() != width || softwareSurface_->height() != height) {
      SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
      softwareSurface_ = SkSurfaces::Raster(info);
      softwareImage_ = juce::Image(juce::Image::ARGB, width, height, true);
  }
  
  if (!softwareSurface_) {
      g.fillAll(juce::Colours::darkred);
      return;
  }

  // Draw Content
  SkCanvas* canvas = softwareSurface_->getCanvas();
  canvas->clear(SkColorSetARGB(255, 20, 20, 25)); 
  drawSkiaContent(canvas);

  // Blit to JUCE Graphics
  SkPixmap pixmap;
  if (softwareSurface_->peekPixels(&pixmap)) {
      juce::Image::BitmapData bd(softwareImage_, juce::Image::BitmapData::writeOnly);
      const size_t size = (size_t)width * height * 4;
      std::memcpy(bd.data, pixmap.addr(), size);
      
      g.drawImageAt(softwareImage_, 0, 0);
  }
}

void SkiaMainWindowIntegration::resized() {
  auto bounds = getLocalBounds();
  updateDimensions(bounds.getWidth(), bounds.getHeight());

#if JUCE_WINDOWS
  if (useD3D12_ && d3d12Context_ && d3d12Context_->isInitialized()) {
      d3d12Context_->resize(bounds.getWidth(), bounds.getHeight());
  }
#endif
}

void SkiaMainWindowIntegration::parentHierarchyChanged() {
  juce::String msg = "SkiaMainWindowIntegration: parentHierarchyChanged called, peer=";
  msg += (getPeer() != nullptr ? "valid" : "null");
  msg += ", attached=";
  msg += (openGLContext_.isAttached() ? "yes" : "no");
  ZENITH_LOG_INFO(msg);

  if (isShowing() && getPeer() != nullptr) {
#if JUCE_WINDOWS
      // If we are using D3D12, we trigger a repaint and let paint() handle init
      if (useD3D12_) {
          repaint();
          return;
      }
#endif
      if (!openGLContext_.isAttached()) {
          scheduleAttachmentCheck();
      }
  }
}

void SkiaMainWindowIntegration::visibilityChanged() {
  juce::String msg = "SkiaMainWindowIntegration: visibilityChanged called, visible=";
  msg += (isVisible() ? "yes" : "no");
  msg += ", peer=";
  msg += (getPeer() != nullptr ? "valid" : "null");
  ZENITH_LOG_INFO(msg);

  if (isShowing() && getPeer() != nullptr) {
#if JUCE_WINDOWS
      if (useD3D12_) {
          repaint();
          return;
      }
#endif
      if (!openGLContext_.isAttached()) {
          scheduleAttachmentCheck();
      }
  }
}

#endif // ZENITH_USE_SKIA

} // namespace zenith