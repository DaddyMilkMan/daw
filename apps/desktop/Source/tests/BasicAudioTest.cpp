#include <JuceHeader.h>
#include "../../engine/Engine.h"
#include "../../engine/ProjectState.h"
#include "../../engine/Track.h"

class BasicAudioTest : public juce::UnitTest
{
public:
    BasicAudioTest() : juce::UnitTest("Basic Audio Processing") {}
    
    void runTest() override
    {
        beginTest("Track processes audio without NaN/Inf");
        
        zenith::Engine engine;
        zenith::ProjectState state;
        engine.setProjectState(&state);
        
        // Prepare engine
        engine.prepareToPlay(512, 48000.0);
        
        // Create a track via ProjectState (which syncs to Engine)
        state.addTrack("Test Track", "audio");
        
        // Wait for sync (since it might be async in real app, but here we expect sync execution or we force it)
        engine.syncWithProjectState();
        
        expect(engine.getNumTracks() >= 1, "Engine should have at least 1 track");
        
        // Process one block
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        
        juce::AudioSourceChannelInfo info;
        info.buffer = &buffer;
        info.startSample = 0;
        info.numSamples = 512;
        
        // In a real test, we'd call engine.processBlock, but Engine::processAudioBlock is private/internal
        // We can use the exposed AudioIODeviceCallback interface if we mock the device, 
        // OR use the renderOfflineBlock method I just added!
        
        engine.renderOfflineBlock(buffer, 512, 0);
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* samples = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                expect(!std::isnan(samples[i]), "Output contains NaN");
                expect(!std::isinf(samples[i]), "Output contains Inf");
            }
        }
        
        engine.releaseResources();
    }
};

static BasicAudioTest basicAudioTest;
