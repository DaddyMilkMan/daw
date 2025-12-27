/*
  ==============================================================================

    PlatformFontUtils_Linux.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "../../../ui/design-system/PlatformFontUtils.h"

#ifdef __linux__
#include <include/ports/SkFontMgr_fontconfig.h>
#include <include/ports/SkFontScanner_FreeType.h>
#include <juce_core/juce_core.h>
#include <memory>

namespace zenith {
namespace design {

sk_sp<SkFontMgr> PlatformFontUtils::createDefaultFontManager() {
    auto fontMgr = SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
    if (!fontMgr) {
        DBG("[FontManager] WARNING: FontConfig font manager unavailable, using empty manager");
        return SkFontMgr::RefEmpty();
    }
    return fontMgr;
}

} // namespace design
} // namespace zenith
#endif
