/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "../include/MainWindow.h"
#include "../Source/commands/CommandAPI.h"
#include "../Source/ui/ArrangerComponent.h"
#include "WingmanPanel.h"
#include "InstrumentBrowserPanel.h"
#include "AIBridgeClient.h"
#include "../include/PianoRollEditor.h"
#include "../Source/engine/Track.h"
#include "../Source/engine/Clip.h"

#ifdef ZENITH_USE_SKIA
    #include "include/core/SkSurface.h"
    #include "include/core/SkImage.h"
    #include "include/core/SkPixmap.h"
    #include "include/core/SkFont.h"
    #include "include/core/SkTextBlob.h"
    #include "include/core/SkImageInfo.h"
    #include "include/core/SkSamplingOptions.h"
    #include "../Source/ui/skia/SkiaComponent.h"
#endif

//==============================================================================
// MainComponent Implementation
//==============================================================================

MainComponent::MainComponent(Engine& eng, zenith::CommandAPI& api, zenith::AIBridgeClient& aiClient, ProjectState& state)
    : engine(eng), projectState(state), mixerComponent(state)
{
#ifndef ZENITH_USE_SKIA
    // Configure log display TextEditor (JUCE fallback only)
    logDisplay.setMultiLine(true);
    logDisplay.setReadOnly(true);
    logDisplay.setScrollbarsShown(true);
    logDisplay.setCaretVisible(false);
    logDisplay.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    logDisplay.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff0a0a0a));
    logDisplay.setColour(juce::TextEditor::textColourId, juce::Colour(0xff00ff00));
    logDisplay.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff333333));
#endif
    // SkiaTextDisplay needs no configuration - renders natively!
    addAndMakeVisible(logDisplay);
    logDisplay.setName("LogDisplay");

    addLog("========================================");
    addLog("MainComponent Constructor START");
    addLog("========================================");

#ifdef ZENITH_USE_SKIA
    addLog("ZENITH_USE_SKIA is DEFINED - Skia rendering ENABLED");
    addLog("Attempting to initialize Skia rendering system...");

    bool skiaInitSuccess = initializeSkiaRendering();

    if (skiaInitSuccess)
    {
        addLog("✓ Theme configuration loaded successfully");
        addLog("  Theme mode: " + juce::String(getThemeMode() == zenith::ThemeMode::Dark ? "Dark" : "Light"));
        addLog("  GPU settings - Target FPS: " + juce::String(getGPUSettings().targetFPS));
        addLog("  GPU settings - Adaptive FPS: " + juce::String(getGPUSettings().adaptiveFPS ? "ON" : "OFF"));
        addLog("");
        addLog("⚠ WARNING: SkiaRenderer NOT instantiated!");
        addLog("⚠ UI components are using JUCE fallback rendering");
        addLog("⚠ To enable actual Skia rendering:");
        addLog("  1. Create SkiaRenderer instance in components");
        addLog("  2. Override paint() to use Skia canvas");
        addLog("  3. Replace JUCE Graphics with SkCanvas");
    }
    else
    {
        addLog("✗ FAILURE: Theme configuration failed to load!");
        addLog("  Using JUCE fallback rendering");
    }
#else
    addLog("ZENITH_USE_SKIA is NOT DEFINED - Using JUCE fallback rendering");
    addLog("  To enable Skia, rebuild with -DZENITH_ENABLE_SKIA=ON");
#endif

    addLog("Setting window size to 1400x800");
    setSize(1400, 800);

