#include "ModernSettingsPanel.h"

#include "../design-system/ZenithDesignSystem.h"
#include <algorithm>
#include <effects/SkGradientShader.h>

namespace zenith {

namespace {
constexpr float kAnimStep = 0.08f;
constexpr float kRowGap = 12.0f;
constexpr float kRowHeightWide = 88.0f;
constexpr float kRowHeightNarrow = 112.0f;
constexpr float kDropdownHeight = 34.0f;
constexpr float kDropdownOptionHeight = 30.0f;
} // namespace

ModernSettingsPanel::ModernSettingsPanel() {
  customPersonaEditor_ = std::make_unique<SkiaTextEditor>("custom_persona_editor");
  customPersonaEditor_->setMultiLine(true);
  customPersonaEditor_->setScrollbarsShown(true);
  customPersonaEditor_->setTextToShowWhenEmpty("Describe your custom Wingman persona prompt...",
                                               design::colors::TEXT_TERTIARY);
  customPersonaEditor_->setBackgroundColour(SkColorSetRGB(20, 30, 48));
  customPersonaEditor_->setTextColour(design::colors::TEXT_PRIMARY);
  customPersonaEditor_->onTextChange = [this]() {
    if (customPersonaEditor_) {
      Settings::getInstance().setCustomPersonaPrompt(customPersonaEditor_->getText());
      resized();
      repaint();
    }
  };
  addChildComponent(customPersonaEditor_.get());

  setVisible(false);
  initializeSettings();
  refreshFromSettings();
}

ModernSettingsPanel::~ModernSettingsPanel() = default;

void ModernSettingsPanel::drawSkia(SkCanvas* canvas) {
  if (canvas == nullptr) return;

  const auto bounds = getLocalBounds().toFloat();
  const float alpha = juce::jlimit(0.0f, 1.0f, animationProgress_);
  const bool narrow = bounds.getWidth() < 640.0f;

  SkColor bgStops[] = {design::withAlpha(SkColorSetRGB(9, 12, 20), alpha),
                       design::withAlpha(SkColorSetRGB(6, 8, 14), alpha)};
  SkPoint bgPts[] = {{0.0f, 0.0f}, {0.0f, bounds.getHeight()}};
  SkPaint bg;
  bg.setAntiAlias(true);
  bg.setShader(SkGradientShader::MakeLinear(bgPts, bgStops, nullptr, 2, SkTileMode::kClamp));
  canvas->drawRoundRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), CORNER_RADIUS, CORNER_RADIUS, bg);

  SkPaint border;
  border.setAntiAlias(true);
  border.setStyle(SkPaint::kStroke_Style);
  border.setStrokeWidth(1.2f);
  border.setColor(SkColorSetARGB(120, 78, 98, 146));
  canvas->drawRoundRect(SkRect::MakeWH(bounds.getWidth(), bounds.getHeight()), CORNER_RADIUS, CORNER_RADIUS, border);

  SkFont titleFont = design::getSkFont(narrow ? 19.0f : 22.0f, design::FontWeight::SemiBold);
  SkFont sectionFont = design::getSkFont(13.0f, design::FontWeight::Medium);
  SkFont rowTitleFont = design::getSkFont(14.0f, design::FontWeight::Medium);
  SkFont rowDescFont = design::getSkFont(12.0f, design::FontWeight::Regular);
  SkFont valueFont = design::getSkFont(13.0f, design::FontWeight::Medium);
  SkPaint textPrimary;
  textPrimary.setAntiAlias(true);
  textPrimary.setColor(design::colors::TEXT_PRIMARY);
  SkPaint textSecondary;
  textSecondary.setAntiAlias(true);
  textSecondary.setColor(design::colors::TEXT_SECONDARY);
  SkPaint valueText;
  valueText.setAntiAlias(true);
  valueText.setColor(SkColorSetRGB(188, 214, 255));

  canvas->drawSimpleText("Wingman Chat Settings", 21, SkTextEncoding::kUTF8, headerRect_.getX(),
                         headerRect_.getY() + 20.0f, titleFont, textPrimary);
  canvas->drawSimpleText("Explicit dropdown controls", 26, SkTextEncoding::kUTF8, headerRect_.getX(),
                         headerRect_.getY() + 42.0f, sectionFont, textSecondary);

  SkPaint viewportFill;
  viewportFill.setAntiAlias(true);
  viewportFill.setColor(SkColorSetARGB(150, 14, 19, 32));
  canvas->drawRoundRect(
      SkRect::MakeXYWH(listViewportRect_.getX(), listViewportRect_.getY(), listViewportRect_.getWidth(),
                       listViewportRect_.getHeight()),
      14.0f, 14.0f, viewportFill);

  auto drawDropdownButton = [&](const DropdownState& dd, const juce::String& value) {
    SkPaint fill;
    fill.setAntiAlias(true);
    fill.setColor(SkColorSetRGB(32, 42, 66));
    canvas->drawRoundRect(SkRect::MakeXYWH(dd.buttonRect.getX(), dd.buttonRect.getY(), dd.buttonRect.getWidth(),
                                           dd.buttonRect.getHeight()),
                          9.0f, 9.0f, fill);

    SkPaint stroke;
    stroke.setAntiAlias(true);
    stroke.setStyle(SkPaint::kStroke_Style);
    stroke.setStrokeWidth(1.0f);
    stroke.setColor(SkColorSetARGB(150, 96, 126, 170));
    canvas->drawRoundRect(SkRect::MakeXYWH(dd.buttonRect.getX(), dd.buttonRect.getY(), dd.buttonRect.getWidth(),
                                           dd.buttonRect.getHeight()),
                          9.0f, 9.0f, stroke);

    canvas->drawSimpleText(value.toRawUTF8(), (size_t)value.getNumBytesAsUTF8(), SkTextEncoding::kUTF8,
                           dd.buttonRect.getX() + 10.0f, dd.buttonRect.getY() + 22.0f, valueFont, valueText);
    canvas->drawSimpleText("v", 1, SkTextEncoding::kUTF8, dd.buttonRect.getRight() - 14.0f, dd.buttonRect.getY() + 22.0f,
                           valueFont, valueText);
  };

  auto drawRow = [&](const juce::Rectangle<float>& row, int rowId, const juce::String& title, const juce::String& desc,
                     const DropdownState& dd, const juce::String& value) {
    SkPaint rowFill;
    rowFill.setAntiAlias(true);
    rowFill.setColor(hoveredRowId_ == rowId ? SkColorSetRGB(30, 38, 59) : SkColorSetRGB(22, 29, 45));
    canvas->drawRoundRect(SkRect::MakeXYWH(row.getX(), row.getY(), row.getWidth(), row.getHeight()), 12.0f, 12.0f,
                          rowFill);

    SkPaint rowStroke;
    rowStroke.setAntiAlias(true);
    rowStroke.setStyle(SkPaint::kStroke_Style);
    rowStroke.setStrokeWidth(1.0f);
    rowStroke.setColor(SkColorSetARGB(120, 82, 102, 140));
    canvas->drawRoundRect(SkRect::MakeXYWH(row.getX(), row.getY(), row.getWidth(), row.getHeight()), 12.0f, 12.0f,
                          rowStroke);

    canvas->drawSimpleText(title.toRawUTF8(), (size_t)title.getNumBytesAsUTF8(), SkTextEncoding::kUTF8, row.getX() + 14.0f,
                           row.getY() + 24.0f, rowTitleFont, textPrimary);
    canvas->drawSimpleText(desc.toRawUTF8(), (size_t)desc.getNumBytesAsUTF8(), SkTextEncoding::kUTF8, row.getX() + 14.0f,
                           row.getY() + 46.0f, rowDescFont, textSecondary);
    drawDropdownButton(dd, value);
  };

  auto drawDropdownOptions = [&](const DropdownState& dd) {
    if (!dd.open || dd.optionRects.empty()) return;
    SkPaint p;
    p.setAntiAlias(true);
    p.setColor(SkColorSetRGB(18, 25, 42));
    const auto& first = dd.optionRects.front();
    const auto& last = dd.optionRects.back();
    const auto backRect = SkRect::MakeXYWH(first.getX(), first.getY(), first.getWidth(), last.getBottom() - first.getY());
    canvas->drawRoundRect(backRect, 9.0f, 9.0f, p);

    for (size_t i = 0; i < dd.optionRects.size(); ++i) {
      const auto& r = dd.optionRects[i];
      const bool selected = (int)i == dd.selectedIndex;
      SkPaint f;
      f.setAntiAlias(true);
      f.setColor(selected ? SkColorSetRGB(45, 73, 132) : SkColorSetRGB(25, 33, 54));
      canvas->drawRoundRect(SkRect::MakeXYWH(r.getX(), r.getY(), r.getWidth(), r.getHeight()), 7.0f, 7.0f, f);
      const juce::String label = dd.options[(int)i];
      canvas->drawSimpleText(label.toRawUTF8(), (size_t)label.getNumBytesAsUTF8(), SkTextEncoding::kUTF8, r.getX() + 9.0f,
                             r.getY() + 20.0f, valueFont, textPrimary);
    }
  };

  canvas->save();
  canvas->clipRect(SkRect::MakeXYWH(listViewportRect_.getX(), listViewportRect_.getY(), listViewportRect_.getWidth(),
                                    listViewportRect_.getHeight()));
  canvas->translate(0.0f, -scrollOffset_);

  drawRow(personaRowRect_, 1, "Wingman Persona", "Response style profile", personaDropdown_,
          personaDropdown_.options[personaDropdown_.selectedIndex]);

  if (customPersonaVisible_) {
    SkPaint rowFill;
    rowFill.setAntiAlias(true);
    rowFill.setColor(SkColorSetRGB(20, 27, 43));
    canvas->drawRoundRect(SkRect::MakeXYWH(customPersonaRowRect_.getX(), customPersonaRowRect_.getY(),
                                           customPersonaRowRect_.getWidth(), customPersonaRowRect_.getHeight()),
                          12.0f, 12.0f, rowFill);
    canvas->drawSimpleText("Customized Persona Prompt", 25, SkTextEncoding::kUTF8, customPersonaRowRect_.getX() + 12.0f,
                           customPersonaRowRect_.getY() + 22.0f, rowTitleFont, textPrimary);
  }

  drawRow(reasoningRowRect_, 2, "Reasoning Preview", "How much thinking is shown", reasoningDropdown_,
          reasoningDropdown_.options[reasoningDropdown_.selectedIndex]);
  drawRow(permissionsRowRect_, 3, "DAW Changes", "Confirmation behavior before edits", permissionsDropdown_,
          permissionsDropdown_.options[permissionsDropdown_.selectedIndex]);
  drawRow(responseLengthRowRect_, 4, "Response Length", "Default output size", responseLengthDropdown_,
          responseLengthDropdown_.options[responseLengthDropdown_.selectedIndex]);
  drawRow(suggestionRowRect_, 5, "Proactive Suggestions", "Suggestion frequency", suggestionDropdown_,
          suggestionDropdown_.options[suggestionDropdown_.selectedIndex]);

  drawDropdownOptions(personaDropdown_);
  drawDropdownOptions(reasoningDropdown_);
  drawDropdownOptions(permissionsDropdown_);
  drawDropdownOptions(responseLengthDropdown_);
  drawDropdownOptions(suggestionDropdown_);

  canvas->restore();
}

