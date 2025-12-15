#include "../../include/ui/DrumPadComponent.h"
#include "../../Source/ui/skia/ZenithDesignSystem.h"
#include "../../include/Engine.h"
#include <core/SkCanvas.h>
#include <core/SkPaint.h>
#include <core/SkRRect.h>
#include <effects/SkGradientShader.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <utils/SkShadowUtils.h>

using namespace zenith;

//==============================================================================
//==============================================================================
DrumPadComponent::DrumPadComponent(zenith::Engine &eng,
                                   zenith::ProjectState &state)
    : projectState(state), engine(eng) {
  pads.resize(kNumPads);
  for (int i = 0; i < kNumPads; ++i) {
    pads[i].noteNumber = baseNote + i;
    pads[i].name =
        juce::MidiMessage::getMidiNoteName(pads[i].noteNumber, true, true, 3);
    pads[i].steps.resize(kSequencerSteps, false);

    // Cycle colors for visual distinctness
    if (i % 4 == 0)
      pads[i].color = juce::Colour(design::colors::NEON_PINK); // Kicks/Bases
    else if (i % 4 == 1)
      pads[i].color = juce::Colour(design::colors::NEON_CYAN); // Snares/Claps
    else if (i % 4 == 2)
      pads[i].color = juce::Colour(design::colors::NEON_YELLOW); // Hats
    else
      pads[i].color = juce::Colour(design::colors::NEON_PURPLE); // Percs
  }

  startTimerHz(60); // Animation loop at 60fps
}

DrumPadComponent::~DrumPadComponent() {
  if (currentClipId.isNotEmpty()) {
    auto [track, clip] = projectState.findClip(currentClipId);
    if (clip.isValid()) {
      auto midiNotes = clip.getChildWithName(zenith::ProjectState::ID_NOTES);
      if (midiNotes.isValid())
        midiNotes.removeListener(this);
    }
  }
}

//==============================================================================
void DrumPadComponent::setClipContext(const juce::String &clipId) {
  if (currentClipId == clipId)
    return;

  // Unsubscribe from old
  if (currentClipId.isNotEmpty()) {
    auto [oldTrack, oldClip] = projectState.findClip(currentClipId);
    if (oldClip.isValid()) {
      auto midiNotes = oldClip.getChildWithName(zenith::ProjectState::ID_NOTES);
      if (midiNotes.isValid())
        midiNotes.removeListener(this);
    }
  }

  currentClipId = clipId;

  if (currentClipId.isNotEmpty()) {
    auto [track, clip] = projectState.findClip(currentClipId);
    if (clip.isValid()) {
      auto midiNotes = clip.getChildWithName(zenith::ProjectState::ID_NOTES);
      if (!midiNotes.isValid()) {
        // Ensure note node exists
        midiNotes = juce::ValueTree(zenith::ProjectState::ID_NOTES);
        clip.appendChild(midiNotes, nullptr);
      }
      midiNotes.addListener(this);
    }
  }

  refreshData();
  repaint();
}

void DrumPadComponent::refreshData() {
  if (currentClipId.isEmpty())
    return;

  // Clear steps
  for (auto &pad : pads) {
    std::fill(pad.steps.begin(), pad.steps.end(), false);
  }

  // Populate steps from ProjectState
  // Assuming 1 bar loop (4 beats) -> 16 steps = 16th notes
  // Step 0 = 0.0 beats
  // Step 1 = 0.25 beats
  // ...
  // Step 15 = 3.75 beats

  auto notes = projectState.getMidiNotesForClip(currentClipId);
  for (const auto &note : notes) {
    int padIndex = note.pitch - baseNote;
    if (padIndex >= 0 && padIndex < kNumPads) {
      // Quantize note start to step index
      int stepIndex = static_cast<int>(std::round(note.startBeats * 4.0));
      if (stepIndex >= 0 && stepIndex < kSequencerSteps) {
        pads[padIndex].steps[stepIndex] = true;
      }
    }
  }
}

