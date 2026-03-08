#include <juce_core/juce_core.h>
int main() {
    juce::File f("test.txt");
    juce::FileInputStream stream(f);
    juce::SHA256 hash(stream);
    return 0;
}
