/**
 * @file WinRtAudioPriority.cpp
 * @brief Implementation of MMCSS audio priority helper
 */

#ifdef _WIN32

#include "WinRtAudioPriority.h"
#include <avrt.h>

// Static member initialization
MMCSSAudioPriority::AvSetMmThreadCharacteristicsWFunc
    MMCSSAudioPriority::avSetMmThreadCharacteristicsW = nullptr;

MMCSSAudioPriority::AvSetMmThreadPriorityFunc
    MMCSSAudioPriority::avSetMmThreadPriority = nullptr;

MMCSSAudioPriority::AvRevertMmThreadCharacteristicsFunc
    MMCSSAudioPriority::avRevertMmThreadCharacteristics = nullptr;

bool MMCSSAudioPriority::mmcssAvailable = false;

//==============================================================================
void MMCSSAudioPriority::loadMMCSSFunctions()
{
    static bool initialized = false;
    if (initialized)
        return;

    initialized = true;

    // Dynamically load avrt.dll (available on Windows Vista+)
    HMODULE avrtDll = LoadLibraryW(L"avrt.dll");
    if (avrtDll == nullptr)
    {
        DBG("MMCSS: avrt.dll not found (Windows Vista+ required)");
        return;
    }

    // Load function pointers
    avSetMmThreadCharacteristicsW = reinterpret_cast<AvSetMmThreadCharacteristicsWFunc>(
        GetProcAddress(avrtDll, "AvSetMmThreadCharacteristicsW"));

    avSetMmThreadPriority = reinterpret_cast<AvSetMmThreadPriorityFunc>(
        GetProcAddress(avrtDll, "AvSetMmThreadPriority"));

    avRevertMmThreadCharacteristics = reinterpret_cast<AvRevertMmThreadCharacteristicsFunc>(
        GetProcAddress(avrtDll, "AvRevertMmThreadCharacteristics"));

    if (avSetMmThreadCharacteristicsW && avRevertMmThreadCharacteristics)
    {
        mmcssAvailable = true;
        DBG("MMCSS: avrt.dll loaded successfully");
    }
    else
    {
        DBG("MMCSS: Failed to load functions from avrt.dll");
        // Don't unload DLL - it's loaded for the lifetime of the process
    }
}

//==============================================================================
MMCSSAudioPriority::MMCSSAudioPriority(const wchar_t* taskName, DWORD priority)
{
    // Load MMCSS functions on first use
    loadMMCSSFunctions();

    if (!mmcssAvailable)
        return;

#ifndef ZENITH_ENABLE_MMCSS
    // MMCSS disabled at compile time
    DBG("MMCSS: Disabled via ZENITH_ENABLE_MMCSS=0");
    return;
#endif

    // Register this thread with MMCSS
    mmcssHandle = avSetMmThreadCharacteristicsW(taskName, &taskIndex);

    if (mmcssHandle == nullptr)
    {
        DWORD error = GetLastError();
        DBG("MMCSS: AvSetMmThreadCharacteristicsW failed with error: " + juce::String((int)error));
        return;
    }

    // Set priority within the MMCSS task (0 = highest)
    if (avSetMmThreadPriority && priority > 0)
    {
        AVRT_PRIORITY avrtPriority = static_cast<AVRT_PRIORITY>(AVRT_PRIORITY_CRITICAL - priority);
        if (!avSetMmThreadPriority(mmcssHandle, avrtPriority))
        {
            DBG("MMCSS: AvSetMmThreadPriority failed");
        }
    }

    DBG("MMCSS: Thread registered with task '" + juce::String(taskName) +
        "', index: " + juce::String((int)taskIndex));
}

//==============================================================================
MMCSSAudioPriority::~MMCSSAudioPriority()
{
    reset();
}

//==============================================================================
void MMCSSAudioPriority::reset()
{
    if (mmcssHandle != nullptr && avRevertMmThreadCharacteristics)
    {
        if (avRevertMmThreadCharacteristics(mmcssHandle))
        {
            DBG("MMCSS: Thread reverted to normal priority");
        }
        else
        {
            DBG("MMCSS: AvRevertMmThreadCharacteristics failed");
        }

        mmcssHandle = nullptr;
        taskIndex = 0;
    }
}

#endif // _WIN32
