/*
  ==============================================================================

    SafeBuffer.cpp
    Implementation of safe buffer operations

  ==============================================================================
*/

#include "SafeBuffer.h"

namespace zenith {

// Template instantiations for common types
template class SafeBuffer<float>;
template class SafeBuffer<double>;
template class SafeBuffer<juce::int8>;
template class SafeBuffer<juce::int16>;
template class SafeBuffer<juce::int32>;
template class SafeBuffer<juce::int64>;
template class SafeBuffer<juce::uint8>;
template class SafeBuffer<juce::uint16>;
template class SafeBuffer<juce::uint32>;
template class SafeBuffer<juce::uint64>;

} // namespace zenith
