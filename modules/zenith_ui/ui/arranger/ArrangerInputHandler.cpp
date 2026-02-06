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

// ArrangerInputHandler.cpp

#include "../../utils/StemSeparationJob.h"
#include "../controls/SkiaPopupMenu.h"
#include "../controls/ContextMenuManager.h"
#include "../controls/SkiaAlertWindow.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"

#include <juce_events/juce_events.h>
#include <cmath>
#include <algorithm>

namespace zenith {

//==============================================================================
// Layout Constants (must match ArrangerComponent.cpp)
//==============================================================================
static constexpr float HEADER_WIDTH = 220.0f;
static constexpr float SECTION_HEIGHT = 24.0f;
static constexpr float RULER_HEIGHT = 30.0f;
static constexpr float TRACK_HEIGHT = 80.0f;
static constexpr float TOP_MARGIN = SECTION_HEIGHT + RULER_HEIGHT;

//==============================================================================
// Constructor
//==============================================================================

ArrangerInputHandler::ArrangerInputHandler(ArrangerComponent& owner, ProjectState& projectState,
                                           ArrangerClipManager& clipManager, ArrangerGridUtils& gridUtils)
    : owner_(owner)
    , projectState_(projectState)
    , clipManager_(clipManager)
    , gridUtils_(gridUtils)
{
}

//==============================================================================
// Mouse Event Handlers
//==============================================================================

void ArrangerInputHandler::mouseDown(const juce::MouseEvent& e) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    owner_.grabKeyboardFocus();

    dragStartPoint_ = e.position;
    currentDragMode_ = DragMode::None;

    auto* clip = clipManager_.findClipAtPoint(e.position);



    // Split Tool Behavior
    if (owner_.getTool() == ArrangerTool::Split) {
        if (clip != nullptr) {
            double splitBeats = gridUtils_.xToBeats(e.position.x);
            if (!e.mods.isShiftDown()) {
                splitBeats = gridUtils_.snapToGrid(splitBeats);
            }
            
            juce::int64 splitSamples = gridUtils_.beatsToSamples(splitBeats);
            projectState_.getUndoManager().beginNewTransaction("Split Clip");
            projectState_.splitClip(clip->trackId, clip->clipId, splitSamples, "Split Clip");
            
            owner_.repaint();
        }
        return; 
    }

    // Pencil Tool Behavior
    if (owner_.getTool() == ArrangerTool::Pencil) {
        if (clip == nullptr) {
            double beat = gridUtils_.xToBeats(e.position.x);
            if (!e.mods.isShiftDown()) {
                beat = gridUtils_.snapToGrid(beat);
            }
            
            int trackIndex = gridUtils_.yToTrackIndex(e.position.y);
            auto tracksNode = projectState_.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
            if (tracksNode.isValid() && trackIndex >= 0 && trackIndex < tracksNode.getNumChildren()) {
                auto track = tracksNode.getChild(trackIndex);
                juce::String trackId = track[zenith::ProjectState::PROP_ID].toString();
                juce::String type = track[zenith::ProjectState::PROP_TYPE].toString();
                bool isMidi = (type != "audio");

                projectState_.getUndoManager().beginNewTransaction("Draw Clip");
                projectState_.createEmptyClip(trackId, beat, 4.0, isMidi, "New Clip", "Pencil Tool Draw");
                owner_.repaint();
            }
        }
        return;
    }

