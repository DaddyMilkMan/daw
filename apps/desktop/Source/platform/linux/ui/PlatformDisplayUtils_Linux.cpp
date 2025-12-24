/*
  ==============================================================================

    PlatformDisplayUtils_Linux.cpp
    Created: 2025-12-22

    Linux implementation of display utilities.
    (Empty/Default implementation)

  ==============================================================================
*/

#include "../../../ui/framework/PlatformDisplayUtils.h"
#include <juce_core/juce_core.h>

namespace zenith {

int PlatformDisplayUtils::getSystemRefreshRate() {
  juce::ChildProcess xrandr;

  if (xrandr.start("xrandr")) {
    auto output = xrandr.readAllProcessOutput();
    auto lines = juce::StringArray::fromLines(output);

    for (auto line : lines) {
      line = line.trim();

      // Look for the active mode marked with '*'
      if (line.contains("*")) {
        // Example lines:
        // "2560x1440    259.96*+"
        // "1920x1080   60.00*"

        auto tokens = juce::StringArray::fromTokens(line, false);
        for (int i = 0; i < tokens.size(); ++i) {
          if (tokens[i].contains("*")) {
            // The token might look like "259.96*+"
            auto rateStr = tokens[i].retainCharacters("0123456789.");
            auto rate = rateStr.getFloatValue();

            if (rate > 0) {
              return juce::roundToInt(rate);
            }
          }
        }
      }
    }
  }

  return 60; // Default fallback
}

} // namespace zenith
