/*
  ==============================================================================

    AudioBufferGuard.cpp
    Implementation of audio buffer guard

  ==============================================================================
*/

#include "AudioBufferGuard.h"

namespace zenith {

// Template instantiations for common audio types
template class AudioBufferGuard<float>;
template class AudioBufferGuard<double>;

} // namespace zenith