    if (clip != nullptr) {
        // Check Fade Handles (Priority over move)
        float ppb = owner_.pixelsPerBeat;
        float fadeInX = clip->bounds.getX() + (float)(clip->fadeInBeats * ppb);
        float fadeOutX = clip->bounds.getRight() - (float)(clip->fadeOutBeats * ppb);
        float handleY = clip->bounds.getY(); // Top edge
        
        // Hit test radius
        if (e.position.getDistanceFrom({fadeInX, handleY}) < 10.0f) {
            currentDragMode_ = DragMode::ResizeFadeIn;
            resizingClipId_ = clip->clipId;
            return;
        }
        if (e.position.getDistanceFrom({fadeOutX, handleY}) < 10.0f) {
            currentDragMode_ = DragMode::ResizeFadeOut;
            resizingClipId_ = clip->clipId;
            return;
        }

        if (e.mods.isRightButtonDown()) {
            // Enhanced Skia Context Menu for clips
            auto menu = ContextMenuManager::createMenu();
            
            juce::String clipId = clip->clipId;
            bool isAudioClip = !clip->isMidi;
            
            // Editing section
            menu->addSectionHeader("Edit");
            
            menu->addItemWithShortcut(1, "Duplicate", "Ctrl+D", true, 
                [this]() { clipManager_.duplicateSelectedClips(); });
            
            menu->addItemWithShortcut(2, "Split at Playhead", "Cmd+E", true,
                [this, clipId]() { 
                    clipManager_.splitSelectedClipsAtPlayhead();
                });
            
            menu->addItem(3, "Consolidate Selected", true, false,
                [this]() { 
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                        "Consolidate", "Consolidation feature coming soon!");
                });
            
            menu->addSeparator();
            
            // Audio-specific options
            if (isAudioClip) {
                menu->addSectionHeader("Audio");
                
                menu->addItem(10, "Split to Stems", true, false,
                    [this, clipId]() { ripAudioToStems(clipId); });
                
                menu->addItem(11, "Render to Audio", true, false,
                    [this, clipId]() { 
                         juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                            "Render", "Render to audio feature coming soon!");
                    });
                
                menu->addItem(12, "Detect Tempo", true, false,
                    [this, clipId]() {
                        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                            "Tempo Detection", "Tempo detection coming soon!");
                    });
                
