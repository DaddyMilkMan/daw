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

#include "SampleGenerator.h"
#include "instruments/ContentPaths.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace zenith {

// Internal Job Class
class SampleGenerationJob : public juce::ThreadPoolJob {
public:
    SampleGenerationJob(std::function<void()> onComplete)

        : juce::ThreadPoolJob("SampleGeneration"), onCompleteCallback(onComplete) {}

    JobStatus runJob() override {
        // Use stack-allocated random generator for thread safety
        juce::Random random;
        random.setSeedRandomly();

        auto contentRoot = ContentPaths::getInstance().getContentRoot();
        auto samplesDir = contentRoot.getChildFile("Examples")
                                .getChildFile("SampleMaps")
                                .getChildFile("Samples");

        if (!samplesDir.exists())
            samplesDir.createDirectory();

        juce::StringArray requiredFiles = {
            // 808 Essentials
            "808-kick.wav", "808-snare.wav", "808-hihat-closed.wav",
            "808-hihat-open.wav", "808-clap.wav", "808-tom-low.wav",
            "808-tom-mid.wav", "808-tom-high.wav", "808-bass.wav",

            // LoFi Keys
            "lofi-piano-C3.wav", "lofi-piano-C4.wav", "lofi-piano-C5.wav",
            "lofi-piano-C3-soft.wav", "lofi-piano-C4-soft.wav", "lofi-piano-C5-soft.wav",
            "lofi-piano-C3-hard.wav", "lofi-piano-C4-hard.wav", "lofi-piano-C5-hard.wav",

            // Trap Pluck
            "trap-pluck-C2.wav", "trap-pluck-C3.wav", "trap-pluck-C4.wav",
            "trap-pluck-C5.wav", "trap-pluck-C6.wav",

            // Orchestral Strings
            "strings-C2.wav", "strings-C2-loud.wav", "strings-E3.wav",
            "strings-E3-loud.wav", "strings-A4.wav", "strings-A4-loud.wav",
            "strings-C6.wav", "strings-C3.wav", "strings-C4.wav",
            "strings-C5.wav",

            // FX & Impacts
            "fx-riser-short.wav", "fx-riser-long.wav", "fx-impact-soft.wav",
            "fx-impact-hard.wav", "fx-reverse-cymbal.wav", "fx-whoosh-left.wav",
            "fx-whoosh-right.wav", "fx-sub-drop.wav",
            // Built-in variants
            "fx-riser.wav", "fx-impact.wav", "fx-reverse.wav", "fx-whoosh.wav"};

        juce::Array<juce::File> targetDirs;
        targetDirs.add(samplesDir);

        auto instrumentsDir = contentRoot.getChildFile("Instruments")
                                    .getChildFile("ZenithSampler")
                                    .getChildFile("Samples");
        if (!instrumentsDir.exists())
            instrumentsDir.createDirectory();
        targetDirs.add(instrumentsDir);

        for (const auto &dir : targetDirs) {
            // Check cancellation between directories
            if (shouldExit()) return jobHasFinished;

            for (const auto &filename : requiredFiles) {
                // Check cancellation between files
                if (shouldExit()) return jobHasFinished;

                auto file = dir.getChildFile(filename);
                if (!file.existsAsFile()) {
                    DBG("Generating missing sample: " << filename);

                    float freq = 440.0f;
                    float duration = 0.5f;
                    bool isNoise = false;

                    if (filename.contains("kick")) {
                        freq = 60.0f;
                        duration = 0.3f;
                    } else if (filename.contains("snare")) {
                        freq = 200.0f;
                        isNoise = true;
                        duration = 0.2f;
                    } else if (filename.contains("hihat")) {
                        freq = 8000.0f;
                        isNoise = true;
                        duration = 0.1f;
                    } else if (filename.contains("bass")) {
                        freq = 100.0f;
                        duration = 1.0f;
                    } else if (filename.contains("piano")) {
                        freq = 261.63f; // C4
                        if (filename.contains("C3")) freq = 130.81f;
                        if (filename.contains("C5")) freq = 523.25f;
                        duration = 1.0f;
                    } else if (filename.contains("riser") || filename.contains("long")) {
                        freq = 300.0f;
                        duration = 2.0f;
                    }

                    createWavFile(file, freq, duration, random, isNoise);
                }
            }
        }

        if (onCompleteCallback && !shouldExit()) onCompleteCallback();
        return jobHasFinished;
    }

private:
    std::function<void()> onCompleteCallback;

    void createWavFile(const juce::File &file, float freq,
                       float durationSecs, juce::Random& random, bool isNoise) {
        juce::WavAudioFormat wavFormat;

        // Write to a temp file first for atomicity
        auto tempFile = file.getParentDirectory().getChildFile(file.getFileName() + ".tmp");

        // Ensure clean start
        if (tempFile.exists()) tempFile.deleteFile();

        std::unique_ptr<juce::OutputStream> outStream(tempFile.createOutputStream());
        if (!outStream)
            return;

        auto options = juce::AudioFormatWriterOptions()
                         .withSampleRate(44100.0)
                         .withNumChannels(1)
                         .withBitsPerSample(16);

        std::unique_ptr<juce::AudioFormatWriter> writer(
          wavFormat.createWriterFor(outStream, options));

        if (writer) {
            outStream.release(); // Writer takes ownership

            int numSamples = (int)(44100.0 * durationSecs);
            juce::AudioBuffer<float> buffer(1, numSamples);

            auto *ch = buffer.getWritePointer(0);
            double phase = 0.0;
            double phaseInc = freq * juce::MathConstants<double>::twoPi / 44100.0;

            for (int i = 0; i < numSamples; ++i) {
                float env = 1.0f - ((float)i / numSamples);
                env = env * env;

                float sample = 0.0f;
                if (isNoise || freq > 2000.0f) {
                    sample = (random.nextFloat() * 2.0f - 1.0f) * env;
                } else {
                    sample = (float)std::sin(phase) * env;
                }

                ch[i] = sample;
                phase += phaseInc;
            }

            writer->writeFromAudioSampleBuffer(buffer, 0, numSamples);
            writer.reset(); // Flush writer

            // Atomic move
            tempFile.moveFileTo(file);
        } else {
            tempFile.deleteFile();
        }
    }
};

void SampleGenerator::generateMissingSamples(juce::ThreadPool* threadPool, std::function<void()> onComplete) {
    if (threadPool) {
        threadPool->addJob(new SampleGenerationJob(onComplete), true); // true = deleteJobWhenFinished
    } else {
        SampleGenerationJob job(onComplete);
        job.runJob();
    }
}

} // namespace zenith