void ModernSettingsPanel::resized() {
  auto b = getLocalBounds().toFloat().reduced(18.0f, 18.0f);
  const bool narrow = b.getWidth() < 640.0f;
  const float rowH = narrow ? kRowHeightNarrow : kRowHeightWide;

  headerRect_ = b.removeFromTop(56.0f);
  listViewportRect_ = b.reduced(0.0f, 8.0f);

  auto layoutRow = [&](juce::Rectangle<float>& row, DropdownState& dd, float y) {
    row = {listViewportRect_.getX() + 10.0f, y, listViewportRect_.getWidth() - 20.0f, rowH};
    const float ddW = narrow ? (row.getWidth() - 24.0f) : std::min(230.0f, row.getWidth() * 0.42f);
    const float ddX = narrow ? (row.getX() + 12.0f) : (row.getRight() - ddW - 12.0f);
    const float ddY = narrow ? (row.getY() + row.getHeight() - kDropdownHeight - 10.0f) : (row.getY() + 26.0f);
    dd.buttonRect = {ddX, ddY, ddW, kDropdownHeight};
    dd.optionRects.clear();
    if (dd.open) {
      for (int i = 0; i < dd.options.size(); ++i) {
        dd.optionRects.push_back({ddX, ddY + kDropdownHeight + 4.0f + i * (kDropdownOptionHeight + 4.0f), ddW, kDropdownOptionHeight});
      }
    }
  };

  float y = listViewportRect_.getY() + 12.0f;
  layoutRow(personaRowRect_, personaDropdown_, y); y += rowH + kRowGap;

  const bool anyDropdownOpen =
      personaDropdown_.open || reasoningDropdown_.open || permissionsDropdown_.open ||
      responseLengthDropdown_.open || suggestionDropdown_.open;
  customPersonaVisible_ = (personaDropdown_.selectedIndex == 3) && !anyDropdownOpen;
  if (customPersonaVisible_) {
    const float editorWidth = listViewportRect_.getWidth() - 44.0f;
    const float wrapFactor = std::max(18.0f, editorWidth / 8.0f);
    const auto text = customPersonaEditor_ ? customPersonaEditor_->getText() : juce::String();
    const int logicalLines = std::max(1, juce::StringArray::fromLines(text).size());
    const int roughWrapLines = std::max(1, (int)std::ceil((double)text.length() / (double)wrapFactor));
    const int lines = std::max(logicalLines, roughWrapLines);
    const float editorHeight = juce::jlimit(84.0f, 240.0f, 24.0f + lines * 20.0f);
    customPersonaRowRect_ = {listViewportRect_.getX() + 10.0f, y, listViewportRect_.getWidth() - 20.0f, editorHeight + 36.0f};
    if (customPersonaEditor_) {
      customPersonaEditor_->setVisible(true);
      customPersonaEditor_->setBounds((int)(customPersonaRowRect_.getX() + 10.0f),
                                      (int)(customPersonaRowRect_.getY() + 28.0f - scrollOffset_),
                                      (int)(customPersonaRowRect_.getWidth() - 20.0f),
                                      (int)editorHeight);
    }
    y += customPersonaRowRect_.getHeight() + kRowGap;
  } else if (customPersonaEditor_) {
    customPersonaEditor_->setVisible(false);
  }

  layoutRow(reasoningRowRect_, reasoningDropdown_, y); y += rowH + kRowGap;
  layoutRow(permissionsRowRect_, permissionsDropdown_, y); y += rowH + kRowGap;
  layoutRow(responseLengthRowRect_, responseLengthDropdown_, y); y += rowH + kRowGap;
  layoutRow(suggestionRowRect_, suggestionDropdown_, y); y += rowH + kRowGap;

  float contentBottom = y;
  auto extendForOpenDropdown = [&](const DropdownState& dd) {
    if (dd.open && !dd.optionRects.empty()) {
      contentBottom = std::max(contentBottom, dd.optionRects.back().getBottom() + 12.0f);
    }
  };
  extendForOpenDropdown(personaDropdown_);
  extendForOpenDropdown(reasoningDropdown_);
  extendForOpenDropdown(permissionsDropdown_);
  extendForOpenDropdown(responseLengthDropdown_);
  extendForOpenDropdown(suggestionDropdown_);

  const float contentH = contentBottom - (listViewportRect_.getY() + 12.0f);
  maxScrollOffset_ = std::max(0.0f, contentH - listViewportRect_.getHeight() + 8.0f);
  scrollOffset_ = juce::jlimit(0.0f, maxScrollOffset_, scrollOffset_);

  if (customPersonaEditor_ && customPersonaVisible_) {
    customPersonaEditor_->setBounds((int)(customPersonaRowRect_.getX() + 10.0f),
                                    (int)(customPersonaRowRect_.getY() + 28.0f - scrollOffset_),
                                    (int)(customPersonaRowRect_.getWidth() - 20.0f),
                                    (int)(customPersonaRowRect_.getHeight() - 38.0f));
  }
}

