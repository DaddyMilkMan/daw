#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>

// Mocking the behavior of the original class
class BaselinePreset {
public:
    std::string id;
    std::string name;
    std::string instrumentId;
    std::string author;

    BaselinePreset(const std::string &name_,
                   const std::string &instrumentId_,
                   const std::string &author_ = "Factory")
        : name(name_), instrumentId(instrumentId_), author(author_) {
        id = generateId(name_);
    }

private:
    static std::string generateId(const std::string &name) {
        // Simulate string processing
        std::string sanitized = name;
        std::transform(sanitized.begin(), sanitized.end(), sanitized.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        std::replace(sanitized.begin(), sanitized.end(), ' ', '_');

        // Remove non-alphanumeric (simplified retainCharacters)
        sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(),
            [](char c) { return !std::isalnum(c) && c != '_'; }),
            sanitized.end());

        // Simulate syscall
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();

        return sanitized + "_" + std::to_string(timestamp);
    }
};

// Mocking the optimized behavior
class OptimizedPreset {
public:
    std::string id;
    std::string name;
    std::string instrumentId;
    std::string author;

    OptimizedPreset(const std::string &name_,
                    const std::string &instrumentId_,
                    const std::string &author_ = "Factory",
                    const std::string &id_ = "")
        : name(name_), instrumentId(instrumentId_), author(author_) {
        if (!id_.empty()) {
            id = id_;
        } else {
            id = generateId(name_);
        }
    }

private:
    static std::string generateId(const std::string &name) {
        // Same logic as above
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

    // Benchmark Baseline (Pattern A: Create then Overwrite)
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        BaselinePreset preset(testName, testInstId, testAuthor);
        // Simulate overwriting the ID (e.g., loading from JSON)
        preset.id = existingId;

        // Prevent optimization
        if (preset.id.empty()) std::cout << "error";
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto baselineDuration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Baseline (Create + Overwrite): " << baselineDuration << " us" << std::endl;

    // Benchmark Optimized (Constructor Injection)
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        OptimizedPreset preset(testName, testInstId, testAuthor, existingId);

        // Prevent optimization
        if (preset.id.empty()) std::cout << "error";
    }
    end = std::chrono::high_resolution_clock::now();
    auto optimizedDuration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Optimized (Constructor Injection): " << optimizedDuration << " us" << std::endl;

    double improvement = (double)(baselineDuration - optimizedDuration) / baselineDuration * 100.0;
    std::cout << "Improvement: " << improvement << "%" << std::endl;
    std::cout << "Speedup: " << (double)baselineDuration / optimizedDuration << "x" << std::endl;

    return 0;
}
