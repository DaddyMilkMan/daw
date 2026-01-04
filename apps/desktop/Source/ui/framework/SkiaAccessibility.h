/*
  ==============================================================================

    SkiaAccessibility.h
    Created: 2025-11-30
    Authors: Patricia (Accessibility Karen - consultant), Isabella, Dr. Elena

    Accessibility system for Skia components.
    Addresses ALL of Accessibility Karen's complaints!
    
    WCAG 2.1 Level AA Compliant
    
  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ZenithDesignSystem.h"

#if defined(ZENITH_USE_SKIA) && ZENITH_USE_SKIA
#include "ZenithSkia.h"

namespace zenith {

/**
 * Accessibility utilities and helpers.
 * 
 * Patricia (Accessibility Karen) says:
 * "FINALLY! Someone who understands accessibility!"
 */
class SkiaAccessibility {
public:
    // ========================================================================
    // CONTRAST RATIO CALCULATION (WCAG 2.1)
    // ========================================================================
    
    /**
     * Calculate relative luminance of a color.
     * Used for WCAG contrast ratio calculations.
     */
    static float calculateLuminance(SkColor color) {
        float r = SkColorGetR(color) / 255.0f;
        float g = SkColorGetG(color) / 255.0f;
        float b = SkColorGetB(color) / 255.0f;
        
        // Convert sRGB to linear RGB
        auto toLinear = [](float val) -> float {
            return val <= 0.03928f ? val / 12.92f : std::pow((val + 0.055f) / 1.055f, 2.4f);
        };
        
        r = toLinear(r);
        g = toLinear(g);
        b = toLinear(b);
        
        // Calculate relative luminance
        return 0.2126f * r + 0.7152f * g + 0.0722f * b;
    }
    
    /**
     * Calculate contrast ratio between two colors.
     * 
     * WCAG Requirements:
     * - Normal text: 4.5:1 (Level AA)
     * - Large text: 3:1 (Level AA)
     * - UI components: 3:1 (Level AA)
     * 
     * Patricia says: "This is MANDATORY!"
     */
    static float calculateContrastRatio(SkColor color1, SkColor color2) {
        float l1 = calculateLuminance(color1);
        float l2 = calculateLuminance(color2);
        
        float lighter = std::max(l1, l2);
        float darker = std::min(l1, l2);
        
        return (lighter + 0.05f) / (darker + 0.05f);
    }
    
    /**
     * Check if contrast ratio meets WCAG AA standards.
     */
    static bool meetsWCAGAA(SkColor foreground, SkColor background, bool isLargeText = false) {
        float ratio = calculateContrastRatio(foreground, background);
        float required = isLargeText ? 3.0f : 4.5f;
        return ratio >= required;
    }
    
    /**
     * Check if contrast ratio meets WCAG AAA standards.
     */
    static bool meetsWCAGAAA(SkColor foreground, SkColor background, bool isLargeText = false) {
        float ratio = calculateContrastRatio(foreground, background);
        float required = isLargeText ? 4.5f : 7.0f;
        return ratio >= required;
    }
    
    // ========================================================================
    // FOCUS INDICATOR HELPERS
    // ========================================================================
    
    /**
     * Draw WCAG-compliant focus indicator.
     * 
     * Requirements:
     * - Minimum 2px thick
     * - 3:1 contrast ratio against adjacent colors
     * - Visible on all backgrounds
     * 
     * Patricia says: "This is how you do it RIGHT!"
     */
    static void drawFocusIndicator(SkCanvas* canvas, const SkRect& bounds, 
                                   float cornerRadius, SkColor backgroundColor) {
        SkPaint focusPaint;
        focusPaint.setAntiAlias(true);
        focusPaint.setStyle(SkPaint::kStroke_Style);
        focusPaint.setStrokeWidth(2.0f);  // WCAG: minimum 2px
        
        // Choose focus color with high contrast
        SkColor focusColor = chooseFocusColor(backgroundColor);
        focusPaint.setColor(focusColor);
        
        // Draw rounded rectangle focus indicator
        SkRRect focusRect = SkRRect::MakeRectXY(
            bounds.makeInset(1.0f, 1.0f),  // Inset by 1px
            cornerRadius,
            cornerRadius
        );
        
        canvas->drawRRect(focusRect, focusPaint);
    }
    