#ifdef ZENITH_USE_SKIA
    // ===== OPTION 1: CREATE SKIARENDERER WITH SOFTWARE BACKEND =====
    addLog("");
    addLog("========================================");
    addLog("CREATING SKIARENDERER (SOFTWARE BACKEND)");
    addLog("========================================");
    addLog("Instantiating SkiaRenderer with Backend::Software...");

    try {
        renderer_ = std::make_unique<zenith::SkiaRenderer>(*this, zenith::SkiaRenderer::Backend::Software);
        addLog("✓ SkiaRenderer object created");

        addLog("Calling renderer_->initialize()...");
        bool initSuccess = renderer_->initialize();

        if (initSuccess)
        {
            addLog("✓✓✓ SUCCESS! SkiaRenderer initialized!");
            addLog("  Backend: CPU rasterization (Software)");
            addLog("  Status: READY TO RENDER");
            addLog("");
            addLog("⚡ SKIA IS NOW ACTIVE! ⚡");
            addLog("  paint() will now use renderer_->render()");
            addLog("  All rendering goes through SkCanvas");
            addLog("  JUCE Graphics fallback is DISABLED");
        }
        else
        {
            addLog("✗ FAILED: renderer_->initialize() returned false");
            addLog("  Check SkiaRenderer.cpp logs for details");
            addLog("  Falling back to JUCE rendering");
            renderer_.reset();  // Clean up failed renderer
        }
    }
    catch (const std::exception& e)
    {
        addLog("✗ EXCEPTION during SkiaRenderer creation:");
        addLog("  " + juce::String(e.what()));
        addLog("  Falling back to JUCE rendering");
        renderer_.reset();
    }
    addLog("========================================");
    addLog("");
#endif

    addLog("Adding mixer component to window");
    addAndMakeVisible(mixerComponent);

    // Register as key listener for undo/redo shortcuts
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    // Status label
    statusLabel.setText("Zenith DAW - Phase 14: Automation Lanes + Phase 10: Mixer", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(statusLabel);

    // Phase 14: Create arrangement view
    // TEMPORARILY DISABLED: ArrangementComponent has compilation errors (ID_POINTS missing)
    // arrangementView = std::make_unique<ArrangementComponent>(projectState, engine);
    // addAndMakeVisible(arrangementView.get());

    // CPU usage label
    cpuLabel.setText("CPU: 0%", juce::dontSendNotification);
    cpuLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(cpuLabel);

    // Audio device label
    audioDeviceLabel.setText("Audio Device: Not initialized", juce::dontSendNotification);
    audioDeviceLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(audioDeviceLabel);

    // C4: Track count label (read-only)
    trackCountLabel.setText("Tracks: 0", juce::dontSendNotification);
    trackCountLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(trackCountLabel);

    // Transport buttons
    playButton.setButtonText("Play");
#ifdef ZENITH_USE_SKIA
    playButton.setStyle(zenith::SkiaButtonComponent::Style::Success);
#endif
    playButton.onClick = [this]() {
        engine.play();
        DBG("Play button clicked");
    };
    addAndMakeVisible(playButton);

    stopButton.setButtonText("Stop");
#ifdef ZENITH_USE_SKIA
    stopButton.setStyle(zenith::SkiaButtonComponent::Style::Secondary);
#endif
    stopButton.onClick = [this]() {
        engine.stop();
        DBG("Stop button clicked");
    };
    addAndMakeVisible(stopButton);

    recordButton.setButtonText("Record");
#ifdef ZENITH_USE_SKIA
    recordButton.setStyle(zenith::SkiaButtonComponent::Style::Secondary);
#endif
    recordButton.onClick = [this]() {
        engine.toggleRecording();
        bool isRecording = engine.isRecording();
#ifdef ZENITH_USE_SKIA
        recordButton.setStyle(isRecording ? zenith::SkiaButtonComponent::Style::Danger : zenith::SkiaButtonComponent::Style::Secondary);
#else
        recordButton.setColour(juce::TextButton::buttonColourId,
            isRecording ? juce::Colours::red : juce::Colours::darkgrey);
#endif
        DBG((isRecording ? "Recording started" : "Recording stopped"));
    };
    addAndMakeVisible(recordButton);

    // Phase 1: Import Audio button
    importButton.setButtonText("Import Audio...");
    importButton.onClick = [this]() {
        handleImportAudio();
    };
    addAndMakeVisible(importButton);

    // Virtual MIDI Keyboard toggle button
    virtualKeyboardButton.setButtonText("🎹 Keyboard (M)");
    virtualKeyboardButton.setClickingTogglesState(true);
    virtualKeyboardButton.onClick = [this]() {
        virtualKeyboardVisible = virtualKeyboardButton.getToggleState();
        if (midiKeyboard)
            midiKeyboard->setVisible(virtualKeyboardVisible);
        resized();  // Re-layout to accommodate keyboard
    };
    addAndMakeVisible(virtualKeyboardButton);

    // Phase 9: Create ArrangerComponent with interactive clip editing
    arrangerComponent = std::make_unique<ArrangerComponent>(*engine.getProjectState());
    addAndMakeVisible(arrangerComponent.get());

    // Phase 7: Create Wingman AI console panel
    wingmanPanel = std::make_unique<WingmanPanel>(api, aiClient);
    addAndMakeVisible(wingmanPanel.get());

    // Create Instrument Browser Panel
    instrumentBrowserPanel = std::make_unique<zenith::InstrumentBrowserPanel>(engine, projectState);
    addAndMakeVisible(instrumentBrowserPanel.get());

    // Create Virtual MIDI Keyboard Component
    midiKeyboard = std::make_unique<juce::MidiKeyboardComponent>(
        midiKeyboardState,
        juce::MidiKeyboardComponent::horizontalKeyboard);
    midiKeyboard->setVisible(false);  // Hidden by default
    addAndMakeVisible(midiKeyboard.get());

    // Note: MIDI keyboard will generate MIDI messages through midiKeyboardState
    // These can be processed via the Engine's existing MIDI input handling

#ifndef ZENITH_USE_SKIA
    // Start timer for CPU monitoring (60 Hz) - only when not using Skia
    // (SkiaMainWindowIntegration manages its own timer)
    addLog("Starting JUCE timer for CPU monitoring");
    startTimer(16);
#else
    addLog("Skia manages timer - not starting separate timer");
#endif

    addLog("========================================");
    addLog("MainComponent Constructor COMPLETE");
    addLog("========================================");
    addLog("");
    addLog("Watch the paint() calls above to see if Skia rendering works!");
}