//==============================================================================
void DrumPadComponent::resized() {
  SkiaComponent::resized(); // Base resize
  updatePadLayout();
}

void DrumPadComponent::updatePadLayout() {
  auto b = getLocalBounds().toFloat();
  float margin = 10.0f;
  float gap = 8.0f;

  float availableWidth = b.getWidth() - 2 * margin - (kCols - 1) * gap;
  float availableHeight = b.getHeight() - 2 * margin - (kRows - 1) * gap;

  float padWidth = availableWidth / kCols;
  float padHeight = availableHeight / kRows;

  for (int row = 0; row < kRows; ++row) {
    for (int col = 0; col < kCols; ++col) {
      int i = row * kCols + col;
      if (i >= kNumPads)
        break;

      float x = margin + col * (padWidth + gap);
      float y = margin + row * (padHeight + gap);

      // The main pad takes upper 80% of the cell
      float mainPadH = padHeight * 0.75f;
      float seqH = padHeight * 0.20f;
      // 5% gap internal

      pads[i].padBounds = juce::Rectangle<float>(x, y, padWidth, mainPadH);
      pads[i].sequencerBounds = juce::Rectangle<float>(
          x, y + mainPadH + (padHeight * 0.05f), padWidth, seqH);

      // Calculate step bounds
      pads[i].stepBounds.resize(kSequencerSteps);
      float stepW = pads[i].sequencerBounds.getWidth() / kSequencerSteps;
      for (int s = 0; s < kSequencerSteps; ++s) {
        pads[i].stepBounds[s] =
            juce::Rectangle<float>(pads[i].sequencerBounds.getX() + s * stepW,
                                   pads[i].sequencerBounds.getY(),
                                   stepW - 1.0f, // 1px gap
                                   pads[i].sequencerBounds.getHeight());
      }
    }
  }
}

void DrumPadComponent::drawSkia(SkCanvas *canvas) {
  // Draw Background
  canvas->clear(design::colors::BG_DARK);

  SkPaint paint;
  paint.setAntiAlias(true);

  for (const auto &pad : pads) {
    // --- Draw Pad ---
    SkRect padRect =
        SkRect::MakeXYWH(pad.padBounds.getX(), pad.padBounds.getY(),
                         pad.padBounds.getWidth(), pad.padBounds.getHeight());

    // Dynamic color based on flash
    SkColor baseColor = pad.color.getARGB();
    if (pad.flashLevel > 0.0f) {
      // Interpolate towards white
      SkColor flashColor = SK_ColorWHITE;
      // Simple manual lerp
      uint8_t r =
          static_cast<uint8_t>(SkColorGetR(baseColor) +
                               (255 - SkColorGetR(baseColor)) * pad.flashLevel);
      uint8_t g =
          static_cast<uint8_t>(SkColorGetG(baseColor) +
                               (255 - SkColorGetG(baseColor)) * pad.flashLevel);
      uint8_t b =
          static_cast<uint8_t>(SkColorGetB(baseColor) +
                               (255 - SkColorGetB(baseColor)) * pad.flashLevel);
      baseColor = SkColorSetRGB(r, g, b);
    }

    // Pad body
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SkColorSetA(baseColor, 100)); // Semi-transparent body
    canvas->drawRoundRect(padRect, 8.0f, 8.0f, paint);

    // Border / Glow
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(2.0f);
    paint.setColor(baseColor);

    // Add glow if flashed
    if (pad.flashLevel > 0.1f) {
      paint.setMaskFilter(
          SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 10.0f * pad.flashLevel));
      canvas->drawRoundRect(padRect, 8.0f, 8.0f, paint);
      paint.setMaskFilter(nullptr); // clear filter
    }
    canvas->drawRoundRect(padRect, 8.0f, 8.0f, paint);

    // Text Label
    // Using built-in font for now, ideally use Zenith typography
    SkFont font;
    font.setSize(16.0f);
    font.setSubpixel(true);

    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SK_ColorWHITE);

    std::string label = pad.name.toStdString();
    // Measure text to center
    float textWidth =
        font.measureText(label.c_str(), label.length(), SkTextEncoding::kUTF8);
    canvas->drawSimpleText(label.c_str(), label.length(), SkTextEncoding::kUTF8,
                           padRect.centerX() - textWidth / 2.0f,
                           padRect.centerY() + 6.0f, font, paint);

    // --- Draw Sequencer ---
    for (int s = 0; s < kSequencerSteps; ++s) {
      const auto &sb = pad.stepBounds[s];
      SkRect stepRect =
          SkRect::MakeXYWH(sb.getX(), sb.getY(), sb.getWidth(), sb.getHeight());

      bool isActive = pad.steps[s];

      // Calculate current beat from engine playhead
      double currentBeat = engine.getPlaybackPositionBeats();
      // Assuming 16 steps = 4 beats (1 bar), 1 step = 0.25 beats
      int currentStep = static_cast<int>(currentBeat * 4.0) % 16;
      bool isCurrentBeat = (s == currentStep) && engine.isPlaying();

      paint.setStyle(SkPaint::kFill_Style);
      if (isCurrentBeat) {
        paint.setColor(SK_ColorWHITE);
      } else if (isActive) {
        paint.setColor(pad.color.getARGB());
      } else {
        // Dim step
        paint.setColor(SkColorSetA(SK_ColorWHITE, 30));
      }

      if (s % 4 == 0) {
        // Emphasize downbeats visually
        if (!isActive && !isCurrentBeat)
          paint.setColor(SkColorSetA(SK_ColorWHITE, 50));
      }

      canvas->drawRoundRect(stepRect, 2.0f, 2.0f, paint);
    }
  }
}

