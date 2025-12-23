/*
  ==============================================================================

    PlatformFontUtils.h
    Created: 2025-12-22

    Interface for platform-specific font utility functions.

  ==============================================================================
*/

#pragma once

#include <include/core/SkFontMgr.h>
#include <include/core/SkRefCnt.h>

namespace zenith {
namespace design {

class PlatformFontUtils {
public:
    /**
     * Creates a new platform-native font manager.
     */
    static sk_sp<SkFontMgr> createDefaultFontManager();
};

} // namespace design
} // namespace zenith
