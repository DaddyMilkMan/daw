/*
  ==============================================================================

    PlatformDisplayUtils.h
    Created: 2025-12-22

    Platform-specific display utilities (refresh rate etc).

  ==============================================================================
*/

#pragma once

namespace zenith {

class PlatformDisplayUtils
{
public:
    static int getSystemRefreshRate();
};

} // namespace zenith
