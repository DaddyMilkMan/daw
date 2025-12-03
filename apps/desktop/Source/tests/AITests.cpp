#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "../network/GrokUtils.h"

namespace zenith {
namespace tests {

/**
 * @class AITests
 * @brief Tests for AI integration logic (parsing, etc.)
 */
class AITests : public juce::UnitTest {
public:
    AITests() : juce::UnitTest("AI Integration", "AI") {}
    
    void runTest() override {
        beginTest("Grok Malformed JSON Handling");
        {
            // 1. Clean JSON
            juce::String cleanJson = "{ \"cutoff\": 500.0 }";
            auto params = GrokUtils::parseJSONResponse(cleanJson);
            expect(params.isObject());
            expectEquals((double)params["cutoff"], 500.0);

            // 2. Markdown wrapped JSON
            juce::String markdownJson = "Here is the preset:\n```json\n{\n  \"cutoff\": 1000.0\n}\n```\nEnjoy!";
            params = GrokUtils::parseJSONResponse(markdownJson);
            expect(params.isObject());
            expectEquals((double)params["cutoff"], 1000.0);

            // 3. Generic code block
            juce::String genericCode = "```\n{ \"resonance\": 0.5 }\n```";
            params = GrokUtils::parseJSONResponse(genericCode);
            expect(params.isObject());
            expectEquals((double)params["resonance"], 0.5);

            // 4. Conversational garbage with JSON inside
            juce::String garbage = "Sure! I made a bass for you. { \"waveform\": \"saw\" } is the patch.";
            params = GrokUtils::parseJSONResponse(garbage);
            expect(params.isObject());
            expectEquals(params["waveform"].toString(), juce::String("saw"));

            // 5. Invalid JSON
            juce::String invalid = "This is just text.";
            params = GrokUtils::parseJSONResponse(invalid);
            expect(!params.isObject());
        }
    }
};

static AITests aiTests;

} // namespace tests
} // namespace zenith

