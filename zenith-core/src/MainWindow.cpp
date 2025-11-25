/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "../include/MainWindow.h"
#include "../Source/commands/CommandAPI.h"
#include "../Source/engine/Clip.h"
#include "../Source/engine/Track.h"
#include "../Source/network/AIBridgeClient.h"
#include "../Source/ui/ArrangerComponent.h"
#include "../Source/ui/InstrumentBrowserPanel.h"
#include "../Source/ui/WingmanPanel.h"
#include "../include/PianoRollEditor.h"

#ifdef ZENITH_USE_SKIA
#include "../Source/ui/skia/SkiaComponent.h"
#include <include/core/SkFont.h>
#include <include/core/SkImage.h>
#include <include/core/SkImageInfo.h>
#include <include/core/SkPixmap.h>
#include <include/core/SkSamplingOptions.h>
#include <include/core/SkSurface.h>
#include <include/core/SkTextBlob.h>

#endif

//==============================================================================
// MainComponent Implementation
//==============================================================================

MainComponent::MainComponent(Engine &eng, zenith::CommandAPI &api,
                             zenith::AIBridgeClient &aiClient,
                             ProjectState &state)
    : engine(eng), projectState(state)
#ifndef ZENITH_USE_SKIA
      ,
      mixerComponent(state)
