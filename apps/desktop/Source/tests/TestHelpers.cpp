#include "TestHelpers.h"

TestHelpers::TestHelpers() {
    // Initialize test helpers
}

TestHelpers::~TestHelpers() {
    // Clean up test helpers
}

juce::String TestHelpers::getTestDataPath() {
    return juce::File::getSpecialLocation(juce::File::currentDirectory).getFullPathName() + "/test_data";
}

void TestHelpers::createTestFile(const juce::String& filename, const juce::String& content) {
    juce::File testFile = juce::File(getTestDataPath()).getChildFile(filename);
    testFile.create();
    testFile.replaceWithText(content);
}