/*
  ==============================================================================

    PlatformFontUtils_Mac.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "../../../ui/design-system/PlatformFontUtils.h"

#ifdef __APPLE__
#include <juce_core/juce_core.h>

namespace zenith {
namespace design {

sk_sp<SkFontMgr> PlatformFontUtils::createDefaultFontManager() {
    return SkFontMgr::RefEmpty(); // Placeholder for Mac
}

} // namespace design
} // namespace zenith
#endif
