#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>

// Mocking the optimized behavior with Static Factory Methods
class OptimizedPreset {
public:
    std::string id;
    std::string name;
    std::string instrumentId;
    std::string author;

    OptimizedPreset() = default;

    static OptimizedPreset createNew(const std::string &name,
                                     const std::string &instrumentId,
                                     const std::string &author = "Factory") {
        OptimizedPreset preset;
        preset.name = name;
        preset.instrumentId = instrumentId;
        preset.author = author;
        preset.id = generateId(name);
        return preset;
    }

    static OptimizedPreset loadExisting(const std::string &id,
                                        const std::string &name,
                                        const std::string &instrumentId,
                                        const std::string &author = "Factory") {
        OptimizedPreset preset;
        preset.id = id;
        preset.name = name;
        preset.instrumentId = instrumentId;
        preset.author = author;
        return preset;
    }

private:
    static std::string generateId(const std::string &name) {
        // Simulate string processing
        std::string sanitized = name;
        std::transform(sanitized.begin(), sanitized.end(), sanitized.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        std::replace(sanitized.begin(), sanitized.end(), ' ', '_');

        sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(),
            [](char c) { return !std::isalnum(c) && c != '_'; }),
            sanitized.end());

        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();

        return sanitized + "_" + std::to_string(timestamp);
    }
};

int main() {
    const int iterations = 100000;
    const std::string testName = "Test Instrument Preset Name";
    const std::string testInstId = "zenith.poly.synth";
    const std::string testAuthor = "User";
    const std::string existingId = "test_instrument_preset_name_1234567890";

    std::cout << "Running benchmark with " << iterations << " iterations..." << std::endl;

    // Benchmark Loading via loadExisting
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto preset = OptimizedPreset::loadExisting(existingId, testName, testInstId, testAuthor);

        // Prevent optimization
        if (preset.id.empty()) std::cout << "error";
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Load Existing (Static Factory): " << duration << " us" << std::endl;

    // Benchmark Creation via createNew
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto preset = OptimizedPreset::createNew(testName, testInstId, testAuthor);

        // Prevent optimization
        if (preset.id.empty()) std::cout << "error";
    }
    end = std::chrono::high_resolution_clock::now();
    auto createDuration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Create New (Static Factory): " << createDuration << " us" << std::endl;

    std::cout << "Performance Ratio (Create / Load): " << (double)createDuration / duration << "x" << std::endl;

    return 0;
}
