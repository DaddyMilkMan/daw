/*
  ==============================================================================
    GrokAITestSuite.cpp
    Comprehensive test suite for all Grok AI systems
    Phase 3: Autonomous Intelligence (10/10) - Complete Testing
  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../ai/GrokAPIClient.h"
#include "../ai/PredictiveMastering.h"
#include "../ai/RealTimeCollaboration.h"
#include "../ai/AudioThreadSafeProcessor.h"
#include "../ai/RealNeuralNetwork.h"
#include "../ai/AutonomousDecisionEngine.h"
#include "../ai/VisualAnalyzer.h"
#include "../ai/GenreDetector.h"
#include "../ai/CreativePartner.h"
#include "../ai/ProjectContext.h"

using namespace zenith::ai;

class GrokAITestSuite : public juce::UnitTest {
public:
    GrokAITestSuite() : UnitTest("Grok AI System Test Suite") {}
    
    void runTest() override {
        beginTest("Phase 1: Learning System Tests");
        testLearningSystem();
        
        beginTest("Phase 2: Context-Aware AI Tests");
        testContextAwareAI();
        
        beginTest("Phase 3: Autonomous Intelligence Tests");
        testAutonomousIntelligence();
        
        beginTest("Integration Tests");
        testIntegration();
        
        beginTest("Performance Tests");
        testPerformance();
        
        beginTest("Real-Time Safety Tests");
        testRealTimeSafety();
        
        beginTest("Neural Network Tests");
        testNeuralNetworks();
        
        beginTest("Decision Engine Tests");
        testDecisionEngine();
    }
    
private:
    void testLearningSystem() {
        // Test GrokJourney initialization
        auto journey = std::make_unique<GrokJourney>();
        auto tempPath = juce::File::getSpecialLocation(juce::File::tempDirectory)
                           .getChildFile("test_grok.db");
        
        expect(journey->initialize(tempPath), "Failed to initialize learning database");
        
        // Test user preference recording
        UserPreference pref;
        pref.genre = "electronic";
        pref.instrument = "synth";
        pref.parameter = "eq_high";
        pref.value = 2.0f;
        pref.confidence = 0.8f;
        pref.timestamp = juce::Time::getCurrentTime();
        
        journey->recordUserAction(pref);
        
        // Test preference retrieval
        auto settings = journey->getRecommendedSettings("electronic", "synth");
        expect(settings.isObject(), "Failed to retrieve recommended settings");
        
        // Test reference matching
        auto refMatcher = std::make_unique<ReferenceMatcher>();
        juce::AudioBuffer<float> testAudio(2, 44100);  // 1 second at 44.1kHz
        
        // Generate test sine wave
        for (int ch = 0; ch < testAudio.getNumChannels(); ++ch) {
            for (int i = 0; i < testAudio.getNumSamples(); ++i) {
                float freq = 440.0f;  // A4
                testAudio.setSample(ch, i, std::sin(2.0f * juce::MathConstants<float>::pi * freq * i / 44100.0f));
            }
        }
        
        auto match = refMatcher->findBestMatch(testAudio, 44100.0);
        expect(match.isValid, "Failed to find reference match");
        
        // Cleanup
        tempPath.deleteFile();
    }
    
    void testContextAwareAI() {
        // Test visual analyzer
        auto visualAnalyzer = std::make_unique<VisualAnalyzer>();
        juce::AudioBuffer<float> testAudio(2, 44100);
        
        // Generate complex test signal
        for (int ch = 0; ch < testAudio.getNumChannels(); ++ch) {
            for (int i = 0; i < testAudio.getNumSamples(); ++i) {
                float t = static_cast<float>(i) / 44100.0f;
                float signal = std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * t) * 0.5f;
                signal += std::sin(2.0f * juce::MathConstants<float>::pi * 880.0f * t) * 0.25f;
                signal += std::sin(2.0f * juce::MathConstants<float>::pi * 220.0f * t) * 0.25f;
                testAudio.setSample(ch, i, signal);
            }
        }
        
        auto visualResult = visualAnalyzer->analyzeAudio(testAudio, 44100.0);
        expect(visualResult.isValid, "Visual analysis failed");
        expect(visualResult.waveform.dynamicRange > 0.0f, "Dynamic range not calculated");
        expect(visualResult.waveform.crestFactor > 0.0f, "Crest factor not calculated");
        
        // Test genre detector
        auto genreDetector = std::make_unique<GenreDetector>();
        auto genreResult = genreDetector->detectGenre(testAudio, 44100.0);
        expect(genreResult.genre.isNotEmpty(), "Genre detection failed");
        expect(genreResult.confidence >= 0.0f && genreResult.confidence <= 1.0f, "Invalid confidence value");
        
        // Test project context
        auto projectContext = std::make_unique<ProjectContext>();
        juce::DynamicObject::Ptr projectState = new juce::DynamicObject();
        projectState->setProperty("tempo", 120.0);
        projectState->setProperty("key", "C");
        projectState->setProperty("tracks", 4);
        
        projectContext->analyzeFromDAWState(juce::var(projectState.get()));
        auto insights = projectContext->getContextualInsights();
        expect(insights.energyLevel.isNotEmpty(), "Energy level not detected");
        
        // Test creative partner
        auto creativePartner = std::make_unique<CreativePartner>();
        CreativeInsight insight;
        insight.observation = "Test track has good dynamics";
        insight.explanation = "Based on waveform analysis";
        
        auto suggestions = creativePartner->generateSuggestions(insight);
        expect(!suggestions.empty(), "No creative suggestions generated");
    }
    
    void testAutonomousIntelligence() {
        // Test predictive mastering
        auto predictiveEngine = std::make_unique<PredictiveMastering>();
        juce::AudioBuffer<float> testAudio(2, 44100);
        
        // Generate test signal
        for (int ch = 0; ch < testAudio.getNumChannels(); ++ch) {
            for (int i = 0; i < testAudio.getNumSamples(); ++i) {
                float t = static_cast<float>(i) / 44100.0f;
                testAudio.setSample(ch, i, std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * t) * 0.7f);
            }
        }
        
        ProjectContext context;
        auto prediction = predictiveEngine->predictMasteringChain(testAudio, 44100.0, context, {"electronic", 0.8f});
        expect(prediction.overallConfidence >= 0.0f, "Invalid prediction confidence");
        expect(!prediction.models.empty(), "No prediction models generated");
        
        // Test quality prediction
        juce::DynamicObject::Ptr settings = new juce::DynamicObject();
        settings->setProperty("eq_gain", 2.0f);
        settings->setProperty("compression_ratio", 2.0f);
        
        auto quality = predictiveEngine->predictQuality(testAudio, juce::var(settings.get()), 44100.0);
        expect(quality.overallScore >= 0.0f && quality.overallScore <= 1.0f, "Invalid quality score");
        
        // Test autonomous agent
        auto agent = std::make_unique<AutonomousMasteringAgent>();
        auto result = agent->processAutonomously(testAudio, 44100.0, context);
        expect(result.isObject(), "Autonomous processing failed");
        
        // Test decision engine
        auto decisionEngine = std::make_unique<AutonomousDecisionEngine>();
        std::vector<float> features = {0.5f, 0.3f, 0.7f, 0.2f};  // Mock features
        
        auto decision = decisionEngine->makeDecision(features, juce::var(), juce::var(), "optimize_quality");
        expect(decision.id.isNotEmpty(), "Decision ID not generated");
        expect(decision.overallUtility >= 0.0f, "Invalid utility score");
    }
    
    void testIntegration() {
        // Test full GrokAPIClient integration
        auto client = std::make_unique<GrokAPIClient>();
        
        // Test API key handling
        juce::Environment::setEnvironmentVariable("GROK_API_KEY", "test_key");
        expect(client->hasAPIKey(), "API key not loaded from environment");
        
        // Test integrated analysis
        juce::AudioBuffer<float> testAudio(2, 44100);
        for (int ch = 0; ch < testAudio.getNumChannels(); ++ch) {
            for (int i = 0; i < testAudio.getNumSamples(); ++i) {
                float t = static_cast<float>(i) / 44100.0f;
                testAudio.setSample(ch, i, std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * t) * 0.5f);
            }
        }
        
        GrokAPIClient::AnalysisRequest request;
        request.audio = testAudio;
        request.sampleRate = 44100.0;
        request.trackName = "test_track";
        
        bool analysisCompleted = false;
        GrokAPIClient::AnalysisResult result;
        
        client->analyzeAudioAsync(request, [&](GrokAPIClient::AnalysisResult asyncResult) {
            result = asyncResult;
            analysisCompleted = true;
        });
        
        // Wait for async completion (simplified for test)
        int timeout = 1000;  // 1 second timeout
        while (!analysisCompleted && timeout-- > 0) {
            juce::Thread::sleep(1);
        }
        
        expect(analysisCompleted, "Async analysis did not complete");
        expect(result.isComplete, "Analysis result not complete");
        
        // Test creative suggestions
        auto suggestions = client->getCreativeSuggestions("test_track");
        expect(!suggestions.empty(), "No creative suggestions generated");
        
        // Test learning from interaction
        client->learnFromUserInteraction("eq_boost", juce::var(2.0f), 0.8f);
    }
    
    void testPerformance() {
        // Performance benchmarks
        auto visualAnalyzer = std::make_unique<VisualAnalyzer>();
        juce::AudioBuffer<float> largeAudio(2, 44100 * 10);  // 10 seconds
        
        // Generate test signal
        for (int ch = 0; ch < largeAudio.getNumChannels(); ++ch) {
            for (int i = 0; i < largeAudio.getNumSamples(); ++i) {
                float t = static_cast<float>(i) / 44100.0f;
                largeAudio.setSample(ch, i, std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * t) * 0.5f);
            }
        }
        
        auto startTime = juce::Time::getCurrentTime();
        auto result = visualAnalyzer->analyzeAudio(largeAudio, 44100.0);
        auto endTime = juce::Time::getCurrentTime();
        
        auto processingTime = endTime - startTime;
        expect(processingTime.inMilliseconds() < 1000, "Visual analysis too slow");  // Should be under 1 second
        
        // Test neural network performance
        auto network = std::make_unique<NeuralNetwork>(std::vector<int>{20, 64, 32, 8});
        std::vector<float> input(20, 0.5f);
        
        startTime = juce::Time::getCurrentTime();
        for (int i = 0; i < 1000; ++i) {
            auto output = network->forward(input);
        }
        endTime = juce::Time::getCurrentTime();
        
        processingTime = endTime - startTime;
        expect(processingTime.inMilliseconds() < 100, "Neural network inference too slow");  // Should be under 100ms for 1000 inferences
    }
    
    void testRealTimeSafety() {
        // Test lock-free circular buffer
        LockFreeCircularBuffer<float, 1024> buffer;
        
        // Test basic operations
        expect(buffer.push(1.0f), "Failed to push to buffer");
        expect(buffer.push(2.0f), "Failed to push to buffer");
        
        float value;
        expect(buffer.pop(value), "Failed to pop from buffer");
        expect(value == 1.0f, "Incorrect value popped");
        
        expect(buffer.pop(value), "Failed to pop from buffer");
        expect(value == 2.0f, "Incorrect value popped");
        expect(!buffer.pop(value), "Should not pop from empty buffer");
        
        // Test buffer overflow
        for (int i = 0; i < 1025; ++i) {
            buffer.push(static_cast<float>(i));
        }
        
        // Should have dropped oldest value
        expect(buffer.size() == 1024, "Buffer size incorrect after overflow");
        
        // Test real-time audio processor
        auto processor = std::make_unique<RealTimeAudioProcessor>();
        processor->setAnalysisRate(10.0f);
        processor->setSensitivity(0.5f);
        
        juce::AudioBuffer<float> testAudio(2, 512);  // Small buffer for real-time test
        
        // Process multiple blocks (simulating real-time)
        for (int block = 0; block < 100; ++block) {
            for (int ch = 0; ch < testAudio.getNumChannels(); ++ch) {
                for (int i = 0; i < testAudio.getNumSamples(); ++i) {
                    float t = static_cast<float>(block * testAudio.getNumSamples() + i) / 44100.0f;
                    testAudio.setSample(ch, i, std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * t) * 0.5f);
                }
            }
            
            processor->processAudio(testAudio, 44100.0);
        }
        
        auto analysis = processor->getAnalysis();
        expect(analysis.isValid, "Real-time analysis failed");
        
        auto suggestions = processor->getSuggestions();
        expect(!suggestions.empty(), "No real-time suggestions generated");
    }
    
    void testNeuralNetworks() {
        // Test basic neural network
        auto network = std::make_unique<NeuralNetwork>(std::vector<int>{4, 8, 4});
        
        // Test forward pass
        std::vector<float> input = {0.5f, 0.3f, 0.7f, 0.2f};
        auto output = network->forward(input);
        
        expect(output.size() == 4, "Incorrect output size");
        for (float val : output) {
            expect(val >= 0.0f && val <= 1.0f, "Invalid output value");
        }
        
        // Test training
        std::vector<std::vector<float>> inputs = {
            {0.1f, 0.2f, 0.3f, 0.4f},
            {0.9f, 0.8f, 0.7f, 0.6f},
            {0.5f, 0.5f, 0.5f, 0.5f}
        };
        
        std::vector<std::vector<float>> targets = {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f}
        };
        
        float initialLoss = network->calculateLoss(network->forward(inputs[0]), targets[0]);
        
        network->train(inputs, targets, 10, 0.1f, 1.0f);
        
        float finalLoss = network->calculateLoss(network->forward(inputs[0]), targets[0]);
        expect(finalLoss < initialLoss, "Training did not reduce loss");
        
        // Test model saving/loading
        auto tempPath = juce::File::getSpecialLocation(juce::File::tempDirectory)
                           .getChildFile("test_network.json");
        
        network->saveModel(tempPath);
        
        auto loadedNetwork = std::make_unique<NeuralNetwork>(std::vector<int>{4, 8, 4});
        expect(loadedNetwork->loadModel(tempPath), "Failed to load model");
        
        auto originalOutput = network->forward(input);
        auto loadedOutput = loadedNetwork->forward(input);
        
        expect(originalOutput.size() == loadedOutput.size(), "Output sizes differ after loading");
        
        for (size_t i = 0; i < originalOutput.size(); ++i) {
            expect(std::abs(originalOutput[i] - loadedOutput[i]) < 0.001f, "Outputs differ after loading");
        }
        
        // Cleanup
        tempPath.deleteFile();
        
        // Test specialized networks
        auto genreNetwork = std::make_unique<GenreClassificationNetwork>();
        
        std::vector<std::pair<std::vector<float>, juce::String>> trainingData = {
            {{0.8f, 0.2f, 0.5f, 0.1f}, "electronic"},
            {{0.3f, 0.7f, 0.4f, 0.6f}, "rock"},
            {{0.5f, 0.5f, 0.3f, 0.7f}, "pop"}
        };
        
        genreNetwork->trainOnDataset(trainingData, 5);
        
        auto classification = genreNetwork->classify({0.7f, 0.3f, 0.4f, 0.2f});
        expect(classification.isNotEmpty(), "Genre classification failed");
        
        auto probabilities = genreNetwork->classifyWithProbabilities({0.7f, 0.3f, 0.4f, 0.2f});
        expect(!probabilities.empty(), "No classification probabilities generated");
        
        float totalProb = 0.0f;
        for (const auto& [genre, prob] : probabilities) {
            totalProb += prob;
            expect(prob >= 0.0f && prob <= 1.0f, "Invalid probability value");
        }
        expect(std::abs(totalProb - 1.0f) < 0.001f, "Probabilities do not sum to 1");
    }
    
    void testDecisionEngine() {
        auto decisionEngine = std::make_unique<AutonomousDecisionEngine>();
        
        // Test decision tree building
        decisionEngine->buildDecisionTree("mastering");
        
        // Test basic decision making
        std::vector<float> features = {0.5f, 0.3f, 0.7f, 0.2f, 0.8f, 0.1f};
        juce::var settings;
        juce::var context;
        
        auto decision = decisionEngine->makeDecision(features, settings, context, "optimize_quality");
        expect(decision.id.isNotEmpty(), "Decision ID not generated");
        expect(decision.type.isNotEmpty(), "Decision type not set");
        expect(decision.overallUtility >= 0.0f, "Invalid utility score");
        expect(decision.reasoning.isNotEmpty(), "Decision reasoning not provided");
        
        // Test multiple options generation
        auto options = decisionEngine->generateMultipleOptions(features, settings, context, 3);
        expect(options.size() <= 3, "Too many options generated");
        
        // Test learning from outcomes
        QualityMetrics beforeQuality;
        beforeQuality.overallScore = 0.5f;
        
        QualityMetrics afterQuality;
        afterQuality.overallScore = 0.8f;
        
        decisionEngine->learnFromOutcome(decision, beforeQuality, afterQuality, 0.9f);
        
        // Test configuration
        decisionEngine->setRiskTolerance(0.3f);
        decisionEngine->setQualityTarget(0.9f);
        decisionEngine->setProcessingBudget(50.0f);
        
        // Test analytics
        auto history = decisionEngine->getDecisionHistory();
        expect(!history.empty(), "No decision history recorded");
        
        float avgQuality = decisionEngine->getAverageDecisionQuality();
        expect(avgQuality >= 0.0f, "Invalid average decision quality");
        
        auto stats = decisionEngine->getDecisionStatistics();
        expect(stats.isNotEmpty(), "No decision statistics available");
        
        // Test decision tree persistence
        auto tempPath = juce::File::getSpecialLocation(juce::File::tempDirectory)
                           .getChildFile("test_decision_tree.json");
        
        decisionEngine->saveDecisionTree(tempPath);
        
        auto newEngine = std::make_unique<AutonomousDecisionEngine>();
        expect(newEngine->loadDecisionTree(tempPath), "Failed to load decision tree");
        
        // Cleanup
        tempPath.deleteFile();
    }
};

static GrokAITestSuite grokAITestSuite;