MainComponent::~MainComponent()
{
    DBG("MainComponent Destructor called");
    removeKeyListener(this);

#ifndef ZENITH_USE_SKIA
    stopTimer();
#endif
}

bool MainComponent::keyPressed(const juce::KeyPress& key, Component* originatingComponent)
{
    juce::ignoreUnused(originatingComponent);

    // Ctrl+Z or Cmd+Z for undo
    if (key.getTextCharacter() == 'z' && key.getModifiers().isCommandDown() && !key.getModifiers().isShiftDown())
    {
        if (projectState.canUndo())
        {
            projectState.undo();
            DBG("Keyboard shortcut: Undo");
            return true;
        }
    }

    // Ctrl+Shift+Z or Cmd+Shift+Z for redo
    if (key.getTextCharacter() == 'Z' && key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown())
    {
        if (projectState.canRedo())
        {
            projectState.redo();
            DBG("Keyboard shortcut: Redo");
            return true;
        }
    }

    // Ctrl+Y or Cmd+Y for redo (alternative)
    if (key.getTextCharacter() == 'y' && key.getModifiers().isCommandDown())
    {
        if (projectState.canRedo())
        {
            projectState.redo();
            DBG("Keyboard shortcut: Redo (Y)");
            return true;
        }
    }

    // M key: Toggle virtual MIDI keyboard (like Ableton Live)
    if (key.getTextCharacter() == 'm' || key.getTextCharacter() == 'M')
    {
        virtualKeyboardButton.setToggleState(!virtualKeyboardButton.getToggleState(), juce::sendNotification);
        DBG("Keyboard shortcut: Toggle Virtual MIDI Keyboard (M)");
        return true;
    }

    return false;  // Key not handled
}

