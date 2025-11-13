/**
 * @file ExportWavJob.cpp
 * @brief Implementation of background export worker
 */

#include "ExportWavJob.h"
#include <editor/ProjectEditorState.h>

//==============================================================================
// Constructor / Destructor
//==============================================================================

ExportWavJob::ExportWavJob(zenith::ProjectEditorState& editor,
                           const juce::File& outputFile,
                           int blockSize,
                           double tailSeconds)
    : juce::Thread("ExportWavJob"),
      editorState_(&editor),
      outputFile_(outputFile),
      blockSize_(blockSize),
      tailSeconds_(tailSeconds)
{
    DBG("ExportWavJob: Created for output: " + outputFile.getFullPathName());
}

ExportWavJob::~ExportWavJob()
{
    // Ensure thread is stopped before destruction
    stopThread(5000);  // 5 second timeout
}

//==============================================================================
// Worker Thread
//==============================================================================

void ExportWavJob::run()
{
    // WORKER THREAD - no UI calls allowed here

    DBG("ExportWavJob: Worker thread started");

    // Check if editor state still exists
    auto* editor = editorState_.getComponent();
    if (editor == nullptr)
    {
        DBG("ExportWavJob: Editor state no longer exists");
        success_.store(false);
        errorMessage_ = "Editor state no longer exists.";
        return;
    }

    // Run the export (this is blocking and may take a while)
    DBG("ExportWavJob: Calling renderCurrentProjectToWav()...");
    auto result = editor->renderCurrentProjectToWav(outputFile_, blockSize_, tailSeconds_);

    // Store results
    success_.store(result.wasOk());

    if (result.wasOk())
    {
        DBG("ExportWavJob: Export succeeded!");
        errorMessage_ = juce::String();  // Clear error message
    }
    else
    {
        DBG("ExportWavJob: Export failed - " + result.getErrorMessage());
        errorMessage_ = result.getErrorMessage();
    }

    DBG("ExportWavJob: Worker thread finished");
}
