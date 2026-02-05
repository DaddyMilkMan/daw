/**
 * @file WCETStressTests.cpp
 * @brief Worst-Case Execution Time stress tests
 * 
 * These tests stress the audio engine to find worst-case execution times.
 * Run with: ./WCETStressTests --duration 60 --max-tracks 50
 */

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>

#include "engine/Engine.h"
#include "engine/WCETMonitor.h"

namespace zenith {
namespace tests {

using namespace std::chrono;

//==============================================================================
/**
 * @class WCETStressTest
 * @brief Stress test that measures audio callback execution times
 */
class WCETStressTest : public juce::UnitTest {
public:
    WCETStressTest() : juce::UnitTest("WCET Stress Test", "Performance") {}
    
    void runTest() override {
        // Parse command line for test parameters
        int durationSeconds = 30;
        int maxTracks = 30;
        juce::String reportFile;
        
        auto* commandLine = juce::JUCEApplicationBase::getInstance();
        if (commandLine != nullptr) {
            auto args = juce::JUCEApplicationBase::getCommandLineParameterArray();
            for (int i = 0; i < args.size(); ++i) {
                if (args[i] == "--duration" && i + 1 < args.size()) {
                    durationSeconds = args[i + 1].getIntValue();
                }
                if (args[i] == "--max-tracks" && i + 1 < args.size()) {
                    maxTracks = args[i + 1].getIntValue();
                }
                if (args[i] == "--report-file" && i + 1 < args.size()) {
                    reportFile = args[i + 1];
                }
            }
        }
        
        runStressTest(durationSeconds, maxTracks, reportFile);
    }
    
    void runStressTest(int durationSeconds, int maxTracks, const juce::String& reportFile) {
        beginTest("WCET Stress Test - " + juce::String(durationSeconds) + "s, " + 
                  juce::String(maxTracks) + " tracks");
        
        // Create engine
        Engine engine;
        
        // Configure WCET monitor
        auto& wcetMonitor = engine.getWCETMonitor();
        wcetMonitor.setEnabled(true);
        
        // Initialize engine
        expect(engine.initialize(), "Engine should initialize");
        
        double sampleRate = engine.getSampleRate();
        int bufferSize = engine.getBufferSize();
        double budgetMs = (bufferSize / sampleRate) * 1000.0 * 0.8; // 80% of buffer time
        wcetMonitor.setBudgetMs(budgetMs);
        
        logMessage("Sample Rate: " + juce::String(sampleRate, 0) + " Hz");
        logMessage("Buffer Size: " + juce::String(bufferSize) + " samples");
        logMessage("Time Budget: " + juce::String(budgetMs, 2) + " ms");
        logMessage("");
        
        // Create tracks progressively
        std::vector<juce::String> trackIds;
        for (int i = 0; i < maxTracks; ++i) {
            auto id = engine.createTrack("Track " + juce::String(i + 1), "audio");
            trackIds.push_back(id);
            
            // Arm every 5th track for recording
            if (i % 5 == 0) {
                engine.setTrackArmed(i, true);
            }
            
            // Add some load with mixer settings
            engine.setTrackVolume(i, 0.7f);
            engine.setTrackPan(i, (i % 2 == 0) ? -0.3f : 0.3f);
        }
        
        logMessage("Created " + juce::String(maxTracks) + " tracks");
        
        // Start playback
        engine.play();
        
        // Run stress test
        auto startTime = steady_clock::now();
        int lastReportedPercent = 0;
        
        while (true) {
            auto elapsed = duration_cast<seconds>(steady_clock::now() - startTime).count();
            if (elapsed >= durationSeconds) {
                break;
            }
            
            int percent = static_cast<int>((elapsed * 100) / durationSeconds);
            if (percent > lastReportedPercent) {
                lastReportedPercent = percent;
                auto stats = wcetMonitor.getStatistics();
                logMessage("Progress: " + juce::String(percent) + "% - " +
                          "Avg: " + juce::String(stats.avgExecutionTimeUs, 1) + "us, " +
                          "Max: " + juce::String(stats.maxExecutionTimeUs, 1) + "us, " +
                          "Overruns: " + juce::String(stats.overruns));
            }
            
            // Simulate some dynamic changes (adds stress)
            if (elapsed % 5 == 0) {
                // Toggle solo/mute on random tracks
                int trackIdx = elapsed % maxTracks;
                engine.setTrackMute(trackIdx, (elapsed / 5) % 2 == 0);
            }
            
            std::this_thread::sleep_for(milliseconds(100));
        }
        
        engine.stop();
        
        // Get final statistics
        auto stats = wcetMonitor.getStatistics();
        
        logMessage("");
        logMessage("=== WCET Results ===");
        logMessage(wcetMonitor.getReport());
        
        // Validate results
        double utilization = (stats.maxExecutionTimeUs / 1000.0) / budgetMs * 100.0;
        
        expect(stats.totalCallbacks > 0, "Should have processed callbacks");
        expect(utilization < 100.0, "Should not exceed 100% budget utilization");
        
        if (stats.overruns > 0) {
            logMessage("WARNING: " + juce::String(stats.overruns) + " overruns detected!");
        }
        
        // Write report file if requested
        if (reportFile.isNotEmpty()) {
            writeReport(reportFile, stats, durationSeconds, sampleRate, bufferSize, budgetMs);
        }
        
        engine.shutdown();
    }
    
    void writeReport(const juce::String& filename, const profiling::WCETStatistics& stats,
                     int duration, double sampleRate, int bufferSize, double budgetMs) {
        std::ofstream file(filename.toStdString());
        if (!file.is_open()) {
            logMessage("Failed to write report file: " + filename);
            return;
        }
        
        file << "{\n";
        file << "  \"testDuration\": " << duration << ",\n";
        file << "  \"sampleRate\": " << sampleRate << ",\n";
        file << "  \"bufferSize\": " << bufferSize << ",\n";
        file << "  \"budgetMs\": " << budgetMs << ",\n";
        file << "  \"statistics\": {\n";
        file << "    \"avgExecutionTimeUs\": " << stats.avgExecutionTimeUs << ",\n";
        file << "    \"maxExecutionTimeUs\": " << stats.maxExecutionTimeUs << ",\n";
        file << "    \"minExecutionTimeUs\": " << stats.minExecutionTimeUs << ",\n";
        file << "    \"currentExecutionTimeUs\": " << stats.currentExecutionTimeUs << ",\n";
        file << "    \"budgetUtilizationPercent\": " << stats.budgetUtilizationPercent << ",\n";
        file << "    \"totalCallbacks\": " << stats.totalCallbacks << ",\n";
        file << "    \"overruns\": " << stats.overruns << ",\n";
        file << "    \"consecutiveOverruns\": " << stats.consecutiveOverruns << ",\n";
        file << "    \"maxConsecutiveOverruns\": " << stats.maxConsecutiveOverruns << "\n";
        file << "  }\n";
        file << "}\n";
        
        file.close();
        logMessage("Report written to: " + filename);
    }
};

static WCETStressTest wcetStressTest;

} // namespace tests
} // namespace zenith
