#include "ZenithApplication.h"

ZenithApplication::ZenithApplication() {
    // Initialize application
    initialise(true);
}

ZenithApplication::~ZenithApplication() {
    // Clean up
    shutdown();
}

void ZenithApplication::initialise(const juce::String& commandLine) {
    // Initialize JUCE application
    mainWindow = std::make_unique<MainWindow>("Zenith DAW");
}

void ZenithApplication::shutdown() {
    // Clean up
    mainWindow = nullptr;
}

void ZenithApplication::systemRequestedQuit() {
    quit();
}

void ZenithApplication::anotherInstanceStarted(const juce::String& commandLine) {
    // Handle another instance
}

const juce::String ZenithApplication::getApplicationName() {
    return "Zenith DAW";
}

const juce::String ZenithApplication::getApplicationVersion() {
    return "1.0.0";
}

bool ZenithApplication::moreThanOneInstanceAllowed() {
    return true;
}

MainWindow::MainWindow(const juce::String& name)
    : DocumentWindow(name, juce::Colours::lightgrey, DocumentWindow::allButtons, true) {
    setSize(800, 600);
    setUsingNativeTitleBar(true);

    // Add content component here
    // setContentOwned(new MainComponent(), true);

    centreWithSize(getWidth(), getHeight());
    setVisible(true);
}

MainWindow::~MainWindow() {
    removeChildComponent(contentComponent.get());
    setContentOwned(nullptr, false);
}

void MainWindow:: closeButtonPressed() {
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void MainWindow::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::grey);
}

void MainWindow::resized() {
    // Resize content component
}