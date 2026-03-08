#pragma once

#include "../../ZenithSkia.h"
#include <core/SkBitmap.h>

namespace zenith::industrial {

class CarbonFiberTexture {
public:
  static SkBitmap generate(int width, int height, float contrast);
};

} // namespace zenith::industrial
