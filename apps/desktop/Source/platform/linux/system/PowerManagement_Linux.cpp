/*
  ==============================================================================

    PowerManagement_Linux.cpp
    Created: 2026-01-13
    Author:  Zenith DAW

  ==============================================================================
*/

#include "PowerManagement.h"
#include "ZenithLogger.h"

#ifdef JUCE_LINUX

namespace zenith {

class PowerManagement::Pimpl {
public:
    Pimpl() = default;
    
    ~Pimpl() {
        stopInhibition();
    }

    void setSleepDisabled(bool disabled) {
        if (disabled) {
            startInhibition();
        } else {
            stopInhibition();
        }
    }

private:
    void startInhibition() {
        if (inhibitProcess != nullptr && inhibitProcess->isRunning()) {
            return;
        }

        inhibitProcess = std::make_unique<juce::ChildProcess>();
        
        // systemd-inhibit --what=sleep --who=Zenith --why="Active Project" --mode=block sleep infinity
        // Note: we use 'sleep 315360000' (10 years) instead of 'infinity' for better compatibility with some versions of sleep
        juce::StringArray args;
        args.add("systemd-inhibit");
        args.add("--what=sleep");
        args.add("--who=Zenith");
        args.add("--why=Project work in progress");
        args.add("--mode=block");
        args.add("sleep");
        args.add("315360000");

        if (!inhibitProcess->start(args)) {
            juce::Logger::writeToLog("PowerManagement: Failed to start systemd-inhibit");
            inhibitProcess.reset();
        } else {
            juce::Logger::writeToLog("PowerManagement: Started sleep inhibition via systemd-inhibit");
        }
    }

    void stopInhibition() {
        if (inhibitProcess != nullptr) {
            inhibitProcess->kill();
            inhibitProcess.reset();
            juce::Logger::writeToLog("PowerManagement: Stopped sleep inhibition");
        }
    }

    std::unique_ptr<juce::ChildProcess> inhibitProcess;
};

PowerManagement::PowerManagement() : pimpl(std::make_unique<Pimpl>()) {}
PowerManagement::~PowerManagement() = default;

void PowerManagement::setSleepDisabled(bool disabled) {
    if (sleepDisabled == disabled)
        return;

    sleepDisabled = disabled;
    pimpl->setSleepDisabled(disabled);
}

} // namespace zenith

#endif // JUCE_LINUX
