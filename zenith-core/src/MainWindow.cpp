/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "../include/MainWindow.h"
#include "../Source/ui/MainComponent.h"

#if defined(ZENITH_USE_CRASHPAD) && defined(JUCE_WINDOWS)
    #include "../Source/win/CrashpadInit.h"
#endif

//==============================================================================
// MenuBar Implementation (W6.1: View menu for Performance HUD)
//==============================================================================

class MainWindow::MenuBar : public juce::MenuBarModel
{
public:
    explicit MenuBar(MainWindow& owner) : owner(owner) {}

    juce::StringArray getMenuBarNames() override
    {
        return { "File", "Edit", "View", "Help" };
    }

    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override
    {
        juce::PopupMenu menu;

        if (menuName == "File")
        {
            menu.addItem(MenuItemIDs::fileNew, "New Project", true, false);
            menu.addItem(MenuItemIDs::fileOpen, "Open Project...", true, false);
            menu.addSeparator();
            menu.addItem(MenuItemIDs::fileSave, "Save Project", true, false);
            menu.addItem(MenuItemIDs::fileSaveAs, "Save Project As...", true, false);
            menu.addSeparator();
            menu.addItem(MenuItemIDs::fileQuit, "Quit", true, false);
        }
        else if (menuName == "Edit")
        {
            menu.addItem(MenuItemIDs::editUndo, "Undo", false, false);
            menu.addItem(MenuItemIDs::editRedo, "Redo", false, false);
            menu.addSeparator();
            menu.addItem(MenuItemIDs::editCut, "Cut", false, false);
            menu.addItem(MenuItemIDs::editCopy, "Copy", false, false);
            menu.addItem(MenuItemIDs::editPaste, "Paste", false, false);
        }
        else if (menuName == "View")
        {
            #if JUCE_DEBUG
                // W6.1: Performance HUD toggle (Debug-only)
                bool hudVisible = false;
                if (auto* mainComp = owner.getMainComponent())
                {
                    if (auto* statsOverlay = mainComp->getStatsOverlay())
                        hudVisible = statsOverlay->isVisible();
                }

                menu.addItem(MenuItemIDs::viewPerfHUD, "Performance HUD",
                            true, hudVisible,
                            [](int result) {});
            #endif

            menu.addItem(MenuItemIDs::viewMixer, "Show Mixer", true, false);
            menu.addItem(MenuItemIDs::viewBrowser, "Show Browser", true, false);
        }
        else if (menuName == "Help")
        {
            #if defined(ZENITH_USE_CRASHPAD) && defined(JUCE_WINDOWS)
                // W8: Diagnostics submenu (crash reporting & testing)
                juce::PopupMenu diagnosticsMenu;

                // Check if crash reports are enabled
                bool crashReportsEnabled = false;
                if (auto* mainComp = owner.getMainComponent())
                {
                    if (auto* userSettings = mainComp->getAppProperties().getUserSettings())
                        crashReportsEnabled = userSettings->getBoolValue("diagnostics.crashReportsEnabled", false);
                }

                diagnosticsMenu.addItem(MenuItemIDs::diagEnableCrashReports,
                                       "Enable Crash Reports",
                                       true,
                                       crashReportsEnabled,
                                       [](int result) {});

                #if JUCE_DEBUG
                    diagnosticsMenu.addSeparator();
                    diagnosticsMenu.addItem(MenuItemIDs::diagTriggerTestCrash,
                                           "Trigger Test Crash",
                                           true,
                                           false,
                                           [](int result) {});
                #endif

                menu.addSubMenu("Diagnostics", diagnosticsMenu);
                menu.addSeparator();
            #endif

            menu.addItem(MenuItemIDs::helpAbout, "About Zenith DAW...", true, false);
        }

        return menu;
    }

    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override
    {
        switch (menuItemID)
        {
            case MenuItemIDs::fileQuit:
                juce::JUCEApplication::getInstance()->systemRequestedQuit();
                break;

            #if JUCE_DEBUG
                case MenuItemIDs::viewPerfHUD:
                    if (auto* mainComp = owner.getMainComponent())
                        mainComp->toggleStatsOverlay();
                    break;
            #endif

            #if defined(ZENITH_USE_CRASHPAD) && defined(JUCE_WINDOWS)
                case MenuItemIDs::diagEnableCrashReports:
                {
                    // W8: Toggle crash reporting enable/disable
                    if (auto* mainComp = owner.getMainComponent())
                    {
                        if (auto* userSettings = mainComp->getAppProperties().getUserSettings())
                        {
                            bool currentState = userSettings->getBoolValue("diagnostics.crashReportsEnabled", false);
                            bool newState = !currentState;
                            userSettings->setValue("diagnostics.crashReportsEnabled", newState);
                            userSettings->saveIfNeeded();

                            DBG("W8: Crash reporting " + juce::String(newState ? "enabled" : "disabled"));

                            // Show alert to user
                            juce::AlertWindow::showMessageBoxAsync(
                                juce::AlertWindow::InfoIcon,
                                "Crash Reporting " + juce::String(newState ? "Enabled" : "Disabled"),
                                newState
                                    ? "Crash reports will be saved locally on your computer.\n\n"
                                      "Location: %APPDATA%\\ZenithDAW\\crashpad_db\\\n\n"
                                      "Restart the application for changes to take effect."
                                    : "Crash reporting has been disabled.\n\n"
                                      "Restart the application for changes to take effect.",
                                "OK");
                        }
                    }
                    break;
                }

                #if JUCE_DEBUG
                    case MenuItemIDs::diagTriggerTestCrash:
                    {
                        // W8: Trigger intentional crash for testing (DEBUG only)
                        auto result = juce::AlertWindow::showOkCancelBox(
                            juce::AlertWindow::WarningIcon,
                            "Trigger Test Crash",
                            "This will intentionally crash the application to test crash reporting.\n\n"
                            "A minidump (.dmp) will be created in:\n"
                            "%APPDATA%\\ZenithDAW\\crashpad_db\\completed\\\n\n"
                            "Continue?",
                            "Crash Now",
                            "Cancel");

                        if (result)
                        {
                            DBG("W8: User triggered test crash");
                            zenith::diag::triggerTestCrash();
                        }
                        break;
                    }
                #endif
            #endif

            default:
                DBG("Menu item " + juce::String(menuItemID) + " not implemented");
                break;
        }
    }

private:
    MainWindow& owner;