                menu->addSeparator();
            }
            
            // Appearance section
            menu->addSectionHeader("Appearance");
            
            // Color submenu
            auto colorMenu = ContextMenuManager::createMenu();
            int colorId = 100;
            for (int i = 0; i < 8; ++i) {
                auto c = design::unified::getTrackColor(i);
                colorMenu->addItem(colorId++, "", true, false, [this, clipId, c]() {
                     projectState_.setClipColor(clipId, design::toJuceColour(c));
                     owner_.repaint();
                });
            }
            menu->addSubMenu("Set Color", std::move(colorMenu));
            
            menu->addItem(20, "Rename...", true, false,
                [this, clipId, clip]() {
                    auto* alert = new SkiaAlertWindow("Rename Clip", "Enter a new name for the clip:", SkiaAlertWindow::IconType::QuestionIcon);
                    alert->addTextEditor("name", projectState_.getClipName(clipId), "Clip Name:");
                    alert->addButton("Rename", SkiaAlertWindow::Result::Button1);
                    alert->addButton("Cancel", SkiaAlertWindow::Result::Cancelled, SkiaButton::Style::Secondary);
                    
                    alert->showAsync([this, clipId, alert](SkiaAlertWindow::Result result) {
                        if (result == SkiaAlertWindow::Result::Button1) {
                            juce::String newName = alert->getTextEditorContents("name");
                            if (newName.isNotEmpty()) {
                                projectState_.renameClip(clipId, newName);
                                owner_.repaint();
                            }
                        }
                        delete alert;
                    });
                    
                    owner_.addAndMakeVisible(alert);
                    alert->setCentreRelative(0.5f, 0.45f);
                });
            
            menu->addSeparator();
            
            // Danger zone
            menu->addItemComplete(99, "Delete", SkPath(), "Del", true, false, true,
                [this]() { clipManager_.deleteSelectedClips(); });
            
            // Show menu at click position
            ContextMenuManager::getInstance().showMenuAt(
                std::move(menu), &owner_, 
                static_cast<int>(e.position.x), 
                static_cast<int>(e.position.y));
            return;
        }

        // Check for resize zones
        if (clip->isInLeftResizeZone(e.position)) {
            currentDragMode_ = DragMode::ResizeClipLeft;
            resizingClipId_ = clip->clipId;
            resizeOriginalStart_ = clip->startBeats;
            resizeOriginalLength_ = clip->lengthBeats;
        } else if (clip->isInRightResizeZone(e.position)) {
            currentDragMode_ = DragMode::ResizeClipRight;
            resizingClipId_ = clip->clipId;
            resizeOriginalStart_ = clip->startBeats;
            resizeOriginalLength_ = clip->lengthBeats;
        } else {
            // Move mode
            currentDragMode_ = DragMode::MoveClips;

            // Determine Edit Mode based on modifiers
            if (e.mods.isAltDown() && e.mods.isShiftDown()) {
                currentEditMode_ = EditMode::Insert;
            } else if (e.mods.isAltDown()) {
                currentEditMode_ = EditMode::Ripple;
            } else {
                currentEditMode_ = EditMode::Overwrite;
            }

            // Populate initial starts for robust Ripple/Insert calculations
            initialClipStarts_.clear();
            for (const auto& view : clipManager_.getClipViews()) {
                initialClipStarts_[view.clipId] = view.startBeats;
            }

            bool isCtrlOrCmd = e.mods.isCommandDown();

            if (!clip->isSelected) {
                clipManager_.selectClip(clip->clipId, isCtrlOrCmd);
            } else if (isCtrlOrCmd) {
                clipManager_.selectClip(clip->clipId, true);
            }

            // Cache original positions
            clipDragStates_.clear();
            for (const auto& clipId : clipManager_.getSelectedClipIds()) {
                if (auto* view = clipManager_.findClipView(clipId)) {
                    int trackIndex = 0;
                    auto tracksNode = projectState_.getState().getChildWithName(
                        zenith::ProjectState::ID_TRACKS);
                    if (tracksNode.isValid()) {
                        for (const auto& track : tracksNode) {
                            if (track[zenith::ProjectState::PROP_ID].toString() == view->trackId)
                                break;
                            trackIndex++;
                        }
                    }
                    ClipDragState state;
                    state.clipId = clipId;
                    state.originalStartBeats = view->startBeats;
                    state.originalTrackIndex = trackIndex;
                    clipDragStates_.add(state);
                }
            }
        }
    } else {
        // Clicked empty area
        if (e.mods.isRightButtonDown()) {
             // Empty area context menu?
             return;
        }
        
        if (e.position.x < HEADER_WIDTH && e.position.y > RULER_HEIGHT) {
            // Track Header Interaction
            int trackIndex = gridUtils_.yToTrackIndex(e.position.y);
            handleTrackHeaderClick(e, trackIndex);
        } else {
            // Timeline Interaction
            bool isShift = e.mods.isShiftDown();
            if (isShift) {
                currentDragMode_ = DragMode::Marquee;
                marqueeRect_ = juce::Rectangle<float>(e.position, e.position);
            } else {
                clipManager_.clearSelection();
            }
        }
    }
}

