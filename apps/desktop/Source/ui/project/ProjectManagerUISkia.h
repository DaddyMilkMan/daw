#pragma once

#include "../controls/SkiaLabel.h"
#include "../controls/SkiaListBox.h"
#include "../controls/SkiaTextEditor.h"
#include "../controls/ZenithButton.h"
#include "../framework/SkiaComponent.h"
#include <functional>
#include <juce_core/juce_core.h>
#include <vector>

namespace zenith::ui {

class ProjectManagerUISkia : public SkiaComponent {
public:
  struct ProjectRow {
    juce::String id;
    juce::String name;
    juce::String path;
    juce::String modified;
  };

  ProjectManagerUISkia();
  ~ProjectManagerUISkia() override;

  void drawSkia(SkCanvas *canvas) override;
  void resized() override;

  void setProjects(const std::vector<ProjectRow> &projects);
  const ProjectRow *getSelectedProject() const;

  std::function<void()> onNewProject;
  std::function<void(const juce::File &)> onOpenProject;
  std::function<void(const juce::File &)> onDeleteProject;
  std::function<void()> onSaveRequested;

private:
  class ProjectListModel : public SkiaListBox::Model {
  public:
    explicit ProjectListModel(ProjectManagerUISkia &owner) : owner_(owner) {}
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, SkCanvas &canvas, int width, int height,
                          bool rowIsSelected) override;

  private:
    ProjectManagerUISkia &owner_;
  };

  void filter();
  void updateStatusForSelection();

  std::unique_ptr<SkiaLabel> title_;
  std::unique_ptr<SkiaTextEditor> search_;
  std::unique_ptr<SkiaListBox> list_;
  std::unique_ptr<ZenithButton> newButton_;
  std::unique_ptr<ZenithButton> openButton_;
  std::unique_ptr<ZenithButton> saveButton_;
  std::unique_ptr<ZenithButton> deleteButton_;
  std::unique_ptr<SkiaLabel> status_;

  ProjectListModel model_;
  std::vector<ProjectRow> projects_;
  std::vector<int> visible_;
  juce::String searchTerm_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectManagerUISkia)
};

} // namespace zenith::ui
