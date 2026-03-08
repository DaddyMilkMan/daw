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
#include <cmath>

namespace zenith {
using namespace design;

BrowserPreviewPanel::BrowserPreviewPanel(BrowserPreviewEngine &engine) 
    : engine_(engine), 
      thumbnailCache_(5),
      thumbnail_(512, formatManager_, thumbnailCache_) {
  formatManager_.registerBasicFormats();
  thumbnail_.addChangeListener(this);
}

BrowserPreviewPanel::~BrowserPreviewPanel() {
  thumbnail_.removeChangeListener(this);
}

void BrowserPreviewPanel::loadWaveform(const juce::File &file) {
  if (file == waveformFile_) return;
  
  waveformFile_ = file;
  waveformPathDirty_ = true;
  
  if (file.existsAsFile()) {
    auto* inputSource = new juce::FileInputSource(file);
    thumbnail_.setSource(inputSource);
  } else {
    thumbnail_.clear();
  }
  
  repaint();
}

void BrowserPreviewPanel::clearWaveform() {
  waveformFile_ = juce::File();
  thumbnail_.clear();
  waveformPathDirty_ = true;
  repaint();
}

void BrowserPreviewPanel::changeListenerCallback(juce::ChangeBroadcaster* source) {
  if (source == &thumbnail_) {
    waveformPathDirty_ = true;
    repaint();
  }
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

  if (thumbnail_.getTotalLength() <= 0.0) {
    SkFont f = zenith::design::getSkFont(zenith::design::typography::FONT_SM);
    SkPaint tp; tp.setColor(design::withAlpha(colors::TEXT_PRIMARY, 0.25f));
    canvas->drawString("Select audio to preview", bounds.centerX() - 65, bounds.centerY() + 4, f, tp);
    return;
  }

  updateWaveformPath(bounds);

  SkColor wcols[3] = {design::withAlpha(unified::waveform_audio(), 0.8f), 
                     design::withAlpha(unified::waveform_audio(), 0.6f), 
                     design::withAlpha(unified::waveform_audio(), 0.4f)};
  float pos[3] = {0, 0.5f, 1};
  SkPaint wp; wp.setShader(SkGradientShader::MakeLinear(pts, wcols, pos, 3, SkTileMode::kClamp));
  wp.setAntiAlias(true);
  canvas->drawPath(cachedWaveformPath_, wp);

  if (engine_.isLoaded()) {
    float px = bounds.x() + engine_.getPlaybackPosition() * bounds.width();
    SkPaint pp; pp.setColor(colors::AMBER); pp.setStrokeWidth(1.5f);
    canvas->drawLine(px, bounds.y() + 4, px, bounds.y() + bounds.height() - 4, pp);
  }
}

void BrowserPreviewPanel::updateWaveformPath(const SkRect& bounds) {
  if (!waveformPathDirty_ && lastWaveformBounds_ == bounds) return;

  cachedWaveformPath_.reset();
  
  const int numPoints = 200;
  const double duration = thumbnail_.getTotalLength();
  
  if (duration > 0.0) {
    float cy = bounds.centerY();
    float mh = bounds.height() * 0.42f;
    float pw = bounds.width() / numPoints;
    
    for (int i = 0; i < numPoints; ++i) {
      float x = bounds.x() + i * pw;
      const double segStart = duration * ((double)i / (double)numPoints);
      const double segEnd = duration * ((double)(i + 1) / (double)numPoints);
      float minLevel = 0.0f, maxLevel = 0.0f;
      thumbnail_.getApproximateMinMax(segStart, segEnd, 0, minLevel, maxLevel);
      const float amp = std::max(std::abs(minLevel), std::abs(maxLevel));
      float h = std::max(0.01f, amp) * mh;
      if (i == 0) cachedWaveformPath_.moveTo(x, cy - h); 
      else cachedWaveformPath_.lineTo(x, cy - h);
    }
    
    for (int i = numPoints - 1; i >= 0; --i) {
      float x = bounds.x() + i * pw;
      const double segStart = duration * ((double)i / (double)numPoints);
      const double segEnd = duration * ((double)(i + 1) / (double)numPoints);
      float minLevel = 0.0f, maxLevel = 0.0f;
      thumbnail_.getApproximateMinMax(segStart, segEnd, 0, minLevel, maxLevel);
      const float amp = std::max(std::abs(minLevel), std::abs(maxLevel));
      float h = std::max(0.01f, amp) * mh;
      cachedWaveformPath_.lineTo(x, cy + h);
    }
    
    cachedWaveformPath_.close();
  }

  waveformPathDirty_ = false;
  lastWaveformBounds_ = bounds;
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
