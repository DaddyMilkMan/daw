/*
  ==============================================================================

    MCPTests.cpp
    QA & Verification Sentinel - AI & MCP Integration Suite

  ==============================================================================
*/

#include <juce_core/juce_core.h>
#include "../mcp/MCPToolSchemas.h"
#include "../network/GrokUtils.h"
#include <string>

class MCPTests : public juce::UnitTest
{
public:
    MCPTests() : juce::UnitTest("MCP Integration", "MCP") {}

    void runTest() override
    {
        testSchemaAllocations();
        testGrokUtilsRegexSafety();
    }

private:
    /**
     * @brief Stress test for schema memory allocations.
     * Calls getAllToolSchemas() 1,000 times to verify no memory leaks.
     * Should be run under Valgrind/ASan for accurate leak detection.
     */
    void testSchemaAllocations()
    {
        beginTest("Schema Allocations - 1000 Iterations (Valgrind/ASan)");
        
        for (int i = 0; i < 1000; ++i) {
            juce::var schemas = zenith::mcp::schemas::getAllToolSchemas();
            
            // Basic verification
            expect(schemas.isArray(), "Schemas should be an array");
            expect(schemas.size() > 0, "Schemas array should not be empty");
        }
        
        logMessage("Completed 1000 schema generation iterations.");
    }

    /**
     * @brief Regex safety test for GrokUtils::parseJSONResponse.
     * Feeds malformed and garbage data to ensure no crashes.
     */
    void testGrokUtilsRegexSafety()
    {
        beginTest("GrokUtils Regex Safety - Malformed Input");
        
        // Test 1: Empty string
        {
            juce::var result = zenith::GrokUtils::parseJSONResponse("");
            expect(result.isVoid() || !result.isObject(), "Empty string should return void");
        }
        
        // Test 2: Pure garbage
        {
            juce::String garbage;
            for (int i = 0; i < 10000; ++i) {
                garbage += static_cast<char>((i % 95) + 32); // Printable ASCII
            }
            juce::var result = zenith::GrokUtils::parseJSONResponse(garbage);
            // Should not crash, result can be void or an attempt
            expect(true, "Garbage input should not crash");
        }
        
        // Test 3: Valid JSON
        {
            juce::String validJson = "{ \"key\": \"value\" }";
            juce::var result = zenith::GrokUtils::parseJSONResponse(validJson);
            expect(result.isObject(), "Valid JSON should parse correctly");
        }
        
        // Test 4: JSON in code block
        {
            juce::String codeBlock = "```json\n{ \"data\": 123 }\n```";
            juce::var result = zenith::GrokUtils::parseJSONResponse(codeBlock);
            expect(result.isObject(), "Code block JSON should be extracted");
        }
        
        // Test 5: Deeply nested JSON (stress)
        {
            juce::String deepNest = "{ \"a\": { \"b\": { \"c\": { \"d\": 1 } } } }";
            juce::var result = zenith::GrokUtils::parseJSONResponse(deepNest);
            expect(result.isObject(), "Deep JSON should parse");
        }
        
        // Test 6: 1MB malformed JSON
        {
            juce::String bigGarbage;
            for (int i = 0; i < 1024 * 1024; ++i) {
                bigGarbage += static_cast<char>('a' + (i % 26));
            }
            juce::var result = zenith::GrokUtils::parseJSONResponse(bigGarbage);
            expect(true, "1MB garbage should not crash");
        }
        
        logMessage("All GrokUtils regex safety tests passed without crash.");
    }
};

static MCPTests mcpTests;
