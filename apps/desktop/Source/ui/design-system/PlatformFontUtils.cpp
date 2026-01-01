/*
  ==============================================================================

    PlatformFontUtils.cpp
    Created: 2025-12-28
    Author:  Zenith DAW

    Implementation of platform-specific font utility functions.

  ==============================================================================
*/

#include "PlatformFontUtils.h"

#if defined(ZENITH_USE_SKIA) && ZENITH_USE_SKIA

#if defined(WIN32) || defined(_WIN32)
#include <include/ports/SkTypeface_win.h>
#elif defined(__APPLE__)
#include <include/ports/SkTypeface_mac.h>
#elif defined(__linux__)
#include <include/ports/SkFontMgr_fontconfig.h>
#include <include/core/SkFontScanner.h>
#include <include/ports/SkFontScanner_FreeType.h>
#else
#include <include/ports/SkFontMgr_empty.h>
#endif

#endif

namespace zenith {
namespace design {

#if defined(ZENITH_USE_SKIA) && ZENITH_USE_SKIA
sk_sp<SkFontMgr> PlatformFontUtils::createDefaultFontManager()
{
#if defined(WIN32) || defined(_WIN32)
    return SkFontMgr_New_DirectWrite();
#elif defined(__APPLE__)
    return SkFontMgr_New_CoreText(nullptr);
#elif defined(__linux__)
    return SkFontMgr_New_FontConfig(nullptr, SkFontScanner_Make_FreeType());
#else
    return SkFontMgr_New_Custom_Empty();
#endif
}
#endif

} // namespace design
} // namespace zenith
