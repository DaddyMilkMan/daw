/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    SPDX-License-Identifier: Apache-2.0 
*/

#pragma once

// PerformanceMonitor.h

#include "../../design-system/ZenithTheme.h"
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace zenith::ui {

/**
 * @brief Performance metrics structure
 */
struct PerformanceHistoryEntry {
    std::chrono::steady_clock::time_point timestamp;
    PerformanceMetrics metrics;

    PerformanceHistoryEntry() : timestamp(std::chrono::steady_clock::now()) {}
};

/**
 * @brief Performance quality levels
 */
enum class QualityLevel {
    Low = 0,      // 25% quality, minimal features
    Medium = 1,   // 50% quality, balanced features
    High = 2,     // 75% quality, most features
    Ultra = 3,    // 100% quality, all features
    Auto = 4      // Adaptive quality based on performance
};

/**
 * @brief Quality configuration
 */

} // namespace
