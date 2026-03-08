/*
  ==============================================================================

    OOMHandlerIntegration.cpp
    Example integration of OOMHandler with application systems

    This file demonstrates how to properly integrate the OOMHandler with
    your application's plugin and project management systems.

  ==============================================================================
*/

#include "OOMHandler.h"
#include <juce_core/juce_core.h>

namespace zenith {

//==============================================================================
/**
 * @brief Example: Integrating OOMHandler with your application
 *
 * Call this function during application initialization to set up
 * the OOM recovery callbacks.
 */
void setupOOMHandlerIntegration() {
    auto& oomHandler = OOMHandler::getInstance();

    //==========================================================================
    // 1. Set up plugin close callback
    //==========================================================================
    oomHandler.setPluginCloseCallback([](juce::uint64 targetBytesToFree) -> juce::uint64 {
        juce::uint64 totalFreed = 0;
        juce::uint32 pluginsClosed = 0;

        std::cout << "OOMHandler Integration: Closing plugins to free "
                  << (targetBytesToFree / (1024 * 1024)) << " MB..." << std::endl;

        // TODO: Access your application's plugin manager here
        //
        // Example for a typical plugin manager:
        //
        // auto& pluginManager = MyApplication::getPluginManager();
        // auto activePlugins = pluginManager.getActivePlugins();
        //
        // // Sort plugins by memory usage (close largest first)
        // activePlugins.sort([](auto* a, auto* b) {
        //     return a->getMemoryUsage() > b->getMemoryUsage();
        // });
        //
        // for (auto* plugin : activePlugins) {
        //     if (totalFreed >= targetBytesToFree) {
        //         break;
        //     }
        //
        //     juce::uint64 pluginMemory = plugin->getMemoryUsage();
        //     pluginManager.closePlugin(plugin);
        //     totalFreed += pluginMemory;
        //     pluginsClosed++;
        //
        //     std::cout << "  Closed: " << plugin->getName()
        //               << " (~" << (pluginMemory / (1024 * 1024)) << " MB)" << std::endl;
        // }

        // For now, return 0 to indicate no callback was set up
        // (The application needs to implement the actual plugin closing logic)
        return totalFreed;
    });

    //==========================================================================
    // 2. Set up project save callback
    //==========================================================================
    oomHandler.setProjectSaveCallback([](const juce::String& backupPath) -> bool {
        std::cout << "OOMHandler Integration: Saving project to: "
                  << backupPath << std::endl;

        // TODO: Access your application's project/document system here
        //
        // Example for a typical project system:
        //
        // auto* app = juce::JUCEApplication::getInstance();
        // if (app == nullptr) {
        //     std::cerr << "  ERROR: No application instance!" << std::endl;
        //     return false;
        // }
        //
        // auto& projectManager = app->getProjectManager();
        //
        // // Perform emergency save
        // bool saveSuccess = projectManager.saveProjectAs(backupPath);
        //
        // if (saveSuccess) {
        //     std::cout << "  Project saved successfully" << std::endl;
        // } else {
        //     std::cerr << "  ERROR: Project save failed!" << std::endl;
        // }
        //
        // return saveSuccess;

        // For now, return false to indicate no callback was set up
        // (The application needs to implement the actual project save logic)
        return false;
    });

    //==========================================================================
    // 3. Set up pressure callback (optional - for monitoring)
    //==========================================================================
    oomHandler.setPressureCallback([](MemoryPressure pressure, const MemorySnapshot& snapshot) {
        if (pressure >= MemoryPressure::High) {
            std::cerr << "OOMHandler: Memory pressure detected - "
                      << snapshot.toString() << std::endl;
        }

        if (pressure == MemoryPressure::Critical) {
            std::cerr << "OOMHandler: CRITICAL memory pressure!" << std::endl;
        }
    });

    //==========================================================================
    // 4. Set up OOM callback (optional - for final warning)
    //==========================================================================
    oomHandler.setOOMCallback([]() {
        std::cerr << "\n========================================" << std::endl;
        std::cerr << "OOMHandler: OUT OF MEMORY IMMINENT" << std::endl;
        std::cerr << "========================================\n" << std::endl;
    });

    std::cout << "OOMHandler Integration: Callbacks registered successfully" << std::endl;
}

//==============================================================================
/**
 * @brief Example: Manual memory monitoring loop
 *
 * Call this periodically (e.g., in a timer callback) to monitor memory
 * and trigger recovery actions when needed.
 */
void monitorMemoryAndRecover() {
    auto& oomHandler = OOMHandler::getInstance();

    // Get current memory snapshot
    auto snapshot = oomHandler.getMemorySnapshot();

    // Get recommended recovery action
    auto action = oomHandler.getRecoveryAction(snapshot);

    // Perform recovery if needed
    if (action.type != OOMRecoveryAction::None) {
        std::cout << "OOMHandler: Memory status: " << snapshot.toString() << std::endl;
        oomHandler.attemptRecovery(action);
    }
}

//==============================================================================
/**
 * @brief Example: Integrating with MainComponent
 *
 * Add this to your MainComponent class to enable automatic OOM monitoring.
 *
 * In MainComponent.h:
 * ```cpp
 * class MainComponent : public juce::Component, private juce::Timer {
 * public:
 *     MainComponent();
 *     ~MainComponent() override;
 *
 * private:
 *     void timerCallback() override;
 *
 *     // OOM monitoring
 *     void setupOOMMonitoring();
 *     void checkMemoryPressure();
 * };
 * ```
 *
 * In MainComponent.cpp:
 * ```cpp
 * MainComponent::MainComponent() {
 *     setupOOMMonitoring();
 * }
 *
 * void MainComponent::setupOOMMonitoring() {
 *     // Set up integration (see setupOOMHandlerIntegration above)
 *     setupOOMHandlerIntegration();
 *
 *     // Start timer to check memory every 1 second
 *     startTimer(1000);
 * }
 *
 * void MainComponent::timerCallback() {
 *     checkMemoryPressure();
 * }
 *
 * void MainComponent::checkMemoryPressure() {
 *     monitorMemoryAndRecover();
 * }
 * ```
 */

} // namespace zenith