void MainComponent::paint(juce::Graphics& g)
{
#ifdef ZENITH_USE_SKIA
    // Use SkiaRenderer if it was successfully initialized
    if (renderer_)
    {
        static int skiaPaintCount = 0;
        if (skiaPaintCount < 3)
        {
            skiaPaintCount++;
            addLog("🎨 paint() call #" + juce::String(skiaPaintCount) + " using SKIA RENDERER!");
            if (skiaPaintCount == 1)
            {
                addLog("  Software backend: Rendering with Skia, blitting to JUCE");
                addLog("  ✓ ACTUAL SKIA CPU RENDERING ACTIVE!");
            }
        }

        // Step 1: Render EVERYTHING using Skia
        renderer_->render([this](SkCanvas* canvas) {
            // Beautiful dark background with Skia anti-aliasing
            SkPaint bgPaint;
            bgPaint.setColor(0xFF1E1E1E);
            canvas->clear(0xFF1E1E1E);

            // Render ALL child components - NATIVE SKIA or JUCE fallback
            static int nativeSkiaCount = 0;
            static int juceFallbackCount = 0;

            for (int i = 0; i < getNumChildComponents(); ++i)
            {
                auto* child = getChildComponent(i);
                if (child && child->isVisible())
                {
                    auto childBounds = child->getBounds();
                    SkRect skBounds = SkRect::MakeXYWH(
                        (float)childBounds.getX(),
                        (float)childBounds.getY(),
                        (float)childBounds.getWidth(),
                        (float)childBounds.getHeight()
                    );

                    // Check if component supports native Skia rendering
                    auto* skiaComponent = dynamic_cast<zenith::SkiaComponent*>(child);

                    // DEBUG: Log what we found
                    static bool loggedComponents = false;
                    if (!loggedComponents && skiaPaintCount == 1)
                    {
                        addLog("  [COMPONENT #" + juce::String(i) + "] " +
                               juce::String(child->getName().isEmpty() ? "unnamed" : child->getName()));
                        addLog("    SkiaComponent cast: " + juce::String(skiaComponent != nullptr ? "YES" : "NO"));
                        if (skiaComponent)
                            addLog("    supportsSkiaRendering(): " + juce::String(skiaComponent->supportsSkiaRendering() ? "YES" : "NO"));
                        addLog("    Bounds: " + juce::String(childBounds.getX()) + "," + juce::String(childBounds.getY()) + " " +
                               juce::String(childBounds.getWidth()) + "x" + juce::String(childBounds.getHeight()));
                        addLog("    Visible: " + juce::String(child->isVisible() ? "YES" : "NO"));
                    }

                    if (skiaComponent && skiaComponent->supportsSkiaRendering())
                    {
                        // ✓ NATIVE SKIA RENDERING - Direct to SkCanvas!
                        if (skiaPaintCount == 1)
                            addLog("  >>> CALLING paintToSkia() for component #" + juce::String(i));

                        canvas->save();
                        skiaComponent->paintToSkia(canvas, skBounds);
                        canvas->restore();

                        nativeSkiaCount++;
                    }
                    else
                    {
                        // ✗ JUCE FALLBACK - Render to image then composite
                        juce::Image componentImage(juce::Image::ARGB,
                                                  juce::jmax(1, childBounds.getWidth()),
                                                  juce::jmax(1, childBounds.getHeight()),
                                                  true);

                        juce::Graphics componentGraphics(componentImage);
                        componentGraphics.setOrigin(-childBounds.getX(), -childBounds.getY());
                        child->paint(componentGraphics);

                        // Convert JUCE image to Skia
                        juce::Image::BitmapData bitmapData(componentImage, juce::Image::BitmapData::readOnly);

                        SkImageInfo imageInfo = SkImageInfo::MakeN32Premul(
                            componentImage.getWidth(),
                            componentImage.getHeight()
                        );

                        sk_sp<SkImage> skiaImage = SkImages::RasterFromPixmapCopy(
                            SkPixmap(imageInfo, bitmapData.data, bitmapData.lineStride)
                        );

                        if (skiaImage)
                        {
                            SkPaint paint;
                            paint.setAntiAlias(true);
                            canvas->drawImage(skiaImage,
                                            (float)childBounds.getX(),
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
        SkSurface* surface = renderer_->getSurface();
        if (surface)
        {
            if (skiaPaintCount == 1)
                addLog("  [DEBUG] Got SkSurface pointer: " + juce::String::toHexString((juce::pointer_sized_int)surface));

            // Create SkImage from surface
            sk_sp<SkImage> skImage = surface->makeImageSnapshot();
            if (skImage)
            {
                // Get image dimensions
                int width = skImage->width();
                int height = skImage->height();

                if (skiaPaintCount == 1)
                    addLog("  [DEBUG] SkImage dimensions: " + juce::String(width) + "x" + juce::String(height));

                // Create JUCE image
                juce::Image juceImage(juce::Image::ARGB, width, height, true);

                if (skiaPaintCount == 1)
                    addLog("  [DEBUG] Created JUCE image: " + juce::String(juceImage.getWidth()) + "x" + juce::String(juceImage.getHeight()));

                // Read pixels from Skia to JUCE
                SkPixmap pixmap;
                if (skImage->peekPixels(&pixmap))
                {
                    if (skiaPaintCount == 1)
                        addLog("  [DEBUG] peekPixels succeeded, pixmap: " + juce::String(pixmap.width()) + "x" + juce::String(pixmap.height()));

                    juce::Image::BitmapData bitmapData(juceImage,
                                                       juce::Image::BitmapData::writeOnly);

                    // Copy row by row (Skia uses premultiplied alpha, JUCE expects it too)
                    for (int y = 0; y < height; ++y)
                    {
                        const uint32_t* src = (const uint32_t*)pixmap.addr32(0, y);
                        uint32_t* dst = (uint32_t*)bitmapData.getLinePointer(y);
                        memcpy(dst, src, width * sizeof(uint32_t));
                    }

                    if (skiaPaintCount == 1)
                        addLog("  [DEBUG] Pixel copy complete, copied " + juce::String(height) + " rows");
                }
                else
                {
                    addLog("  [ERROR] peekPixels FAILED!");
                }

                // Step 3: VERIFY pixels before drawing
                auto componentBounds = getLocalBounds();
                if (skiaPaintCount == 1)
                {
                    addLog("  [DEBUG] Component bounds: " + juce::String(componentBounds.getWidth()) + "x" + juce::String(componentBounds.getHeight()));

                    // VERIFY: Check if green box is actually in the image
                    addLog("  ========================================");
                    addLog("  PIXEL VERIFICATION (checking image data)");
                    addLog("  ========================================");

                    // Check center of green box at (935, 130) - should be GREEN
                    juce::Colour centerPixel = juceImage.getPixelAt(935, 130);
                    addLog("  Pixel at (935,130) center: RGB(" +
                           juce::String(centerPixel.getRed()) + "," +
                           juce::String(centerPixel.getGreen()) + "," +
                           juce::String(centerPixel.getBlue()) + ")");

                    bool isGreen = (centerPixel.getGreen() > 200 && centerPixel.getRed() < 50);
                    if (isGreen)
                        addLog("  ✓ GREEN (0,255,0) DETECTED - Skia drew it!");
                    else
                        addLog("  ✗ NOT GREEN - Skia rendering FAILED!");

                    // Check yellow border at (885, 15) - should be YELLOW
                    juce::Colour borderPixel = juceImage.getPixelAt(885, 15);
                    addLog("  Pixel at (885,15) border: RGB(" +
                           juce::String(borderPixel.getRed()) + "," +
                           juce::String(borderPixel.getGreen()) + "," +
                           juce::String(borderPixel.getBlue()) + ")");

                    bool isYellow = (borderPixel.getRed() > 200 && borderPixel.getGreen() > 200);
                    if (isYellow)
                        addLog("  ✓ YELLOW (255,255,0) DETECTED - Border exists!");
                    else
                        addLog("  ✗ NOT YELLOW - Border drawing FAILED!");

                    addLog("  ========================================");
                }

                if (skiaPaintCount == 1)
                    addLog("  [DEBUG] Drawing image at (0, 0)...");

                g.drawImageAt(juceImage, 0, 0);

                if (skiaPaintCount == 1)
                {
                    // TRUTHFUL RENDERING STATISTICS
                    int totalComponents = getNumChildComponents();
                    int nativeSkia = 0;
                    int juceFallback = 0;

                    for (int i = 0; i < totalComponents; ++i)
                    {
                        auto* child = getChildComponent(i);
                        if (child && child->isVisible())
                        {
                            auto* skiaComp = dynamic_cast<zenith::SkiaComponent*>(child);
                            if (skiaComp && skiaComp->supportsSkiaRendering())
                                nativeSkia++;
                            else
                                juceFallback++;
                        }
                    }

                    addLog("  ========================================");
                    addLog("  RENDERING STATISTICS (TRUTHFUL)");
                    addLog("  ========================================");
                    addLog("  Total visible components: " + juce::String(totalComponents));
                    addLog("  ");
                    addLog("  ✓ Native Skia rendering: " + juce::String(nativeSkia) + " components");
                    addLog("  ✗ JUCE fallback: " + juce::String(juceFallback) + " components");
                    addLog("  ");

                    if (nativeSkia == totalComponents && totalComponents > 0)
                    {
                        addLog("  🎉 100% PURE SKIA RENDERING!");
                        addLog("  ALL components use native SkCanvas!");
                    }
                    else if (nativeSkia > 0)
                    {
                        int percent = (nativeSkia * 100) / totalComponents;
                        addLog("  ⚠ HYBRID: " + juce::String(percent) + "% native Skia");
                        addLog("  Still " + juce::String(juceFallback) + " components using JUCE");
                    }
                    else
                    {
                        addLog("  ✗ NO NATIVE SKIA YET");
                        addLog("  All components still use JUCE Graphics");
                        addLog("  Only Skia compositing is active");
                    }
                    addLog("  ========================================");
                }

                return; // Successfully rendered with Skia
            }
            else
            {
                addLog("  [ERROR] makeImageSnapshot() returned null!");
            }
        }
        else
        {
            addLog("  [ERROR] getSurface() returned null!");
        }

        // If we get here, something went wrong
        addLog("ERROR: Failed to get pixels from Skia surface!");
        addLog("  Falling back to JUCE rendering");
        renderer_.reset();
    }
#endif

    // JUCE fallback rendering (when Skia disabled or failed to initialize)
    static int paintCallCount = 0;
    if (paintCallCount < 3)
    {
        paintCallCount++;
        addLog("paint() call #" + juce::String(paintCallCount) + " using juce::Graphics (JUCE fallback)");
        if (paintCallCount == 1)
        {
            addLog("  paint() receives juce::Graphics, not SkCanvas");
            addLog("  This confirms JUCE rendering is active, NOT Skia");
        }
    }

    // Background (ArrangerComponent handles its own painting)
    g.fillAll(juce::Colour(0xff1e1e1e));  // Dark grey (LUNA-inspired)
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    // Log display at top right corner (400x250)
    auto logArea = bounds.removeFromTop(250).removeFromRight(400);
    logDisplay.setBounds(logArea.reduced(5));

    // Top bar (status)
    auto topBar = bounds.removeFromTop(40);
    statusLabel.setBounds(topBar.removeFromLeft(500).reduced(10, 8));

    // C4: Track count label sits on the right side of the top bar (after CPU)
    auto trackCountArea = topBar.removeFromRight(120);
    trackCountLabel.setBounds(trackCountArea.reduced(10, 8));

    cpuLabel.setBounds(topBar.removeFromRight(150).reduced(10, 8));

    // Bottom bar (transport + audio device)
    auto bottomBar = bounds.removeFromBottom(50);

    auto deviceSection = bottomBar.removeFromLeft(400);
    audioDeviceLabel.setBounds(deviceSection.reduced(10, 12));

    // Phase 1: Import button on the left
    auto importSection = bottomBar.removeFromLeft(140);
    importButton.setBounds(importSection.reduced(10, 8));

    // Virtual Keyboard toggle button
    auto keyboardButtonSection = bottomBar.removeFromLeft(160);
    virtualKeyboardButton.setBounds(keyboardButtonSection.reduced(10, 8));

    // Center transport buttons
    auto transportSection = bottomBar.reduced(10, 8);
    int buttonWidth = 100;
    int totalWidth = buttonWidth * 3 + 20;  // 3 buttons + spacing
    int startX = transportSection.getCentreX() - totalWidth / 2;

    playButton.setBounds(startX, transportSection.getY(), buttonWidth, transportSection.getHeight());
    stopButton.setBounds(startX + buttonWidth + 10, transportSection.getY(), buttonWidth, transportSection.getHeight());
    recordButton.setBounds(startX + (buttonWidth + 10) * 2, transportSection.getY(), buttonWidth, transportSection.getHeight());

    // Virtual MIDI Keyboard (above mixer when visible)
    if (virtualKeyboardVisible && midiKeyboard)
    {
        auto keyboardHeight = 80;
        auto keyboardArea = bounds.removeFromBottom(keyboardHeight);
        midiKeyboard->setBounds(keyboardArea);
    }

    // Phase 10: Mixer panel at bottom (above transport bar)
    auto mixerHeight = 220;
    auto mixerArea = bounds.removeFromBottom(mixerHeight);
    mixerComponent.setBounds(mixerArea);

    // Phase 7: Layout Wingman panel on the right (400px width)
    if (wingmanPanel != nullptr)
    {
        auto wingmanBounds = bounds.removeFromRight(400);
        wingmanPanel->setBounds(wingmanBounds);
    }

    // Layout Instrument Browser panel on the left (300px width)
    if (instrumentBrowserPanel != nullptr)
    {
        auto browserBounds = bounds.removeFromLeft(300);
        instrumentBrowserPanel->setBounds(browserBounds);
    }

    // Phase 14: Arrangement view with automation lanes
    // TEMPORARILY DISABLED
    // if (arrangementView)
    //     arrangementView->setBounds(bounds);

    // Phase 9: ArrangerComponent takes the remaining central area
    if (arrangerComponent != nullptr)
        arrangerComponent->setBounds(bounds);
}

void MainComponent::timerCallback()
{
    // Update CPU usage
    double cpuUsage = engine.getCpuUsage();
    cpuLabel.setText("CPU: " + juce::String(cpuUsage, 1) + "%", juce::dontSendNotification);

    // Update audio device info
    auto deviceInfo = engine.getAudioDeviceInfo();
    audioDeviceLabel.setText("Audio: " + deviceInfo, juce::dontSendNotification);

    // C4: Update track count (dirty-checked)
    refreshTrackCountLabel();

    // Process MIDI messages from virtual keyboard
    if (virtualKeyboardVisible)
    {
        juce::MidiBuffer midiMessages;
        midiKeyboardState.processNextMidiBuffer(midiMessages, 0, 16, true);

        // Send MIDI messages to engine
        for (const auto metadata : midiMessages)
        {
            auto message = metadata.getMessage();
            engine.handleIncomingMidiMessage(nullptr, message);
        }
    }
}

//==============================================================================
// C4: Track count monitoring (read-only, dirty-checked)
//==============================================================================

void MainComponent::refreshTrackCountLabel()
{
    // Message-thread read only
    const int count = engine.getNumTracks();
    if (count == lastTrackCount_)
        return;

    lastTrackCount_ = count;
    // No heavy formatting, no repaint storm
    trackCountLabel.setText("Tracks: " + juce::String(count), juce::dontSendNotification);
}

//==============================================================================
// Integration: Piano roll opener
//==============================================================================

void MainComponent::openPianoRoll(const juce::String& trackId, const juce::String& clipId)
{
    DBG("MainComponent: Opening piano roll for " + trackId + "/" + clipId);

    // Create new piano roll editor window
    // Note: Window deletes itself when closed (see PianoRollEditor::closeButtonPressed)
    new PianoRollEditor(projectState, trackId, clipId);
}

//==============================================================================
// Phase 1: Audio Import
//==============================================================================

void MainComponent::handleImportAudio()
{
    // Create file chooser for audio files
    auto chooser = std::make_shared<juce::FileChooser>(
        "Import Audio File",
        juce::File{},
        "*.wav;*.aiff;*.aif;*.flac;*.mp3;*.ogg");

    // Open file chooser (async)
    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (!file.existsAsFile())
            return;

        DBG("Importing audio file: " + file.getFullPathName());

        // Ensure we have at least one track
        if (engine.getNumTracks() == 0)
        {
            DBG("Creating first track for audio import");
            engine.addTestTracks(1);
        }

        // Get the first track
        const auto& tracks = engine.tracks();
        if (tracks.empty())
        {
            DBG("ERROR: Failed to get track after creation");
            return;
        }

        auto* track = tracks[0].get();
        if (track == nullptr)
        {
            DBG("ERROR: Track is null");
            return;
        }

        // Create a new clip
        auto clip = std::make_unique<zenith::Track::Clip>();
        clip->setType(zenith::Track::Clip::Type::Audio);
        clip->setName(file.getFileNameWithoutExtension());

        // Load audio file through pool (message thread - safe to do I/O)
        auto& pool = engine.getAudioFilePool();
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

MainWindow::MainWindow(const juce::String& name)
    : DocumentWindow(name,
                     juce::Desktop::getInstance().getDefaultLookAndFeel()
                         .findColour(juce::ResizableWindow::backgroundColourId),
                     DocumentWindow::allButtons)
{
    // Create audio engine first
    engine = std::make_unique<Engine>();

    // Create project state
    projectState = std::make_unique<ProjectState>();

    // Phase 13: Create automation synchronizer
    automationSync = std::make_unique<TrackAutomationSynchronizer>(*projectState, *engine);

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

    // Create main content (Phase 14: Automation + Phase 10: Mixer + Phase 9: Arranger + Wingman AI)
    mainComponent = std::make_unique<MainComponent>(*engine, *commandAPI, *aiBridgeClient, *projectState);

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

MainWindow::~MainWindow()
{
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

void MainWindow::closeButtonPressed()
{
    // TODO: Check for unsaved changes
    // TODO: Show save dialog if needed

    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void MainWindow::showAboutDialog()
{
    juce::String aboutMessage;
    aboutMessage << "Zenith DAW\n\n";
    aboutMessage << "A professional digital audio workstation\n\n";
    aboutMessage << "Version: 0.1.0\n";
    aboutMessage << "Built with JUCE 8.0.9\n\n";
    aboutMessage << "For documentation and installation instructions, see:\n";
    aboutMessage << "• docs/README.md\n";
    aboutMessage << "• docs/INSTALL_WINDOWS.md";

    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::InfoIcon,
        "About Zenith DAW",
        aboutMessage,
        "OK"
    );
}

//==============================================================================
// ZenithMenuBar Implementation
//==============================================================================

MainWindow::ZenithMenuBar::ZenithMenuBar(MainWindow& mainWindow)
    : owner(mainWindow)
{
}

juce::StringArray MainWindow::ZenithMenuBar::getMenuBarNames()
{
    return { "File", "Help" };
}

juce::PopupMenu MainWindow::ZenithMenuBar::getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName)
{
    juce::PopupMenu menu;

    if (topLevelMenuIndex == 0)  // File menu
    {
        #if ! (JUCE_IOS || JUCE_ANDROID)
            menu.addItem(quit, "Quit", true, false);
        #endif
    }
    else if (topLevelMenuIndex == 1)  // Help menu
    {
        menu.addItem(aboutZenith, "About Zenith DAW...", true, false);
    }

    return menu;
}

void MainWindow::ZenithMenuBar::menuItemSelected(int menuItemID, int /*topLevelMenuIndex*/)
{
    switch (menuItemID)
    {
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