#endif
{
  // Register as key listener for undo/redo shortcuts
  addKeyListener(this);
  setWantsKeyboardFocus(true);

  setSize(1400, 800);

  DBG("========================================");
  DBG("MainComponent Constructor - Modern DAW Layout");
  DBG("========================================");

#ifdef ZENITH_USE_SKIA
  DBG(">>> ZENITH_USE_SKIA IS DEFINED - MODERN SKIA DAW LAYOUT BRANCH "
      "EXECUTING <<<");

  // Initialize Skia rendering system
  bool skiaInitSuccess = initializeSkiaRendering();
  if (skiaInitSuccess) {
    DBG("✓ Skia theme configuration loaded successfully");
  }

  // Instantiate the SkiaRenderer
  DBG("→ Initializing SkiaRenderer...");
  renderer_ = std::make_unique<zenith::SkiaRenderer>(*this);
  if (renderer_->initialize()) {
    DBG("✓ SkiaRenderer initialized successfully");
  } else {
    DBG("✗ SkiaRenderer initialization FAILED - falling back to software");
    renderer_.reset();
  }

  // ============================================================================
  // Create Modern DAW Layout Panels
  // ============================================================================

  // Top: Transport Bar
  DBG("→ Creating TransportBar...");
  transportBar = std::make_unique<zenith::TransportBar>();
  transportBar->setProjectName("Zenith DAW");
  transportBar->setTempo(120.0);
  transportBar->setTimeSignature(4, 4);

  // Hook up transport callbacks
  transportBar->onPlayClicked = [this]() {
    engine.play();
    DBG("Play clicked");
  };
  transportBar->onStopClicked = [this]() {
    engine.stop();
    DBG("Stop clicked");
  };
  transportBar->onRecordClicked = [this]() {
    engine.toggleRecording();
    bool isRec = engine.isRecording();
    transportBar->setRecording(isRec);
    if (isRec) {
      DBG("Recording started");
    } else {
      DBG("Recording stopped");
    }
  };

  addAndMakeVisible(transportBar.get());
  DBG("✓ TransportBar created and made visible at " +
      juce::String::toHexString((juce::pointer_sized_int)transportBar.get()));

  // Left: Browser Panel
  DBG("→ Creating BrowserPanel...");
  browserPanel = std::make_unique<zenith::BrowserPanel>();
  addAndMakeVisible(browserPanel.get());
  DBG("✓ BrowserPanel created and made visible at " +
      juce::String::toHexString((juce::pointer_sized_int)browserPanel.get()));

  // Create Wingman panel (will be hosted in RightSidePanel)
  DBG("→ Creating WingmanPanel...");
  auto wingmanPanel = std::make_unique<WingmanPanel>(api, aiClient);
  DBG("✓ WingmanPanel created at " +
      juce::String::toHexString((juce::pointer_sized_int)wingmanPanel.get()));

  // Right: Scratch Pads + Wingman Console
  DBG("→ Creating RightSidePanel...");
  rightSidePanel = std::make_unique<zenith::RightSidePanel>();
  rightSidePanel->setWingmanPanel(wingmanPanel.get());
  addAndMakeVisible(rightSidePanel.get());
  DBG("✓ RightSidePanel created and made visible at " +
      juce::String::toHexString((juce::pointer_sized_int)rightSidePanel.get()));

  // Keep wingmanPanel alive (owned by MainComponent)
  wingmanPanelPtr_ = std::move(wingmanPanel);
  DBG("✓ WingmanPanel ownership transferred to MainComponent member");

  // Bottom: Piano Keyboard + Mixer Strip
  DBG("→ Creating BottomBar...");
  bottomBar = std::make_unique<zenith::BottomBar>(midiKeyboardState);
  bottomBar->setKeyboardVisible(false); // Hidden by default
  addAndMakeVisible(bottomBar.get());
  DBG("✓ BottomBar created and made visible at " +
      juce::String::toHexString((juce::pointer_sized_int)bottomBar.get()));

  // Center: Arranger Component
  DBG("→ Creating ArrangerComponent...");
  arrangerComponent =
      std::make_unique<ArrangerComponent>(*engine.getProjectState());
  addAndMakeVisible(arrangerComponent.get());
  DBG("✓ ArrangerComponent created and made visible at " +
      juce::String::toHexString(
          (juce::pointer_sized_int)arrangerComponent.get()));

  // Start animation timer (SkiaMainWindowIntegration handles this)
  DBG("✓ Animation timer managed by SkiaMainWindowIntegration");

#else
  // ============================================================================
  // JUCE Fallback Layout (Legacy)
  // ============================================================================

  DBG("ZENITH_USE_SKIA is NOT DEFINED - Using JUCE fallback layout");

  // Status label
  statusLabel.setText("Zenith DAW - JUCE Fallback Mode",
                      juce::dontSendNotification);
  statusLabel.setJustificationType(juce::Justification::centredLeft);
  statusLabel.setFont(juce::Font(16.0f, juce::Font::bold));
  addAndMakeVisible(statusLabel);

  // CPU usage label
  cpuLabel.setText("CPU: 0%", juce::dontSendNotification);
  cpuLabel.setJustificationType(juce::Justification::centredRight);
  addAndMakeVisible(cpuLabel);

  // Audio device label
  audioDeviceLabel.setText("Audio Device: Not initialized",
                           juce::dontSendNotification);
  audioDeviceLabel.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(audioDeviceLabel);

  // Track count label
  trackCountLabel.setText("Tracks: 0", juce::dontSendNotification);
  trackCountLabel.setJustificationType(juce::Justification::centredRight);
  addAndMakeVisible(trackCountLabel);

  // Transport buttons
  playButton.setButtonText("Play");
  playButton.onClick = [this]() {
    engine.play();
    DBG("Play button clicked");
  };
  addAndMakeVisible(playButton);

  stopButton.setButtonText("Stop");
  stopButton.onClick = [this]() {
    engine.stop();
    DBG("Stop button clicked");
  };
  addAndMakeVisible(stopButton);

  recordButton.setButtonText("Record");
  recordButton.onClick = [this]() {
    engine.toggleRecording();
    bool isRecording = engine.isRecording();
    recordButton.setColour(juce::TextButton::buttonColourId,
                           isRecording ? juce::Colours::red
                                       : juce::Colours::darkgrey);
    DBG((isRecording ? "Recording started" : "Recording stopped"));
  };
  addAndMakeVisible(recordButton);

  // Import Audio button
  importButton.setButtonText("Import Audio...");
  importButton.onClick = [this]() { handleImportAudio(); };
  addAndMakeVisible(importButton);

  // Virtual MIDI Keyboard toggle button
  virtualKeyboardButton.setButtonText("🎹 Keyboard (M)");
  virtualKeyboardButton.setClickingTogglesState(true);
  virtualKeyboardButton.onClick = [this]() {
    virtualKeyboardVisible = virtualKeyboardButton.getToggleState();
    if (midiKeyboard)
      midiKeyboard->setVisible(virtualKeyboardVisible);
    resized();
  };
  addAndMakeVisible(virtualKeyboardButton);

  // Mixer component
  addAndMakeVisible(mixerComponent);

  // Arranger component
  arrangerComponent =
      std::make_unique<ArrangerComponent>(*engine.getProjectState());
  addAndMakeVisible(arrangerComponent.get());

  // Wingman panel
  wingmanPanel = std::make_unique<WingmanPanel>(api, aiClient);
  addAndMakeVisible(wingmanPanel.get());

  // Instrument Browser
  instrumentBrowserPanel =
      std::make_unique<zenith::InstrumentBrowserPanel>(engine, projectState);
  addAndMakeVisible(instrumentBrowserPanel.get());

  // Virtual MIDI Keyboard
  midiKeyboard = std::make_unique<juce::MidiKeyboardComponent>(
      midiKeyboardState, juce::MidiKeyboardComponent::horizontalKeyboard);
  midiKeyboard->setVisible(false);
  addAndMakeVisible(midiKeyboard.get());

  // Start timer for CPU monitoring
  startTimer(16);
#endif

  DBG("========================================");
  DBG("MainComponent Constructor COMPLETE");
  DBG("========================================");
}

