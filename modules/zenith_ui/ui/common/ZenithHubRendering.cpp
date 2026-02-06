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

#include "zenith_core/engine/ZenithLogger.h"
#include "../design-system/ZenithTypography.h"
#include <map>

namespace zenith {

using namespace design;

static const std::map<juce::String, SkPath (*)()> kGenreIconMap = {
    {"electronic", &icons::Synth},    {"techno", &icons::Synth},
    {"edm", &icons::Synth},           {"synth", &icons::Synth},
    {"cinematic", &icons::MusicNote}, {"orchestral", &icons::MusicNote},
    {"jazz", &icons::MusicNote},      {"ambient", &icons::Cloud},
    {"chill", &icons::Cloud},         {"rock", &icons::Waveform},
    {"metal", &icons::Waveform}};

static const std::map<juce::String, SkPath (*)()> kTemplateIconMap = {
    {"icon_synth", &icons::Synth},
    {"icon_note", &icons::MusicNote},
    {"icon_mic", &icons::Microphone}};

SkColor ZenithHubComponent::getAccentColorForGenre(const juce::String &genre) {
  juce::String g = genre.toLowerCase();
  if (g.contains("electronic") || g.contains("edm") || g.contains("synth"))
    return colors::CYAN;
  if (g.contains("orchestral") || g.contains("cinematic") ||
      g.contains("score"))
    return colors::VIOLET;
  if (g.contains("jazz") || g.contains("swing"))
    return colors::NEON_PINK;
  if (g.contains("techno") || g.contains("house") || g.contains("dance"))
    return colors::NEON_GREEN;
  if (g.contains("ambient") || g.contains("chill"))
    return colors::BLUE;
  if (g.contains("rock") || g.contains("metal"))
    return colors::AMBER;
  if (g.contains("hip") || g.contains("rap") || g.contains("trap"))
    return colors::MAGENTA;
  return colors::CYAN;
}

void ZenithHubComponent::drawSkia(SkCanvas *canvas) {
  float opacity = alpha_.get();
  
  if (mainCardBounds_.isEmpty() && getWidth() >= 400 && getHeight() >= 300) {
      updateLayout();
  }

  if (opacity <= 0.001f)
    return;

  if (mainCardBounds_.isEmpty()) {
    drawBackground(canvas);
    return;
  }

  if (opacity < 0.999f) {
      canvas->saveLayerAlpha(nullptr, (U8CPU)(opacity * 255));
  } else {
      canvas->save();
  }
  
  drawBackground(canvas);

  GlassmorphicPanel::draw(canvas, mainCardBounds_,
                          GlassmorphicPanel::Style::Elevated);

  textPaint_.setColor(colors::TEXT_PRIMARY);

  // Position header text - DRAMATIC move: more left and more down
  float headerX = mainCardBounds_.fLeft + 10;   // Even more left (was 20)
  float headerY = mainCardBounds_.fTop + 120;   // Even more down (was 100)

  SkRect titleBounds =
      SkRect::MakeXYWH(headerX, headerY - 50, mainCardBounds_.width() - 80, 60);
  drawText(canvas, "Zenith Hub", titleBounds, titleFont_, textPaint_, false);

  subPaint_.setColor(withAlpha(colors::TEXT_PRIMARY, opacity::GLOW_STRONG));

  SkString greeting(greetingText_.toRawUTF8());
  SkRect bounds;
  subFont_.measureText(greeting.c_str(), greeting.size(), SkTextEncoding::kUTF8,
                       &bounds);

  float subX = headerX;
  float subY = headerY + 32;

  greetingTextBounds_ = SkRect::MakeXYWH(subX, subY - bounds.height(),
                                         bounds.width(), bounds.height() + 4);

  SkRect helperBounds = SkRect::MakeXYWH(subX, subY - bounds.height(),
                                         bounds.width(), bounds.height());
  
  juce::String currentGreeting = greetingText_;
  
  drawText(canvas, currentGreeting, helperBounds, subFont_, subPaint_, false);

  constexpr float kGreetingIconSize = 16.0f;

  greetingEditIconBounds_ =
      SkRect::MakeXYWH(subX + bounds.width() + 10, subY - 14, kGreetingIconSize,
                       kGreetingIconSize);

  icons::IconStyle iconStyle;
  iconStyle.color = isGreetingHovered_
                        ? colors::CYAN
                        : withAlpha(colors::TEXT_SECONDARY, opacity::SECONDARY);
  iconStyle.strokeWidth = icons::STROKE_THIN;

  icons::drawIconCentered(canvas, icons::Edit(), greetingEditIconBounds_,
                          kGreetingIconSize, iconStyle);

  drawRecentProjects(canvas);
  drawNewProjectButton(canvas);
  drawTemplates(canvas);
  drawProfileIcon(canvas);
  drawProfileMenu(canvas);

  canvas->restore();
}

void ZenithHubComponent::drawBackground(SkCanvas *canvas) {
    // Shared Aurora flow handled elsewhere
}

void ZenithHubComponent::drawProjectList(SkCanvas *canvas) {
    drawRecentProjects(canvas);
}

void ZenithHubComponent::drawRecentProjects(SkCanvas *canvas) {
  std::lock_guard<std::mutex> lock(projectsMutex_);
  textPaint_.setColor(colors::TEXT_PRIMARY);
  drawText(canvas, "Recent Projects", recentHeaderBounds_, headerFont_, textPaint_, true);

  if (recentProjects_.empty()) {
    SkPaint emptyStatePaint = textPaint_;
    emptyStatePaint.setColor(withAlpha(colors::TEXT_PRIMARY, opacity::GLASS_SOLID));
    drawText(
        canvas, "No recent projects yet.",
        SkRect::MakeXYWH(recentGridBounds_.fLeft, recentGridBounds_.fTop, 300, 20),
        bodyFont_, emptyStatePaint, false);
    return;
  }

  for (size_t i = 0; i < recentProjects_.size(); ++i) {
    const auto &proj = recentProjects_[i];
    bool isSelected = (selectedSection_ == SelectionSection::Recent &&
                       (int)i == selectedIndex_);

    bool active = proj.isHovered || isSelected;

    SkRRect rrect = SkRRect::MakeRectXY(proj.bounds, dimensions::RADIUS_MD, dimensions::RADIUS_MD);
    SkPaint cardPaint;
    cardPaint.setColor(active ? withAlpha(colors::BG_LIGHT, opacity::ACTIVE)
                              : withAlpha(colors::BG_LIGHT, opacity::HOVER));
    cardPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, cardPaint);

    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(isSelected ? 2.0f : 1.0f);
    borderPaint.setColor(active ? withAlpha(proj.accent, opacity::GLOW_STRONG)
                                : withAlpha(colors::TEXT_PRIMARY, design::opacity::GLASS_SUBTLE));

    borderPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, borderPaint);

