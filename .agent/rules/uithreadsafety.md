# UI Thread Safety
1. **Message Thread Only**: All UI modifications (setBounds, repaint, addAndMakeVisible) MUST happen on the Message Thread. Check juce::MessageManager::getInstance()->isThisTheMessageThread() if unsure.
2. **Async Updates**: For updates from other threads, use juce::AsyncUpdater or juce::MessageManager::callAsync.
3. **No Direct Thread Access**: Use SafePointers (juce::Component::SafePointer) if a callback might outlive the component.
4. **Audio Logic Separation**: UI components should read state, not hold it. Push logic to the Engine/Processor.