MainComponent::~MainComponent() {
  DBG("MainComponent Destructor called");
  removeKeyListener(this);

#ifndef ZENITH_USE_SKIA
  stopTimer();
#endif
}

bool MainComponent::keyPressed(const juce::KeyPress &key,
                               Component *originatingComponent) {
  juce::ignoreUnused(originatingComponent);

  // Ctrl+Z or Cmd+Z for undo
  if (key.getTextCharacter() == 'z' && key.getModifiers().isCommandDown() &&
      !key.getModifiers().isShiftDown()) {
    if (projectState.canUndo()) {
      projectState.undo();
      DBG("Keyboard shortcut: Undo");
      return true;
    }
  }

  // Ctrl+Shift+Z or Cmd+Shift+Z for redo
  if (key.getTextCharacter() == 'Z' && key.getModifiers().isCommandDown() &&
      key.getModifiers().isShiftDown()) {
    if (projectState.canRedo()) {
      projectState.redo();
      DBG("Keyboard shortcut: Redo");
      return true;
    }
  }

  // Ctrl+Y or Cmd+Y for redo (alternative)
  if (key.getTextCharacter() == 'y' && key.getModifiers().isCommandDown()) {
    if (projectState.canRedo()) {
      projectState.redo();
      DBG("Keyboard shortcut: Redo (Y)");
      return true;
    }
  }

  // M key: Toggle virtual MIDI keyboard (like Ableton Live)
  if (key.getTextCharacter() == 'm' || key.getTextCharacter() == 'M') {
#ifndef ZENITH_USE_SKIA
    virtualKeyboardButton.setToggleState(
        !virtualKeyboardButton.getToggleState(), juce::sendNotification);
#else
    if (bottomBar) {
      bottomBar->setKeyboardVisible(!bottomBar->isKeyboardVisible());
      resized();
    }
#endif
    DBG("Keyboard shortcut: Toggle Virtual MIDI Keyboard (M)");
    return true;
  }

  return false; // Key not handled
}