    SkRect thumbRect =
        SkRect::MakeXYWH(proj.bounds.fLeft + 12, proj.bounds.fTop + 12, 86, 86);
    SkPaint thumbPaint;
    thumbPaint.setColor(withAlpha(proj.accent, opacity::SELECTED));
    thumbPaint.setAntiAlias(true);
    canvas->drawRRect(SkRRect::MakeRectXY(thumbRect, dimensions::RADIUS_SM, dimensions::RADIUS_SM), thumbPaint);

    SkPath iconPath = icons::Project();
    auto it = kGenreIconMap.find(proj.genre.toLowerCase());
    if (it != kGenreIconMap.end()) {
      iconPath = it->second();
    }

    icons::IconStyle iconStyle;
    iconStyle.color = active ? proj.accent : withAlpha(proj.accent, opacity::GLOW_STRONG);
    iconStyle.strokeWidth = icons::STROKE_REGULAR;
    if (active) {
      iconStyle.glowRadius = 4.0f;
      iconStyle.glowColor = withAlpha(proj.accent, opacity::GLOW_MEDIUM);
    }

    icons::drawIconCentered(canvas, iconPath, thumbRect, 40.0f, iconStyle);

    float textX = thumbRect.right() + 16;
    drawText(canvas, proj.name,
             SkRect::MakeXYWH(textX, proj.bounds.fTop + 20,
                              proj.bounds.width() - 110, 24),
             cardTitleFont_, textPaint_, false);
    drawText(canvas, proj.date,
             SkRect::MakeXYWH(textX, proj.bounds.fTop + 45,
                              proj.bounds.width() - 110, 20),
             cardDateFont_, subPaint_, false);