void ArrangerInputHandler::ripAudioToStems(const juce::String& clipId) {
    auto [foundTrack, clipNode] = projectState_.findClip(clipId);
    if (!clipNode.isValid()) return;

    juce::String audioPath = clipNode[ProjectState::PROP_AUDIO_FILE].toString();
    if (audioPath.isEmpty()) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            "Stem Separation", "This feature only works on audio clips with a valid file.");
        return;
    }

    juce::File audioFile(audioPath);
    if (!audioFile.existsAsFile()) return;

    // Create output directory
    juce::File outputDir = audioFile.getParentDirectory().getChildFile(audioFile.getFileNameWithoutExtension() + "_Stems");
    outputDir.createDirectory();

    // Trigger separation job
    auto callback = [this, clipId, outputDir](const utils::StemSeparationJob::StemFiles& results) {
        if (!results.success) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "Stem Separation Failed", results.error);
            return;
        }

        // Create new tracks and clips
        projectState_.getUndoManager().beginNewTransaction("Rip Audio to Stems");

        auto [origTrack, origClip] = projectState_.findClip(clipId);
        double startBeats = origClip[ProjectState::PROP_START_BEATS];
        juce::String origName = origClip[ProjectState::PROP_NAME].toString();

        struct ResultMapping {
            juce::String suffix;
            juce::File file;
        };

        ResultMapping mappings[] = {
            {"Vocals", results.vocals},
            {"Drums", results.drums},
            {"Bass", results.bass},
            {"Other", results.other}
        };

        for (const auto& m : mappings) {
            juce::String newTrackId = projectState_.addTrack(origName + " (" + m.suffix + ")", "audio");
            juce::String newClipId = projectState_.createEmptyClip(newTrackId, startBeats, 4.0, false, m.suffix, "Stem creation");
            
            auto [t, c] = projectState_.findClip(newClipId);
            if (c.isValid()) {
                c.setProperty(ProjectState::PROP_AUDIO_FILE, m.file.getFullPathName(), &projectState_.getUndoManager());
                
                // Update length from reader
                juce::AudioFormatManager fmt;
                fmt.registerBasicFormats();
                if (auto reader = std::unique_ptr<juce::AudioFormatReader>(fmt.createReaderFor(m.file))) {
                    double lengthBeats = gridUtils_.samplesToBeats(reader->lengthInSamples);
                    projectState_.setClipRange(newClipId, startBeats, lengthBeats, "Update length");
                }
            }
        }
        
        owner_.repaint();

        juce::String msg = results.usedNeuralEngine ? 
            "Neural Stem Separation Complete!" : 
            "Stem Separation Complete (DSP Fallback)";
        
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
            "Stem Separation", msg);
    };


    auto* job = new utils::StemSeparationJob(audioFile, outputDir, callback);
    owner_.engine_.getThreadPool().addJob(job, true);
    
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
        "Stem Separation", "Neural processing started in background...");
}



void ArrangerInputHandler::handleTrackHeaderClick(const juce::MouseEvent& e, int trackIndex) {
    auto tracksNode = projectState_.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
    if (!tracksNode.isValid() || trackIndex < 0 || trackIndex >= tracksNode.getNumChildren())
        return;

    auto track = tracksNode.getChild(trackIndex);
    
    // Basic hit testing for M/S/R buttons
    float relativeX = e.position.x;
    float rowY = gridUtils_.trackIndexToY(trackIndex);
    float relY = e.position.y - rowY;

    // Layout: Name (0-110), M(120), S(150), R(180)
    if (relY >= 45 && relY <= 70) { // Button row
        if (relativeX >= 120 && relativeX <= 145) {
            bool m = track[zenith::ProjectState::PROP_MUTE];
            track.setProperty(zenith::ProjectState::PROP_MUTE, !m,
                            &projectState_.getUndoManager());
        } else if (relativeX >= 150 && relativeX <= 175) {
            bool s = track[zenith::ProjectState::PROP_SOLO];
            track.setProperty(zenith::ProjectState::PROP_SOLO, !s,
                            &projectState_.getUndoManager());
        } else if (relativeX >= 180 && relativeX <= 205) {
            bool r = track[zenith::ProjectState::PROP_ARMED];
            track.setProperty(zenith::ProjectState::PROP_ARMED, !r,
                            &projectState_.getUndoManager());
        }
    }
}

void ArrangerInputHandler::mouseDrag(const juce::MouseEvent& e) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (currentDragMode_ == DragMode::None) {
        if (e.getDistanceFromDragStart() > 5) {
            currentDragMode_ = DragMode::Marquee;
            marqueeRect_ = juce::Rectangle<float>(dragStartPoint_, e.position);
            owner_.repaint();
        }
        return;
    }

    if (currentDragMode_ == DragMode::ResizeFadeIn) {
        if (auto* view = clipManager_.findClipView(resizingClipId_)) {
             double clipStartX = gridUtils_.beatsToX(view->startBeats);
             double newFadeIn = gridUtils_.xToBeats(e.position.x - clipStartX);
             newFadeIn = std::max(0.0, std::min(newFadeIn, view->lengthBeats));
             projectState_.setClipFade(resizingClipId_, newFadeIn, view->fadeOutBeats, "Resize Fade In");
             owner_.repaint();
        }
    } else if (currentDragMode_ == DragMode::ResizeFadeOut) {
         if (auto* view = clipManager_.findClipView(resizingClipId_)) {
            double clipEndX = gridUtils_.beatsToX(view->startBeats + view->lengthBeats);
            double newFadeOut = gridUtils_.xToBeats(clipEndX - e.position.x);
            newFadeOut = std::max(0.0, std::min(newFadeOut, view->lengthBeats));
            projectState_.setClipFade(resizingClipId_, view->fadeInBeats, newFadeOut, "Resize Fade Out");
            owner_.repaint();
        }
    } else if (currentDragMode_ == DragMode::MoveClips) {
        handleClipMoveDrag(e);
    } else if (currentDragMode_ == DragMode::ResizeClipLeft) {
        handleClipResizeDrag(e, true);
    } else if (currentDragMode_ == DragMode::ResizeClipRight) {
        handleClipResizeDrag(e, false);
    } else if (currentDragMode_ == DragMode::Marquee) {
        marqueeRect_ = juce::Rectangle<float>(dragStartPoint_, e.position);
        owner_.repaint();
    }
}

