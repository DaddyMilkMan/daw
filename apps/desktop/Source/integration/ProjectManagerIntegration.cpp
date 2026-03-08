/*
  ==============================================================================

    ProjectManagerIntegration.cpp
    Example: How to integrate OOMHandler with project saving

  ==============================================================================
*/

#include "../memory/OOMHandler.h"
#include <juce_core/juce_core.h>
#include <fstream>

using namespace zenith;

//==============================================================================
/**
 * @brief Example project manager with emergency save
 */
class ProjectManagerWithOOM {
public:
    //==========================================================================
    ProjectManagerWithOOM() {
        setupOOMCallbacks();
        std::cout << "ProjectManager: Initialized with OOM protection" << std::endl;
    }

    //==========================================================================
    /**
     * @brief Save project to specific path
     */
    bool saveProject(const juce::String& path) {
        std::cout << "ProjectManager: Saving to " << path << std::endl;

        // In real implementation, you'd serialize your project state
        // For this example, create a simple backup file

        std::ofstream file(path.toRawUTF8());
        if (!file.is_open()) {
            std::cerr << "ProjectManager: Failed to create file" << std::endl;
            return false;
        }

        // Write project data
        file << "# Zenith Emergency Backup\n";
        file << "# Created: " << juce::Time::getCurrentTime().toString(true, true) << "\n";
        file << "# TODO: Add actual project state here\n";

        file.close();

        std::cout << "ProjectManager: Save complete" << std::endl;
        return true;
    }

    //==========================================================================
    /**
     * @brief Get current project
     */
    juce::String getCurrentProjectName() const {
        return currentProjectName_;
    }

private:
    //==========================================================================
    juce::String currentProjectName_;

    //==========================================================================
    void setupOOMCallbacks() {
        auto& oom = OOMHandler::getInstance();

        // Project save callback - OOMHandler calls this during emergency
        oom.setProjectSaveCallback([this](const juce::String& backupPath) -> bool {
            std::cout << "ProjectManager: EMERGENCY SAVE to " << backupPath << std::endl;

            // Perform emergency save
            bool success = saveProject(backupPath);

            if (success) {
                std::cout << "ProjectManager: Emergency save succeeded" << std::endl;
            } else {
                std::cerr << "ProjectManager: Emergency save FAILED!" << std::endl;
            }

            return success;
        });

        std::cout << "ProjectManager: OOM callbacks registered" << std::endl;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectManagerWithOOM)
};
