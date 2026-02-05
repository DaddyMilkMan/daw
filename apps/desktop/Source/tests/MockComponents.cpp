#include "MockComponents.h"

MockComponent::MockComponent() {
    setName("MockComponent");
}

void MockComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::grey);
}

void MockComponent::resized() {
    // Mock implementation
}