void ArrangerInputHandler::handleClipMoveDrag(const juce::MouseEvent& e) {
    // Calculate delta
    double deltaBeats = gridUtils_.xToBeats(e.position.x) - gridUtils_.xToBeats(dragStartPoint_.x);
    int deltaTrackIndex = gridUtils_.yToTrackIndex(e.position.y) - gridUtils_.yToTrackIndex(dragStartPoint_.y);

    // OPTIMIZATION: Early exit if visual delta is negligible
    if (std::abs(deltaBeats - lastDragDeltaBeats_) < 0.001 &&
        deltaTrackIndex == lastDragDeltaTrack_) {
        return;
    }
    lastDragDeltaBeats_ = deltaBeats;
    lastDragDeltaTrack_ = deltaTrackIndex;

    // Reset Insertion Guide
    insertionGuideX_ = -1.0f;
    double minNewStartForGuide = 10000000.0;

    // Find earliest original start of SELECTED clips on each track
    std::map<juce::String, double> trackEarliestSelectedStart;
    std::map<juce::String, bool> isClipSelectedMap;

    for (const auto& clipId : clipManager_.getSelectedClipIds()) {
        if (auto* view = clipManager_.findClipView(clipId)) {
            isClipSelectedMap[clipId] = true;
            double start = initialClipStarts_[clipId];
            if (trackEarliestSelectedStart.find(view->trackId) == trackEarliestSelectedStart.end()) {
                trackEarliestSelectedStart[view->trackId] = start;
            } else {
                trackEarliestSelectedStart[view->trackId] =
                    std::min(trackEarliestSelectedStart[view->trackId], start);
            }
        }
    }

    // Update Clip Positions
    for (auto& view : clipManager_.getClipViews()) {
        if (isClipSelectedMap[view.clipId]) {
            // Selected Clip: Follow Mouse
            double initial = initialClipStarts_[view.clipId];
            double newStart = initial + deltaBeats;
            newStart = juce::jmax(0.0, newStart);
            view.startBeats = newStart;

            minNewStartForGuide = std::min(minNewStartForGuide, newStart);

            // Update Track ID (only for selected clips)
            for (const auto& ds : clipDragStates_) {
                if (ds.clipId == view.clipId) {
                    int newTrackIndex = ds.originalTrackIndex + deltaTrackIndex;
                    newTrackIndex = juce::jmax(0, newTrackIndex);

                    auto tracksNode = projectState_.getState().getChildWithName(
                        zenith::ProjectState::ID_TRACKS);
                    if (tracksNode.isValid() && newTrackIndex < tracksNode.getNumChildren()) {
                        auto newTrack = tracksNode.getChild(newTrackIndex);
                        view.trackId = newTrack[zenith::ProjectState::PROP_ID].toString();
                    }
                    break;
                }
            }
        } else if (currentEditMode_ == EditMode::Ripple || currentEditMode_ == EditMode::Insert) {
            // Unselected Clip: Apply Ripple/Insert
            auto trackIt = trackEarliestSelectedStart.find(view.trackId);
            if (trackIt != trackEarliestSelectedStart.end()) {
                double earliestSel = trackIt->second;
                double initial = initialClipStarts_[view.clipId];

                if (initial >= earliestSel) {
                    double newStart = initial + deltaBeats;
                    newStart = juce::jmax(0.0, newStart);
                    view.startBeats = newStart;
                }
            }
        }
    }

    // Set Insertion Guide Visibility
    if ((currentEditMode_ == EditMode::Ripple || currentEditMode_ == EditMode::Insert) &&
        minNewStartForGuide < 10000000.0) {
        insertionGuideX_ = gridUtils_.beatsToX(minNewStartForGuide);
    }

    clipManager_.recomputeClipBounds();
    owner_.repaint();
}

