#include <juce_core/juce_core.h>

namespace juce {
    extern const char* const juce_compilationDate = "2024-01-01";
    extern const char* const juce_compilationTime = "12:00:00";
}

int main() {
    juce::UnitTestRunner runner;
    runner.setPassesAreLogged(true);
    runner.runAllTests();
    return 0;
}
