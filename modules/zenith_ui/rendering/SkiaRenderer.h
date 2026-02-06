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

#pragma once

// SkiaRenderer.h


#include <functional>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

#ifdef ZENITH_USE_SKIA
#if ZENITH_ENABLE_SKIA
#include <core/SkRefCnt.h> // For sk_sp
#endif
#endif

// Forward declare Skia types to avoid including headers here
class SkCanvas;
class SkSurface;
class GrDirectContext;

#ifndef ZENITH_USE_SKIA
// Dummy sk_sp when Skia is disabled
#ifndef SK_SP_DEFINED
#define SK_SP_DEFINED
template <typename T> using sk_sp = std::shared_ptr<T>;
#endif
#endif

namespace zenith {

/**
 * @class SkiaRenderer
 * @brief GPU-accelerated rendering engine using Skia
 *
 * Manages the Skia graphics context and rendering surface. Provides
 * integration between JUCE's windowing system and Skia's rendering.
 *
 * Usage:
 * @code
 * // In your JUCE Component
 * class MyComponent : public juce::Component {
 *     std::unique_ptr<SkiaRenderer> renderer_;
 *
 *     void paint(juce::Graphics& g) override {
 *         // Render with Skia instead
 *         renderer_->render([](SkCanvas* canvas) {
 *             // Skia drawing code
 *             SkPaint paint;
 *             paint.setColor(SK_ColorBLUE);
 *             canvas->drawRect(SkRect::MakeWH(100, 100), paint);
 *         });
 *     }
 * };
 * @endcode
 */
class SkiaRenderer {
public:
  /**
   * Backend type for rendering
   */
  enum class Backend {
    Auto,     ///< Automatically select best backend for platform
    Direct3D, ///< Direct3D 12 (Windows)
    Metal,    ///< Metal (macOS)
    Vulkan,   ///< Vulkan (Linux, Android)
    OpenGL,   ///< OpenGL (fallback)
    Software  ///< CPU rendering (for testing/fallback)
  };

  /**
   * Rendering statistics for performance monitoring
   */
  struct Stats {
    double frameTime = 0.0;        ///< Last frame time in milliseconds
    double averageFrameTime = 0.0; ///< Average frame time (60 frame window)
    int fps = 0;                   ///< Current frames per second
    int droppedFrames = 0;         ///< Frames dropped in last second
    size_t gpuMemoryUsed = 0;      ///< Estimated GPU memory usage in bytes
  };

  /**
   * @brief Construct renderer for a JUCE component
   * @param component Component to render into
   * @param backend Rendering backend (auto-detect if Auto)
   * @param enableVSync Enable vertical synchronization
   */
  explicit SkiaRenderer(juce::Component &component,
                        Backend backend = Backend::Auto,
                        bool enableVSync = true);

  /// Destructor - cleans up Skia resources
  ~SkiaRenderer();

  // Non-copyable
  SkiaRenderer(const SkiaRenderer &) = delete;
  SkiaRenderer &operator=(const SkiaRenderer &) = delete;

  /**
   * @brief Initialize the renderer
   * @return true if initialization succeeded
   *
   * Must be called before rendering. Sets up GPU context and surfaces.
   */
  bool initialize();

  /**
   * @brief Shutdown and cleanup resources
   */
  void shutdown();

  /**
   * @brief Check if renderer is initialized and ready
   */
  bool isInitialized() const { return initialized_; }

  /**
   * @brief Render a frame using Skia
   * @param drawCallback Function that receives SkCanvas for drawing
   *
   * Example:
   * @code
   * renderer.render([](SkCanvas* canvas) {
   *     SkPaint paint;
   *     paint.setColor(SK_ColorRED);
   *     canvas->drawCircle(50, 50, 25, paint);
   * });
   * @endcode
   */
  void render(std::function<void(SkCanvas *)> drawCallback);

  /**
   * @brief Handle window resize
   * @param width New width in pixels
   * @param height New height in pixels
   */
  void resize(int width, int height);

  /**
   * @brief Set target frames per second
   * @param fps Target FPS (60, 120, 144, etc.)
   *
   * On displays that support high refresh rates, this allows
   * rendering at 120Hz or higher for smoother animations.
   */
  void setTargetFPS(int fps);

  /**
   * @brief Get current target FPS
   */
  int getTargetFPS() const { return targetFPS_; }

  /**
   * @brief Enable or disable VSync
   * @param enable true to enable VSync, false for uncapped FPS
   */
  void setVSyncEnabled(bool enable);

  /**
   * @brief Check if VSync is enabled
   */
  bool isVSyncEnabled() const { return vsyncEnabled_; }

  /**
   * @brief Get rendering statistics
   */
  const Stats &getStats() const { return stats_; }

  /**
   * @brief Get the Skia GPU context (advanced use)
   * @return Pointer to GrDirectContext, or nullptr if not initialized
   */
  GrDirectContext *getGpuContext() const { return grContext_.get(); }

  /**
   * @brief Get the Skia surface (advanced use)
   * @return Pointer to SkSurface, or nullptr if not initialized
   *
   * Use this to read pixels for software rendering or custom blitting
   */
  SkSurface *getSurface() const { return surface_.get(); }

  /**
   * @brief Get current rendering backend
   */
  Backend getBackend() const { return backend_; }

  /**
   * @brief Get backend name as string
   */
  static const char *getBackendName(Backend backend);

private:
  // Component we're rendering into
  juce::Component &component_;

  // Skia objects
  sk_sp<GrDirectContext> grContext_; ///< GPU context
  sk_sp<SkSurface> surface_;         ///< Rendering surface

  // Configuration
  Backend backend_;
  bool vsyncEnabled_;
  int targetFPS_ = 60;
  bool initialized_ = false;

  // Rendering stats
  Stats stats_;
  juce::Time lastFrameTime_;
  std::vector<double> frameTimes_; ///< For averaging
  
  // FBO Metadata Cache (Optimizes surface recreation)
  struct FboMetadata {
      int fboId = -1;
      int width = 0;
      int height = 0;
      int samples = 0;
      int stencilBits = 0;
      unsigned int format = 0; // e.g. GL_RGBA8
      
      bool matches(int otherFboId, int otherWidth, int otherHeight) const {
          return fboId == otherFboId && width == otherWidth && height == otherHeight;
      }
  } fboCache_;

  // Platform-specific handles (void* to avoid platform headers)
  void *platformHandle_ = nullptr;

  // Internal methods
  bool createGpuContext();
  bool createSurface(int width, int height);
  void renderFrame(std::function<void(SkCanvas *)>& drawCallback);
  void updateStats();
  Backend detectBestBackend() const;

#if JUCE_WINDOWS
  bool createD3DContext();
#elif JUCE_MAC
  bool createMetalContext();
#elif JUCE_LINUX
  bool createVulkanContext();
#endif
};

} // namespace zenith