void ArrangerInputHandler::handleClipResizeDrag(const juce::MouseEvent& e, bool isLeftEdge) {
    if (auto* view = clipManager_.findClipView(resizingClipId_)) {
        if (isLeftEdge) {
            double newStart = gridUtils_.xToBeats(e.position.x);
            double originalEnd = resizeOriginalStart_ + resizeOriginalLength_;
            double newLength = originalEnd - newStart;

            // Enforce minimum length
            if (newLength < 0.25) {
                newStart = originalEnd - 0.25;
                newLength = 0.25;
            }

            view->startBeats = newStart;
            view->lengthBeats = newLength;
        } else {
            double newEnd = gridUtils_.xToBeats(e.position.x);
            double newLength = newEnd - resizeOriginalStart_;

            // Enforce minimum length
            newLength = juce::jmax(0.25, newLength);

            view->lengthBeats = newLength;
        }

        clipManager_.recomputeClipBounds();
        owner_.repaint();
    }
}

void ArrangerInputHandler::mouseUp(const juce::MouseEvent& e) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    juce::ignoreUnused(e);

    if (currentDragMode_ == DragMode::MoveClips) {
        commitClipMove();
        clipDragStates_.clear();
        initialClipStarts_.clear();
        insertionGuideX_ = -1.0f;
    } else if (currentDragMode_ == DragMode::ResizeClipLeft ||
               currentDragMode_ == DragMode::ResizeClipRight) {
        commitClipResize();
        resizingClipId_.clear();
    } else if (currentDragMode_ == DragMode::Marquee) {
        clipManager_.selectClipsInRect(marqueeRect_);
        marqueeRect_ = juce::Rectangle<float>();
    }

    currentDragMode_ = DragMode::None;
    owner_.repaint();
}

void ArrangerInputHandler::commitClipMove() {
    if (clipManager_.getSelectedClipIds().isEmpty())
        return;

    projectState_.getUndoManager().beginNewTransaction("Move clips");

    // Iterate over ALL clips to see which ones moved
    for (const auto& view : clipManager_.getClipViews()) {
        auto it = initialClipStarts_.find(view.clipId);
        if (it != initialClipStarts_.end()) {
            double initialStart = it->second;
            double currentStart = view.startBeats;

            auto [track, clip] = projectState_.findClip(view.clipId);
            if (clip.isValid()) {
                juce::String currentTrackIdInState =
                    track[zenith::ProjectState::PROP_ID].toString();

                double snappedStart = gridUtils_.snapToGrid(currentStart);

                if (std::abs(snappedStart - gridUtils_.snapToGrid(initialStart)) > 0.001 ||
                    view.trackId != currentTrackIdInState) {
                    projectState_.moveClip(view.clipId, view.trackId, snappedStart, "Move clips");
                }
            }
        }
    }

    DBG("ArrangerInputHandler: Committed move for clips (EditMode: " +
        juce::String(static_cast<int>(currentEditMode_)) + ")");
}

void ArrangerInputHandler::commitClipResize() {
    if (auto* view = clipManager_.findClipView(resizingClipId_)) {
        double snappedStart = gridUtils_.snapToGrid(view->startBeats);
        double snappedLength = gridUtils_.snapToGrid(view->lengthBeats);

        projectState_.setClipRange(resizingClipId_, snappedStart, snappedLength, "Resize clip");

        DBG("ArrangerInputHandler: Committed resize for clip " + resizingClipId_);
    }
}

