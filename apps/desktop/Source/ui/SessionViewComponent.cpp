/*
  ==============================================================================

    SessionViewComponent.cpp
    Created: 2025-12-08
    Author:  Zenith DAW

    Real implementation of Session View (Clip Launcher).
    - Visualizes the Session Grid state from ProjectState.
    - Handles mouse interactions to trigger clips/scenes.
    - Updates ProjectState properties (which would drive the Engine).

  ==============================================================================
*/

#include "../../include/ui/SessionViewComponent.h"
#include "skia/ZenithDesignSystem.h"
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRRect.h>
#include <include/core/SkPath.h>

namespace zenith {

// Property identifier for session clips
static const juce::Identifier PROP_SCENE_INDEX("sceneIndex");
static const juce::Identifier PROP_IS_PLAYING("isSessionPlaying");
static const juce::Identifier PROP_IS_QUEUED("isSessionQueued");

SessionViewComponent::SessionViewComponent(ProjectState& state, Engine& engine)
    : projectState_(state), engine_(engine)
{
    projectState_.getState().addListener(this);
}

SessionViewComponent::~SessionViewComponent()
{
    projectState_.getState().removeListener(this);
}

void SessionViewComponent::drawSkia(SkCanvas* canvas)
{
    // Background
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetRGB(20, 20, 25));
    canvas->drawRect(SkRect::MakeWH(getWidth(), getHeight()), bgPaint);

    drawGrid(canvas);
}

void SessionViewComponent::drawGrid(SkCanvas* canvas)
{
    // Get tracks
    auto tracksNode = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
    int numTracks = tracksNode.getNumChildren();

    // Offset for Scene Headers
    float startX = SCENE_HEADER_WIDTH;
    float startY = GAP;

    // Draw Scene Headers (Master Triggers)
    for (int scene = 0; scene < numScenes_; ++scene) {
        float y = startY + scene * (ROW_HEIGHT + GAP);
        drawSceneHeader(canvas, SkRect::MakeXYWH(GAP, y, SCENE_HEADER_WIDTH - GAP, ROW_HEIGHT), scene);
    }

    // Draw Track Slots
    for (int t = 0; t < numTracks; ++t) {
        float x = startX + t * (COLUMN_WIDTH + GAP);

        for (int scene = 0; scene < numScenes_; ++scene) {
            float y = startY + scene * (ROW_HEIGHT + GAP);
            SkRect slotRect = SkRect::MakeXYWH(x, y, COLUMN_WIDTH, ROW_HEIGHT);
            drawSlot(canvas, slotRect, t, scene);
        }
    }
}

juce::ValueTree SessionViewComponent::getClipAt(int trackIndex, int sceneIndex)
{
    auto tracksNode = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
    auto track = tracksNode.getChild(trackIndex);
    if (!track.isValid()) return {};

    auto clipsNode = track.getChildWithName(ProjectState::ID_CLIPS);
    if (!clipsNode.isValid()) return {};

    // Linear search for clip with matching scene index
    for (const auto& clip : clipsNode) {
        if (clip.hasProperty(PROP_SCENE_INDEX)) {
            if (static_cast<int>(clip.getProperty(PROP_SCENE_INDEX)) == sceneIndex) {
                return clip;
            }
        }
    }
    
    return {};
}