//==============================================================================
void DrumPadComponent::mouseDown(const juce::MouseEvent &e) {
  float x = (float)e.x;
  float y = (float)e.y;

  // Check Pads
  int padIndex = getPadIndexAt(x, y);
  if (padIndex >= 0) {
    hitPad(padIndex, 1.0f);
    return;
  }

  // Check Sequencer
  auto [seqPad, seqStep] = getSequencerStepAt(x, y);
  if (seqPad >= 0 && seqStep >= 0) {
    toggleStep(seqPad, seqStep);
    return;
  }
}

void DrumPadComponent::mouseUp(const juce::MouseEvent &e) {
  // If we implemented note off, do it here
}

void DrumPadComponent::mouseDrag(const juce::MouseEvent &e) {
  // Could implement swipe to paint steps
}

//==============================================================================
void DrumPadComponent::hitPad(int index, float velocity) {
  if (index < 0 || index >= kNumPads)
    return;

  // Animation
  pads[index].flashLevel = 1.0f;
  repaint();

  // Trigger Audio - Placeholder for future implementation
  // Currently Engine does not expose direct note injection from UI.
  // This will be connected when MIDI routing via EngineEvent is supported.
  DBG("DrumPad hit: " + juce::String(index) +
      " Velocity: " + juce::String(velocity));

  // If we were recording, we would add the note to ProjectState here.
  if (engine.isRecording() && currentClipId.isNotEmpty()) {
    auto [track, clip] = projectState.findClip(currentClipId);
    if (!clip.isValid())
        return;

    double position = engine.getPlaybackPositionBeats();
    double clipStart = static_cast<double>(clip.getProperty(zenith::ProjectState::ID_START));
    double clipLength = static_cast<double>(clip.getProperty(zenith::ProjectState::ID_LENGTH));
    double clipOffset = static_cast<double>(clip.getProperty(zenith::ProjectState::ID_OFFSET)); // Start offset

    // Calculate relative position with loop wrapping
    // Master logic adapted to current context
    double relativeStart = position - clipStart + clipOffset;
    
    // For a drum pad component, recording should always wrap within the clip's
    // length, regardless of the global transport's loop state.
    if (clipLength > 0.0) {
      relativeStart = std::fmod(relativeStart, clipLength);
      if (relativeStart < 0.0) {
        relativeStart += clipLength;
      }
    }

    zenith::ProjectState::MidiNoteSpec note;
    note.pitch = pads[index].noteNumber;
    note.startBeats = relativeStart;
    note.lengthBeats = 0.25; // Default short length for hits
    note.velocity = static_cast<int>(velocity * 127.0f);
    note.muted = false;

    projectState.addMidiNote(currentClipId, note, "Drum Pad Rec");
  }
}

