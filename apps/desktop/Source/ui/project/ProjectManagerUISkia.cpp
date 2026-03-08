#include "ProjectManagerUISkia.h"
#include "../design-system/ZenithDesignSystem.h"

namespace zenith::ui {

ProjectManagerUISkia::ProjectManagerUISkia() : model_(*this) {
  title_ = std::make_unique<SkiaLabel>("pm_title", "Project Manager");
  title_->setFont(18.0f, SkFontStyle::kBold_Weight);
  title_->setTextColour(design::colors::TEXT_PRIMARY());
  addAndMakeVisible(title_.get());

  search_ = std::make_unique<SkiaTextEditor>("pm_search");
  search_->setTextToShowWhenEmpty("Search projects...",
                                  design::withAlpha(design::colors::TEXT_SECONDARY(), 0.7f));
  search_->onTextChange = [this]() {
    searchTerm_ = search_->getText().toLowerCase().trim();
    filter();
  };
  addAndMakeVisible(search_.get());

  list_ = std::make_unique<SkiaListBox>("pm_list");
  list_->setModel(&model_);
  list_->setRowHeight(36);
  addAndMakeVisible(list_.get());

  newButton_ = std::make_unique<ZenithButton>("New");
  newButton_->setStyle(ZenithButton::Style::Primary);
  newButton_->onClick = [this]() {
    if (onNewProject) {
      onNewProject();
    }
  };
  addAndMakeVisible(newButton_.get());

  openButton_ = std::make_unique<ZenithButton>("Open");
  openButton_->setStyle(ZenithButton::Style::Secondary);
  openButton_->onClick = [this]() {
    if (const auto *selected = getSelectedProject()) {
      if (onOpenProject) {
        onOpenProject(juce::File(selected->path));
      }
    } else {
      status_->setText("Select a project to open", juce::dontSendNotification);
      status_->setTextColour(design::colors::WARNING());
    }
  };
  addAndMakeVisible(openButton_.get());

  saveButton_ = std::make_unique<ZenithButton>("Save");
  saveButton_->setStyle(ZenithButton::Style::Secondary);
  saveButton_->onClick = [this]() {
    if (onSaveRequested) {
      onSaveRequested();
    }
    status_->setText("Project list saved", juce::dontSendNotification);
    status_->setTextColour(design::colors::SUCCESS());
  };
  addAndMakeVisible(saveButton_.get());

  deleteButton_ = std::make_unique<ZenithButton>("Delete");
  deleteButton_->setStyle(ZenithButton::Style::Danger);
  deleteButton_->onClick = [this]() {
    if (const auto *selected = getSelectedProject()) {
      if (onDeleteProject) {
        onDeleteProject(juce::File(selected->path));
      }
    } else {
      status_->setText("Select a project to remove", juce::dontSendNotification);
      status_->setTextColour(design::colors::WARNING());
    }
  };
  addAndMakeVisible(deleteButton_.get());

  status_ = std::make_unique<SkiaLabel>("pm_status", "No projects loaded");
  status_->setTextColour(design::colors::TEXT_SECONDARY());
  addAndMakeVisible(status_.get());

  list_->onSelectionChange = [this]() { updateStatusForSelection(); };
  list_->onRowDoubleClicked = [this](int) {
    if (const auto *selected = getSelectedProject()) {
      if (onOpenProject) {
        onOpenProject(juce::File(selected->path));
      }
    }
  };
}

ProjectManagerUISkia::~ProjectManagerUISkia() = default;

void ProjectManagerUISkia::drawSkia(SkCanvas *canvas) {
  SkPaint bg;
  bg.setColor(design::colors::BG_01());
  canvas->drawRect(SkRect::MakeWH((float)getWidth(), (float)getHeight()), bg);
}

void ProjectManagerUISkia::resized() {
  auto area = getLocalBounds().reduced(10);
  auto head = area.removeFromTop(34);
  title_->setBounds(head.removeFromLeft(260));

  auto searchRow = area.removeFromTop(32);
  search_->setBounds(searchRow.removeFromLeft(300));

  area.removeFromTop(8);
  auto footer = area.removeFromBottom(36);
  deleteButton_->setBounds(footer.removeFromRight(110));
  footer.removeFromRight(6);
  saveButton_->setBounds(footer.removeFromRight(110));
  footer.removeFromRight(6);
  openButton_->setBounds(footer.removeFromRight(110));
  footer.removeFromRight(6);
  newButton_->setBounds(footer.removeFromRight(110));
  status_->setBounds(footer);

  list_->setBounds(area);
}

void ProjectManagerUISkia::setProjects(const std::vector<ProjectRow> &projects) {
  juce::String selectedPath;
  if (const auto *selected = getSelectedProject()) {
    selectedPath = selected->path;
  }

  projects_ = projects;
  filter();

  if (selectedPath.isNotEmpty()) {
    for (int i = 0; i < (int)visible_.size(); ++i) {
      const auto &row = projects_[(size_t)visible_[(size_t)i]];
      if (row.path == selectedPath) {
        list_->selectRow(i, false);
        break;
      }
    }
  }
  updateStatusForSelection();
}

const ProjectManagerUISkia::ProjectRow *ProjectManagerUISkia::getSelectedProject() const {
  const int selectedVisibleRow = list_->getSelectedRow();
  if (selectedVisibleRow < 0 || selectedVisibleRow >= (int)visible_.size()) {
    return nullptr;
  }
  const int sourceIndex = visible_[(size_t)selectedVisibleRow];
  if (sourceIndex < 0 || sourceIndex >= (int)projects_.size()) {
    return nullptr;
  }
  return &projects_[(size_t)sourceIndex];
}

int ProjectManagerUISkia::ProjectListModel::getNumRows() {
  return (int)owner_.visible_.size();
}

void ProjectManagerUISkia::ProjectListModel::paintListBoxItem(
    int rowNumber, SkCanvas &canvas, int width, int height, bool rowIsSelected) {
  if (rowNumber < 0 || rowNumber >= (int)owner_.visible_.size())
    return;

  const auto &row = owner_.projects_[(size_t)owner_.visible_[(size_t)rowNumber]];

  if (rowIsSelected) {
    SkPaint sel;
    sel.setColor(design::withAlpha(design::colors::ACCENT_PRIMARY(), 0.2f));
    canvas.drawRect(SkRect::MakeWH((float)width, (float)height), sel);
  }

  SkFont titleFont = design::getSkFont(12.0f, design::FontWeight::SemiBold);
  SkPaint titlePaint;
  titlePaint.setAntiAlias(true);
  titlePaint.setColor(design::colors::TEXT_PRIMARY());
  canvas.drawString(row.name.toRawUTF8(), 10.0f, 15.0f, titleFont, titlePaint);

  SkFont metaFont = design::getSkFont(10.5f, design::FontWeight::Regular);
  SkPaint metaPaint;
  metaPaint.setAntiAlias(true);
  metaPaint.setColor(design::colors::TEXT_SECONDARY());
  canvas.drawString((row.path + "  " + row.modified).toRawUTF8(), 10.0f, 29.0f,
                    metaFont, metaPaint);
}

void ProjectManagerUISkia::filter() {
  visible_.clear();
  for (int i = 0; i < (int)projects_.size(); ++i) {
    const auto &p = projects_[(size_t)i];
    if (searchTerm_.isEmpty() || p.name.toLowerCase().contains(searchTerm_) ||
        p.path.toLowerCase().contains(searchTerm_)) {
      visible_.push_back(i);
    }
  }
  list_->updateContent();
  status_->setText("Projects: " + juce::String((int)visible_.size()),
                   juce::dontSendNotification);
  status_->setTextColour(design::colors::TEXT_SECONDARY());
}

void ProjectManagerUISkia::updateStatusForSelection() {
  if (const auto *selected = getSelectedProject()) {
    status_->setText("Selected: " + selected->name, juce::dontSendNotification);
    status_->setTextColour(design::colors::TEXT_PRIMARY());
  } else if (visible_.empty()) {
    status_->setText("No projects match filter", juce::dontSendNotification);
    status_->setTextColour(design::colors::WARNING());
  } else {
    status_->setText("Projects: " + juce::String((int)visible_.size()),
                     juce::dontSendNotification);
    status_->setTextColour(design::colors::TEXT_SECONDARY());
  }
}

} // namespace zenith::ui
