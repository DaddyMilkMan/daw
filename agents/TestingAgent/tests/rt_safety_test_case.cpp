#include <vector>
#include <string>
#include <iostream>
#include <mutex>

class AudioProcessor {
public:
    virtual void processBlock(float* buffer, int numSamples) = 0;
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;
};

class RTSafetyTestProcessor : public AudioProcessor {
public:
    std::mutex mutex_;
    std::vector<float> data_;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        // SAFE: Allocations allowed here
        data_.resize(samplesPerBlock);
        data_.push_back(0.0f);

        // SAFE: Locks allowed here
        std::lock_guard<std::mutex> lock(mutex_);

        // SAFE: I/O allowed here
        std::cout << "Preparing to play" << std::endl;

        // SAFE: Dynamic cast allowed here (though discouraged generally)
        // (Simulated)
    }

    void processBlock(float* buffer, int numSamples) override {
        // VIOLATION: Heap allocation (new)
        float* temp = new float[128];

        // VIOLATION: Heap allocation (malloc)
        void* ptr = malloc(1024);

        // VIOLATION: Blocking I/O (cout)
        std::cout << "Processing block" << std::endl;

        // VIOLATION: Blocking I/O (printf)
        printf("Debug info\n");

        // VIOLATION: Blocking I/O (DBG macro - common in JUCE)
        // DBG("Processing");

        // VIOLATION: Lock
        std::lock_guard<std::mutex> lock(mutex_);

        // VIOLATION: Vector reallocation risk
        data_.push_back(1.0f);

        // VIOLATION: String concatenation (likely allocation)
        std::string s = "audio" + std::string(" processing");

        // VIOLATION: Exception
        try {
            if (numSamples < 0) throw std::runtime_error("Invalid");
        } catch (...) {
            // handle
        }

        // VIOLATION: Dynamic cast
        AudioProcessor* p = dynamic_cast<AudioProcessor*>(this);

        // TRICKY: String with braces (should not confuse parser)
        const char* tricky = "This string has { braces } inside";

        // Cleanup (ignoring the fact that delete is also unsafe)
        delete[] temp;
        free(ptr);
    }

    // Helper function (Parser might not descend here unless we scan everything,
    // but plan says we focus on processBlock bodies for now)
    void helperFunction() {
        // This is outside processBlock, but if called from it, it's unsafe.
        // Current plan is intra-procedural scan of processBlock.
    }
};