void MainComponent::paint(juce::Graphics &g) {
#ifdef ZENITH_USE_SKIA
  // Use SkiaRenderer if it was successfully initialized
  if (renderer_) {
    static int skiaPaintCount = 0;
    if (skiaPaintCount < 3) {
      skiaPaintCount++;
      DBG("🎨 paint() call #" + juce::String(skiaPaintCount) +
          " using SKIA RENDERER!");
      if (skiaPaintCount == 1) {
        DBG("  Software backend: Rendering with Skia, blitting to JUCE");
        DBG("  ✓ ACTUAL SKIA CPU RENDERING ACTIVE!");
      }
    }

    // Step 1: Render EVERYTHING using Skia
    renderer_->render([this](SkCanvas *canvas) {
      // Beautiful dark background with Skia anti-aliasing
      SkPaint bgPaint;
      bgPaint.setColor(0xFF1E1E1E);
      canvas->clear(0xFF1E1E1E);

      // DEBUG: Visual indicator to verify Skia rendering is active
      // Set to true to show red banner, false to hide
      constexpr bool kShowSkiaDebugBanner = true;
      if (kShowSkiaDebugBanner) {
        SkPaint bannerPaint;
        bannerPaint.setColor(SkColorSetARGB(255, 255, 0, 0)); // Bright red
        bannerPaint.setAntiAlias(true);
        canvas->drawRect(SkRect::MakeXYWH(10, 10, 350, 40), bannerPaint);

        SkFont bannerFont;
        bannerFont.setSize(16.0f);
        bannerFont.setEdging(SkFont::Edging::kAntiAlias);

        SkPaint textPaint;
        textPaint.setColor(SkColorSetARGB(255, 255, 255, 255)); // White text
        textPaint.setAntiAlias(true);

        canvas->drawString("SKIA MAIN LAYOUT ACTIVE", 20, 35, bannerFont,
                           textPaint);
      }

      // Render ALL child components - NATIVE SKIA or JUCE fallback
      static int nativeSkiaCount = 0;
      static int juceFallbackCount = 0;

      for (int i = 0; i < getNumChildComponents(); ++i) {
        auto *child = getChildComponent(i);
        if (child && child->isVisible()) {
          auto childBounds = child->getBounds();
          SkRect skBounds = SkRect::MakeXYWH(
              (float)childBounds.getX(), (float)childBounds.getY(),
              (float)childBounds.getWidth(), (float)childBounds.getHeight());

          // Check if component supports native Skia rendering
          auto *skiaComponent = dynamic_cast<zenith::SkiaComponent *>(child);

          // DEBUG: Log what we found
          static bool loggedComponents = false;
          if (!loggedComponents && skiaPaintCount == 1) {
            DBG("  [COMPONENT #" + juce::String(i) + "] " +
                juce::String(child->getName().isEmpty() ? "unnamed"
                                                        : child->getName()));
            DBG("    SkiaComponent cast: " +
                juce::String(skiaComponent != nullptr ? "YES" : "NO"));
            if (skiaComponent)
              DBG("    supportsSkiaRendering(): " +
                  juce::String(skiaComponent->supportsSkiaRendering() ? "YES"
                                                                      : "NO"));
            DBG("    Bounds: " + juce::String(childBounds.getX()) + "," +
                juce::String(childBounds.getY()) + " " +
                juce::String(childBounds.getWidth()) + "x" +
                juce::String(childBounds.getHeight()));
            DBG("    Visible: " +
                juce::String(child->isVisible() ? "YES" : "NO"));
          }

          if (skiaComponent && skiaComponent->supportsSkiaRendering()) {
            // ✓ NATIVE SKIA RENDERING - Direct to SkCanvas!
            if (skiaPaintCount == 1)
              DBG("  >>> CALLING paintToSkia() for component #" +
                  juce::String(i));

            canvas->save();
            skiaComponent->paintToSkia(canvas, skBounds);
            canvas->restore();

            nativeSkiaCount++;
          } else {
            // ✗ JUCE FALLBACK - Render to image then composite
            juce::Image componentImage(
                juce::Image::ARGB, juce::jmax(1, childBounds.getWidth()),
                juce::jmax(1, childBounds.getHeight()), true);

            juce::Graphics componentGraphics(componentImage);
            componentGraphics.setOrigin(-childBounds.getX(),
                                        -childBounds.getY());
            child->paint(componentGraphics);

            // Convert JUCE image to Skia
            juce::Image::BitmapData bitmapData(
                componentImage, juce::Image::BitmapData::readOnly);

            SkImageInfo imageInfo = SkImageInfo::MakeN32Premul(
                componentImage.getWidth(), componentImage.getHeight());

            sk_sp<SkImage> skiaImage = SkImages::RasterFromPixmapCopy(
                SkPixmap(imageInfo, bitmapData.data, bitmapData.lineStride));

            if (skiaImage) {
              SkPaint paint;
              paint.setAntiAlias(true);
              canvas->drawImage(skiaImage, (float)childBounds.getX(),
                                (float)childBounds.getY(),
                                SkSamplingOptions(SkFilterMode::kLinear),
                                &paint);
            }

            juceFallbackCount++;
          }
        }
      }
    });

    // Step 2: Get the Skia surface and read pixels
    SkSurface *surface = renderer_->getSurface();
    if (surface) {
      if (skiaPaintCount == 1)
        DBG("  [DEBUG] Got SkSurface pointer: " +
            juce::String::toHexString((juce::pointer_sized_int)surface));

      // Create SkImage from surface
      sk_sp<SkImage> skImage = surface->makeImageSnapshot();
      if (skImage) {
        // Get image dimensions
        int width = skImage->width();
        int height = skImage->height();

        if (skiaPaintCount == 1)
          DBG("  [DEBUG] SkImage dimensions: " + juce::String(width) + "x" +
              juce::String(height));

        // Create JUCE image
        juce::Image juceImage(juce::Image::ARGB, width, height, true);

        if (skiaPaintCount == 1)
          DBG("  [DEBUG] Created JUCE image: " +
              juce::String(juceImage.getWidth()) + "x" +
              juce::String(juceImage.getHeight()));

        // Read pixels from Skia to JUCE
        SkPixmap pixmap;
        if (skImage->peekPixels(&pixmap)) {
          if (skiaPaintCount == 1)
            DBG("  [DEBUG] peekPixels succeeded, pixmap: " +
                juce::String(pixmap.width()) + "x" +
                juce::String(pixmap.height()));

          juce::Image::BitmapData bitmapData(
              juceImage, juce::Image::BitmapData::writeOnly);

          // Copy row by row (Skia uses premultiplied alpha, JUCE expects it
          // too)
          for (int y = 0; y < height; ++y) {
            const uint32_t *src = (const uint32_t *)pixmap.addr32(0, y);
            uint32_t *dst = (uint32_t *)bitmapData.getLinePointer(y);
            memcpy(dst, src, width * sizeof(uint32_t));
          }

          if (skiaPaintCount == 1)
            DBG("  [DEBUG] Pixel copy complete, copied " +
                juce::String(height) + " rows");
        } else {
          DBG("  [ERROR] peekPixels FAILED!");
        }

        // Step 3: VERIFY pixels before drawing
        auto componentBounds = getLocalBounds();
        if (skiaPaintCount == 1) {
          DBG("  [DEBUG] Component bounds: " +
              juce::String(componentBounds.getWidth()) + "x" +
              juce::String(componentBounds.getHeight()));

          // VERIFY: Check if green box is actually in the image
          DBG("  ========================================");
          DBG("  PIXEL VERIFICATION (checking image data)");
          DBG("  ========================================");

          // Check center of green box at (935, 130) - should be GREEN
          juce::Colour centerPixel = juceImage.getPixelAt(935, 130);
          DBG("  Pixel at (935,130) center: RGB(" +
              juce::String(centerPixel.getRed()) + "," +
              juce::String(centerPixel.getGreen()) + "," +
              juce::String(centerPixel.getBlue()) + ")");

          bool isGreen =
              (centerPixel.getGreen() > 200 && centerPixel.getRed() < 50);
          if (isGreen)
            DBG("  ✓ GREEN (0,255,0) DETECTED - Skia drew it!");
          else
            DBG("  ✗ NOT GREEN - Skia rendering FAILED!");

          // Check yellow border at (885, 15) - should be YELLOW
          juce::Colour borderPixel = juceImage.getPixelAt(885, 15);
          DBG("  Pixel at (885,15) border: RGB(" +
              juce::String(borderPixel.getRed()) + "," +
              juce::String(borderPixel.getGreen()) + "," +
              juce::String(borderPixel.getBlue()) + ")");

          bool isYellow =
              (borderPixel.getRed() > 200 && borderPixel.getGreen() > 200);
          if (isYellow)
            DBG("  ✓ YELLOW (255,255,0) DETECTED - Border exists!");
          else
            DBG("  ✗ NOT YELLOW - Border drawing FAILED!");

          DBG("  ========================================");
        }

        if (skiaPaintCount == 1)
          DBG("  [DEBUG] Drawing image at (0, 0)...");

        g.drawImageAt(juceImage, 0, 0);

        if (skiaPaintCount == 1) {
          // TRUTHFUL RENDERING STATISTICS
          int totalComponents = getNumChildComponents();
          int nativeSkia = 0;
          int juceFallback = 0;

          for (int i = 0; i < totalComponents; ++i) {
            auto *child = getChildComponent(i);
            if (child && child->isVisible()) {
              auto *skiaComp = dynamic_cast<zenith::SkiaComponent *>(child);
              if (skiaComp && skiaComp->supportsSkiaRendering())
                nativeSkia++;
              else
                juceFallback++;
            }
          }

          DBG("  ========================================");
          DBG("  RENDERING STATISTICS (TRUTHFUL)");
          DBG("  ========================================");
          DBG("  Total visible components: " + juce::String(totalComponents));
          DBG("  ");
          DBG("  ✓ Native Skia rendering: " + juce::String(nativeSkia) +
              " components");
          DBG("  ✗ JUCE fallback: " + juce::String(juceFallback) +
              " components");
          DBG("  ");

          if (nativeSkia == totalComponents && totalComponents > 0) {
            DBG("  🎉 100% PURE SKIA RENDERING!");
            DBG("  ALL components use native SkCanvas!");
          } else if (nativeSkia > 0) {
            int percent = (nativeSkia * 100) / totalComponents;
            DBG("  ⚠ HYBRID: " + juce::String(percent) + "% native Skia");
            DBG("  Still " + juce::String(juceFallback) +
                " components using JUCE");
          } else {
            DBG("  ✗ NO NATIVE SKIA YET");
            DBG("  All components still use JUCE Graphics");
            DBG("  Only Skia compositing is active");
          }
          DBG("  ========================================");
        }

        return; // Successfully rendered with Skia
      } else {
        DBG("  [ERROR] makeImageSnapshot() returned null!");
      }
    } else {
      DBG("  [ERROR] getSurface() returned null!");
    }

    // If we get here, something went wrong
    DBG("ERROR: Failed to get pixels from Skia surface!");
    DBG("  Falling back to JUCE rendering");
    renderer_.reset();
  }
#endif

  // JUCE fallback rendering (when Skia disabled or failed to initialize)
  static int paintCallCount = 0;
  if (paintCallCount < 3) {
    paintCallCount++;
    DBG("paint() call #" << paintCallCount
                         << " using juce::Graphics (JUCE fallback)");
    if (paintCallCount == 1) {
      DBG("  paint() receives juce::Graphics, not SkCanvas");
      DBG("  This confirms JUCE rendering is active, NOT Skia");
    }
  }

  // Background (ArrangerComponent handles its own painting)
  g.fillAll(juce::Colour(0xff1e1e1e)); // Dark grey (LUNA-inspired)
}