void ModernSettingsPanel::mouseMove(const juce::MouseEvent& e) { updateHoverState(e.getPosition()); }

void ModernSettingsPanel::mouseExit(const juce::MouseEvent&) {
  hoveredRowId_ = 0;
  repaint();
}

void ModernSettingsPanel::mouseDown(const juce::MouseEvent& e) {
  const auto p = e.getPosition().toFloat();
  auto toContent = [&](juce::Rectangle<float> r) {
    r.setY(r.getY() - scrollOffset_);
    return r;
  };

  auto closeOthers = [&](DropdownState& keep) {
    if (&keep != &personaDropdown_) personaDropdown_.open = false;
    if (&keep != &reasoningDropdown_) reasoningDropdown_.open = false;
    if (&keep != &permissionsDropdown_) permissionsDropdown_.open = false;
    if (&keep != &responseLengthDropdown_) responseLengthDropdown_.open = false;
    if (&keep != &suggestionDropdown_) suggestionDropdown_.open = false;
  };

  auto applyDropDownClick = [&](DropdownState& dd, auto onSelected) -> bool {
    if (toContent(dd.buttonRect).contains(p)) {
      dd.open = !dd.open;
      closeOthers(dd);
      resized();
      if (dd.open && !dd.optionRects.empty()) {
        const float visibleBottom = listViewportRect_.getBottom() + scrollOffset_;
        const float neededBottom = dd.optionRects.back().getBottom() + 8.0f;
        if (neededBottom > visibleBottom) {
          scrollOffset_ = juce::jlimit(0.0f, maxScrollOffset_, scrollOffset_ + (neededBottom - visibleBottom));
        }
      }
      repaint();
      return true;
    }
    if (dd.open) {
      for (size_t i = 0; i < dd.optionRects.size(); ++i) {
        if (toContent(dd.optionRects[i]).contains(p)) {
          dd.selectedIndex = (int)i;
          dd.open = false;
          onSelected(dd.selectedIndex);
          resized();
          repaint();
          return true;
        }
      }
    }
    return false;
  };

  if (applyDropDownClick(personaDropdown_, [&](int idx) {
        const auto style = fromChatStyleSelectorId(idx + 1);
        Settings::getInstance().setChatStyle(style);
        customPersonaVisible_ = (style == Settings::ChatStyle::Customized);
        if (customPersonaVisible_) {
          // Force layout + scroll so the custom prompt editor is immediately visible.
          resized();
          const float visibleTop = listViewportRect_.getY() + scrollOffset_;
          const float visibleBottom = listViewportRect_.getBottom() + scrollOffset_;
          const float editorTop = customPersonaRowRect_.getY();
          const float editorBottom = customPersonaRowRect_.getBottom();
          if (editorBottom > visibleBottom) {
            scrollOffset_ = juce::jlimit(0.0f, maxScrollOffset_, scrollOffset_ + (editorBottom - visibleBottom) + 8.0f);
          } else if (editorTop < visibleTop) {
            scrollOffset_ = juce::jlimit(0.0f, maxScrollOffset_, scrollOffset_ - (visibleTop - editorTop) - 8.0f);
          }
          resized();
          if (customPersonaEditor_) {
            customPersonaEditor_->setVisible(true);
            customPersonaEditor_->toFront(false);
            customPersonaEditor_->grabKeyboardFocus();
          }
        }
      })) return;

  if (applyDropDownClick(reasoningDropdown_, [&](int idx) {
        Settings::getInstance().setReasoningPreview((Settings::ReasoningPreview)idx);
      })) return;

  if (applyDropDownClick(permissionsDropdown_, [&](int idx) {
        // Ask / YOLO labels mapped onto existing approval enum.
        Settings::getInstance().setApprovalMode(idx == 0 ? Settings::WingmanApprovalMode::AskBeforeApplying
                                                          : Settings::WingmanApprovalMode::AutoApproveSafeChanges);
      })) return;

  if (applyDropDownClick(responseLengthDropdown_, [&](int idx) {
        Settings::getInstance().setResponseLength((Settings::WingmanResponseLength)juce::jlimit(0, 2, idx));
      })) return;
  if (applyDropDownClick(suggestionDropdown_, [&](int idx) {
        Settings::getInstance().setSuggestionFrequency((Settings::WingmanSuggestionFrequency)juce::jlimit(0, 2, idx));
      })) return;

  personaDropdown_.open = false;
  reasoningDropdown_.open = false;
  permissionsDropdown_.open = false;
  responseLengthDropdown_.open = false;
  suggestionDropdown_.open = false;
  resized();
  repaint();
}