void ArrangerInputHandler::mouseMove(const juce::MouseEvent& e) {
    auto* clip = clipManager_.findClipAtPoint(e.position);

    // Split Tool Cursor
    if (owner_.getTool() == ArrangerTool::Split) {
        owner_.setMouseCursor(juce::MouseCursor::IBeamCursor);
        return;
    }

    if (owner_.macroToolbar) {
        owner_.macroToolbar->checkProximity(e.position);
    }

    if (clip != nullptr) {
        // Check Fade Handles
        float ppb = owner_.pixelsPerBeat;
        float fadeInX = clip->bounds.getX() + (float)(clip->fadeInBeats * ppb);
        float fadeOutX = clip->bounds.getRight() - (float)(clip->fadeOutBeats * ppb);
        float handleY = clip->bounds.getY();
        
        if (e.position.getDistanceFrom({fadeInX, handleY}) < 10.0f ||
            e.position.getDistanceFrom({fadeOutX, handleY}) < 10.0f) {
            owner_.setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
            return;
        }

        if (clip->isInLeftResizeZone(e.position) || clip->isInRightResizeZone(e.position))
            owner_.setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        else
            owner_.setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    } else {
        owner_.setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

juce::String ArrangerInputHandler::getTooltip() {
    auto mousePos = owner_.getMouseXYRelative();
    auto* clip = clipManager_.findClipAtPoint(mousePos.toFloat());

    if (clip != nullptr) {
        auto [track, clipNode] = projectState_.findClip(clip->clipId);
        if (clipNode.isValid()) {
            auto clipName = clipNode[zenith::ProjectState::PROP_NAME].toString();
            auto startBeats = clipNode[zenith::ProjectState::PROP_START_BEATS].toString();
            auto lengthBeats = clipNode[zenith::ProjectState::PROP_LENGTH_BEATS].toString();
            return clipName + " (" + startBeats + " beats, " + lengthBeats + " beats)";
        }
    }

    return {};
}

void ArrangerInputHandler::mouseDoubleClick(const juce::MouseEvent& e) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto* clip = clipManager_.findClipAtPoint(e.position);

    if (clip != nullptr) {
        // Double-clicked existing clip - trigger callback
        if (owner_.onClipDoubleClicked) {
            owner_.onClipDoubleClicked(clip->trackId, clip->clipId);
        }
        return;
    }

    // Double-clicked empty area - create clip
    if (e.position.x < HEADER_WIDTH || e.position.y < SECTION_HEIGHT)
        return;

    double beat = owner_.viewStartBeats + (e.position.x - HEADER_WIDTH) / owner_.pixelsPerBeat;
    int trackIndex = gridUtils_.yToTrackIndex(e.position.y);

    // Snap to nearest integer beat
    beat = std::floor(beat);

    if (beat < 0)
        beat = 0;

    auto tracksNode = projectState_.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
    if (tracksNode.isValid() && trackIndex >= 0 && trackIndex < tracksNode.getNumChildren()) {
        auto track = tracksNode.getChild(trackIndex);
        juce::String trackId = track[zenith::ProjectState::PROP_ID].toString();
        juce::String type = track[zenith::ProjectState::PROP_TYPE].toString();
        bool isMidi = (type != "audio");

        projectState_.createEmptyClip(trackId, beat, 4.0, isMidi, "New Clip", "Double Click Create");
        owner_.repaint();
    }
}

void ArrangerInputHandler::mouseWheelMove(const juce::MouseEvent& e,
                                          const juce::MouseWheelDetails& wheel) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    bool isShift = e.mods.isShiftDown();
    bool isCtrlOrCmd = e.mods.isCommandDown();

    if (isCtrlOrCmd) {
        // Zoom horizontal
        double zoomFactor = 1.0 + (wheel.deltaY * 0.5);
        owner_.pixelsPerBeat *= zoomFactor;
        owner_.pixelsPerBeat = juce::jlimit(10.0, 200.0, owner_.pixelsPerBeat);

        // Zoom around mouse position
        double beatsAtMouse = gridUtils_.xToBeats(e.position.x);
        double pixelsAtMouse = e.position.x;
        owner_.viewStartBeats = beatsAtMouse - (pixelsAtMouse / owner_.pixelsPerBeat);
        owner_.viewStartBeats = juce::jmax(0.0, owner_.viewStartBeats);

        clipManager_.recomputeClipBounds();

        // Update MiniMap
        double visibleBeats = static_cast<double>(owner_.getWidth() - HEADER_WIDTH) / owner_.pixelsPerBeat;
        int visibleTracks = static_cast<int>((owner_.getHeight() - RULER_HEIGHT) / TRACK_HEIGHT);
        owner_.miniMap.setVisibleRange(owner_.viewStartBeats, visibleBeats,
                                       owner_.firstVisibleTrackIndex, visibleTracks);

        owner_.repaint();
    } else if (isShift) {
        // Scroll horizontal
        owner_.viewStartBeats -= wheel.deltaY * 2.0;
        owner_.viewStartBeats = juce::jmax(0.0, owner_.viewStartBeats);

        clipManager_.recomputeClipBounds();

        // Update MiniMap
        double visibleBeats = static_cast<double>(owner_.getWidth() - HEADER_WIDTH) / owner_.pixelsPerBeat;
        int visibleTracks = static_cast<int>((owner_.getHeight() - RULER_HEIGHT) / TRACK_HEIGHT);
        owner_.miniMap.setVisibleRange(owner_.viewStartBeats, visibleBeats,
                                       owner_.firstVisibleTrackIndex, visibleTracks);

        owner_.repaint();
    } else {
        // Scroll vertical
        owner_.firstVisibleTrackIndex -= static_cast<int>(wheel.deltaY * 2.0);

        auto tracksNode = projectState_.getState().getChildWithName(zenith::ProjectState::ID_TRACKS);
        int maxTrackIndex = tracksNode.isValid() ? tracksNode.getNumChildren() - 1 : 0;

        owner_.firstVisibleTrackIndex = juce::jlimit(0, maxTrackIndex, owner_.firstVisibleTrackIndex);

        clipManager_.recomputeClipBounds();
        owner_.repaint();
    }
}

