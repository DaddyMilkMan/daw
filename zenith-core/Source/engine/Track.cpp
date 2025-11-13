/**
 * @file Track.cpp
 * @brief Track implementation
 */

#include "Track.h"
#include "../../include/Engine.h"
#include <algorithm>

namespace zenith {

Track::Track()
{
}

Track::~Track()
{
}

//==============================================================================
// Clip Definition Management (MESSAGE THREAD)
//==============================================================================

void Track::addClipDefinition(const ClipDef& def)
{
    // TODO: Add jassert(isMessageThread()) when JUCE macros available
    // TODO: Add jassert(!engine.isPlaying()) when engine reference available
    clipDefs_.push_back(def);
}

void Track::clearClipDefinitions()
{
    clipDefs_.clear();
}

const ClipDef* Track::findClipDef(int64_t clipId) const noexcept
{
    for (const auto& def : clipDefs_)
    {
        if (def.id == clipId)
            return &def;
    }
    return nullptr;
}

//==============================================================================
// Transport Event Handling (AUDIO THREAD)
//==============================================================================

void Track::handleEventRT(const TransportEvent& ev,
                          int offsetInBlock,
                          int64_t timelineSample)
{
    juce::ignoreUnused(offsetInBlock, timelineSample);

    switch (ev.type)
    {
        case Engine::TransportEventType::ClipStart:
            startClipRT(ev.clipId);
            break;

        case Engine::TransportEventType::ClipStop:
            stopClipRT(ev.clipId);
            break;
    }
}

//==============================================================================
// Voice Management (AUDIO THREAD)
//==============================================================================

void Track::startClipRT(int64_t clipId)
{
#if ZENITH_ENABLE_PHASE1_AUDIO
    const ClipDef* def = findClipDef(clipId);
    if (!def || !def->clip.isValid())
        return;

    auto* v = allocVoiceRT(def);
    if (!v)
        return; // All voices busy, drop (could log to non-RT queue)

    // Voice is already initialized by allocVoiceRT
#endif
}

void Track::stopClipRT(int64_t clipId)
{
#if ZENITH_ENABLE_PHASE1_AUDIO
    for (auto& v : voices_)
    {
        if (v.def && v.def->id == clipId && !v.fadingOut)
        {
            v.fadingOut = true;
            v.fadeOutDone = 0; // Start fade from current position
        }
    }
#endif
}

ActiveVoice* Track::allocVoiceRT(const ClipDef* def)
{
#if ZENITH_ENABLE_PHASE1_AUDIO
    // 1) Find free voice
    for (auto& v : voices_)
    {
        if (!v.isActive())
            return initVoice(v, def);
    }

    // 2) Steal oldest (simple: reuse first voice)
    // TODO: Better stealing heuristic (find voice closest to end)
    return initVoice(voices_[0], def);
#else
    juce::ignoreUnused(def);
    return nullptr;
#endif
}

ActiveVoice* Track::initVoice(ActiveVoice& v, const ClipDef* def)
{
    v.def = def;
    v.clipPos = 0;
    v.fadeInDone = 0;
    v.fadeOutDone = 0;
    v.fadingOut = false;
    v.oneShot = true;
    return &v;
}

//==============================================================================
// Audio Processing (AUDIO THREAD)
//==============================================================================

void Track::processSegment(juce::AudioBuffer<float>& mixBuffer,
                           int segmentOffset,
                           int segmentLength,
                           int64_t timelineSample)
{
    juce::ignoreUnused(timelineSample);

#if ZENITH_ENABLE_PHASE1_AUDIO
    const int outChans = mixBuffer.getNumChannels();

    for (auto& v : voices_)
    {
        if (!v.isActive())
            continue;

        const auto& clip = v.def->clip;
        auto* pcm = clip.pcm.get();
        if (!pcm)
            continue;

        const int64_t clipLength = clip.getPlayableLengthSamples();
        if (v.isFinished(clipLength))
        {
            v.def = nullptr; // Deactivate voice
            continue;
        }

        const int clipChans = pcm->getNumChannels();
        const int samplesAvailable = static_cast<int>(clipLength - v.clipPos);
        const int samplesToRead = juce::jmin(segmentLength, samplesAvailable);

        if (samplesToRead <= 0)
        {
            v.def = nullptr; // Clip ended
            continue;
        }

        // Render each channel with fades
        for (int ch = 0; ch < outChans; ++ch)
        {
            const int srcCh = juce::jmin(ch, clipChans - 1);
            const int srcSample = static_cast<int>(clip.srcOffset + v.clipPos);

            if (srcSample + samplesToRead > pcm->getNumSamples())
                continue; // Safety check

            const float* src = pcm->getReadPointer(srcCh, srcSample);
            float* dst = mixBuffer.getWritePointer(ch, segmentOffset);

            // Apply fades and mix
            for (int i = 0; i < samplesToRead; ++i)
            {
                float gain = clip.gain;

                // Fade-in
                if (v.fadeInDone < clip.fadeInSamples)
                {
                    const float fadeInGain = static_cast<float>(v.fadeInDone) / clip.fadeInSamples;
                    gain *= fadeInGain;
                    if (ch == 0) // Only increment once per sample
                        v.fadeInDone++;
                }

                // Fade-out
                if (v.fadingOut && v.fadeOutDone < clip.fadeOutSamples)
                {
                    const float fadeOutGain = 1.0f - (static_cast<float>(v.fadeOutDone) / clip.fadeOutSamples);
                    gain *= fadeOutGain;
                    if (ch == 0) // Only increment once per sample
                        v.fadeOutDone++;
                }

                dst[i] += src[i] * gain; // Additive mix
            }
        }

        v.clipPos += samplesToRead;

        // Check if fade-out complete
        if (v.fadingOut && v.fadeOutDone >= clip.fadeOutSamples)
        {
            v.def = nullptr; // Deactivate voice
        }
    }
#else
    juce::ignoreUnused(mixBuffer, segmentOffset, segmentLength);
#endif
}

} // namespace zenith
