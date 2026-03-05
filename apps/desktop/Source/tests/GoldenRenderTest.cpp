/*
  ==============================================================================

    GoldenRenderTest.cpp
    Phase 3 – A+ Audio Production Quality

    Golden render test harness: verifies that the engine produces bit-identical
    output across runs and platforms by comparing a rolling FNV-1a hash of all
    rendered audio samples against a stored golden reference value.

    Golden reference values are stored in:
        tests/golden/canonical_session_golden.json

    To regenerate the golden file (e.g. after an intentional engine change):
        python3 scripts/run_golden_tests.py --regenerate

  ==============================================================================
*/

#include "../engine/Clip.h"
#include "../engine/Engine.h"
#include "../engine/Track.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <string>

namespace zenith {
namespace tests {

//==============================================================================
/** FNV-1a 64-bit hash accumulator – deterministic and endian-stable. */
static juce::uint64 fnv1a64(juce::uint64 hash, const float* samples, int count)
{
    constexpr juce::uint64 kFNVPrime  = 0x00000100000001B3ULL;
    constexpr juce::uint64 kFNVOffset = 0xcbf29ce484222325ULL;

    if (hash == 0)
        hash = kFNVOffset;

    // Treat each float as 4 raw bytes (little-endian order for portability)
    for (int i = 0; i < count; ++i)
    {
        union { float f; juce::uint8 b[4]; } u;
        u.f = samples[i];
        for (int b = 0; b < 4; ++b)
        {
            hash ^= static_cast<juce::uint64>(u.b[b]);
            hash *= kFNVPrime;
        }
    }
    return hash;
}

//==============================================================================
/**
 * @class GoldenRenderTest
 * @brief Verifies deterministic, cross-platform engine output against golden
 *        reference hashes stored in tests/golden/canonical_session_golden.json.
 *
 * Test cases
 * ----------
 * 1. Intra-run determinism   – two runs of identical input must produce the
 *                              same hash within a single process invocation.
 * 2. Golden file comparison  – the computed hash must match the value stored
 *                              in the golden JSON (when the file is present).
 * 3. NaN / Inf guard         – rendered samples must be finite (no NaNs).
 */
class GoldenRenderTest : public juce::UnitTest
{
public:
    GoldenRenderTest() : juce::UnitTest("Golden Render Test", "AudioEngine") {}

    void runTest() override
    {
        testIntraRunDeterminism();
        testNaNGuard();
        testGoldenFileComparison();
    }

private:
    // -------------------------------------------------------------------------
    // Parameters for the canonical test session
    // -------------------------------------------------------------------------
    static constexpr double kSampleRate  = 44100.0;
    static constexpr int    kBlockSize   = 512;
    /// Render ~1.2 seconds of audio (100 blocks × 512 samples / 44100 ≈ 1.16 s)
    static constexpr int    kNumBlocks   = 100;
    static constexpr int    kNumChannels = 2;
    /// Deterministic PRNG seed for input noise
    static constexpr int    kRNGSeed     = 0xDEADBEEF;

    // -------------------------------------------------------------------------
    // Path helpers
    // -------------------------------------------------------------------------
    static juce::File getGoldenDir()
    {
        // Walk up from the binary location to locate tests/golden
        // In CI the binary lives in build/; we go up to the repo root.
        juce::File exe = juce::File::getSpecialLocation(
            juce::File::SpecialLocationType::currentExecutableFile);

        // Try up to 6 levels
        juce::File dir = exe.getParentDirectory();
        for (int i = 0; i < 6; ++i)
        {
            juce::File candidate = dir.getChildFile("tests/golden");
            if (candidate.isDirectory())
                return candidate;
            dir = dir.getParentDirectory();
        }
        // Fallback: create alongside the binary
        return exe.getParentDirectory().getChildFile("tests/golden");
    }

    static juce::File getGoldenFile()
    {
        return getGoldenDir().getChildFile("canonical_session_golden.json");
    }