void MainComponent::resized() {
  auto bounds = getLocalBounds();

  DBG("MainComponent::resized() called - Total bounds: " +
      juce::String(bounds.getWidth()) + "x" + juce::String(bounds.getHeight()));

#ifdef ZENITH_USE_SKIA
  // ============================================================================
  // Modern DAW Layout with Skia Panels
  // ============================================================================
  DBG("  Using ZENITH_USE_SKIA layout branch");

  // Top: Transport Bar (60px height)
  if (transportBar) {
    auto transportBounds = bounds.removeFromTop(60);
    transportBar->setBounds(transportBounds);
    DBG("  ✓ TransportBar positioned at: " + transportBounds.toString());
  } else {
    DBG("  ✗ TransportBar is NULL!");
  }

  // Bottom: Piano Keyboard + Mixer Strip (96px height when visible)
  if (bottomBar) {
    auto bottomBounds = bounds.removeFromBottom(96);
    bottomBar->setBounds(bottomBounds);
    DBG("  ✓ BottomBar positioned at: " + bottomBounds.toString());
  } else {
    DBG("  ✗ BottomBar is NULL!");
  }

  // Left: Browser Panel (260px width, collapsible)
  if (browserPanel) {
    int browserWidth = browserPanel->isCollapsed() ? 48 : 260;
    auto browserBounds = bounds.removeFromLeft(browserWidth);
    browserPanel->setBounds(browserBounds);
    DBG("  ✓ BrowserPanel positioned at: " + browserBounds.toString());
  } else {
    DBG("  ✗ BrowserPanel is NULL!");
  }

  // Right: Scratch Pads + Wingman Console (400px width)
  if (rightSidePanel) {
    auto rightBounds = bounds.removeFromRight(400);
    rightSidePanel->setBounds(rightBounds);
    DBG("  ✓ RightSidePanel positioned at: " + rightBounds.toString());
  } else {
    DBG("  ✗ RightSidePanel is NULL!");
  }

  // Center: Arranger Component (takes remaining space)
  if (arrangerComponent) {
    arrangerComponent->setBounds(bounds);
    DBG("  ✓ ArrangerComponent positioned at: " + bounds.toString());
  } else {
    DBG("  ✗ ArrangerComponent is NULL!");
  }

#else
  // ============================================================================
  // JUCE Fallback Layout
  // ============================================================================

  // Top bar (status)
  auto topBar = bounds.removeFromTop(40);
  statusLabel.setBounds(topBar.removeFromLeft(500).reduced(10, 8));

  auto trackCountArea = topBar.removeFromRight(120);
  trackCountLabel.setBounds(trackCountArea.reduced(10, 8));

  cpuLabel.setBounds(topBar.removeFromRight(150).reduced(10, 8));

  // Bottom bar (transport + audio device)
  auto bottomBar = bounds.removeFromBottom(50);

  auto deviceSection = bottomBar.removeFromLeft(400);
  audioDeviceLabel.setBounds(deviceSection.reduced(10, 12));

  auto importSection = bottomBar.removeFromLeft(140);
  importButton.setBounds(importSection.reduced(10, 8));

  auto keyboardButtonSection = bottomBar.removeFromLeft(160);
  virtualKeyboardButton.setBounds(keyboardButtonSection.reduced(10, 8));

  // Center transport buttons
  auto transportSection = bottomBar.reduced(10, 8);
  int buttonWidth = 100;
  int totalWidth = buttonWidth * 3 + 20;
  int startX = transportSection.getCentreX() - totalWidth / 2;

  playButton.setBounds(startX, transportSection.getY(), buttonWidth,
                       transportSection.getHeight());
  stopButton.setBounds(startX + buttonWidth + 10, transportSection.getY(),
                       buttonWidth, transportSection.getHeight());
  recordButton.setBounds(startX + (buttonWidth + 10) * 2,
                         transportSection.getY(), buttonWidth,
                         transportSection.getHeight());

  // Virtual MIDI Keyboard
  if (virtualKeyboardVisible && midiKeyboard) {
    auto keyboardArea = bounds.removeFromBottom(80);
    midiKeyboard->setBounds(keyboardArea);
  }

  // Mixer panel
  auto mixerArea = bounds.removeFromBottom(220);
  mixerComponent.setBounds(mixerArea);

  // Wingman panel (right)
  if (wingmanPanel) {
    auto wingmanBounds = bounds.removeFromRight(400);
    wingmanPanel->setBounds(wingmanBounds);
  }

  // Instrument Browser (left)
  if (instrumentBrowserPanel) {
    auto browserBounds = bounds.removeFromLeft(300);
    instrumentBrowserPanel->setBounds(browserBounds);
  }

  // Arranger Component (center)
  if (arrangerComponent)
    arrangerComponent->setBounds(bounds);
#endif
}

