#include "IndustrialUI.h"
#include <algorithm>
#include <map>
#include <juce_audio_processors/juce_audio_processors.h>

namespace zenith::industrial {

namespace {
constexpr int kUiWidth = 1360;
constexpr int kHeaderHeight = 46;
constexpr int kBasePadding = 12;
constexpr int kModeAnimationMs = 100;
constexpr float kComponentGap = 8.0f;
constexpr float kSectionGap = 16.0f;
constexpr float kPanelPadding = 12.0f;

float easeOutQuad(float t) {
  const float inv = 1.0f - t;
  return 1.0f - inv * inv;
}

std::string modeButtonLabel(UIMode buttonMode, UIMode currentMode) {
  const bool active = buttonMode == currentMode;
  switch (buttonMode) {
  case UIMode::Beginner:
    return active ? "[O] BEGINNER" : "[ ] BEGINNER";
  case UIMode::Advanced:
    return active ? "[O] ADVANCED" : "[ ] ADVANCED";
  case UIMode::Expert:
    return active ? "[O] EXPERT" : "[ ] EXPERT";
  }
  return "[ ] UNKNOWN";
}
} // namespace

IndustrialUI::IndustrialUI(ZenithPolySynthProcessor &processor)
    : juce::AudioProcessorEditor(&processor), processor_(processor),
      renderer_(std::make_unique<zenith::SkiaRenderer>(
          *this, zenith::SkiaRenderer::Backend::Auto)) {
  setOpaque(false);
  setBufferedToImage(true);
  setWantsKeyboardFocus(true);

  theme_.initializeFonts();
  currentMode_ = modeManager_.getMode();

  const int initialHeight = modeManager_.targetHeightForMode(currentMode_);
  setSize(kUiWidth, initialHeight);
  setResizable(true, true);
  setResizeLimits(1080, 500, 1920, 1800);

  beginnerButton_ = std::make_unique<IndustrialButton>(
      "[ ] BEGINNER", theme_, IndustrialButton::Shape::Pill, false);
  advancedButton_ = std::make_unique<IndustrialButton>(
      "[ ] ADVANCED", theme_, IndustrialButton::Shape::Pill, false);
  expertButton_ = std::make_unique<IndustrialButton>(
      "[ ] EXPERT", theme_, IndustrialButton::Shape::Pill, false);

  beginnerButton_->onToggle = [this](bool) { setMode(UIMode::Beginner); };
  advancedButton_->onToggle = [this](bool) { setMode(UIMode::Advanced); };
  expertButton_->onToggle = [this](bool) { setMode(UIMode::Expert); };

  carbonTexture_ = CarbonFiberTexture::generate(
      96, 96, theme_.carbonContrastForMode(currentMode_));

  buildControlsForMode(currentMode_);
  syncModeButtons();

  if (renderer_) {
    renderer_->initialize();
  }

  startTimerHz(60);
}

IndustrialUI::~IndustrialUI() {
  stopTimer();
  if (renderer_) {
    renderer_->shutdown();
  }
}

void IndustrialUI::paint(juce::Graphics &g) {
  juce::ignoreUnused(g);
  if (renderer_) {
    renderer_->render([this](SkCanvas *canvas) { drawSkiaContent(canvas); });
  }
}

void IndustrialUI::resized() { updateLayoutForMode(currentMode_); }

void IndustrialUI::mouseDown(const juce::MouseEvent &e) {
  MouseEvent event{static_cast<float>(e.position.x),
                   static_cast<float>(e.position.y), e.mods.isShiftDown()};

  if (beginnerButton_ && beginnerButton_->hitTest(event.x, event.y)) {
    beginnerButton_->handleMouseDown(event);
    activeComponent_ = beginnerButton_.get();
    return;
  }
  if (advancedButton_ && advancedButton_->hitTest(event.x, event.y)) {
    advancedButton_->handleMouseDown(event);
    activeComponent_ = advancedButton_.get();
    return;
  }
  if (expertButton_ && expertButton_->hitTest(event.x, event.y)) {
    expertButton_->handleMouseDown(event);
    activeComponent_ = expertButton_.get();
    return;
  }

  activeComponent_ = nullptr;
  for (auto it = components_.rbegin(); it != components_.rend(); ++it) {
    if (!it->component->isVisible()) {
      continue;
    }
    if (it->component->hitTest(event.x, event.y)) {
      it->component->handleMouseDown(event);
      activeComponent_ = it->component.get();
      break;
    }
  }
}

void IndustrialUI::mouseDrag(const juce::MouseEvent &e) {
  if (!activeComponent_) {
    return;
  }
  MouseEvent event{static_cast<float>(e.position.x),
                   static_cast<float>(e.position.y), e.mods.isShiftDown()};
  activeComponent_->handleMouseDrag(event);
}

void IndustrialUI::mouseUp(const juce::MouseEvent &e) {
  MouseEvent event{static_cast<float>(e.position.x),
                   static_cast<float>(e.position.y), e.mods.isShiftDown()};
  if (activeComponent_) {
    activeComponent_->handleMouseUp(event);
  }

  if (activeComponent_ == beginnerButton_.get() ||
      activeComponent_ == advancedButton_.get() ||
      activeComponent_ == expertButton_.get()) {
    syncModeButtons();
  }

  activeComponent_ = nullptr;
}

void IndustrialUI::mouseMove(const juce::MouseEvent &e) {
  MouseEvent event{static_cast<float>(e.position.x),
                   static_cast<float>(e.position.y), e.mods.isShiftDown()};

  if (beginnerButton_) {
    beginnerButton_->handleMouseMove(event);
  }
  if (advancedButton_) {
    advancedButton_->handleMouseMove(event);
  }
  if (expertButton_) {
    expertButton_->handleMouseMove(event);
  }

  for (auto &entry : components_) {
    entry.component->handleMouseMove(event);
  }
}

bool IndustrialUI::keyPressed(const juce::KeyPress &key) {
  if (key.getTextCharacter() == 'b' || key.getTextCharacter() == 'B') {
    setMode(UIMode::Beginner);
    return true;
  }
  if (key.getTextCharacter() == 'a' || key.getTextCharacter() == 'A') {
    setMode(UIMode::Advanced);
    return true;
  }
  if (key.getTextCharacter() == 'e' || key.getTextCharacter() == 'E') {
    setMode(UIMode::Expert);
    return true;
  }
  return false;
}

void IndustrialUI::timerCallback() {
  if (visualizer_) {
    visualizer_->tick();
  }

  if (animating_) {
    const double elapsed =
        juce::Time::getMillisecondCounterHiRes() - animationStartMs_;
    const float t = juce::jlimit(
        0.0f, 1.0f,
        static_cast<float>(elapsed / static_cast<double>(kModeAnimationMs)));
    const float eased = easeOutQuad(t);
    modeTransitionProgress_ = eased;
    const int h = juce::roundToInt(
        juce::jmap(eased, static_cast<float>(animationFromHeight_),
                   static_cast<float>(animationToHeight_)));
    setSize(getWidth(), h);
    if (t >= 1.0f) {
      animating_ = false;
      modeTransitionProgress_ = 1.0f;
      setSize(getWidth(), animationToHeight_);
    }
  }

  repaint();
}

void IndustrialUI::drawSkiaContent(SkCanvas *canvas) {
  canvas->clear(IndustrialTheme::CARBON_BLACK);

  if (!carbonTexture_.drawsNothing()) {
    SkPaint p;
    p.setAlphaf(0.9f);
    if (auto image = carbonTexture_.asImage()) {
      canvas->drawImageRect(image,
                            SkRect::MakeWH(static_cast<float>(getWidth()),
                                           static_cast<float>(getHeight())),
                            SkSamplingOptions(), &p);
    }
  }

  drawModeLegend(canvas);

  const int modeRank = (currentMode_ == UIMode::Beginner)
                           ? 0
                           : (currentMode_ == UIMode::Advanced ? 1 : 2);
  const int previousRank = (previousMode_ == UIMode::Beginner)
                               ? 0
                               : (previousMode_ == UIMode::Advanced ? 1 : 2);
  for (auto &entry : components_) {
    const int minRank = (entry.minMode == UIMode::Beginner)
                            ? 0
                            : (entry.minMode == UIMode::Advanced ? 1 : 2);
    float alpha = (modeRank >= minRank) ? 1.0f : 0.0f;
    if (animating_) {
      if (modeRank > previousRank && minRank > previousRank &&
          minRank <= modeRank) {
        const float yRatio =
            juce::jlimit(0.0f, 1.0f,
                         entry.component->getBounds().y() /
                             juce::jmax(1.0f, static_cast<float>(getHeight())));
        const float delay = yRatio * 0.2f;
        alpha = juce::jlimit(0.0f, 1.0f,
                             (modeTransitionProgress_ - delay) /
                                 juce::jmax(0.05f, 1.0f - delay));
      } else if (modeRank < previousRank && minRank > modeRank &&
                 minRank <= previousRank) {
        alpha = 1.0f - modeTransitionProgress_;
      }
    }
    entry.component->setVisible(alpha > 0.0f);
    entry.component->setAlpha(alpha);
    if (alpha >= 1.0f) {
      entry.component->render(canvas);
    } else if (alpha > 0.0f) {
      SkPaint layerPaint;
      layerPaint.setAlphaf(alpha);
      const SkRect layerRect =
          entry.component->getBounds().makeOutset(8.0f, 8.0f);
      canvas->saveLayer(&layerRect, &layerPaint);
      entry.component->render(canvas);
      canvas->restore();
    }
  }

  beginnerButton_->render(canvas);
  advancedButton_->render(canvas);
  expertButton_->render(canvas);
}

void IndustrialUI::drawModeLegend(SkCanvas *canvas) {
  SkPaint paint;
  const SkRect headerRect =
      SkRect::MakeXYWH(0.0f, 0.0f, static_cast<float>(getWidth()),
                       static_cast<float>(kHeaderHeight));
  paint.setColor(IndustrialTheme::CARBON_BLACK);
  canvas->drawRect(headerRect, paint);

  paint.setColor(0x33000000);
  canvas->drawRect(SkRect::MakeXYWH(0.0f, static_cast<float>(kHeaderHeight - 2),
                                    static_cast<float>(getWidth()), 2.0f),
                   paint);

  SkFont titleFont(theme_.getTypeface(IndustrialTheme::FontWeight::SemiBold),
                   20.0f);
  SkFont metaFont(theme_.getTypeface(IndustrialTheme::FontWeight::Medium),
                  11.0f);
  paint.setColor(IndustrialTheme::WHITE);
  canvas->drawString("ZENITH POLY SYNTH", 12.0f, 29.0f, titleFont, paint);

  const auto modeText =
      "MODE: " + modeManager_.modeLabel(currentMode_).toStdString();
  paint.setColor(IndustrialTheme::LIGHT_GRAY);
  canvas->drawString(modeText.c_str(), static_cast<float>(getWidth() - 260),
                     17.0f, metaFont, paint);
  canvas->drawString("SHORTCUTS: B / A / E",
                     static_cast<float>(getWidth() - 260), 33.0f, metaFont,
                     paint);
}

void IndustrialUI::addComponent(std::unique_ptr<IndustrialComponent> component,
                                UIMode minMode) {
  components_.push_back({std::move(component), minMode});
}

std::vector<ControlSpec> IndustrialUI::controlsForMode(UIMode mode) const {
  switch (mode) {
  case UIMode::Beginner:
    return BeginnerLayout::build();
  case UIMode::Advanced:
    return AdvancedLayout::build();
  case UIMode::Expert:
    return ExpertLayout::build();
  }
  return BeginnerLayout::build();
}

void IndustrialUI::buildControlsForMode(UIMode mode) {
  components_.clear();
  visualizer_ = nullptr;

  const auto specs = controlsForMode(mode);
  std::vector<std::string> sectionOrder;
  std::unordered_map<std::string, std::vector<ControlSpec>> sectionControls;

  for (const auto &spec : specs) {
    if (spec.kind == ControlSpec::Kind::Panel) {
      sectionOrder.push_back(spec.section);
      sectionControls.try_emplace(spec.section, std::vector<ControlSpec>{});
      continue;
    }

    auto it = sectionControls.find(spec.section);
    if (it == sectionControls.end()) {
      sectionOrder.push_back(spec.section);
      sectionControls[spec.section] = {spec};
    } else {
      it->second.push_back(spec);
    }
  }

  const int width = getWidth();
  float y = static_cast<float>(kHeaderHeight + kBasePadding);

  for (const auto &sectionName : sectionOrder) {
    const auto &controls = sectionControls[sectionName];
    if (controls.empty()) {
      continue;
    }

    const bool allScrews = std::all_of(
        controls.begin(), controls.end(), [](const ControlSpec &spec) {
          return spec.kind == ControlSpec::Kind::Screw;
        });
    if (allScrews) {
      continue;
    }

    const int cols =
        mode == UIMode::Beginner ? 4 : (mode == UIMode::Advanced ? 6 : 8);
    int placeableCount = 0;
    bool hasMatrix = false;
    bool hasVisualizer = false;
    for (const auto &c : controls) {
      hasMatrix |= c.kind == ControlSpec::Kind::ModMatrix;
      hasVisualizer |= c.kind == ControlSpec::Kind::Visualizer;
      if (c.kind != ControlSpec::Kind::Panel &&
          c.kind != ControlSpec::Kind::ModMatrix &&
          c.kind != ControlSpec::Kind::Visualizer &&
          c.kind != ControlSpec::Kind::Screw) {
        ++placeableCount;
      }
    }

    const float defaultKnobSize =
        mode == UIMode::Beginner ? 80.0f
                                 : (mode == UIMode::Advanced ? 64.0f : 48.0f);
    const float controlCellH = defaultKnobSize + 36.0f;
    const int rows = juce::jmax(1, (placeableCount + cols - 1) / cols);
    float panelHeight = 32.0f + static_cast<float>(rows) * controlCellH + 14.0f;
    if (hasMatrix) {
      panelHeight = 188.0f;
    }
    if (hasVisualizer) {
      panelHeight = 176.0f;
    }

    const float panelX = static_cast<float>(kBasePadding);
    const float panelW = static_cast<float>(width - 2 * kBasePadding);
    auto panel = std::make_unique<CarbonPanel>(
        sectionName, theme_, &carbonTexture_, mode == UIMode::Expert);
    panel->setBounds(SkRect::MakeXYWH(panelX, y, panelW, panelHeight));
    addComponent(std::move(panel),
                 (sectionName.find("ADV") != std::string::npos ||
                  sectionName == "MODULATION MATRIX" ||
                  sectionName == "VISUALIZER")
                     ? UIMode::Advanced
                     : ((sectionName == "UNISON" || sectionName == "OSC DEEP" ||
                         sectionName == "LFO ADV" || sectionName == "CURVES")
                            ? UIMode::Expert
                            : UIMode::Beginner));

    float cx = panelX + kPanelPadding;
    float cy = y + 32.0f;
    const float usableW = panelW - 2.0f * kPanelPadding;
    const float cellW = usableW / static_cast<float>(cols);
    int col = 0;

    for (const auto &spec : controls) {
      if (spec.kind == ControlSpec::Kind::ModMatrix) {
        auto table = std::make_unique<ModMatrixTable>(theme_);
        table->setBounds(SkRect::MakeXYWH(panelX + 16.0f, y + 34.0f,
                                          panelW - 32.0f, panelHeight - 46.0f));
        addComponent(std::move(table), UIMode::Advanced);
        continue;
      }

      if (spec.kind == ControlSpec::Kind::Visualizer) {
        auto viz = std::make_unique<VisualizerDisplay>(theme_);
        viz->setBounds(SkRect::MakeXYWH(panelX + 16.0f, y + 34.0f,
                                        panelW - 32.0f, panelHeight - 46.0f));
        visualizer_ = viz.get();
        addComponent(std::move(viz), UIMode::Advanced);
        continue;
      }

      if (spec.kind == ControlSpec::Kind::Screw) {
        continue;
      }

      const float x = cx + static_cast<float>(col) * cellW;

      switch (spec.kind) {
      case ControlSpec::Kind::Knob: {
        const juce::String label = juce::String(spec.label).toUpperCase();
        const bool isPrimary = mode == UIMode::Beginner || label == "VOLUME" ||
                               label == "CUTOFF" || label == "RESONANCE" ||
                               label == "A" || label == "D" || label == "S" ||
                               label == "R" || label.startsWith("MACRO");
        const float knobSize =
            isPrimary ? 80.0f : (mode == UIMode::Advanced ? 64.0f : 48.0f);
        auto knob = std::make_unique<IndustrialKnob>(
            spec.label, spec.unit, spec.value, theme_,
            isPrimary
                ? IndustrialKnob::Size::Primary
                : (mode == UIMode::Advanced ? IndustrialKnob::Size::Standard
                                            : IndustrialKnob::Size::Small),
            mode == UIMode::Expert);

        // Bind to parameter if paramID is specified
        if (!spec.paramID.empty()) {
          auto& params = processor_.getParameters();
          if (auto* param = params.getParameter(spec.paramID)) {
            knob->setValueCallback([param](float newValue) {
              param->setValueNotifyingHost(newValue);
            });
            // Set initial value from parameter
            knob->setValue(param->getValue());
          }
        }

        knob->setBounds(SkRect::MakeXYWH(x + (cellW - knobSize) * 0.5f, cy,
                                         knobSize, knobSize + 30.0f));
        addComponent(std::move(knob),
                     (sectionName == "UNISON" || sectionName == "OSC DEEP" ||
                      sectionName == "LFO ADV")
                         ? UIMode::Expert
                         : (sectionName.find("ADV") != std::string::npos
                                ? UIMode::Advanced
                                : UIMode::Beginner));
        break;
      }
      case ControlSpec::Kind::Slider: {
        auto slider =
            std::make_unique<IndustrialSlider>(spec.label, spec.value, theme_);
        slider->setBounds(
            SkRect::MakeXYWH(x + (cellW - 40.0f) * 0.5f, cy, 40.0f, 106.0f));
        addComponent(std::move(slider), UIMode::Advanced);
        break;
      }
      case ControlSpec::Kind::Button: {
        auto button = std::make_unique<IndustrialButton>(
            spec.label, theme_, IndustrialButton::Shape::Square, true);
        button->setBounds(SkRect::MakeXYWH(x, cy + 8.0f, 64.0f, 32.0f));
        addComponent(std::move(button));
        break;
      }
      case ControlSpec::Kind::Toggle: {
        auto toggle = std::make_unique<IndustrialToggle>(theme_);
        toggle->setBounds(SkRect::MakeXYWH(x + (cellW - 32.0f) * 0.5f,
                                           cy + 18.0f, 32.0f, 16.0f));
        addComponent(std::move(toggle), UIMode::Advanced);
        break;
      }
      case ControlSpec::Kind::LED: {
        auto led = std::make_unique<IndustrialLED>(theme_);
        led->setActive(spec.value > 0.5f);
        led->setBounds(SkRect::MakeXYWH(x + (cellW - 3.0f) * 0.5f, cy + 25.0f,
                                        3.0f, 3.0f));
        addComponent(std::move(led), UIMode::Advanced);
        break;
      }
      case ControlSpec::Kind::Dropdown: {
        auto dropdown = std::make_unique<IndustrialDropdown>(
            spec.label,
            std::vector<std::string>{"SAW", "SQUARE", "TRI", "NOISE"}, theme_);
        dropdown->setBounds(SkRect::MakeXYWH(
            x + 2.0f, cy + 34.0f, juce::jmin(150.0f, cellW - 10.0f), 22.0f));
        addComponent(std::move(dropdown),
                     sectionName == "CURVES"
                         ? UIMode::Expert
                         : ((sectionName.find("ADV") != std::string::npos ||
                             sectionName == "LFOS")
                                ? UIMode::Advanced
                                : UIMode::Beginner));
        break;
      }
      case ControlSpec::Kind::Screw: {
        break;
      }
      case ControlSpec::Kind::Panel:
      case ControlSpec::Kind::ModMatrix:
      case ControlSpec::Kind::Visualizer:
        break;
      }

      ++col;
      if (col >= cols) {
        col = 0;
        cy += controlCellH + kComponentGap;
      }
    }

    y += panelHeight + kSectionGap;
  }
}

void IndustrialUI::updateLayoutForMode(UIMode mode) {
  juce::ignoreUnused(mode);
  constexpr float buttonY = 10.0f;
  constexpr float buttonH = 22.0f;
  constexpr float gap = 10.0f;
  constexpr float wBeginner = 130.0f;
  constexpr float wAdvanced = 136.0f;
  constexpr float wExpert = 118.0f;

  const float totalW = wBeginner + wAdvanced + wExpert + gap * 2.0f;
  const float startX = (static_cast<float>(getWidth()) - totalW) * 0.5f;
  beginnerButton_->setBounds(
      SkRect::MakeXYWH(startX, buttonY, wBeginner, buttonH));
  advancedButton_->setBounds(
      SkRect::MakeXYWH(startX + wBeginner + gap, buttonY, wAdvanced, buttonH));
  expertButton_->setBounds(SkRect::MakeXYWH(
      startX + wBeginner + gap + wAdvanced + gap, buttonY, wExpert, buttonH));
}

void IndustrialUI::syncModeButtons() {
  beginnerButton_->setOn(currentMode_ == UIMode::Beginner);
  advancedButton_->setOn(currentMode_ == UIMode::Advanced);
  expertButton_->setOn(currentMode_ == UIMode::Expert);
  beginnerButton_->setLabel(modeButtonLabel(UIMode::Beginner, currentMode_));
  advancedButton_->setLabel(modeButtonLabel(UIMode::Advanced, currentMode_));
  expertButton_->setLabel(modeButtonLabel(UIMode::Expert, currentMode_));
}

void IndustrialUI::setMode(UIMode mode) {
  if (mode == currentMode_) {
    syncModeButtons();
    return;
  }

  const auto previous = currentMode_;
  previousMode_ = previous;
  currentMode_ = mode;
  modeManager_.setMode(mode);
  carbonTexture_ =
      CarbonFiberTexture::generate(96, 96, theme_.carbonContrastForMode(mode));
  buildControlsForMode(mode);
  syncModeButtons();

  startModeAnimation(previous, mode);
}

void IndustrialUI::startModeAnimation(UIMode from, UIMode to) {
  animating_ = true;
  animationStartMs_ = juce::Time::getMillisecondCounterHiRes();
  animationFromHeight_ = modeManager_.targetHeightForMode(from);
  animationToHeight_ = modeManager_.targetHeightForMode(to);
  modeTransitionProgress_ = 0.0f;
}

} // namespace zenith::industrial
