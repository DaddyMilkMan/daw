/*
  ==============================================================================
    agents/ClockSyncAgent/protocols/SyncProtocol.h
    Interface for clock synchronization protocols.
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <chrono>
#include <functional>

namespace zenith {
namespace agents {
namespace protocols {

/**
    Abstract base class for synchronization protocols (NTP, PTP, etc.).
*/
class SyncProtocol {
public:
    virtual ~SyncProtocol() = default;

    /** Starts the synchronization process. */
    virtual void start() = 0;

    /** Stops the synchronization process. */
    virtual void stop() = 0;

    /** Returns the current estimated clock offset in nanoseconds. */
    virtual int64_t getOffset() const = 0;

    /** Returns true if the protocol considers itself synchronized. */
    virtual bool isSynchronized() const = 0;

    /** Returns the current estimated drift ratio (1.0 = no drift). */
    virtual double getDrift() const = 0;

    // Callbacks for updates
    std::function<void(int64_t)> onOffsetChanged;
    std::function<void(bool)> onSyncStateChanged;
};

} // namespace protocols
} // namespace agents
} // namespace zenith