#ifndef ZENITH_USE_SKIA
void MainComponent::timerCallback() {
  // JUCE fallback timer updates
  double cpuUsage = engine.getCpuUsage();
  cpuLabel.setText("CPU: " + juce::String(cpuUsage, 1) + "%",
                   juce::dontSendNotification);

  auto deviceInfo = engine.getAudioDeviceInfo();
  audioDeviceLabel.setText("Audio: " + deviceInfo, juce::dontSendNotification);

  refreshTrackCountLabel();

  if (virtualKeyboardVisible) {
    juce::MidiBuffer midiMessages;
    midiKeyboardState.processNextMidiBuffer(midiMessages, 0, 16, true);

    for (const auto metadata : midiMessages) {
      auto message = metadata.getMessage();
      engine.handleIncomingMidiMessage(nullptr, message);
    }
  }
}
#endif

//==============================================================================
// C4: Track count monitoring (read-only, dirty-checked)
//==============================================================================

void MainComponent::refreshTrackCountLabel() {
  // Message-thread read only
  const int count = engine.getNumTracks();
  if (count == lastTrackCount_)
    return;

  lastTrackCount_ = count;
  // No heavy formatting, no repaint storm
#ifndef ZENITH_USE_SKIA
  trackCountLabel.setText("Tracks: " + juce::String(count),
                          juce::dontSendNotification);
#endif
}