void SessionViewComponent::drawSlot(SkCanvas* canvas, const SkRect& rect, int trackIndex, int sceneIndex)
{
    auto clip = getClipAt(trackIndex, sceneIndex);
    bool hasClip = clip.isValid();
    
    SkColor clipColor = SkColorSetRGB(60, 60, 70); // Empty slot color
    bool isPlaying = false;
    bool isQueued = false;

    if (hasClip) {
        // Use clip color if available, else default
        if (clip.hasProperty(ProjectState::PROP_COLOR)) {
            clipColor = static_cast<uint32_t>((int)clip.getProperty(ProjectState::PROP_COLOR));
        } else {
            clipColor = SkColorSetRGB(0, 150, 200); // Zenith Blue
        }
        
        if (clip.getProperty(PROP_IS_PLAYING)) isPlaying = true;
        if (clip.getProperty(PROP_IS_QUEUED)) isQueued = true;
    }

    // Visuals
    SkRRect rrect = SkRRect::MakeRectXY(rect, 4.0f, 4.0f);
    
    SkPaint fillPaint;
    fillPaint.setColor(hasClip ? clipColor : SkColorSetRGB(30, 30, 35));
    fillPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, fillPaint);

    SkPaint borderPaint;
    if (isPlaying) {
        borderPaint.setColor(SkColorSetRGB(0, 255, 100)); // Green flash for playing
        borderPaint.setStrokeWidth(2.0f);
    } else if (isQueued) {
        borderPaint.setColor(SkColorSetRGB(255, 200, 50)); // Amber for queued
        borderPaint.setStrokeWidth(2.0f);
    } else {
        borderPaint.setColor(SkColorSetARGB(50, 255, 255, 255));
        borderPaint.setStrokeWidth(1.0f);
    }
    
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, borderPaint);

    // Play Button / Name
    if (hasClip) {
        // Draw Name
        SkFont font;
        font.setSize(12.0f);
        SkPaint textPaint;
        textPaint.setColor(SK_ColorWHITE);
        juce::String name = clip.getProperty(ProjectState::PROP_NAME).toString();
        canvas->drawString(name.toRawUTF8(), rect.left() + 5, rect.top() + 15, font, textPaint);

        // Draw Play Icon
        SkPath playPath;
        float size = 10.0f;
        float cx = rect.left() + 15.0f;
        float cy = rect.bottom() - 15.0f;
        playPath.moveTo(cx - size/2, cy - size/2);
        playPath.lineTo(cx + size/2, cy);
        playPath.lineTo(cx - size/2, cy + size/2);
        playPath.close();

        SkPaint iconPaint;
        iconPaint.setColor(SK_ColorWHITE);
        iconPaint.setAntiAlias(true);
        canvas->drawPath(playPath, iconPaint);
    } else {
        // Circle indicator for empty slots
        SkPaint dotPaint;
        dotPaint.setColor(SkColorSetARGB(20, 255, 255, 255));
        dotPaint.setAntiAlias(true);
        canvas->drawCircle(rect.centerX(), rect.centerY(), 2.0f, dotPaint);
    }
}

void SessionViewComponent::drawSceneHeader(SkCanvas* canvas, const SkRect& rect, int sceneIndex)
{
    SkRRect rrect = SkRRect::MakeRectXY(rect, 4.0f, 4.0f);
    
    SkPaint fillPaint;
    fillPaint.setColor(SkColorSetRGB(40, 40, 45));
    fillPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, fillPaint);

    // Play arrow for Scene Launch
    SkPath playPath;
    float size = 8.0f;
    float cx = rect.centerX();
    float cy = rect.centerY();
    playPath.moveTo(cx - size/2, cy - size/2);
    playPath.lineTo(cx + size/2, cy);
    playPath.lineTo(cx - size/2, cy + size/2);
    playPath.close();

    SkPaint iconPaint;
    iconPaint.setColor(SkColorSetARGB(100, 255, 255, 255));
    iconPaint.setAntiAlias(true);
    canvas->drawPath(playPath, iconPaint);

    // Scene Number
    SkFont font;
    font.setSize(10.0f);
    SkPaint textPaint;
    textPaint.setColor(SkColorSetARGB(150, 255, 255, 255));
    juce::String numStr = juce::String(sceneIndex + 1);
    canvas->drawString(numStr.toRawUTF8(), rect.right() - 15.0f, rect.top() + 12.0f, font, textPaint);
}

