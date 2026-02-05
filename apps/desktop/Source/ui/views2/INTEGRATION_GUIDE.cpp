/*
  ==============================================================================

    ZENITH UI INTEGRATION GUIDE
    ===========================
    
    This file documents how to integrate the new Skia-based UI system into
    Zenith DAW. The new UI is in ui/views2/ and provides a complete replacement
    for the old UI with:
    
    - Tab/Shift+Tab view switching
    - Modern glassmorphism aesthetics
    - 60fps Skia rendering
    - AI Jam view for real-time AI collaboration
    
  ==============================================================================
    
    QUICK START
    -----------
    
    1. Include the master header:
    
        #include "ui/views2/ZenithUI.h"
    
    2. Create the main layout in your MainComponent:
    
        class MainComponent : public juce::Component {
        public:
            MainComponent() {
                mainLayout_ = std::make_unique<zenith::ui::ZenithMainLayout>();
                addAndMakeVisible(mainLayout_.get());
            }
            
            void resized() override {
                mainLayout_->setBounds(getLocalBounds());
            }
            
        private:
            std::unique_ptr<zenith::ui::ZenithMainLayout> mainLayout_;
        };
    
    3. Connect to your audio engine:
    
        // In your timer callback or audio callback:
        mainLayout_->setPosition(audioEngine.getPositionInBeats());
        mainLayout_->setTempo(audioEngine.getTempo());
        mainLayout_->setCPULoad(audioEngine.getCPULoad());
    
  ==============================================================================
    
    VIEW SYSTEM
    -----------
    
    There are three main views:
    
    1. ARRANGEMENT VIEW (SkiaArrangementView)
       - Traditional timeline with tracks stacked vertically
       - Clips, automation lanes, playhead
       - Navigate with Tab key
    
    2. SESSION VIEW (SkiaSessionView)  
       - Ableton-style clip launcher grid
       - Scene triggers, mixer strips
       - Navigate with Tab key
    
    3. AI JAM VIEW (SkiaAIJamView)
       - Overlay on top of Arrangement/Session
       - AI-powered music generation
       - Navigate with Shift+Tab
    
    Keyboard shortcuts:
    - Tab: Toggle between Arrangement and Session
    - Shift+Tab: Toggle AI Jam overlay
    - Cmd+1: Go to Arrangement
    - Cmd+2: Go to Session
    - Cmd+3: Go to AI Jam
    - Space: Play/Stop
    - R: Toggle record arm
    - L: Toggle loop
    
  ==============================================================================
    
    ARCHITECTURE
    ------------
    
    ```
    ZenithMainLayout (root)
    ├── SkiaTransportBar (48px, top)
    │   ├── Transport controls (play/stop/record/loop/metro)
    │   ├── Tempo + time signature
    │   ├── Position display + scrubber
    │   └── View toggle pills (Arr/Ses/Jam)
    │
    ├── ViewSwitcher (fills middle)
    │   ├── SkiaArrangementView (when active)
    │   │   ├── Track headers (left, 180px)
    │   │   └── Timeline area (right, fills)
    │   │       ├── Grid lines
    │   │       ├── Clips
    │   │       ├── Playhead
    │   │       └── Selection rectangle
    │   │
    │   ├── SkiaSessionView (when active)
    │   │   ├── Track headers (left, 120px)
    │   │   ├── Clip grid (center)
    │   │   │   └── ClipSlots with states:
    │   │   │       Empty, Stopped, Playing, Queued, Recording
    │   │   ├── Scene launcher (right, 60px)
    │   │   └── Mixer strips (bottom, 80px)
    │   │
    │   └── SkiaAIJamView (overlay, always available)
    │       ├── Glassmorphic background (blur 20px)
    │       ├── Prompt bar (top)
    │       ├── Stem cards (4x: Drums/Bass/Chords/Melody)
    │       ├── Variation buttons (A/B/C/D)
    │       ├── Quick actions grid
    │       ├── Loop bar with BPM
    │       └── Chat panel
    │
    └── Status bar (24px, bottom)
        ├── CPU load
        ├── MIDI activity
        ├── Audio latency
        └── Zoom level
    ```
    
  ==============================================================================
    
    CONNECTING TO YOUR DATA MODEL
    -----------------------------
    
    The UI components expose methods to update their state. You'll need to
    connect these to your actual data model.
    
    TRACKS AND CLIPS (Arrangement View):
    
        auto* arranger = mainLayout_->getViewSwitcher()->getArrangementView();
        
        // Add a track
        arranger->addTrack("My Track", TrackType::Audio);
        
        // Add a clip (you'll need to implement this method)
        // arranger->addClip(trackIndex, startBeat, lengthBeats, clipData);
        
        // Update playhead
        arranger->setPlayheadPosition(currentBeat);
    
    CLIP SLOTS (Session View):
    
        auto* session = mainLayout_->getViewSwitcher()->getSessionView();
        
        // Update a clip slot state
        session->setClipSlotState(trackIndex, sceneIndex, ClipSlotState::Playing);
        
        // Update mixer channel values
        session->setChannelVolume(trackIndex, 0.8f);
        session->setChannelPan(trackIndex, 0.0f);
    
    AI JAM:
    
        auto* aiJam = mainLayout_->getViewSwitcher()->getAIJamView();
        
        // Update stem states
        aiJam->setStemEnabled(StemType::Drums, true);
        aiJam->setStemMuted(StemType::Bass, false);
        
        // Add AI response to chat
        aiJam->addChatMessage("AI", "Generated a new groove!");
        
        // Set current prompt
        aiJam->setPrompt("Create a funky bass line in Em");
    
  ==============================================================================
    
    ANIMATION SYSTEM
    ----------------
    
    All views are animated at 60fps using the AnimationCoordinator. The views
    implement onAnimationTick(float deltaMs) for smooth updates:
    
    - Playhead position updates
    - Playing clip indicators (breathing glow)
    - Queued clip indicators (blinking)
    - Record button pulse
    - View transition animations
    
    The AnimationCoordinator should already be set up in your main app. If not:
    
        #include "ui/framework/AnimationCoordinator.h"
        
        // In your MainComponent or Application
        zenith::animation::AnimationCoordinator coordinator;
        coordinator.startAnimating(60); // 60fps
        
        // Register your layout for animation
        coordinator.addAnimatable(mainLayout_.get());
    
  ==============================================================================
    
    THEME CUSTOMIZATION
    -------------------
    
    The UI uses ZenithTheme for colors, typography, and spacing. To customize:
    
        #include "ui/design-system/ZenithTheme.h"
        
        // Colors are defined as constants, but you can override the theme
        // by modifying ZenithTheme.h or creating a runtime theme system.
        
        Key color tokens:
        - bg_00 through bg_04: Background layers
        - accent_primary: Main accent color (default: #3b82f6 blue)
        - accent_secondary: Secondary accent (default: #8b5cf6 purple)
        - text_primary, text_secondary, text_tertiary: Text hierarchy
        - success, warning, error, info: Semantic colors
        - record_red, play_green, solo_yellow, mute_slate: Transport colors
    
  ==============================================================================
    
    PERFORMANCE NOTES
    -----------------
    
    1. DIRTY RECT OPTIMIZATION
       Views use markDirtyRect() for partial repainting. Only the changed
       region is redrawn, not the entire view.
    
    2. SKIA GPU ACCELERATION
       Ensure your SkiaRenderer is using GPU backend (Metal on macOS,
       Vulkan/OpenGL on Linux/Windows) for best performance.

    3. LARGE TRACK COUNTS
       For projects with 100+ tracks, consider implementing virtualization
       in the arrangement view (only render visible tracks).

    4. WAVEFORM RENDERING
       Clip waveforms are fully implemented with:
       - Real audio file loading via AudioFormatManager
       - Async background computation using ThreadPool
       - In-memory LRU cache (max 100 entries)
       - Persistent disk cache in user app data directory
       - Automatic cache invalidation on file modification

       For GPU optimization in the future, consider caching in GPU textures.

  ==============================================================================
    
    FILES REFERENCE
    ---------------
    
    ui/views2/
    ├── ZenithUI.h              - Master include header
    ├── ZenithMainLayout.h/cpp  - Root container component
    ├── core/
    │   └── ViewSwitcher.h/cpp  - Tab/Shift+Tab navigation
    ├── arranger/
    │   └── SkiaArrangementView.h/cpp  - Timeline view
    ├── session/
    │   └── SkiaSessionView.h/cpp      - Clip launcher grid
    ├── ai-jam/
    │   └── SkiaAIJamView.h/cpp        - AI collaboration overlay
    └── common/
        └── SkiaTransportBar.h/cpp     - Transport controls
    
  ==============================================================================
*/

// This is a documentation file - no code to compile
