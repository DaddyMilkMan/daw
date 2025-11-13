/**
 * @file ProjectIDs.h
 * @brief ValueTree identifiers for project persistence
 *
 * JUCE Identifier constants for serializing ProjectModel to/from ValueTree.
 */

#pragma once

#include <JuceHeader.h>

namespace zenith::ProjectIDs
{
    using juce::Identifier;

    //==========================================================================
    // Types
    //==========================================================================

    static const Identifier project   ("project");
    static const Identifier track     ("track");
    static const Identifier audioClip ("audioClip");
    static const Identifier fxSlot    ("fxSlot");

    //==========================================================================
    // Project properties
    //==========================================================================

    static const Identifier projName        ("name");
    static const Identifier projSampleRate  ("sampleRate");
    static const Identifier projBlockSize   ("blockSize");
    static const Identifier projLengthHint  ("lengthHint");
    static const Identifier projVersion     ("version");
    static const Identifier projNextClipId  ("nextClipId");

    //==========================================================================
    // Track properties
    //==========================================================================

    static const Identifier trackId    ("id");
    static const Identifier trackName  ("name");
    static const Identifier trackGain  ("gain");
    static const Identifier trackPan   ("pan");
    static const Identifier trackMuted ("muted");
    static const Identifier trackSolo  ("solo");

    //==========================================================================
    // Clip properties
    //==========================================================================

    static const Identifier clipId          ("id");
    static const Identifier clipFilePath    ("filePath");
    static const Identifier clipStartSample ("startSample");
    static const Identifier clipLength      ("lengthSamples");
    static const Identifier clipSrcOffset   ("srcOffset");
    static const Identifier clipGain        ("gain");
    static const Identifier clipFadeIn      ("fadeInSamples");
    static const Identifier clipFadeOut     ("fadeOutSamples");
    static const Identifier clipMuted       ("muted");
    static const Identifier clipLoopEnabled ("loopEnabled");
    static const Identifier clipLoopLength  ("loopLength");

    //==========================================================================
    // FX slot properties (v0.1: not persisted, reserved for v0.2+)
    //==========================================================================

    static const Identifier fxSlotIndex ("slotIndex");
    static const Identifier fxType      ("type");       // "GainPan", "VST3"
    static const Identifier fxPluginId  ("pluginId");
    static const Identifier fxBypassed  ("bypassed");
    static const Identifier fxStateXml  ("stateXml");

} // namespace zenith::ProjectIDs
