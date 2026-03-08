#include "CarbonFiberTexture.h"
#include "IndustrialTheme.h"
#include <juce_core/juce_core.h>

namespace zenith::industrial {

SkBitmap CarbonFiberTexture::generate(int width, int height, float contrast) {
  SkBitmap bitmap;
  bitmap.allocN32Pixels(width, height);

  const uint8_t delta = static_cast<uint8_t>(juce::jlimit(1.0f, 40.0f, contrast * 255.0f));
  const auto blend = [delta](uint8_t base, bool add) {
    const int value = add ? static_cast<int>(base) + delta : static_cast<int>(base) - delta;
    return static_cast<uint8_t>(juce::jlimit(0, 255, value));
  };

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const bool diagonalA = ((x + y) & 1) == 0;
      const bool diagonalB = (((x - y) & 3) == 0);
      const uint8_t base = diagonalA ? 0x14 : 0x0A;
      const bool brighten = diagonalB;
      bitmap.eraseArea(SkIRect::MakeXYWH(x, y, 1, 1),
                       SkColorSetARGB(0xFF,
                                      blend(base, brighten),
                                      blend(base, !brighten),
                                      blend(base, brighten)));
    }
  }

  return bitmap;
}

} // namespace zenith::industrial
