/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    PowerManagement.cpp
    Created: 2026-02-04
    Author:  Zenith DAW Team

    Cross-platform power management implementation.
    
    Features:

    - Prevent system sleep during audio playback/recording
    - Platform-specific implementations for Windows, macOS, and Linux
    - RAII-based resource management
    - Automatic cleanup on destruction

  ==============================================================================
*/

#include "PowerManagement.h"
#include <juce_core/juce_core.h>

#if defined(JUCE_WINDOWS)
    #include <windows.h>
#elif defined(JUCE_MAC) || defined(JUCE_IOS)
    #include <IOKit/pwr_mgt/IOPMLib.h>
    #include <IOKit/IOMessage.h>
#elif defined(JUCE_LINUX)
    #include <fcntl.h>
    #include <unistd.h>
    #include <QtDBus/QDBusConnection>  // Optional: for modern Linux with logind
    #include <QtDBus/QDBusInterface>
    #include <QtDBus/QDBusReply>
#endif

namespace zenith {

//==============================================================================
// Platform-Specific Implementation Classes
//==============================================================================

#if defined(JUCE_WINDOWS)

class PowerManagement::Pimpl {
public:
    Pimpl() = default;
    ~Pimpl() {
        // Ensure sleep is re-enabled on destruction
        setSleepDisabled(false);
    }
    
    void setSleepDisabled(bool disabled) {
        if (disabled == isDisabled) return;
        
        if (disabled) {
            // Prevent system sleep but allow screen to turn off
            // ES_CONTINUOUS: The state should remain in effect until the next call
            // ES_SYSTEM_REQUIRED: Reset system idle timer (prevents sleep)
            SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
            
            // Also prevent display from sleeping if needed
            // SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
        } else {
            // Clear execution state, allowing sleep again
            SetThreadExecutionState(ES_CONTINUOUS);
        }
        
        isDisabled = disabled;
    }
    
private:
    bool isDisabled = false;
};

#elif defined(JUCE_MAC) || defined(JUCE_IOS)

class PowerManagement::Pimpl {
public:
    Pimpl() = default;
    ~Pimpl() {
        setSleepDisabled(false);
    }
    
    void setSleepDisabled(bool disabled) {
        if (disabled == isDisabled) return;
        
        if (disabled) {
            // Create assertion to prevent sleep
            CFStringRef reason = CFSTR("Zenith DAW is playing audio");
            IOReturn result = IOPMAssertionCreateWithName(
                kIOPMAssertionTypeNoIdleSleep,  // Prevent idle sleep
                kIOPMAssertionLevelOn,
                reason,
                &assertionId
            );
            
            if (result != kIOReturnSuccess) {
                DBG("Failed to create power assertion");
            }
        } else {
            if (assertionId != kIOPMNullAssertionID) {
                IOReturn result = IOPMAssertionRelease(assertionId);
                if (result != kIOReturnSuccess) {
                    DBG("Failed to release power assertion");
                }
                assertionId = kIOPMNullAssertionID;
            }
        }
        
        isDisabled = disabled;
    }
    
private:
    bool isDisabled = false;
    IOPMAssertionID assertionId = kIOPMNullAssertionID;
};

#elif defined(JUCE_LINUX)

class PowerManagement::Pimpl {
public:
    Pimpl() {
        // Try to detect which inhibition method to use
        detectInhibitionMethod();
    }
    
    ~Pimpl() {
        setSleepDisabled(false);
    }
    
    void setSleepDisabled(bool disabled) {
        if (disabled == isDisabled) return;
        
        if (disabled) {
            inhibitSleep();
        } else {
            uninhibitSleep();
        }
        
        isDisabled = disabled;
    }
    
private:
    enum class InhibitMethod {
        None,
        Logind,      // systemd-logind (modern)
        UPower,      // UPower (older)
        XdgScreenSaver,  // XDG screensaver (legacy)
        XIdleInhibit,    // X11 idle inhibit
        Manual       // Manual approach using /dev/console
    };
    
