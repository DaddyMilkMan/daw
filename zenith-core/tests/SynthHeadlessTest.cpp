#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include "../Source/instruments/ZenithPolySynth.h"

int main()
{
    // 1. Initialize
    juce::ScopedJuceInitialiser_GUI juceInit; 

    std::cout << "Initializing ZenithPolySynth..." << std::endl;
    zenith::ZenithPolySynthProcessor synth;

    double sampleRate = 44100.0;
    int samplesPerBlock = 512;
    int numChannels = 2;

    synth.prepareToPlay(sampleRate, samplesPerBlock);

    // 2. Create buffers
    juce::AudioBuffer<float> buffer(numChannels, samplesPerBlock);
    juce::MidiBuffer midiBuffer;

    // 3. Send Note On
    int noteNumber = 60; // C4
    float velocity = 0.8f;
    midiBuffer.addEvent(juce::MidiMessage::noteOn(1, noteNumber, velocity), 0);

    std::cout << "Sending Note On (60) at velocity " << velocity << std::endl;

    // 4. Render loop
    int totalSamples = 44100 * 2; // 2 seconds
    int numBlocks = totalSamples / samplesPerBlock;
    float maxPeak = 0.0f;
    float rmsSum = 0.0f;

    // Output file
    juce::File outputFile = juce::File::getCurrentWorkingDirectory().getChildFile("synth_test_output.wav");
    if (outputFile.exists()) outputFile.deleteFile();

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
        new juce::FileOutputStream(outputFile),
        sampleRate,
        numChannels,
        24,
        {},
        0));

    if (writer)
    {
        std::cout << "Writing output to: " << outputFile.getFullPathName() << std::endl;
    }

    for (int i = 0; i < numBlocks; ++i)
    {
        buffer.clear();
        
        // Only send MIDI in the first block
        if (i > 0) midiBuffer.clear();

        // Note Off after 1 second
        if (i == numBlocks / 2)
        {
            midiBuffer.addEvent(juce::MidiMessage::noteOff(1, noteNumber), 0);
            std::cout << "Sending Note Off" << std::endl;
        }

        synth.processBlock(buffer, midiBuffer);

        float blockPeak = buffer.getMagnitude(0, buffer.getNumSamples());
        maxPeak = juce::jmax(maxPeak, blockPeak);
        rmsSum += buffer.getRMSLevel(0, 0, buffer.getNumSamples()); 

        if (writer)
        {
            writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
        }
    }

    std::cout << "Max Peak: " << maxPeak << std::endl;
    std::cout << "Average RMS: " << (rmsSum / numBlocks) << std::endl;

    if (maxPeak > 0.001f)
    {
        std::cout << "SUCCESS: Audio detected." << std::endl;
        return 0;
    }
    else
    {
        std::cout << "FAILURE: Output is silent!" << std::endl;
        return 1;
    }
}