    if (proj.genre.isNotEmpty()) {
      SkPaint genrePaint = textPaint_;
      genrePaint.setColor(proj.accent);
      drawText(canvas, proj.genre,
               SkRect::MakeXYWH(textX, proj.bounds.fTop + 65,
                                proj.bounds.width() - 110, 20),
               cardGenreFont_, genrePaint, false);
    }
  }
}

void ZenithHubComponent::drawTemplates(SkCanvas *canvas) {
  if (templatesArea_.isEmpty() || quickStartHeaderBounds_.isEmpty()) {
    return;
  }

  drawText(
      canvas, "Quick Start",
      quickStartHeaderBounds_,
      headerFont_, textPaint_, true);

  for (size_t i = 0; i < templates_.size(); ++i) {
    const auto &tmpl = templates_[i];
    bool isSelected = (this->selectedSection_ == SelectionSection::Templates &&
                       (int)i == selectedIndex_);

    bool active = tmpl.isHovered || isSelected;

    SkRRect rrect = SkRRect::MakeRectXY(tmpl.bounds, dimensions::RADIUS_MD, dimensions::RADIUS_MD);
    SkPaint cardPaint;
    cardPaint.setColor(active ? withAlpha(tmpl.color, opacity::ACTIVE)
                              : withAlpha(colors::BG_LIGHT, opacity::HOVER));
    cardPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, cardPaint);

    SkPaint borderPaint;
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(isSelected ? 2.0f : 1.0f);
    borderPaint.setColor(active ? withAlpha(tmpl.color, opacity::GLOW_STRONG)
                                : withAlpha(colors::TEXT_PRIMARY, design::opacity::GLASS_SUBTLE));
    borderPaint.setAntiAlias(true);
    canvas->drawRRect(rrect, borderPaint);

    float iconSize = 48.0f;
    SkRect iconBounds = SkRect::MakeXYWH(
        tmpl.bounds.fLeft + 20, tmpl.bounds.centerY() - iconSize * 0.5f,
        iconSize, iconSize);
    SkPaint iconBgPaint;
    iconBgPaint.setColor(withAlpha(tmpl.color, opacity::FOCUS));
    iconBgPaint.setAntiAlias(true);
    canvas->drawRoundRect(iconBounds, 8.0f, 8.0f, iconBgPaint);

    SkPath iconPath = icons::Template();
    auto it = kTemplateIconMap.find(tmpl.icon);
    if (it != kTemplateIconMap.end()) {
      iconPath = it->second();
    }

    icons::IconStyle iconStyle;
    iconStyle.color = tmpl.color;
    iconStyle.strokeWidth = active ? icons::STROKE_BOLD : icons::STROKE_REGULAR;
    if (active) {
      iconStyle.glowRadius = 8.0f;
      iconStyle.glowColor = withAlpha(tmpl.color, opacity::SECONDARY);
    }
    icons::drawIconCentered(canvas, iconPath, iconBounds,
                            iconBounds.width() * 0.6f, iconStyle);

    drawText(canvas, tmpl.name,
             SkRect::MakeXYWH(iconBounds.right() + 16,
                              tmpl.bounds.centerY() - 12, 200, 24),
             templateFont_, textPaint_, false);
  }
}