//==============================================================================
// Integration: Piano roll opener
//==============================================================================

void MainComponent::openPianoRoll(const juce::String &trackId,
                                  const juce::String &clipId) {
  DBG("MainComponent: Opening piano roll for " + trackId + "/" + clipId);

  // Create new piano roll editor window
  // Note: Window deletes itself when closed (see
  // PianoRollEditor::closeButtonPressed)
  new PianoRollEditor(projectState, trackId, clipId);
}

//==============================================================================
// Phase 1: Audio Import
//==============================================================================

void MainComponent::handleImportAudio() {
  // Create file chooser for audio files
  auto chooser = std::make_shared<juce::FileChooser>(
      "Import Audio File", juce::File{},
      "*.wav;*.aiff;*.aif;*.flac;*.mp3;*.ogg");

  // Open file chooser (async)
  auto chooserFlags = juce::FileBrowserComponent::openMode |
                      juce::FileBrowserComponent::canSelectFiles;

  chooser->launchAsync(
      chooserFlags, [this, chooser](const juce::FileChooser &fc) {
        auto file = fc.getResult();
        if (!file.existsAsFile())
          return;

        DBG("Importing audio file: " + file.getFullPathName());

        // Ensure we have at least one track
        if (engine.getNumTracks() == 0) {
          DBG("Creating first track for audio import");
          engine.addTestTracks(1);
        }

        // Get the first track
        const auto &tracks = engine.tracks();
        if (tracks.empty()) {
          DBG("ERROR: Failed to get track after creation");
          return;
        }

        auto *track = tracks[0].get();
        if (track == nullptr) {
          DBG("ERROR: Track is null");
          return;
        }

        // Create a new clip
        auto clip = std::make_unique<zenith::Track::Clip>();
        clip->setType(zenith::Track::Clip::Type::Audio);
        clip->setName(file.getFileNameWithoutExtension());

        // Load audio file through pool (message thread - safe to do I/O)
        auto &pool = engine.getAudioFilePool();
        clip->setAudioFileFromPool(file, pool);

        // Set clip timing: start at position 0, play immediately
        clip->setStartPosition(0);
        clip->setPlaying(true);

        DBG("Clip created: " + clip->getName() +
            ", length: " + juce::String(clip->getLength()) + " samples");

        // Add clip to track
        track->addClip(std::move(clip));

        DBG("Audio import complete! Track now has " +
            juce::String(track->getNumClips()) + " clip(s)");
      });
}