    // -------------------------------------------------------------------------
    // Core rendering helper
    // -------------------------------------------------------------------------
    /**
     * Initialises the engine, drives the canonical deterministic scenario, and
     * returns a 64-bit FNV-1a hash over every rendered sample.
     *
     * The scenario:
     *   • One stereo audio track with input-monitor enabled.
     *   • Deterministic pseudo-random input (fixed seed).
     *   • 100 blocks at 44100 Hz / 512 samples.
     */
    juce::uint64 renderCanonicalSession() const
    {
        zenith::Engine engine;
        engine.initialize();

        // Create a single audio track with input monitoring so the input
        // signal is passed through to the output – giving us non-trivial,
        // deterministic output to hash.
        engine.createTrack("Golden Track", "audio");
        if (!engine.tracks().empty())
            engine.tracks()[0]->setInputMonitor(true);

        juce::AudioBuffer<float> inputBuf(kNumChannels, kBlockSize);
        juce::AudioBuffer<float> outputBuf(kNumChannels, kBlockSize);
        juce::Random rng(kRNGSeed);

        juce::uint64 hash = 0;

        for (int block = 0; block < kNumBlocks; ++block)
        {
            // Fill input with deterministic pseudo-random noise
            for (int ch = 0; ch < kNumChannels; ++ch)
            {
                float* ptr = inputBuf.getWritePointer(ch);
                for (int s = 0; s < kBlockSize; ++s)
                    ptr[s] = rng.nextFloat() * 2.0f - 1.0f;
            }
            outputBuf.clear();

            const float* inputPtrs[]  = { inputBuf.getReadPointer(0),
                                          inputBuf.getReadPointer(1) };
            float*       outputPtrs[] = { outputBuf.getWritePointer(0),
                                          outputBuf.getWritePointer(1) };

            juce::AudioIODeviceCallbackContext ctx{};
            engine.audioDeviceIOCallbackWithContext(
                inputPtrs,  kNumChannels,
                outputPtrs, kNumChannels,
                kBlockSize, ctx);

            // Accumulate hash over all channels
            for (int ch = 0; ch < kNumChannels; ++ch)
                hash = fnv1a64(hash, outputBuf.getReadPointer(ch), kBlockSize);
        }

        engine.shutdown();
        return hash;
    }

    // -------------------------------------------------------------------------
    // Test cases
    // -------------------------------------------------------------------------
    void testIntraRunDeterminism()
    {
        beginTest("Intra-run determinism (two identical renders must match)");

        const juce::uint64 hash1 = renderCanonicalSession();
        const juce::uint64 hash2 = renderCanonicalSession();

        DBG("  Run 1 hash: " + juce::String::toHexString((juce::int64)hash1));
        DBG("  Run 2 hash: " + juce::String::toHexString((juce::int64)hash2));

        expect(hash1 != 0,  "Hash must not be zero (engine produced no audio)");
        expectEquals(hash1, hash2);
    }

    void testNaNGuard()
    {
        beginTest("NaN/Inf guard – all rendered samples must be finite");

        zenith::Engine engine;
        engine.initialize();
        engine.createTrack("NaN Guard Track", "audio");
        if (!engine.tracks().empty())
            engine.tracks()[0]->setInputMonitor(true);

        juce::AudioBuffer<float> inputBuf(kNumChannels, kBlockSize);
        juce::AudioBuffer<float> outputBuf(kNumChannels, kBlockSize);
        juce::Random rng(kRNGSeed);

        bool foundNaN = false;
        bool foundInf = false;

        for (int block = 0; block < kNumBlocks; ++block)
        {
            for (int ch = 0; ch < kNumChannels; ++ch)
            {
                float* ptr = inputBuf.getWritePointer(ch);
                for (int s = 0; s < kBlockSize; ++s)
                    ptr[s] = rng.nextFloat() * 2.0f - 1.0f;
            }
            outputBuf.clear();

            const float* inPtrs[]  = { inputBuf.getReadPointer(0),
                                       inputBuf.getReadPointer(1) };
            float*       outPtrs[] = { outputBuf.getWritePointer(0),
                                       outputBuf.getWritePointer(1) };

            juce::AudioIODeviceCallbackContext ctx{};
            engine.audioDeviceIOCallbackWithContext(
                inPtrs, kNumChannels, outPtrs, kNumChannels, kBlockSize, ctx);

            for (int ch = 0; ch < kNumChannels && !foundNaN && !foundInf; ++ch)
            {
                const float* out = outputBuf.getReadPointer(ch);
                for (int s = 0; s < kBlockSize; ++s)
                {
                    if (std::isnan(out[s]))  { foundNaN = true; break; }
                    if (std::isinf(out[s]))  { foundInf = true; break; }
                }
            }

            if (foundNaN || foundInf)
                break;
        }

        engine.shutdown();

        expect(!foundNaN, "Engine produced NaN samples");
        expect(!foundInf, "Engine produced Inf samples");
    }