    // Menu item IDs
    enum MenuItemIDs
    {
        fileNew = 1000,
        fileOpen,
        fileSave,
        fileSaveAs,
        fileQuit,

        editUndo = 2000,
        editRedo,
        editCut,
        editCopy,
        editPaste,

        viewPerfHUD = 3000,
        viewMixer,
        viewBrowser,

        helpAbout = 4000,
        diagEnableCrashReports = 4100,  // W8: Crash reporting toggle
        diagTriggerTestCrash = 4101      // W8: Test crash (DEBUG only)
    };
};

//==============================================================================
// MainWindow Implementation
//==============================================================================

MainWindow::MainWindow(const juce::String& name)
    : DocumentWindow(name,
                     juce::Desktop::getInstance().getDefaultLookAndFeel()
                         .findColour(juce::ResizableWindow::backgroundColourId),
                     DocumentWindow::allButtons)
{
    // Create audio engine first
    engine = std::make_unique<Engine>();

    // Create project state
    projectState = std::make_unique<ProjectState>();

    // Create main content (new custom JUCE UI)
    mainComponent = std::make_unique<::MainComponent>(*engine);

    // Create menu bar (W6.1)
    menuBar = std::make_unique<MenuBar>(*this);
    setMenuBar(menuBar.get());

    // Set up window
    setUsingNativeTitleBar(true);
    setContentOwned(mainComponent.get(), true);

    #if JUCE_IOS || JUCE_ANDROID
        setFullScreen(true);
    #else
        setResizable(true, true);
        centreWithSize(getWidth(), getHeight());
    #endif

    setVisible(true);

    // Initialize audio engine after window is visible
    engine->initialize();

    DBG("MainWindow created and initialized");
}

MainWindow::~MainWindow()
{
    // Shutdown audio engine before destroying components
    if (engine)
        engine->shutdown();

    // Clear content
    clearContentComponent();

    DBG("MainWindow destroyed");
}

void MainWindow::closeButtonPressed()
{
    // TODO: Check for unsaved changes
    // TODO: Show save dialog if needed

    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