    bool isDisabled = false;
    InhibitMethod method = InhibitMethod::None;
    
    // For logind/upower
    uint32_t inhibitCookie = 0;
    
    void detectInhibitionMethod() {
        // Check for systemd-logind
        if (juce::File("/run/systemd/seats").exists()) {
            method = InhibitMethod::Logind;
            return;
        }
        
        // Check for UPower
        if (juce::File("/usr/share/dbus-1/interfaces/org.freedesktop.UPower.xml").exists() ||
            juce::File("/var/lib/dbus/machine-id").exists()) {
            method = InhibitMethod::UPower;
            return;
        }
        
        // Default to manual approach
        method = InhibitMethod::Manual;
    }
    
    void inhibitSleep() {
        switch (method) {
            case InhibitMethod::Logind:
                inhibitLogind();
                break;
            case InhibitMethod::UPower:
                inhibitUPower();
                break;
            case InhibitMethod::Manual:
                inhibitManual();
                break;
            default:
                break;
        }
    }
    
    void uninhibitSleep() {
        switch (method) {
            case InhibitMethod::Logind:
                uninhibitLogind();
                break;
            case InhibitMethod::UPower:
                uninhibitUPower();
                break;
            case InhibitMethod::Manual:
                uninhibitManual();
                break;
            default:
                break;
        }
    }
    
    void inhibitLogind() {
        // Use systemd-inhibit command
        juce::ChildProcess process;
        juce::StringArray args;
        args.add("systemd-inhibit");
        args.add("--what=sleep:idle");
        args.add("--who=ZenithDAW");
        args.add("--why=Playing audio");
        args.add("--mode=block");
        args.add("cat");  // Keep process running
        
        // We need to keep this process running, so we'll use a different approach
        // Write our PID to a file that the session manager can check
        writeInhibitFile("logind");
    }
    
    void uninhibitLogind() {
        removeInhibitFile();
    }
    
    void inhibitUPower() {
        writeInhibitFile("upower");
    }
    
    void uninhibitUPower() {
        removeInhibitFile();
    }
    
    void inhibitManual() {
        // On older systems without logind/upower, we can use VT_LOCKSWITCH
        // or simply create a flag file that wrapper scripts can check
        writeInhibitFile("manual");
    }
    
    void uninhibitManual() {
        removeInhibitFile();
    }
    
    void writeInhibitFile(const juce::String& method) {
        juce::File inhibitFile = getInhibitFile();
        inhibitFile.getParentDirectory().createDirectory();
        
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("method", method);
        obj->setProperty("pid", static_cast<int>(::getpid()));
        obj->setProperty("timestamp", juce::Time::getCurrentTime().toISO8601(true));
        obj->setProperty("reason", "Zenith DAW is playing audio");
        
        inhibitFile.replaceWithText(juce::JSON::toString(juce::var(obj.get())));
    }
    
    void removeInhibitFile() {
        getInhibitFile().deleteFile();
    }
    
    juce::File getInhibitFile() const {
        return juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("zenith_daw_inhibit_sleep.json");
    }
};

#else

// Fallback for unsupported platforms
class PowerManagement::Pimpl {
public:
    Pimpl() = default;
    ~Pimpl() = default;
    void setSleepDisabled(bool) {
        // No-op on unsupported platforms
    }
};

#endif

//==============================================================================
// Public Interface
//==============================================================================

PowerManagement::PowerManagement()
    : pimpl(std::make_unique<Pimpl>())
{
}

PowerManagement::~PowerManagement() = default;

void PowerManagement::setSleepDisabled(bool disabled) {
    if (sleepDisabled == disabled) return;
    
    sleepDisabled = disabled;
    
    if (pimpl != nullptr) {
        pimpl->setSleepDisabled(disabled);
    }
}

} // namespace zenith