    void testGoldenFileComparison()
    {
        beginTest("Golden file hash comparison");

        const juce::uint64 computedHash = renderCanonicalSession();
        const juce::File   goldenFile   = getGoldenFile();

        if (!goldenFile.existsAsFile())
        {
            // No golden file present – write it and pass with a warning.
            writeGoldenFile(goldenFile, computedHash);
            logMessage("  ⚠  Golden file not found – created: " +
                       goldenFile.getFullPathName());
            logMessage("  ⚠  Commit this file to lock the golden reference.");
            // First run always passes (bootstrapping).
            expect(true);
            return;
        }

        // Parse stored hash
        juce::var root;
        juce::Result parseResult =
            juce::JSON::parse(goldenFile.loadFileAsString(), root);

        if (parseResult.failed() || !root.isObject())
        {
            logMessage("  ⚠  Golden file could not be parsed: " +
                       parseResult.getErrorMessage());
            expect(false, "Golden file parse failed");
            return;
        }

        juce::String storedStr =
            root.getProperty("canonical_hash", juce::var("")).toString();

        if (storedStr.isEmpty())
        {
            logMessage("  ⚠  Golden file missing 'canonical_hash' field");
            expect(false, "Golden file missing canonical_hash");
            return;
        }

        // The hash is stored as a hex string, e.g. "0xABCDEF0123456789"
        juce::uint64 storedHash = static_cast<juce::uint64>(
            storedStr.getHexValue64());

        DBG("  Stored  hash: " + storedStr);
        DBG("  Computed hash: " + juce::String::toHexString(
                                      static_cast<juce::int64>(computedHash)));

        expect(computedHash != 0, "Computed hash must not be zero");
        expectEquals(computedHash, storedHash);
    }

    // -------------------------------------------------------------------------
    // Golden file I/O
    // -------------------------------------------------------------------------
    void writeGoldenFile(const juce::File& file, juce::uint64 hash) const
    {
        file.getParentDirectory().createDirectory();

        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("description",
            "Canonical golden render hash for Zenith DAW engine. "
            "Regenerate with: python3 scripts/run_golden_tests.py --regenerate");
        obj->setProperty("sample_rate",   (int)kSampleRate);
        obj->setProperty("block_size",    kBlockSize);
        obj->setProperty("num_blocks",    kNumBlocks);
        obj->setProperty("num_channels",  kNumChannels);
        obj->setProperty("rng_seed",      kRNGSeed);
        obj->setProperty("hash_algo",     "FNV-1a 64-bit");
        obj->setProperty("canonical_hash",
            "0x" + juce::String::toHexString(static_cast<juce::int64>(hash)).toUpperCase());
        obj->setProperty("generated_at",
            juce::Time::getCurrentTime().toISO8601(true));

        file.replaceWithText(juce::JSON::toString(juce::var(obj), false));
    }
};

static GoldenRenderTest goldenRenderTest;

} // namespace tests
} // namespace zenith