void ModernSettingsPanel::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) {
  if (maxScrollOffset_ <= 0.0f) return;
  scrollOffset_ -= wheel.deltaY * (wheel.isSmooth ? 520.0f : 240.0f);
  scrollOffset_ = juce::jlimit(0.0f, maxScrollOffset_, scrollOffset_);
  repaint();
}

void ModernSettingsPanel::refreshFromSettings() {
  const auto& settings = Settings::getInstance();
  personaDropdown_.options = {"Obedient", "Teacher", "Creative", "Customized"};
  reasoningDropdown_.options = {"Hidden", "Compact", "Detailed"};
  permissionsDropdown_.options = {"Ask", "YOLO"};
  responseLengthDropdown_.options = {"Short", "Balanced", "Long"};
  suggestionDropdown_.options = {"Low", "Balanced", "High"};

  personaDropdown_.selectedIndex = juce::jlimit(0, 3, toChatStyleSelectorId(settings.getChatStyle()) - 1);
  reasoningDropdown_.selectedIndex = juce::jlimit(0, 2, (int)settings.getReasoningPreview());
  permissionsDropdown_.selectedIndex =
      settings.getApprovalMode() == Settings::WingmanApprovalMode::AskBeforeApplying ? 0 : 1;
  responseLengthDropdown_.selectedIndex = juce::jlimit(0, 2, (int)settings.getResponseLength());
  suggestionDropdown_.selectedIndex = juce::jlimit(0, 2, (int)settings.getSuggestionFrequency());

  if (responseLengthDropdown_.selectedIndex < 0 || responseLengthDropdown_.selectedIndex > 2) {
    responseLengthDropdown_.selectedIndex = 1;
  }
  if (suggestionDropdown_.selectedIndex < 0 || suggestionDropdown_.selectedIndex > 2) {
    suggestionDropdown_.selectedIndex = 1;
  }
  if (customPersonaEditor_ && customPersonaEditor_->getText() != settings.getCustomPersonaPrompt()) {
    customPersonaEditor_->setText(settings.getCustomPersonaPrompt());
  }

  resized();
  repaint();
}

