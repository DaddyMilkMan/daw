/*
  ==============================================================================

    SvgIcon.cpp
    Created: 2026-02-01
    Author:  Zenith DAW

  ==============================================================================
*/

#include "SvgIcon.h"
#include "../../engine/ZenithLogger.h"
#include "../../instruments/ContentPaths.h"
#include <core/SkBlendMode.h>
#include <core/SkSize.h>
#include <core/SkStream.h>
#include <core/SkColorFilter.h>
#include <effects/SkImageFilters.h>
#include <modules/svg/include/SkSVGDOM.h>
#include <unordered_map>

namespace zenith::svgicons {
namespace {

constexpr float kSvgViewport = 24.0f;

const char *getIconBaseName(IconId id) {
  switch (id) {
  case IconId::Play:
    return "play";
  case IconId::Stop:
    return "stop";
  case IconId::Record:
    return "record";
  case IconId::Settings:
    return "settings";
  case IconId::Wing:
    return "wing";
  case IconId::ViewToggle:
    return "view_toggle";
  case IconId::File:
    return "file";
  case IconId::Edit:
    return "edit";
  case IconId::Info:
    return "info";
  }
  return "unknown";
}

juce::File getSvgFile(IconId id, bool filled) {
  auto root = ContentPaths::getInstance().getContentRoot();
  auto iconDir = root.getChildFile("Icons");
  juce::String name = getIconBaseName(id);
  name << (filled ? "_fill.svg" : "_stroke.svg");
  return iconDir.getChildFile(name);
}

sk_sp<SkSVGDOM> loadDom(const juce::File &file) {
  auto path = file.getFullPathName().toStdString();
  SkFILEStream stream(path.c_str());
  if (!stream.isValid()) {
    ZENITH_LOG_UI(zenith::LogLevel::Warning,
                  "SvgIcon: Failed to load " + file.getFullPathName());
    return nullptr;
  }

  auto dom = SkSVGDOM::MakeFromStream(stream);
  if (!dom) {
    ZENITH_LOG_UI(zenith::LogLevel::Warning,
                  "SvgIcon: Failed to parse " + file.getFullPathName());
    return nullptr;
  }

  dom->setContainerSize(SkSize::Make(kSvgViewport, kSvgViewport));
  return dom;
}

struct CacheKey {
  IconId id;
  bool filled;
  bool operator==(const CacheKey &other) const {
    return id == other.id && filled == other.filled;
  }
};

struct CacheKeyHash {
  std::size_t operator()(const CacheKey &key) const {
    return (static_cast<std::size_t>(key.id) << 1) ^ (key.filled ? 1u : 0u);
  }
};

class SvgCache {
public:
  sk_sp<SkSVGDOM> get(IconId id, bool filled) {
    CacheKey key{id, filled};
    auto it = cache_.find(key);
    if (it != cache_.end()) {
      return it->second;
    }

    auto file = getSvgFile(id, filled);
    if (!file.existsAsFile()) {
      if (!filled) {
        ZENITH_LOG_UI(zenith::LogLevel::Warning,
                      "SvgIcon: Missing " + file.getFullPathName());
      }
      return nullptr;
    }

    auto dom = loadDom(file);
    if (dom) {
      cache_.emplace(key, dom);
    }
    return dom;
  }

  void preload(std::initializer_list<IconId> ids) {
    for (auto id : ids) {
      juce::ignoreUnused(get(id, false));
      juce::ignoreUnused(get(id, true));
    }
  }

private:
  std::unordered_map<CacheKey, sk_sp<SkSVGDOM>, CacheKeyHash> cache_;
};

SvgCache &getCache() {
  static SvgCache cache;
  return cache;
}

void renderDom(SkCanvas *canvas, const sk_sp<SkSVGDOM> &dom,
               const Style &style) {
  if (!canvas || !dom) {
    return;
  }

  SkColor glowColor = style.glowColor == 0x00000000 ? style.color
                                                    : style.glowColor;
  SkColor glowTint = SkColorSetA(glowColor, 110);

  auto drawWithPaint = [&](const SkPaint &paint) {
    canvas->saveLayer(nullptr, &paint);
    dom->render(canvas);
    canvas->restore();
  };

  if (style.glowRadius > 0.0f) {
    SkPaint glowPaint;
    glowPaint.setAntiAlias(true);
    glowPaint.setColorFilter(
        SkColorFilters::Blend(glowTint, SkBlendMode::kSrcIn));
    glowPaint.setImageFilter(
        SkImageFilters::Blur(style.glowRadius, style.glowRadius, nullptr));
    drawWithPaint(glowPaint);
  }

  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColorFilter(
      SkColorFilters::Blend(style.color, SkBlendMode::kSrcIn));
  drawWithPaint(paint);
}

} // namespace

void preload(std::initializer_list<IconId> icons) {
  getCache().preload(icons);
}

void drawIconCentered(SkCanvas *canvas, IconId id, const SkRect &bounds,
                      float size, const Style &style) {
  if (!canvas)
    return;

  auto dom = getCache().get(id, style.filled);
  if (!dom) {
    dom = getCache().get(id, false);
  }
  if (!dom)
    return;

  float scale = size / kSvgViewport;
  float x = bounds.centerX() - (kSvgViewport * scale) * 0.5f;
  float y = bounds.centerY() - (kSvgViewport * scale) * 0.5f;

  canvas->save();
  canvas->translate(x, y);
  canvas->scale(scale, scale);
  renderDom(canvas, dom, style);
  canvas->restore();
}

} // namespace zenith::svgicons
