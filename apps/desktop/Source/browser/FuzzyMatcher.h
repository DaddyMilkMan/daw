/*
  ==============================================================================

    FuzzyMatcher.h
    Created: 2025-12-26
    Author:  Zenith DAW

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace zenith {

class FuzzyMatcher {
public:
    /**
     * Scores a candidate string against a pattern.
     * Higher score = better match.
     * Returns 0 if no match.
     */
    static int score(const juce::String& pattern, const juce::String& candidate) {
        if (pattern.isEmpty()) return 100;
        
        juce::String p = pattern.toLowerCase();
        juce::String c = candidate.toLowerCase();
        
        if (p == c) return 1000;
        if (c.startsWith(p)) return 800 + p.length();
        if (c.contains(p)) return 500 + p.length();
        
        // Subsequence matching
        int score = 0;
        int pIdx = 0;
        int lastMatchIdx = -1;
        
        for (int i = 0; i < c.length() && pIdx < p.length(); ++i) {
            if (c[i] == p[pIdx]) {
                // Bonus for proximity
                if (lastMatchIdx != -1) {
                    int dist = i - lastMatchIdx;
                    if (dist == 1) score += 20;
                    else if (dist < 3) score += 5;
                }
                
                score += 10;
                lastMatchIdx = i;
                pIdx++;
            }
        }
        
        if (pIdx == p.length()) return score;
        return 0;
    }
};

} // namespace zenith