void DrumPadComponent::toggleStep(int padIndex, int stepIndex) {
  if (currentClipId.isEmpty())
    return;
  if (padIndex < 0 || padIndex >= kNumPads)
    return;
  if (stepIndex < 0 || stepIndex >= kSequencerSteps)
    return;

  bool newState = !pads[padIndex].steps[stepIndex];
  pads[padIndex].steps[stepIndex] = newState; // optimistic update

  double startBeat = stepIndex * 0.25; // 16th notes
  double lengthBeat = 0.25;
  int pitch = pads[padIndex].noteNumber;

  if (newState) {
    // Add Note
    zenith::ProjectState::MidiNoteSpec note;
    note.pitch = pitch;
    note.startBeats = startBeat;
    note.lengthBeats = lengthBeat;
    note.velocity = 100;
    note.muted = false;

    projectState.addMidiNote(currentClipId, note, "Step Seq Add");
  } else {
    // Remove Note(s) at this step
    // We need to find the ID.
    // This is complex because we just stored boolean state, not IDs.
    // We should query ProjectState for notes at this time/pitch.
    auto notes = projectState.getMidiNotesForClip(currentClipId);
    for (const auto &n : notes) {
      if (n.pitch == pitch && std::abs(n.startBeats - startBeat) < 0.01) {
        projectState.removeMidiNote(currentClipId, n.id, "Step Seq Remove");
      }
    }
  }
  repaint();
}

//==============================================================================
int DrumPadComponent::getPadIndexAt(float x, float y) const {
  for (int i = 0; i < kNumPads; ++i) {
    if (pads[i].padBounds.contains(x, y))
      return i;
  }
  return -1;
}

std::pair<int, int> DrumPadComponent::getSequencerStepAt(float x,
                                                         float y) const {
  for (int i = 0; i < kNumPads; ++i) {
    if (pads[i].sequencerBounds.contains(x, y)) {
      // Find specific step
      for (int s = 0; s < kSequencerSteps; ++s) {
        if (pads[i].stepBounds[s].contains(x, y)) {
          return {i, s};
        }
      }
    }
  }
  return {-1, -1};
}

//==============================================================================
void DrumPadComponent::valueTreeChildAdded(juce::ValueTree &parent,
                                           juce::ValueTree &child) {
  if (parent.hasType(zenith::ProjectState::ID_NOTES))
    refreshData();
}

void DrumPadComponent::valueTreeChildRemoved(juce::ValueTree &parent,
                                             juce::ValueTree &child,
                                             int index) {
  if (parent.hasType(zenith::ProjectState::ID_NOTES))
    refreshData();
}

void DrumPadComponent::valueTreePropertyChanged(
    juce::ValueTree &tree, const juce::Identifier &property) {
  refreshData();
}

void DrumPadComponent::triggerPad(int noteNumber, float velocity) {
  int index = noteNumber - baseNote;
  if (index >= 0 && index < kNumPads) {
    pads[index].flashLevel = velocity; // or 1.0f
    repaint();
  }
}

void DrumPadComponent::updateAnimations() {
  bool needsUpdate = false;
  for (auto &pad : pads) {
    if (pad.flashLevel > 0.001f) {
      pad.flashLevel *= 0.85f; // Decay speed
      if (pad.flashLevel < 0.001f)
        pad.flashLevel = 0.0f;
      needsUpdate = true;
    }
  }

  if (needsUpdate)
    repaint();
}

void DrumPadComponent::timerCallback() {
  SkiaComponent::timerCallback();
  updateAnimations();
}