//==============================================================================
// MainWindow Implementation
//==============================================================================

MainWindow::MainWindow(const juce::String &name)
    : DocumentWindow(
          name,
          juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
              juce::ResizableWindow::backgroundColourId),
          DocumentWindow::allButtons) {
  // Create audio engine first
  engine = std::make_unique<Engine>();

  // Create project state
  projectState = std::make_unique<ProjectState>();

  // Phase 13: Create automation synchronizer
  automationSync =
      std::make_unique<TrackAutomationSynchronizer>(*projectState, *engine);

  // Phase 5: Create Wingman command API
  commandAPI = std::make_unique<zenith::CommandAPI>(*engine, *projectState);

  // Phase 7: Create AI bridge client
  aiBridgeClient = std::make_unique<zenith::AIBridgeClient>();

  // Phase 13: Connect project state to engine for automation
  engine->setProjectState(projectState.get());

  // Integration: Create clip synchronizer
  clipSynchronizer = std::make_unique<ClipSynchronizer>(*projectState, *engine);

  // Add some demo tracks for testing (Phase 9 + existing features)
  projectState->addTrack("Audio 1", "audio");
  projectState->addTrack("MIDI 1", "midi");
  projectState->addTrack("Audio 2", "audio");

  // Create main content (Phase 14: Automation + Phase 10: Mixer + Phase 9:
  // Arranger + Wingman AI)
  mainComponent = std::make_unique<MainComponent>(
      *engine, *commandAPI, *aiBridgeClient, *projectState);

  // Create menu bar
  menuBar = std::make_unique<ZenithMenuBar>(*this);
  setMenuBar(menuBar.get());

  // Set up window
  setUsingNativeTitleBar(true);
  setContentOwned(mainComponent.get(), true);

#if JUCE_IOS || JUCE_ANDROID
  setFullScreen(true);
#else
  setResizable(true, true);
  centreWithSize(getWidth(), getHeight());
#endif

  setVisible(true);

  // Initialize audio engine after window is visible
  engine->initialize();

  // Start automation synchronizer (Phase 13)
  automationSync->start(60); // 60 Hz update rate

  DBG("MainWindow created and initialized");
}

MainWindow::~MainWindow() {
  // Clear menu bar first
  setMenuBar(nullptr);
  menuBar.reset();

  // Shutdown audio engine before destroying components
  if (engine)
    engine->shutdown();

  // Clear content
  clearContentComponent();

  DBG("MainWindow destroyed");
}

void MainWindow::closeButtonPressed() {
  // TODO: Check for unsaved changes
  // TODO: Show save dialog if needed

  juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void MainWindow::showAboutDialog() {
  juce::String aboutMessage;
  aboutMessage << "Zenith DAW\n\n";
  aboutMessage << "A professional digital audio workstation\n\n";
  aboutMessage << "Version: 0.1.0\n";
  aboutMessage << "Built with JUCE 8.0.9\n\n";
  aboutMessage << "For documentation and installation instructions, see:\n";
  aboutMessage << "• docs/README.md\n";
  aboutMessage << "• docs/INSTALL_WINDOWS.md";

  juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                                         "About Zenith DAW", aboutMessage,
                                         "OK");
}

//==============================================================================
// ZenithMenuBar Implementation
//==============================================================================

MainWindow::ZenithMenuBar::ZenithMenuBar(MainWindow &mainWindow)
    : owner(mainWindow) {}

juce::StringArray MainWindow::ZenithMenuBar::getMenuBarNames() {
  return {"File", "Help"};
}

juce::PopupMenu
MainWindow::ZenithMenuBar::getMenuForIndex(int topLevelMenuIndex,
                                           const juce::String &menuName) {
  juce::PopupMenu menu;

  if (topLevelMenuIndex == 0) // File menu
  {
#if !(JUCE_IOS || JUCE_ANDROID)
    menu.addItem(quit, "Quit", true, false);
#endif
  } else if (topLevelMenuIndex == 1) // Help menu
  {
    menu.addItem(aboutZenith, "About Zenith DAW...", true, false);
  }

  return menu;
}

void MainWindow::ZenithMenuBar::menuItemSelected(int menuItemID,
                                                 int /*topLevelMenuIndex*/) {
  switch (menuItemID) {
  case aboutZenith:
    owner.showAboutDialog();
    break;

  case quit:
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
    break;

  default:
    break;
  }
}
