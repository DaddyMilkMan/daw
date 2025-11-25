// POLISH: spacing normalized to 8px grid (label text 12pt)
// POLISH: typography now uses SkiaTheme::Typography (tiny)

#include "PianoKeyboardViewSkia.h"
#include "../skia/SkiaTheme.h"

#ifdef ZENITH_USE_SKIA
#include <include/core/SkCanvas.h>
#include <include/core/SkFont.h>
#include <include/core/SkPaint.h>
#include <include/core/SkRRect.h>
#include <include/core/SkTextBlob.h>
#include <include/effects/SkGradientShader.h>
#endif

namespace zenith {

PianoKeyboardViewSkia::PianoKeyboardViewSkia(juce::MidiKeyboardState &state)
    : midiState(state) {
  midiState.addListener(this);
}

PianoKeyboardViewSkia::~PianoKeyboardViewSkia() {
  midiState.removeListener(this);
}

bool PianoKeyboardViewSkia::isBlackKey(int noteNumber) const {
  int n = noteNumber % 12;
  return (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
}

int PianoKeyboardViewSkia::getNoteAtPosition(juce::Point<float> pos) {
  // Simple hit testing
  // Check black keys first (they are on top)
  for (int note = rangeStart; note < rangeEnd; ++note) {
    if (isBlackKey(note)) {
      if (getKeyBounds(note).contains(pos))
        return note;
    }
  }

  // Check white keys
  for (int note = rangeStart; note < rangeEnd; ++note) {
    if (!isBlackKey(note)) {
      if (getKeyBounds(note).contains(pos))
        return note;
    }
  }

  return -1;
}

juce::Rectangle<float> PianoKeyboardViewSkia::getKeyBounds(int noteNumber) {
  if (noteNumber < rangeStart || noteNumber >= rangeEnd)
    return {};

  int whiteKeyIndex = 0;
  for (int i = rangeStart; i < noteNumber; ++i) {
    if (!isBlackKey(i))
      whiteKeyIndex++;
  }

  float x = whiteKeyIndex * whiteKeyWidth;
  float h = (float)getHeight();

  if (!isBlackKey(noteNumber)) {
    return juce::Rectangle<float>(x, 0, whiteKeyWidth, h);
  } else {
    // Black key is offset
    // C# is between C and D. C is index 0.
    // C# is offset by whiteKeyWidth - blackKeyWidth/2
    // But it depends on the pattern.
    // C D E F G A B
    // W B W B W W B W B W B W

    // Offset from the previous white key's right edge
    // C# (1) -> after C (0)
    // D# (3) -> after D (2)
    // F# (6) -> after F (5)
    // G# (8) -> after G (7)
    // A# (10) -> after A (9)

    float offset = x - (blackKeyWidth / 2.0f);

    // Adjust for specific keys to look nice
    int n = noteNumber % 12;
    if (n == 1)
      offset += blackKeyWidth * 0.1f; // C#
    if (n == 3)
      offset -= blackKeyWidth * 0.1f; // D#
    if (n == 6)
      offset += blackKeyWidth * 0.1f; // F#
    // G# centered
    if (n == 10)
      offset -= blackKeyWidth * 0.1f; // A#

    return juce::Rectangle<float>(offset, 0, blackKeyWidth,
                                  h * blackKeyHeightRatio);
  }
}

#ifdef ZENITH_USE_SKIA
void PianoKeyboardViewSkia::paintSkia(SkCanvas &canvas,
                                      const juce::Rectangle<int> &bounds) {
  auto &theme = SkiaTheme::getInstance();
  auto colors = theme.getColors();

  // Background
  canvas.clear(colors.bg0);

  // Draw White Keys
  SkPaint whiteKeyPaint;
  whiteKeyPaint.setColor(SkColorSetRGB(227, 230, 235)); // Light grey #E3E6EB
  whiteKeyPaint.setAntiAlias(true);

  SkPaint whiteKeyStroke;
  whiteKeyStroke.setColor(SkColorSetRGB(178, 184, 196)); // #B2B8C4
  whiteKeyStroke.setStyle(SkPaint::kStroke_Style);
  whiteKeyStroke.setStrokeWidth(1.0f);
  whiteKeyStroke.setAntiAlias(true);

  for (int note = rangeStart; note < rangeEnd; ++note) {
    if (!isBlackKey(note)) {
      auto rect = getKeyBounds(note);
      SkRect skRect = SkRect::MakeXYWH(rect.getX(), rect.getY(),
                                       rect.getWidth(), rect.getHeight());

      // Active note glow
      if (midiState.isNoteOn(1, note)) {
        // Glow effect
        SkPaint glowPaint;
        glowPaint.setColor(colors.accentMain);
        glowPaint.setAlpha(180);
        glowPaint.setAntiAlias(true);
        canvas.drawRect(skRect, glowPaint);

        // Top gradient overlay
        SkPoint gradPoints[2] = {SkPoint::Make(skRect.centerX(), skRect.top()),
                                 SkPoint::Make(skRect.centerX(), skRect.bottom())};
        SkColor gradColors[2] = {
            SkColorSetARGB(60, SkColorGetR(colors.accentMain),
                           SkColorGetG(colors.accentMain),
                           SkColorGetB(colors.accentMain)),
            SkColorSetARGB(20, SkColorGetR(colors.accentMain),
                           SkColorGetG(colors.accentMain),
                           SkColorGetB(colors.accentMain))};
        sk_sp<SkShader> gradient =
            SkGradientShader::MakeLinear(gradPoints, gradColors, nullptr, 2,
                                         SkTileMode::kClamp);
        SkPaint gradPaint;
        gradPaint.setShader(gradient);
        gradPaint.setAntiAlias(true);
        canvas.drawRect(skRect, gradPaint);
      } else {
        canvas.drawRect(skRect, whiteKeyPaint);
      }

      canvas.drawRect(skRect, whiteKeyStroke);

      // C labels
      if (note % 12 == 0) {
        auto& typo = SkiaTheme::getInstance().getTypography();
        SkFont font;
        font.setSize(typo.tiny.size);
        font.setEdging(SkFont::Edging::kAntiAlias);

        SkPaint textPaint;
        textPaint.setColor(colors.textSubtle);
        textPaint.setAntiAlias(true);

        int octave = (note / 12) - 1;
        juce::String label = "C" + juce::String(octave);

        canvas.drawString(label.toRawUTF8(), skRect.fLeft + 8,
                          skRect.fBottom - 8, font, textPaint);
      }
    }
  }

  // Draw Black Keys
  for (int note = rangeStart; note < rangeEnd; ++note) {
    if (isBlackKey(note)) {
      auto rect = getKeyBounds(note);
      SkRect skRect = SkRect::MakeXYWH(rect.getX(), rect.getY(),
                                       rect.getWidth(), rect.getHeight());

      if (midiState.isNoteOn(1, note)) {
        // Active state
        SkPaint activePaint;
        activePaint.setColor(colors.accentMain);
        activePaint.setAntiAlias(true);
        canvas.drawRect(skRect, activePaint);
      } else {
        // Normal state
        SkPaint blackKeyPaint;
        blackKeyPaint.setColor(SkColorSetRGB(37, 42, 51)); // Dark #252A33
        blackKeyPaint.setAntiAlias(true);
        canvas.drawRect(skRect, blackKeyPaint);

        // Top highlight
        SkPaint highlight;
        highlight.setColor(SkColorSetARGB(30, 255, 255, 255));
        highlight.setAntiAlias(true);
        SkRect highlightRect =
            SkRect::MakeXYWH(skRect.fLeft, skRect.fTop, skRect.width(),
                             skRect.height() * 0.2f);
        canvas.drawRect(highlightRect, highlight);
      }

      // Border
      SkPaint borderPaint;
      borderPaint.setColor(colors.bg0);
      borderPaint.setStyle(SkPaint::kStroke_Style);
      borderPaint.setStrokeWidth(1.0f);
      borderPaint.setAntiAlias(true);
      canvas.drawRect(skRect, borderPaint);
    }
  }
}
#endif

void PianoKeyboardViewSkia::mouseDown(const juce::MouseEvent &e) {
  int note = getNoteAtPosition(e.position);
  if (note != -1) {
    currentNote = note;
    midiState.noteOn(1, currentNote, 0.8f);
  }
}

void PianoKeyboardViewSkia::mouseUp(const juce::MouseEvent &e) {
  if (currentNote != -1) {
    midiState.noteOff(1, currentNote, 0.0f);
    currentNote = -1;
  }
}

void PianoKeyboardViewSkia::mouseDrag(const juce::MouseEvent &e) {
  int note = getNoteAtPosition(e.position);
  if (note != -1 && note != currentNote) {
    if (currentNote != -1)
      midiState.noteOff(1, currentNote, 0.0f);

    currentNote = note;
    midiState.noteOn(1, currentNote, 0.8f);
  }
}

void PianoKeyboardViewSkia::handleNoteOn(juce::MidiKeyboardState *,
                                         int midiChannel, int midiNoteNumber,
                                         float velocity) {
  repaint();
}

void PianoKeyboardViewSkia::handleNoteOff(juce::MidiKeyboardState *,
                                          int midiChannel, int midiNoteNumber,
                                          float velocity) {
  repaint();
}

} // namespace zenith