bool ArrangerInputHandler::keyPressed(const juce::KeyPress& key) {
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    // Delete / Backspace
    if (key.isKeyCode(juce::KeyPress::deleteKey) ||
        key.isKeyCode(juce::KeyPress::backspaceKey)) {
        clipManager_.deleteSelectedClips();
        return true;
    }

    // Ctrl/Cmd+D - Duplicate
    if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'D') {
        clipManager_.duplicateSelectedClips();
        return true;
    }

    // Cmd+E - Split at Playhead
    if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'E') {
        clipManager_.splitSelectedClipsAtPlayhead();
        return true;
    }

    // Ctrl/Cmd+Z - Undo
    if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'Z') {
        projectState_.undo();
        return true;
    }

    // Ctrl/Cmd+Shift+Z or Ctrl/Cmd+Y - Redo
    if ((key.getModifiers().isCommandDown() && key.getModifiers().isShiftDown() &&
         key.getKeyCode() == 'Z') ||
        (key.getModifiers().isCommandDown() && key.getKeyCode() == 'Y')) {
        projectState_.redo();
        return true;
    }

    // + key - Zoom in
    if (key.getKeyCode() == '+' || key.getKeyCode() == '=') {
        owner_.pixelsPerBeat *= 1.2;
        owner_.pixelsPerBeat = juce::jmin(200.0, owner_.pixelsPerBeat);
        clipManager_.recomputeClipBounds();
        owner_.repaint();
        return true;
    }

    // - key - Zoom out
    if (key.getKeyCode() == '-') {
        owner_.pixelsPerBeat /= 1.2;
        owner_.pixelsPerBeat = juce::jmax(10.0, owner_.pixelsPerBeat);
        clipManager_.recomputeClipBounds();
        owner_.repaint();
        return true;
    }

    return false;
}

} // namespace zenith
