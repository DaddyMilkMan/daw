/**
 * @file MenuBar.cpp
 * @brief Custom Skia-based Menu Bar implementation
 */

#include "MenuBar.h"

namespace zenith {

ZenithMenuBar::ZenithMenuBar() {
  setOpaque(true);

  // Define Menu Items
  items = {{"File", {}, false},
           {"Edit", {}, false},
           {"View", {}, false},
           {"Help", {}, false}};
}

void ZenithMenuBar::paint(juce::Graphics &g) {
  // Delegate to Skia
  SkiaComponent::paint(g);
}

void ZenithMenuBar::drawSkia(SkCanvas *canvas) {
  // Background
  canvas->clear(SkColorSetRGB(30, 30, 35)); // Slightly lighter than main bg

  // Draw Items
  SkPaint textPaint;
  textPaint.setColor(SK_ColorWHITE);
  textPaint.setAntiAlias(true);

  // Use generic font directly if Typeface provider not hooked up yet, or use
  // default
  sk_sp<SkTypeface> typeface =
      SkTypeface::MakeFromName("Roboto", SkFontStyle());
  SkFont font(typeface, 14.0f);

  SkPaint hoverPaint;
  hoverPaint.setColor(SkColorSetARGB(40, 255, 255, 255));

  for (size_t i = 0; i < items.size(); ++i) {
    auto &item = items[i];

    if (i == hoveredItemIndex) {
      canvas->drawRect(SkRect::MakeXYWH(item.bounds.getX(), 0,
                                        item.bounds.getWidth(), getHeight()),
                       hoverPaint);
    }

    // Centered text
    float textWidth = font.measureText(item.name.toUTF8(), item.name.length(),
                                       SkTextEncoding::kUTF8);
    float x = item.bounds.getX() + (item.bounds.getWidth() - textWidth) / 2.0f;
    float y = getHeight() / 2.0f + 5.0f; // Approximate vertical center

    canvas->drawSimpleText(item.name.toUTF8(), item.name.length(),
                           SkTextEncoding::kUTF8, x, y, font, textPaint);
  }

  // Bottom Border
  SkPaint borderPaint;
  borderPaint.setColor(SkColorSetRGB(60, 60, 60));
  borderPaint.setStrokeWidth(1.0f);
  canvas->drawLine(0, getHeight(), getWidth(), getHeight(), borderPaint);
}

void ZenithMenuBar::resized() { updateLayout(); }

void ZenithMenuBar::updateLayout() {
  int x = 10;
  int itemWidth = 60;

  for (auto &item : items) {
    item.bounds = juce::Rectangle<int>(x, 0, itemWidth, getHeight());
    x += itemWidth;
  }
}

void ZenithMenuBar::mouseMove(const juce::MouseEvent &e) {
  int prevHover = hoveredItemIndex;
  hoveredItemIndex = -1;

  for (size_t i = 0; i < items.size(); ++i) {
    if (items[i].bounds.contains(e.getPosition())) {
      hoveredItemIndex = (int)i;
      break;
    }
  }

  if (prevHover != hoveredItemIndex) {
    repaint();
  }
}

void ZenithMenuBar::mouseExit(const juce::MouseEvent &e) {
  if (hoveredItemIndex != -1) {
    hoveredItemIndex = -1;
    repaint();
  }
}

void ZenithMenuBar::mouseDown(const juce::MouseEvent &e) {
  if (hoveredItemIndex == -1)
    return;

  const auto &item = items[hoveredItemIndex];

  if (item.name == "File")
    showFileMenu();
  else if (item.name == "Edit")
    showEditMenu();
  else if (item.name == "View")
    showViewMenu();
  else if (item.name == "Help")
    showHelpMenu();
}

void ZenithMenuBar::showFileMenu() {
  juce::PopupMenu menu;
  menu.addItem(1, "New Project");
  menu.addItem(2, "Open Project...");
  menu.addSeparator();
  menu.addItem(3, "Save Project", true, false); // Enabled by default
  menu.addItem(4, "Save Project As...");
  menu.addSeparator();
  menu.addItem(5, "Import Audio...");
  menu.addSeparator();
  menu.addItem(6, "Quit");

  menu.showMenuAsync(
      juce::PopupMenu::Options().withTargetComponent(this).withTargetScreenArea(
          items[0].bounds.getSmallestIntegerContainer().translated(
              getScreenX(), getScreenY() + getHeight())),
      [this](int result) {
        if (result == 3 && onSaveProject)
          onSaveProject();
        else if (result == 4 && onSaveProjectAs)
          onSaveProjectAs();
        else if (result == 5 && onImportAudio)
          onImportAudio();
        else if (result == 6)
          juce::JUCEApplication::getInstance()->systemRequestedQuit();
      });
}

void ZenithMenuBar::showEditMenu() {
  juce::PopupMenu menu;
  menu.addItem(1, "Undo");
  menu.addItem(2, "Redo");
  menu.addSeparator();
  menu.addItem(3, "Cut");
  menu.addItem(4, "Copy");
  menu.addItem(5, "Paste");

  menu.showMenuAsync(
      juce::PopupMenu::Options().withTargetComponent(this).withTargetScreenArea(
          items[1].bounds.getSmallestIntegerContainer().translated(
              getScreenX(), getScreenY() + getHeight())),
      [this](int result) {
        if (result == 1 && onUndo)
          onUndo();
        else if (result == 2 && onRedo)
          onRedo();
      });
}

void ZenithMenuBar::showViewMenu() {
  juce::PopupMenu menu;
  menu.addItem(1, "Toggle Session/Arranger (Tab)");
  menu.addItem(2, "Toggle Mixer");

  menu.showMenuAsync(
      juce::PopupMenu::Options().withTargetComponent(this).withTargetScreenArea(
          items[2].bounds.getSmallestIntegerContainer().translated(
              getScreenX(), getScreenY() + getHeight())));
}

void ZenithMenuBar::showHelpMenu() {
  juce::PopupMenu menu;
  menu.addItem(1, "About Zenith DAW...");

  menu.showMenuAsync(
      juce::PopupMenu::Options().withTargetComponent(this).withTargetScreenArea(
          items[3].bounds.getSmallestIntegerContainer().translated(
              getScreenX(), getScreenY() + getHeight())));
}

} // namespace zenith
