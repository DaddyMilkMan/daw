/*
  ==============================================================================

    JuceHeader.h
    Custom header to include all necessary JUCE modules.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_opengl/juce_opengl.h>
#include <juce_cryptography/juce_cryptography.h>

#if defined(JUCE_PROJUCER_VERSION) && JUCE_PROJUCER_VERSION < JUCE_VERSION
 /** If you've hit this error, then you've set the JUCE_PROJUCER_VERSION macro in your
     preprocessor definitions to a version of the Projucer that's older than the version
     of the JUCE modules that you're using. This is likely to happen if you're using
     an old version of the Projucer to save a project that uses a newer version of JUCE.
 */
 #error "This project is being built with a version of the Projucer that is older than the JUCE modules it imports"
#endif

#if ! DONT_SET_USING_JUCE_NAMESPACE
 // If your code uses a lot of JUCE classes, then this will obviously save you
 // a lot of typing, but can be disabled by setting DONT_SET_USING_JUCE_NAMESPACE.
 using namespace juce;
#endif

#if ! JUCE_DONT_DECLARE_PROJECTINFO
namespace ProjectInfo
{
    const char* const  projectName    = "Zenith DAW";
    const char* const  companyName    = "Zenith Audio";
    const char* const  versionString  = "0.1.0";
    const int          versionNumber  =  0x100;
}
#endif
