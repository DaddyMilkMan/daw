/**
 * @file ZenithPCH.h
 * @brief Precompiled Header for Zenith DAW
 *
 * This file contains frequently-used standard library headers
 * that are parsed once and shared across all translation units.
 *
 * Build Time Impact:
 * - Without PCH: Each of ~127 .cpp files parses these headers individually
 * - With PCH: Parsed once, reused everywhere
 * - Expected improvement: 15-30% reduction in incremental build times
 *
 * Guidelines:
 * 1. Only add headers that are used by MANY source files
 * 2. Avoid project-specific headers that change frequently
 * 3. Keep this file stable - changes here trigger full rebuilds
 *
 * IMPORTANT: JUCE module headers are NOT included here because JUCE's
 * monolithic module system requires specific compile-time setup that
 * conflicts with standard PCH mechanisms. The JUCE library is already
 * optimized via its own compilation strategy.
 */

#pragma once

// Platform-specific defines and system headers are now handled in Source/platform/

// ==============================================================================
// Standard Library Headers (Stable, rarely change)
// These headers are used throughout the codebase and provide significant
// compilation time savings when precompiled.
// ==============================================================================

// Containers
#include <algorithm>
#include <array>
#include <deque>
#include <forward_list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Smart Pointers and Memory
#include <memory>
#include <memory_resource>

// Utilities
#include <any>
#include <bitset>
#include <functional>
#include <optional>
#include <tuple>
#include <utility>
#include <variant>

// Numerics
#include <cmath>
#include <complex>
#include <limits>
#include <numeric>
#include <random>
#include <ratio>

// Type Support
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <typeindex>
#include <typeinfo>

// Iterators
#include <iterator>

// Time
#include <chrono>
#include <ctime>

// Concurrency
#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <shared_mutex>
#include <thread>

// I/O (lightweight, not heavy iostream)
#include <iosfwd>
#include <sstream>

// Filesystem (C++17)
#include <filesystem>

// Exception handling
#include <exception>
#include <stdexcept>

// Span (C++20)
#include <span>

// ==============================================================================
// C Standard Library Headers
// ==============================================================================
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cfloat>
#include <clocale>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