void ModernSettingsPanel::applySettings() {}

void ModernSettingsPanel::startShowAnimation() {
  isAnimatingIn_ = true;
  animationProgress_ = juce::jlimit(0.0f, 1.0f, animationProgress_ + kAnimStep);
  refreshFromSettings();
  setVisible(true);
  repaint();
}

void ModernSettingsPanel::startHideAnimation() {
  isAnimatingIn_ = false;
  animationProgress_ = juce::jlimit(0.0f, 1.0f, animationProgress_ - kAnimStep);
  if (animationProgress_ <= 0.0f) setVisible(false);
  repaint();
}

void ModernSettingsPanel::initializeSettings() {}
void ModernSettingsPanel::setupAudioSettings() {}
void ModernSettingsPanel::setupInterfaceSettings() {}
void ModernSettingsPanel::setupPerformanceSettings() {}
void ModernSettingsPanel::setupAdvancedSettings() {}
void ModernSettingsPanel::drawBackground(SkCanvas*) {}
void ModernSettingsPanel::drawSidebar(SkCanvas*) {}
void ModernSettingsPanel::drawMainContent(SkCanvas*) {}
void ModernSettingsPanel::drawSettingCard(SkCanvas*, const SettingItem&, float) {}
void ModernSettingsPanel::drawCategoryButton(SkCanvas*, Category, const juce::String&, const juce::String&, float, bool) {}

