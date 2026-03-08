#include "IndustrialTheme.h"
#include <juce_core/juce_core.h>

#ifdef ZENITH_USE_SKIA
#include <include/core/SkFontMgr.h>

// Platform-specific includes
#ifdef __linux__
#include <include/ports/SkFontMgr_fontconfig.h>
#include <include/ports/SkFontScanner_FreeType.h>
#endif
#include <effects/SkGradientShader.h>
#endif

namespace zenith::industrial {

void IndustrialTheme::initializeFonts() {
#ifdef ZENITH_USE_SKIA
  sk_sp<SkFontMgr> fontMgr = SkFontMgr::RefEmpty();
#ifdef __linux__
  fontMgr = SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
  if (!fontMgr) {
    fontMgr = SkFontMgr::RefEmpty();
  }
#endif

  const juce::StringArray filenames{
      "IBMPlexMono-Regular.ttf", "IBMPlexMono-Medium.ttf",
      "IBMPlexMono-SemiBold.ttf", "IBMPlexMono-Bold.ttf"};

  juce::Array<juce::File> roots;
  roots.add(juce::File::getCurrentWorkingDirectory());
  roots.add(juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                .getParentDirectory());
  roots.add(juce::File::getSpecialLocation(juce::File::currentApplicationFile)
                .getParentDirectory());

  for (int i = 0; i < roots.size(); ++i) {
    auto parent = roots.getReference(i);
    for (int depth = 0; depth < 6 && parent != parent.getParentDirectory();
         ++depth) {
      roots.addIfNotAlreadyThere(parent);
      parent = parent.getParentDirectory();
    }
  }

  auto findFont = [&roots](const juce::String &filename) {
    for (const auto &root : roots) {
      juce::File candidate =
          root.getChildFile(
                  "apps/desktop/Source/ui/industrial/fonts/IBMPlexMono")
              .getChildFile(filename);
      if (candidate.existsAsFile()) {
        return candidate;
      }

      candidate = root.getChildFile("fonts/IBMPlexMono").getChildFile(filename);
      if (candidate.existsAsFile()) {
        return candidate;
      }
    }
    return juce::File();
  };

  for (int i = 0; i < filenames.size(); ++i) {
    const auto fontFile = findFont(filenames[i]);
    if (fontFile.existsAsFile()) {
      typefaces_[static_cast<size_t>(i)] =
          fontMgr->makeFromFile(fontFile.getFullPathName().toRawUTF8());
    }
  }

  for (auto &face : typefaces_) {
    if (!face) {
      face = fontMgr->legacyMakeTypeface("monospace", SkFontStyle());
    }
  }
#endif
}

#ifdef ZENITH_USE_SKIA
sk_sp<SkTypeface> IndustrialTheme::getTypeface(FontWeight weight) const {
  const auto index = static_cast<int>(weight);
  if (index >= 0 && index < static_cast<int>(typefaces_.size()) &&
      typefaces_[index]) {
    return typefaces_[index];
  }
  // Fallback to empty typeface (will use default)
  return nullptr;
}
#endif

float IndustrialTheme::carbonContrastForMode(UIMode mode) const {
  switch (mode) {
  case UIMode::Beginner:
    return 0.03f;
  case UIMode::Advanced:
    return 0.08f;
  case UIMode::Expert:
    return 0.15f;
  }
  return 0.08f;
}

#ifdef ZENITH_USE_SKIA
void IndustrialTheme::drawMetallicGradient(SkCanvas* canvas, const SkRect& rect, float angleDegrees) const {
    SkColor colors[] = {
        SkColorSetRGB(60, 60, 60),   // Dark edge
        SkColorSetRGB(180, 180, 180), // Highlight
        SkColorSetRGB(80, 80, 80),    // Mid-tone
        SkColorSetRGB(140, 140, 140), // Secondary highlight
        SkColorSetRGB(50, 50, 50)     // Dark edge
    };
    SkScalar pos[] = { 0.0f, 0.2f, 0.5f, 0.8f, 1.0f };
    
    // Rotate gradient points based on angle
    SkPoint pts[2] = { {rect.fLeft, rect.fTop}, {rect.fRight, rect.fBottom} }; // Simple linear approximation for now
    
    SkPaint paint;
    paint.setShader(SkGradientShader::MakeLinear(pts, colors, pos, 5, SkTileMode::kClamp));
    paint.setAntiAlias(true);
    
    canvas->drawRect(rect, paint);
    
    // Add noise for "brushed" effect (simulated with lines)
    SkPaint noisePaint;
    noisePaint.setColor(SkColorSetARGB(30, 255, 255, 255));
    noisePaint.setStyle(SkPaint::kStroke_Style);
    noisePaint.setStrokeWidth(1.0f);
    noisePaint.setAntiAlias(true);
    noisePaint.setBlendMode(SkBlendMode::kOverlay);
    
    for (int i = 0; i < 10; ++i) {
        float y = rect.fTop + (rect.height() * (i / 10.0f));
        canvas->drawLine(rect.fLeft, y, rect.fRight, y, noisePaint);
    }
}

void IndustrialTheme::drawHexScrew(SkCanvas* canvas, float cx, float cy, float radius) const {
    // Screw Head (Dark Metal)
    SkRect headRect = SkRect::MakeXYWH(cx - radius, cy - radius, radius * 2, radius * 2);
    SkColor headColors[] = { SkColorSetRGB(40, 40, 40), SkColorSetRGB(20, 20, 20) };
    SkPaint headPaint;
    headPaint.setShader(SkGradientShader::MakeRadial({cx, cy}, radius, headColors, nullptr, 2, SkTileMode::kClamp));
    headPaint.setAntiAlias(true);
    canvas->drawOval(headRect, headPaint);
    
    // Hex Socket (Inset)
    SkPath hexPath;
    float hexRad = radius * 0.6f;
    for (int i = 0; i < 6; ++i) {
        float angle = (i * 60.0f) * (3.14159f / 180.0f);
        float px = cx + hexRad * std::cos(angle);
        float py = cy + hexRad * std::sin(angle);
        if (i == 0) hexPath.moveTo(px, py);
        else hexPath.lineTo(px, py);
    }
    hexPath.close();
    
    SkPaint socketPaint;
    socketPaint.setColor(SkColorSetRGB(10, 10, 10)); // Almost black
    socketPaint.setStyle(SkPaint::kFill_Style);
    socketPaint.setAntiAlias(true);
    canvas->drawPath(hexPath, socketPaint);
    
    // Specular Highlight on rim
    SkPaint highlightPaint;
    highlightPaint.setColor(SkColorSetARGB(100, 255, 255, 255));
    highlightPaint.setStyle(SkPaint::kStroke_Style);
    highlightPaint.setStrokeWidth(1.0f);
    highlightPaint.setAntiAlias(true);
    canvas->drawArc(headRect, 225, 90, false, highlightPaint);
}
#endif

} // namespace zenith::industrial
