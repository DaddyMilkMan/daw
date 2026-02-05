#pragma once

#include "juce_core/juce_core.h"

class TestHelpers {
public:
    TestHelpers();
    ~TestHelpers();

    static juce::String getTestDataPath();
    static void createTestFile(const juce::String& filename, const juce::String& content);
};