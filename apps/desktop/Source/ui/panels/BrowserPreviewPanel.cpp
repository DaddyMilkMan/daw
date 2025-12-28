/*
  ==============================================================================

    BrowserPreviewPanel.cpp
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/

#include "BrowserPreviewPanel.h"
#include "../design-system/ZenithDesignSystem.h"
#include "../design-system/ColorBridge.h"
#include "ZenithIcons.h"
#include <effects/SkGradientShader.h>

namespace zenith {
using namespace design;

BrowserPreviewPanel::BrowserPreviewPanel(BrowserPreviewEngine &engine) : engine_(engine) {
}

BrowserPreviewPanel::~BrowserPreviewPanel() {
}

void BrowserPreviewPanel::loadWaveform(const juce::File &file) {
  if (file == waveformFile_ && !waveformData_.empty()) return;
  waveformData_.clear();
  waveformFile_ = file;
  if (!file.existsAsFile()) return;

  juce::AudioFormatManager fm; fm.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file));
  if (!reader) return;

  const int numPoints = 200;
  const int64_t samplesPerPoint = reader->lengthInSamples / numPoints;
  if (samplesPerPoint <= 0) return;

  juce::AudioBuffer<float> buffer(1, (int)samplesPerPoint);
  waveformData_.reserve(numPoints);
  for (int i = 0; i < numPoints; ++i) {
    buffer.clear();
    reader->read(&buffer, 0, (int)samplesPerPoint, i * samplesPerPoint, true, false);
    float maxVal = 0.0f;
    for (int s = 0; s < buffer.getNumSamples(); ++s) maxVal = std::max(maxVal, std::abs(buffer.getSample(0, s)));
    waveformData_.push_back(maxVal);
  }
  repaint();
}

void BrowserPreviewPanel::clearWaveform() {
  waveformData_.clear();
  waveformFile_ = juce::File();
  repaint();
}

void BrowserPreviewPanel::drawSkia(SkCanvas *canvas) {
  auto bounds = getLocalBounds();
  SkPaint bgPaint; bgPaint.setColor(unified::bg_02());
  canvas->drawRect(SkRect::MakeXYWH(0, 0, (float)bounds.getWidth(), (float)bounds.getHeight()), bgPaint);

  SkPaint borderPaint; borderPaint.setColor(colors::BORDER_DEFAULT);
  canvas->drawLine(0, 0, (float)bounds.getWidth(), 0, borderPaint);

  drawIconButton(canvas, playButtonBounds_, engine_.isPlaying() ? icons::Pause() : icons::Play(), engine_.isPlaying());
  drawIconButton(canvas, stopButtonBounds_, icons::Stop(), false);
  drawIconButton(canvas, loopButtonBounds_, icons::Loop(), engine_.isLooping());
  drawButton(canvas, autoPlayButtonBounds_, "AUTO", engine_.isAutoPlayEnabled());

  if (engine_.isLoaded()) {
    SkFont nf = design::getSkFont(11.0f, design::FontWeight::Medium);
    SkPaint np; np.setColor(colors::TEXT_PRIMARY); np.setAntiAlias(true);
    juce::String name = engine_.getCurrentFile().getFileNameWithoutExtension();
    if (name.length() > 25) name = name.substring(0, 23) + "...";
    canvas->drawString(name.toStdString().c_str(), (float)autoPlayButtonBounds_.getRight() + 10, 18+8, nf, np);
  }

  drawWaveform(canvas, SkRect::MakeXYWH((float)waveformBounds_.getX(), (float)waveformBounds_.getY(), (float)waveformBounds_.getWidth(), (float)waveformBounds_.getHeight()));
}

void BrowserPreviewPanel::drawWaveform(SkCanvas *canvas, const SkRect &bounds) {
  SkPaint bg;
  SkPoint pts[2] = {{0, bounds.fTop}, {0, bounds.fBottom}};
  SkColor cols[2] = {colors::BG_DARKEST, colors::BG_DARKER};
  bg.setShader(SkGradientShader::MakeLinear(pts, cols, nullptr, 2, SkTileMode::kClamp));
  canvas->drawRoundRect(bounds, 6, 6, bg);

  if (waveformData_.empty()) {
    SkFont f = zenith::design::getSkFont(zenith::design::typography::FONT_SM);
    SkPaint tp; tp.setColor(design::withAlpha(colors::TEXT_PRIMARY, 0.25f));
    canvas->drawString("Select audio to preview", bounds.centerX() - 65, bounds.centerY() + 4, f, tp);
    return;
  }

  float cy = bounds.centerY();
  float mh = bounds.height() * 0.42f;
  float pw = bounds.width() / waveformData_.size();
  SkPath path;
  for (size_t i = 0; i < waveformData_.size(); ++i) {
    float x = bounds.x() + i * pw;
    float h = waveformData_[i] * mh;
    if (i == 0) path.moveTo(x, cy - h); else path.lineTo(x, cy - h);
  }
  for (int i = (int)waveformData_.size() - 1; i >= 0; --i) {
    float x = bounds.x() + i * pw;
    float h = waveformData_[i] * mh;
    path.lineTo(x, cy + h);
  }
  path.close();

  SkColor wcols[3] = {design::withAlpha(unified::waveform_audio(), 0.8f), 
                     design::withAlpha(unified::waveform_audio(), 0.6f), 
                     design::withAlpha(unified::waveform_audio(), 0.4f)};
  float pos[3] = {0, 0.5f, 1};
  SkPaint wp; wp.setShader(SkGradientShader::MakeLinear(pts, wcols, pos, 3, SkTileMode::kClamp));
  wp.setAntiAlias(true);
  canvas->drawPath(path, wp);

  if (engine_.isLoaded()) {
    float px = bounds.x() + engine_.getPlaybackPosition() * bounds.width();
    SkPaint pp; pp.setColor(colors::AMBER); pp.setStrokeWidth(1.5f);
    canvas->drawLine(px, bounds.y() + 4, px, bounds.y() + bounds.height() - 4, pp);
  }
}

void BrowserPreviewPanel::drawIconButton(SkCanvas *canvas, const juce::Rectangle<int> &bounds, const SkPath &iconPath, bool active) {
  SkPaint bg; bg.setColor(active ? colors::ACCENT_PRIMARY : colors::BG_MEDIUM); bg.setAntiAlias(true);
  canvas->drawRoundRect(SkRect::MakeXYWH((float)bounds.getX(), (float)bounds.getY(), (float)bounds.getWidth(), (float)bounds.getHeight()), 4, 4, bg);
  icons::IconStyle s; s.color = active ? colors::TEXT_PRIMARY : design::withAlpha(colors::TEXT_PRIMARY, 0.8f); s.filled = active; s.strokeWidth = 1.8f;
  icons::drawIconCentered(canvas, iconPath, SkRect::MakeXYWH((float)bounds.getX(), (float)bounds.getY(), (float)bounds.getWidth(), (float)bounds.getHeight()), bounds.getWidth()*0.5f, s);
}

void BrowserPreviewPanel::drawButton(SkCanvas *canvas, const juce::Rectangle<int> &bounds, const juce::String &text, bool active) {
  SkPaint bg; bg.setColor(active ? colors::ACCENT_PRIMARY : colors::BG_MEDIUM);
  canvas->drawRoundRect(SkRect::MakeXYWH((float)bounds.getX(), (float)bounds.getY(), (float)bounds.getWidth(), (float)bounds.getHeight()), 4, 4, bg);
  SkFont f; f.setSize(11); f.setEdging(SkFont::Edging::kAntiAlias);
  SkPaint tp; tp.setColor(colors::TEXT_PRIMARY); tp.setAntiAlias(true);
  SkRect tb; f.measureText(text.toStdString().c_str(), text.length(), SkTextEncoding::kUTF8, &tb);
  canvas->drawString(text.toStdString().c_str(), (float)bounds.getCentreX() - tb.width()/2, (float)bounds.getCentreY() + tb.height()/2 - 1, f, tp);
}

void BrowserPreviewPanel::mouseDown(const juce::MouseEvent &e) {
  if (playButtonBounds_.contains(e.getPosition())) { engine_.togglePlayback(); repaint(); }
  else if (stopButtonBounds_.contains(e.getPosition())) { engine_.stop(); repaint(); }
  else if (loopButtonBounds_.contains(e.getPosition())) { engine_.setLooping(!engine_.isLooping()); repaint(); }
  else if (autoPlayButtonBounds_.contains(e.getPosition())) { engine_.setAutoPlayEnabled(!engine_.isAutoPlayEnabled()); repaint(); }
}

void BrowserPreviewPanel::resized() {
  auto b = getLocalBounds();
  int p = 8, s = 28;
  int x = p, y = p;
  playButtonBounds_ = {x, y, s, s}; x += s + 4;
  stopButtonBounds_ = {x, y, s, s}; x += s + 4;
  loopButtonBounds_ = {x, y, s, s}; x += s + 8;
  autoPlayButtonBounds_ = {x, y, s + 20, s};
  waveformBounds_ = {p, y + s + 4, b.getWidth() - p * 2, b.getHeight() - s - p * 2 - 4};
}

} // namespace zenith
