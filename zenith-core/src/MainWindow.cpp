/**
 * @file MainWindow.cpp
 * @brief Main window implementation
 */

#include "../include/MainWindow.h"
#include "../Source/ui/MainComponent.h"

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

        helpAbout = 4000
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
