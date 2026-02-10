/*
  ==============================================================================
    AutonomousDecisionEngine.h
    Real autonomous decision logic with utility functions and decision trees
    Phase 3: Autonomous Intelligence (10/10) - Fixed Implementation
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PredictiveMastering.h"
#include "RealNeuralNetwork.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace zenith {
namespace ai {

// Decision utility functions
struct UtilityFactors {
    float qualityImprovement = 0.0f;      // How much it improves quality
    float userPreference = 0.0f;           // How well it matches user preferences
    float riskLevel = 0.0f;               // How risky the change is
    float processingCost = 0.0f;          // CPU/memory cost
    float reversibility = 0.0f;            // How easy to undo
    float genreAppropriateness = 0.0f;     // How appropriate for the genre
    float projectConsistency = 0.0f;       // How consistent with project
};

// Decision option with calculated utility
struct DecisionOption {
    juce::String id;
    juce::String type;                    // "eq", "compression", "reverb", etc.
    juce::String action;                  // "boost", "cut", "add", etc.
    juce::var parameters;                 // Specific settings
    UtilityFactors utility;
    float overallUtility;                 // Combined utility score
    juce::String reasoning;               // Why this decision was made
    float confidence;                      // Confidence in this decision
};

// Decision tree node
class DecisionNode {
public:
    enum class NodeType {
        Feature,           // Checks a feature value
        Utility,           // Calculates utility
        Action,            // Performs an action
        Composite          // Combines multiple decisions
    };
    
    DecisionNode(NodeType type);
    virtual ~DecisionNode() = default;
    
    virtual DecisionOption evaluate(const std::vector<float>& features,
                                   const juce::var& context) = 0;
    
protected:
    NodeType nodeType;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DecisionNode)
};

// Feature checking node
class FeatureNode : public DecisionNode {
public:
    FeatureNode(const juce::String& featureName, float threshold, bool greaterThan);
    
    DecisionOption evaluate(const std::vector<float>& features,
                           const juce::var& context) override;
    
    void setTrueBranch(std::unique_ptr<DecisionNode> branch);
    void setFalseBranch(std::unique_ptr<DecisionNode> branch);
    
private:
    juce::String featureName;
    float threshold;
    bool greaterThan;
    std::unique_ptr<DecisionNode> trueBranch;
    std::unique_ptr<DecisionNode> falseBranch;
    
    int getFeatureIndex(const juce::String& name) const;
};

// Utility calculation node
class UtilityNode : public DecisionNode {
public:
    UtilityNode(std::function<UtilityFactors(const std::vector<float>&, const juce::var&)> calculator);
    
    DecisionOption evaluate(const std::vector<float>& features,
                           const juce::var& context) override;
    
private:
    std::function<UtilityFactors(const std::vector<float>&, const juce::var&)> utilityCalculator;
    float calculateOverallUtility(const UtilityFactors& factors) const;
};

// Action execution node
class ActionNode : public DecisionNode {
public:
    ActionNode(const juce::String& type, const juce::String& action, const juce::var& parameters);
    
    DecisionOption evaluate(const std::vector<float>& features,
                           const juce::var& context) override;
    
private:
    juce::String actionType;
    juce::String action;
    juce::var actionParameters;
};

// Composite decision node
class CompositeNode : public DecisionNode {
public:
    enum class CombinationMethod {
        WeightedAverage,
        MaxUtility,
        Consensus,
        RiskAware
    };
    
    CompositeNode(CombinationMethod method);
    
    void addChild(std::unique_ptr<DecisionNode> child);
    void setWeight(const juce::String& childId, float weight);
    
    DecisionOption evaluate(const std::vector<float>& features,
                           const juce::var& context) override;
    
private:
    CombinationMethod combinationMethod;
    std::vector<std::unique_ptr<DecisionNode>> children;
    std::unordered_map<juce::String, float> weights;
    
    DecisionOption combineDecisions(const std::vector<DecisionOption>& options) const;
};

// Main autonomous decision engine
class AutonomousDecisionEngine {
public:
    AutonomousDecisionEngine();
    ~AutonomousDecisionEngine();
    
    // Decision making
    DecisionOption makeDecision(const std::vector<float>& audioFeatures,
                              const juce::var& currentSettings,
                              const juce::var& projectContext,
                              const juce::String& goal = "optimize_quality");
    
    std::vector<DecisionOption> generateMultipleOptions(const std::vector<float>& audioFeatures,
                                                        const juce::var& currentSettings,
                                                        const juce::var& projectContext,
                                                        int numOptions = 5);
    
    // Learning and adaptation
    void learnFromOutcome(const DecisionOption& decision,
                         const QualityMetrics& beforeQuality,
                         const QualityMetrics& afterQuality,
                         float userSatisfaction);
    
    void updateUserPreferences(const juce::var& preferences);
    void updateGenreKnowledge(const juce::String& genre, const juce::var& characteristics);
    
    // Decision tree management
    void buildDecisionTree(const juce::String& task);
    void loadDecisionTree(const juce::File& filePath);
    void saveDecisionTree(const juce::File& filePath) const;
    
    // Configuration
    void setRiskTolerance(float tolerance);  // 0.0 = conservative, 1.0 = aggressive
    void setQualityTarget(float target);     // 0.0 to 1.0
    void setProcessingBudget(float budget);  // CPU time budget
    
    // Analytics
    std::vector<DecisionOption> getDecisionHistory() const;
    float getAverageDecisionQuality() const;
    juce::String getDecisionStatistics() const;
    
private:
    // Decision trees for different tasks
    std::unordered_map<juce::String, std::unique_ptr<DecisionNode>> decisionTrees;
    
    // Learning data
    std::vector<std::tuple<DecisionOption, QualityMetrics, QualityMetrics, float>> outcomeHistory;
    juce::var userPreferences;
    std::unordered_map<juce::String, juce::var> genreKnowledge;
    
    // Configuration
    float riskTolerance = 0.5f;
    float qualityTarget = 0.8f;
    float processingBudget = 100.0f;  // milliseconds
    
    // Feature extraction
    std::vector<float> extractFeatures(const std::vector<float>& audioFeatures,
                                     const juce::var& currentSettings,
                                     const juce::var& projectContext);
    
    // Utility calculation
    UtilityFactors calculateUtility(const DecisionOption& option,
                                  const std::vector<float>& features,
                                  const juce::var& context);
    
    // Decision tree builders
    std::unique_ptr<DecisionNode> buildEQDecisionTree();
    std::unique_ptr<DecisionNode> buildCompressionDecisionTree();
    std::unique_ptr<DecisionNode> buildReverbDecisionTree();
    std::unique_ptr<DecisionNode> buildMasteringDecisionTree();
    
    // Learning algorithms
    void updateDecisionWeights(const DecisionOption& decision, float outcome);
    void refineUtilityFunctions();
    
    // Analytics
    void recordDecision(const DecisionOption& decision);
    float calculateDecisionSuccess(const DecisionOption& decision) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutonomousDecisionEngine)
};

} // namespace ai
} // namespace zenith
