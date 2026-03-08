/*
  ==============================================================================

    ThreadSafeQueue.cpp
    Implementation of thread-safe queue

  ==============================================================================
*/

#include "ThreadSafeQueue.h"

namespace zenith {

// Template instantiations for common types
template class ThreadSafeQueue<int>;
template class ThreadSafeQueue<float>;
template class ThreadSafeQueue<double>;
template class ThreadSafeQueue<juce::String>;
template class ThreadSafeQueue<juce::MemoryBlock>;

// Priority queue instantiations
template class PriorityQueue<juce::String, int>;
template class PriorityQueue<std::function<void()>, int>;

} // namespace zenith
