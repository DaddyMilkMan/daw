/*
  ==============================================================================

    AsyncOperation.cpp
    Implementation of async operations

  ==============================================================================
*/

#include "AsyncOperation.h"

namespace zenith {

// Template instantiations for common types
template class AsyncOperation<int>;
template class AsyncOperation<float>;
template class AsyncOperation<double>;
template class AsyncOperation<bool>;
template class AsyncOperation<juce::String>;
template class AsyncOperation<juce::MemoryBlock>;

} // namespace zenith
