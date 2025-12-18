/*
  ==============================================================================
    Example: Using AI Mastering Agent in Your DAW
    Ready-to-use integration code
  ==============================================================================
*/

#include "AIMasteringAgent.h"

namespace zenith {

//==============================================================================
/**
    Example Component that integrates AI Mastering into your DAW
*/
class MasteringPanel : public juce::Component,
                       public juce::Button::Listener
{
public:
    MasteringPanel(Engine& engine)
        : masteringAgent_(engine)
    {
        // Create UI components
        masterButton_.setButtonText("AI Master");
        masterButton_.addListener(this);
        addAndMakeVisible(masterButton_);
        
        bypassButton_.setButtonText("Bypass");
        bypassButton_.setClickingTogglesState(true);
        bypassButton_.addListener(this);
        addAndMakeVisible(bypassButton_);
        
        statusLabel_.setText("Ready", juce::dontSendNotification);
        addAndMakeVisible(statusLabel_);
        
        reasoningLabel_.setText("", juce::dontSendNotification);
        reasoningLabel_.setJustificationType(juce::Justification::topLeft);
        addAndMakeVisible(reasoningLabel_);
        
        // Prepare mastering agent
        masteringAgent_.prepare(44100.0, 512, 2);
    }
    
    void buttonClicked(juce::Button* button) override {
        if (button == &masterButton_) {
            triggerAIMastering();
        }
        else if (button == &bypassButton_) {
            masteringAgent_.setBypass(bypassButton_.getToggleState());
        }
    }
    
    void triggerAIMastering() {
        statusLabel_.setText("Analyzing audio...", juce::dontSendNotification);
        masterButton_.setEnabled(false);
        
        // Get mixdown buffer from your engine
        // You need to implement this based on your Engine class
        juce::AudioBuffer<float> mixBuffer = getMixdownBuffer();
        
        // Launch analysis on background thread
        juce::Thread::launch([this, mixBuffer]() mutable {
            // Configure options
            ai::AIMasteringAgent::Options options;
            options.useAI = true;
            options.targetLoudness = -14.0f;
            options.userIntent = "balanced, professional streaming master";
            options.genre = ""; // Auto-detect
            
            // Call AI mastering
            masteringAgent_.analyzeAndConfigure(mixBuffer, options);
            
            // Update UI on message thread
            juce::MessageManager::callAsync([this]() {
                auto decision = masteringAgent_.getLastDecision();
                
                if (decision.valid) {
                    statusLabel_.setText("✓ AI Mastering Active", juce::dontSendNotification);
                    reasoningLabel_.setText("AI says: " + decision.reasoning, 
                                          juce::dontSendNotification);
                } else {
                    statusLabel_.setText("✗ Mastering Failed", juce::dontSendNotification);
                }
                
                masterButton_.setEnabled(true);
            });
        });
    }
    
    // Call this from your audio callback
    void processAudio(juce::AudioBuffer<float>& buffer) {
        masteringAgent_.processBlock(buffer);
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(10);
        
        masterButton_.setBounds(bounds.removeFromTop(30));
        bounds.removeFromTop(5);
        
        bypassButton_.setBounds(bounds.removeFromTop(30));
        bounds.removeFromTop(5);
        
        statusLabel_.setBounds(bounds.removeFromTop(30));
        bounds.removeFromTop(5);
        
        reasoningLabel_.setBounds(bounds);
    }
    
private:
    ai::AIMasteringAgent masteringAgent_;
    
    juce::TextButton masterButton_;
    juce::TextButton bypassButton_;
    juce::Label statusLabel_;
    juce::Label reasoningLabel_;
    
    // You need to implement this based on your Engine
    juce::AudioBuffer<float> getMixdownBuffer() {
        // Example: render current project to buffer for analysis
        juce::AudioBuffer<float> buffer(2, 44100 * 4);  // 4 seconds
        
        // TODO: Fill buffer with your mix
        // This depends on how your Engine works
        
        return buffer;
    }
};

//==============================================================================
/**
    Example: Integration in Audio Callback
*/
class AudioProcessor : public juce::AudioProcessor
{
public:
    AudioProcessor(Engine& engine)
        : masteringAgent_(engine)
    {
    }
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        masteringAgent_.prepare(sampleRate, samplesPerBlock, 2);
    }
    
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        // Your normal processing...
        
        // Apply AI mastering at the end of the chain
        masteringAgent_.processBlock(buffer);
    }
    
    void releaseResources() override {
        masteringAgent_.reset();
    }
    
    // ... other AudioProcessor methods ...
    
private:
    ai::AIMasteringAgent masteringAgent_;
};

//==============================================================================
/**
    Example: Standalone Master Button
*/
void exampleMasterButtonClicked(Engine& engine) {
    // Create mastering agent
    static ai::AIMasteringAgent masteringAgent(engine);
    masteringAgent.prepare(44100.0, 512, 2);
    
    // Get audio to analyze
    juce::AudioBuffer<float> mixBuffer(2, 44100 * 4);  // 4 seconds
    // Fill buffer with your mix...
    
    // Launch analysis
    juce::Thread::launch([&]() {
        ai::AIMasteringAgent::Options options;
        options.userIntent = "warm, punchy master for Spotify";
        options.targetLoudness = -14.0f;
        
        masteringAgent.analyzeAndConfigure(mixBuffer, options);
        
        auto decision = masteringAgent.getLastDecision();
        DBG("Mastering complete!");
        DBG("AI reasoning: " + decision.reasoning);
    });
}

//==============================================================================
/**
    Example: With Custom Intent
*/
void exampleWithCustomIntent(ai::AIMasteringAgent& agent, 
                             const juce::AudioBuffer<float>& mixBuffer,
                             const juce::String& userInput) {
    // User can describe what they want
    ai::AIMasteringAgent::Options options;
    
    if (userInput.containsIgnoreCase("loud")) {
        options.targetLoudness = -8.0f;  // Loud master
        options.userIntent = "loud, energetic master with maximum impact";
    }
    else if (userInput.containsIgnoreCase("dynamic")) {
        options.targetLoudness = -16.0f;  // Dynamic master
        options.userIntent = "preserve dynamics, natural and transparent";
    }
    else if (userInput.containsIgnoreCase("warm")) {
        options.userIntent = "warm, analog-style master with character";
    }
    else {
        options.userIntent = userInput;  // Use exactly what they said
    }
    
    agent.analyzeAndConfigure(mixBuffer, options);
}

//==============================================================================
/**
    Example: Genre-Specific Mastering
*/
void exampleGenreSpecific(ai::AIMasteringAgent& agent,
                          const juce::AudioBuffer<float>& mixBuffer,
                          const juce::String& genre) {
    ai::AIMasteringAgent::Options options;
    options.genre = genre;
    
    if (genre == "electronic") {
        options.targetLoudness = -9.0f;
        options.userIntent = "punchy, club-ready electronic master with tight low end";
    }
    else if (genre == "rock") {
        options.targetLoudness = -11.0f;
        options.userIntent = "powerful rock master with aggressive presence";
    }
    else if (genre == "acoustic" || genre == "classical") {
        options.targetLoudness = -18.0f;
        options.userIntent = "natural, transparent master preserving dynamics";
    }
    else if (genre == "hip-hop") {
        options.targetLoudness = -10.0f;
        options.userIntent = "modern hip-hop master with deep bass and clear vocals";
    }
    else {
        options.targetLoudness = -14.0f;
        options.userIntent = "balanced, professional streaming master";
    }
    
    agent.analyzeAndConfigure(mixBuffer, options);
}

} // namespace zenith