juce::Rectangle<float> ModernSettingsPanel::getMainContentBounds() const {
  auto b = getLocalBounds().toFloat();
  return b.withTrimmedLeft(static_cast<float>(SIDEBAR_WIDTH));
}

juce::Rectangle<float> ModernSettingsPanel::getSidebarBounds() const {
  auto b = getLocalBounds().toFloat();
  return b.withWidth(static_cast<float>(SIDEBAR_WIDTH));
}

std::vector<ModernSettingsPanel::SettingItem*> ModernSettingsPanel::getSettingsForCategory(Category) { return {}; }
void ModernSettingsPanel::handleCategoryClick(Category) {}
void ModernSettingsPanel::handleSettingClick(int) {}

void ModernSettingsPanel::updateHoverState(const juce::Point<int>& mousePos) {
  const auto p = mousePos.toFloat();
  auto toContent = [&](juce::Rectangle<float> r) {
    r.setY(r.getY() - scrollOffset_);
    return r;
  };
  hoveredRowId_ = 0;
  if (toContent(personaRowRect_).contains(p)) hoveredRowId_ = 1;
  else if (toContent(reasoningRowRect_).contains(p)) hoveredRowId_ = 2;
  else if (toContent(permissionsRowRect_).contains(p)) hoveredRowId_ = 3;
  else if (toContent(responseLengthRowRect_).contains(p)) hoveredRowId_ = 4;
  else if (toContent(suggestionRowRect_).contains(p)) hoveredRowId_ = 5;
  repaint();
}