void ZenithHubComponent::drawNewProjectButton(SkCanvas *canvas) {
  if (newProjectButtonBounds_.isEmpty()) {
    return;
  }

  bool isSelected = (this->selectedSection_ == SelectionSection::New);
  bool active = isNewProjectHovered_ || isSelected;

  GlassmorphicPanel::Options opts;
  opts.style = active ? GlassmorphicPanel::Style::Elevated
                      : GlassmorphicPanel::Style::Subtle;
  opts.accentColor = active ? withAlpha(colors::BLUE, opacity::GLOW_MEDIUM) : 0x00000000;
  opts.cornerRadius = 12.0f;
  opts.glowIntensity = active ? 0.05f : 0.0f;
  opts.drawTopHighlight = false;

  GlassmorphicPanel::drawWithOptions(canvas, newProjectButtonBounds_, opts);

  float iconSize = 20.0f;
  SkRect iconBounds = SkRect::MakeXYWH(
      newProjectButtonBounds_.fLeft + 20,
      newProjectButtonBounds_.centerY() - iconSize * 0.5f, iconSize, iconSize);

  icons::IconStyle iconStyle;
  iconStyle.color = colors::TEXT_PRIMARY;
  iconStyle.strokeWidth = icons::STROKE_BOLD;
  icons::drawIconCentered(canvas, icons::Plus(), iconBounds, iconSize,
                          iconStyle);

  SkRect textBounds = newProjectButtonBounds_;
  textBounds.fLeft += 45;
  drawText(canvas, "New Project", textBounds, buttonFont_, textPaint_, true);
}

void ZenithHubComponent::drawProfileIcon(SkCanvas *canvas) {
  if (profileIconBounds_.isEmpty()) {
    return;
  }

  bool active = isProfileIconHovered_;
  float centerX = profileIconBounds_.centerX();
  float centerY = profileIconBounds_.centerY();
  float radius = profileIconBounds_.width() * 0.5f;

   if (active) {
       SkPaint glowPaint;
       glowPaint.setAntiAlias(true);
       glowPaint.setColor(withAlpha(colors::CYAN, opacity::FOCUS));
       glowPaint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 8.0f));
       canvas->drawCircle(centerX, centerY, radius, glowPaint);
   }

  icons::IconStyle iconStyle;
  iconStyle.color = active ? colors::CYAN : colors::TEXT_SECONDARY;
  iconStyle.strokeWidth = icons::STROKE_REGULAR;
  icons::drawIconCentered(canvas, icons::Users(), profileIconBounds_,
                          radius * 1.1f, iconStyle);
}

void ZenithHubComponent::drawText(SkCanvas *canvas, const juce::String &text,
                                  const SkRect &bounds, const SkFont &font,
                                  const SkPaint &paint, bool centerVertical) {
  if (text.isEmpty() || bounds.width() <= 0) return;

  SkString skText(text.toRawUTF8());
  SkScalar width = font.measureText(skText.c_str(), skText.size(), SkTextEncoding::kUTF8);
  
  if (width > bounds.width()) {
    SkString ellipsis("...");
    SkScalar ellipsisWidth = font.measureText(ellipsis.c_str(), ellipsis.size(), SkTextEncoding::kUTF8);
    
    if (ellipsisWidth > bounds.width()) {
      skText = ellipsis; 
    } else {
      juce::String jStr = text;
      float avgCharWidth = width / (float)jStr.length();
      int estimatedChars = static_cast<int>((bounds.width() - ellipsisWidth) / avgCharWidth);
      
      jStr = jStr.substring(0, std::clamp(estimatedChars, 0, jStr.length()));
      skText = SkString(jStr.toRawUTF8());
      width = font.measureText(skText.c_str(), skText.size(), SkTextEncoding::kUTF8);

      while (jStr.length() > 0 && width + ellipsisWidth > bounds.width()) {
        jStr = jStr.substring(0, jStr.length() - 1);
        skText = SkString(jStr.toRawUTF8());
        width = font.measureText(skText.c_str(), skText.size(), SkTextEncoding::kUTF8);
      }
      skText.append("...");
    }
  }

  SkRect textBounds;
  font.measureText(skText.c_str(), skText.size(), SkTextEncoding::kUTF8,
                   &textBounds);

  float x = bounds.left();
  float y = bounds.fTop - textBounds.fTop;

  if (centerVertical) {
    y = bounds.centerY() + (textBounds.height() * 0.5f) - textBounds.fBottom;
  }
  canvas->drawString(skText, x, y, font, paint);
}

