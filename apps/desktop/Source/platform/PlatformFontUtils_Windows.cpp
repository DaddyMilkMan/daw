/*
  ==============================================================================

    PlatformFontUtils_Windows.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "ui/design-system/PlatformFontUtils.h"

#ifdef _WIN32
#include <ports/SkTypeface_win.h>
#include <juce_core/juce_core.h>

namespace zenith {
namespace design {

sk_sp<SkFontMgr> PlatformFontUtils::createDefaultFontManager() {
    auto fontMgr = SkFontMgr_New_DirectWrite();
    if (!fontMgr) {
        DBG("[FontManager] WARNING: DirectWrite font manager unavailable, using empty manager");
        return SkFontMgr::RefEmpty();
    }
    return fontMgr;
}

} // namespace design
} // namespace zenith
#endif