void ModernSettingsPanel::updateChatStyleHelpText(Settings::ChatStyle) {}

Settings::ChatStyle ModernSettingsPanel::fromChatStyleSelectorId(int selectorId) {
  switch (selectorId) {
    case 2: return Settings::ChatStyle::Teacher;
    case 3: return Settings::ChatStyle::Creative;
    case 4: return Settings::ChatStyle::Customized;
    case 1:
    default: return Settings::ChatStyle::Obedient;
  }
}

int ModernSettingsPanel::toChatStyleSelectorId(Settings::ChatStyle style) {
  switch (style) {
    case Settings::ChatStyle::Teacher: return 2;
    case Settings::ChatStyle::Creative: return 3;
    case Settings::ChatStyle::Customized: return 4;
    case Settings::ChatStyle::Obedient:
    default: return 1;
  }
}

juce::String ModernSettingsPanel::toChatStyleLabel(Settings::ChatStyle style) {
  switch (style) {
    case Settings::ChatStyle::Teacher: return "Teacher";
    case Settings::ChatStyle::Creative: return "Creative";
    case Settings::ChatStyle::Customized: return "Customized";
    case Settings::ChatStyle::Obedient:
    default: return "Obedient";
  }
}

juce::String ModernSettingsPanel::toReasoningPreviewLabel(Settings::ReasoningPreview mode) {
  switch (mode) {
    case Settings::ReasoningPreview::Hidden: return "Hidden";
    case Settings::ReasoningPreview::Detailed: return "Detailed";
    case Settings::ReasoningPreview::Compact:
    default: return "Compact";
  }
}

juce::String ModernSettingsPanel::toApprovalModeLabel(Settings::WingmanApprovalMode mode) {
  switch (mode) {
    case Settings::WingmanApprovalMode::AutoApproveSafeChanges: return "YOLO";
    case Settings::WingmanApprovalMode::AskBeforeApplying:
    default: return "Ask";
  }
}

juce::String ModernSettingsPanel::toResponseLengthLabel(int id) {
  switch (id) {
    case 0: return "Short";
    case 2: return "Long";
    case 1:
    default: return "Balanced";
  }
}

juce::String ModernSettingsPanel::toSuggestionLabel(int id) {
  switch (id) {
    case 0: return "Low";
    case 2: return "High";
    case 1:
    default: return "Balanced";
  }
}

} // namespace zenith