void ZenithHubComponent::drawProfileMenu(SkCanvas* canvas) {
  float rawProgress = 0.0f;
  {
      std::lock_guard<std::mutex> lock(projectsMutex_);
      rawProgress = menuSpring_.getCurrent();
  }
  
  // Skip drawing entirely if almost closed
  if (rawProgress <= 0.01f) return;

  float menuWidth = 220.0f;
  bool isLoggedIn = false;
  AuthUser currentUser;
  if (auto* auth = AuthenticationService::getInstance()) {
      isLoggedIn = auth->isLoggedIn();
      currentUser = auth->getCurrentUser();
  }
  float menuHeight = isLoggedIn ? 240.0f : 120.0f; 
  
  float anchorX = profileIconBounds_.right();
  float anchorY = profileIconBounds_.bottom() + 12.0f;

  SkRect menuBounds = SkRect::MakeXYWH(
      anchorX - menuWidth,
      anchorY,
      menuWidth,
      menuHeight
  );

  // Smoother animation: ease-out curve
  float progress = 1.0f - (1.0f - rawProgress) * (1.0f - rawProgress);
  float alpha = juce::jlimit(0.0f, 1.0f, progress);
  
  // Simple transform instead of expensive saveLayer
  canvas->save();
  
  // Scale from 0.95 to 1.0 for subtle pop-in effect
  float scale = 0.95f + 0.05f * progress;
  float centerX = menuBounds.centerX();
  float centerY = menuBounds.top(); // Scale from top
  canvas->translate(centerX, centerY);
  canvas->scale(scale, scale);
  canvas->translate(-centerX, -centerY);

  GlassmorphicPanel::Options opts;
  opts.style = GlassmorphicPanel::Style::Floating;
  opts.cornerRadius = 16.0f;
  opts.drawShadow = true; 
  opts.glowIntensity = alpha;
  opts.useBackdropBlur = false;
  
  GlassmorphicPanel::drawWithOptions(canvas, menuBounds, opts);

  // Apply alpha to text drawing
  textPaint_.setColor(SkColorSetA(colors::TEXT_PRIMARY, static_cast<U8CPU>(alpha * 255)));


  float itemHeight = 40.0f;
  float currentY = menuBounds.fTop + 16.0f;
  float contentLeft = menuBounds.fLeft + 16.0f;

  auto drawMenuItem = [&](const char* text, bool isDestructive = false) {
      SkRect itemBounds = SkRect::MakeXYWH(contentLeft, currentY, menuWidth - 32, itemHeight);
      
      SkPaint itemTextPaint = textPaint_;
      if (isDestructive) {
        itemTextPaint.setColor(colors::RED);
      }
      
      drawText(canvas, text, itemBounds, buttonFont_, itemTextPaint, false);
      currentY += itemHeight;
  };

  if (isLoggedIn) {
      SkPaint subLabelPaint = textPaint_;
      subLabelPaint.setColor(withAlpha(colors::TEXT_SECONDARY, opacity::SECONDARY));
      
      juce::String userName = currentUser.displayName.isNotEmpty() ? currentUser.displayName : currentUser.email;
      if (userName.isEmpty()) userName = "User";
      
      drawText(canvas, "Signed in as " + userName, SkRect::MakeXYWH(contentLeft, currentY, menuWidth, 20), subFont_, subLabelPaint, false);
      currentY += 24.0f;

      SkPaint divPaint;
      divPaint.setColor(withAlpha(colors::TEXT_SECONDARY, opacity::HOVER));
      canvas->drawLine(menuBounds.fLeft, currentY, menuBounds.fRight, currentY, divPaint);
      currentY += 12.0f;

      drawMenuItem("My Account");
      drawMenuItem("Friends");
      drawMenuItem("Zenith Settings");
      
      currentY += 8.0f;
      drawMenuItem("Sign Out", true);
  } else {
      drawMenuItem("Sign In");
      drawMenuItem("Create Account");
  }

  canvas->restore();
}

} // namespace zenith