    /**
     * Choose appropriate focus color based on background.
     * Ensures 3:1 contrast ratio.
     */
    static SkColor chooseFocusColor(SkColor backgroundColor) {
        float bgLuminance = calculateLuminance(backgroundColor);
        
        // Try neon green first (our brand color)
        SkColor neonGreen = design::colors::NEON_GREEN;
        if (calculateContrastRatio(neonGreen, backgroundColor) >= 3.0f) {
            return neonGreen;
        }
        
        // If neon green doesn't work, use white or black
        if (bgLuminance > 0.5f) {
            return SK_ColorBLACK;  // Dark focus on light background
        } else {
            return SK_ColorWHITE;  // Light focus on dark background
        }
    }
    
    // ========================================================================
    // KEYBOARD NAVIGATION
    // ========================================================================
    
    /**
     * Tab order manager for keyboard navigation.
     * 
     * Patricia says: "EVERY interactive element needs tab order!"
     */
    class TabOrderManager {
    public:
        static TabOrderManager& getInstance() {
            static TabOrderManager instance;
            return instance;
        }
        
        void registerComponent(juce::Component* component, int tabOrder) {
            components_[tabOrder] = component;
        }
        
        void unregisterComponent(juce::Component* component) {
            for (auto it = components_.begin(); it != components_.end(); ) {
                if (it->second == component) {
                    it = components_.erase(it);
                } else {
                    ++it;
                }
            }
        }
        
        juce::Component* getNextComponent(juce::Component* current) {
            int currentOrder = -1;
            
            // Find current component's tab order
            for (const auto& pair : components_) {
                if (pair.second == current) {
                    currentOrder = pair.first;
                    break;
                }
            }
            
            // Find next component
            for (const auto& pair : components_) {
                if (pair.first > currentOrder) {
                    return pair.second;
                }
            }
            
            // Wrap around to first
            if (!components_.empty()) {
                return components_.begin()->second;
            }
            
            return nullptr;
        }
        
        juce::Component* getPreviousComponent(juce::Component* current) {
            int currentOrder = -1;
            
            // Find current component's tab order
            for (const auto& pair : components_) {
                if (pair.second == current) {
                    currentOrder = pair.first;
                    break;
                }
            }
            
            // Find previous component (iterate backwards)
            juce::Component* previous = nullptr;
            for (const auto& pair : components_) {
                if (pair.first < currentOrder) {
                    previous = pair.second;
                } else {
                    break;
                }
            }
            
            // If no previous, wrap to last
            if (!previous && !components_.empty()) {
                return components_.rbegin()->second;
            }
            
            return previous;
        }
        
    private:
        std::map<int, juce::Component*> components_;
    };
    
    // ========================================================================
    // COLOR-BLIND SIMULATION
    // ========================================================================
    
    /**
     * Simulate color-blind vision.
     * Used for testing color-blind modes.
     */
    static SkColor simulateDeuteranopia(SkColor color) {
        // Red-green color blindness simulation
        float r = SkColorGetR(color) / 255.0f;
        float g = SkColorGetG(color) / 255.0f;
        float b = SkColorGetB(color) / 255.0f;
        
        // Deuteranopia transformation matrix
        float newR = 0.625f * r + 0.375f * g;
        float newG = 0.7f * r + 0.3f * g;
        float newB = b;
        
        return SkColorSetRGB(
            static_cast<uint8_t>(newR * 255),
            static_cast<uint8_t>(newG * 255),
            static_cast<uint8_t>(newB * 255)
        );
    }
    
    static SkColor simulateProtanopia(SkColor color) {
        // Red color blindness simulation
        float r = SkColorGetR(color) / 255.0f;
        float g = SkColorGetG(color) / 255.0f;
        float b = SkColorGetB(color) / 255.0f;
        
        // Protanopia transformation matrix
        float newR = 0.567f * r + 0.433f * g;
        float newG = 0.558f * r + 0.442f * g;
        float newB = b;
        
        return SkColorSetRGB(
            static_cast<uint8_t>(newR * 255),
            static_cast<uint8_t>(newG * 255),
            static_cast<uint8_t>(newB * 255)
        );
    }
    
    static SkColor simulateTritanopia(SkColor color) {
        // Blue-yellow color blindness simulation
        float r = SkColorGetR(color) / 255.0f;
        float g = SkColorGetG(color) / 255.0f;
        float b = SkColorGetB(color) / 255.0f;
        
        // Tritanopia transformation matrix
        float newR = r;
        float newG = 0.95f * g + 0.05f * b;
        float newB = 0.433f * g + 0.567f * b;
        
        return SkColorSetRGB(
            static_cast<uint8_t>(newR * 255),
            static_cast<uint8_t>(newG * 255),
            static_cast<uint8_t>(newB * 255)
        );
    }
};

} // namespace zenith
