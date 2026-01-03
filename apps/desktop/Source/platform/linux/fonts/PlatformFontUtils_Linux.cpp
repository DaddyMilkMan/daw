/*
  ==============================================================================

    PlatformFontUtils_Linux.cpp
    Created: 2025-12-22

  ==============================================================================
*/

#include "../../../ui/design-system/PlatformFontUtils.h"

#ifdef __linux__
#include <core/SkFontMgr.h>
#include <ports/SkFontMgr_fontconfig.h>
#include <ports/SkFontScanner_FreeType.h>
#include <juce_core/juce_core.h>
#include <memory>

namespace zenith {
namespace design {

sk_sp<SkFontMgr> PlatformFontUtils::createDefaultFontManager() {
    // Manually create FontConfig-based manager as RefDefault is missing in this Skia version
    auto scanner = SkFontScanner_Make_FreeType();
    if (!scanner) {
        DBG("[FontManager] ERROR: Failed to create FreeType scanner");
        return nullptr;
    }
    
    auto fontMgr = SkFontMgr_New_FontConfig(nullptr, std::move(scanner));
    if (!fontMgr) {
        DBG("[FontManager] WARNING: FontConfig font manager creation failed");
        return SkFontMgr::RefEmpty();
    }
    return fontMgr;
}

} // namespace design
} // namespace zenith
#endif