void SessionViewComponent::triggerClip(int trackIndex, int sceneIndex)
{
    auto clip = getClipAt(trackIndex, sceneIndex);
    if (clip.isValid()) {
        // REAL IMPLEMENTATION: Update State
        clip.setProperty(PROP_IS_QUEUED, true, &projectState_.getUndoManager());
        
        // Stop other clips on this track
        auto tracksNode = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
        auto track = tracksNode.getChild(trackIndex);
        auto clips = track.getChildWithName(ProjectState::ID_CLIPS);
        for (int i = 0; i < clips.getNumChildren(); ++i) {
            auto c = clips.getChild(i);
            if (c != clip) {
                c.setProperty(PROP_IS_PLAYING, false, nullptr);
                c.setProperty(PROP_IS_QUEUED, false, nullptr);
            }
        }
        DBG("Session: Triggered Clip on Track " + juce::String(trackIndex) + " Scene " + juce::String(sceneIndex));
    } else {
        stopTrack(trackIndex);
    }
}

void SessionViewComponent::triggerScene(int sceneIndex)
{
    auto tracksNode = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
    int numTracks = tracksNode.getNumChildren();
    
    for (int t = 0; t < numTracks; ++t) {
        triggerClip(t, sceneIndex);
    }
    DBG("Session: Triggered Scene " + juce::String(sceneIndex));
}

void SessionViewComponent::stopTrack(int trackIndex)
{
    auto tracksNode = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
    auto track = tracksNode.getChild(trackIndex);
    if (track.isValid()) {
        auto clips = track.getChildWithName(ProjectState::ID_CLIPS);
        for (auto clip : clips) {
            clip.setProperty(PROP_IS_PLAYING, false, nullptr);
            clip.setProperty(PROP_IS_QUEUED, false, nullptr);
        }
    }
    DBG("Session: Stopped Track " + juce::String(trackIndex));
}

void SessionViewComponent::resized()
{
}

void SessionViewComponent::mouseDown(const juce::MouseEvent& e)
{
    float x = e.position.x;
    float y = e.position.y;

    // Hit Testing
    
    // Check Scene Headers
    if (x < SCENE_HEADER_WIDTH) {
        int scene = (int)((y - GAP) / (ROW_HEIGHT + GAP));
        if (scene >= 0 && scene < numScenes_) {
            triggerScene(scene);
            return;
        }
    }
    
    // Check Slots
    if (x > SCENE_HEADER_WIDTH) {
        int track = (int)((x - SCENE_HEADER_WIDTH) / (COLUMN_WIDTH + GAP));
        int scene = (int)((y - GAP) / (ROW_HEIGHT + GAP));
        
        auto tracksNode = projectState_.getState().getChildWithName(ProjectState::ID_TRACKS);
        int numTracks = tracksNode.getNumChildren();

        if (track >= 0 && track < numTracks && scene >= 0 && scene < numScenes_) {
            
            // If Right Click or Alt Click -> Create new clip
            if (e.mods.isRightButtonDown() || e.mods.isAltDown()) {
                auto trackTree = projectState_.getTrackByIndex(track);
                if (trackTree.isValid()) {
                    juce::String trackId = trackTree.getProperty(ProjectState::PROP_ID);
                    
                    // Create default 4-bar MIDI clip
                    double sampleRate = 44100.0; // Approximation for UI
                    juce::int64 lengthSamples = static_cast<juce::int64>(4.0 * sampleRate * (60.0 / 120.0));
                    
                    juce::String clipId = projectState_.createClip(trackId, "midi", 0, lengthSamples, "Scene " + juce::String(scene + 1), "Create Session Clip");
                    
                    if (clipId.isNotEmpty()) {
                        auto clip = projectState_.getClip(trackId, clipId);
                        clip.setProperty(PROP_SCENE_INDEX, scene, nullptr);
                        DBG("Session: Created Clip at T" + juce::String(track) + " S" + juce::String(scene));
                    }
                }
            } else {
                // Normal click -> Trigger
                triggerClip(track, scene);
            }
        }
    }
}

void SessionViewComponent::mouseUp(const juce::MouseEvent&) {}

void SessionViewComponent::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) { repaint(); }
void SessionViewComponent::valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) { repaint(); }
void SessionViewComponent::valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) { repaint(); }

} // namespace zenith
