#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>

/**
 * Benchmark for ZenithInstrumentPreset factory methods.
 *
 * To compile and run manually:
 * g++ -O3 InstrumentPresetBenchmark.cpp -o benchmark_preset && ./benchmark_preset
 */

// Mocking JUCE String behavior for benchmarking purposes
namespace juce {
    class String {
    public:
        std::string s;
        String(const std::string& str) : s(str) {}
        String(const char* str) : s(str) {}
        String(long long val) : s(std::to_string(val)) {}

        String toLowerCase() const {
            std::string copy = s;
            std::transform(copy.begin(), copy.end(), copy.begin(), ::tolower);
            return String(copy);
        }

        String replaceCharacter(char oldChar, char newChar) const {
            std::string copy = s;
            std::replace(copy.begin(), copy.end(), oldChar, newChar);
            return String(copy);
        }

        String retainCharacters(const std::string& allowed) const {
            std::string copy;
            for (char c : s) {
                if (allowed.find(c) != std::string::npos) {
                    copy += c;
                }
            }
            return String(copy);
        }

        std::string toStdString() const { return s; }

        String operator+(const String& other) const {
            return String(s + other.s);
        }
    };

    class Time {
    public:
        static long long getCurrentTime() {
             using namespace std::chrono;
             return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        }
        long long toMilliseconds() { return getCurrentTime(); }
    };
}

// The logic under test - Updated to match Refactored Code in InstrumentPreset.h
class BenchmarkPreset {
public:
    std::string id;
    std::string name;

    // Logic from InstrumentPreset.h
    static std::string generateId(const std::string &name) {
        juce::String sanitized = name;
        sanitized = sanitized.toLowerCase().replaceCharacter(' ', '_');
        sanitized = sanitized.retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789_");

        auto timestamp = juce::Time::getCurrentTime();
        return sanitized.toStdString() + "_" + std::to_string(timestamp);
    }

    // Factory Methods
    static BenchmarkPreset createNew(const std::string& name_) {
        return BenchmarkPreset(name_);
    }

    static BenchmarkPreset loadExisting(const std::string& id_, const std::string& name_) {
        return BenchmarkPreset(id_, name_);
    }

private:
    // Private constructors
    BenchmarkPreset(const std::string& name_) : name(name_) {
        id = generateId(name_);
    }

    BenchmarkPreset(const std::string& id_, const std::string& name_) : id(id_), name(name_) {
        // No generation
    }
};

int main() {
    const int ITERATIONS = 1000000;
    std::string testName = "Super Saw Lead 2024";
    std::string testId = "super_saw_lead_2024_123456789";

    std::cout << "Benchmarking " << ITERATIONS << " iterations..." << std::endl;

    // 1. Benchmark "Creation" Path (Generating ID)
    auto startSlow = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERATIONS; ++i) {
        auto p = BenchmarkPreset::createNew(testName);
        if (p.id.empty()) std::cerr << "Error" << std::endl;
    }
    auto endSlow = std::chrono::high_resolution_clock::now();

    // 2. Benchmark "Loading" Path (Existing ID)
    auto startFast = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERATIONS; ++i) {
        auto p = BenchmarkPreset::loadExisting(testId, testName);
        if (p.id.empty()) std::cerr << "Error" << std::endl;
    }
    auto endFast = std::chrono::high_resolution_clock::now();

    auto durationSlow = std::chrono::duration_cast<std::chrono::milliseconds>(endSlow - startSlow).count();
    auto durationFast = std::chrono::duration_cast<std::chrono::milliseconds>(endFast - startFast).count();

    std::cout << "createNew (Generating ID): " << durationSlow << " ms" << std::endl;
    std::cout << "loadExisting (Existing ID): " << durationFast << " ms" << std::endl;

    if (durationFast < durationSlow) {
        double speedup = (double)durationSlow / durationFast;
        std::cout << "Improvement: " << speedup << "x faster" << std::endl;
    }

    return 0;
}
