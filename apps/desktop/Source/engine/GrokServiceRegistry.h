/*
  ==============================================================================
    GrokServiceRegistry.h
    Dependency injection container for Grok AI services
    Replaces singleton anti-pattern with proper DI
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include "../engine/Engine.h"
#include "../browser/BrowserModel.h"
#include "../network/AudioAnalysisService.h"
#include <memory>
#include <unordered_map>

namespace zenith {

class GrokServiceRegistry {
public:
    static GrokServiceRegistry& getInstance() {
        static GrokServiceRegistry instance;
        return instance;
    }

    // Service registration with proper ownership
    template<typename T>
    void registerService(const juce::String& name, std::shared_ptr<T> service) {
        services[name.toStdString()] = std::static_pointer_cast<void>(service);
    }

    template<typename T>
    std::shared_ptr<T> getService(const juce::String& name) const {
        auto it = services.find(name.toStdString());
        if (it != services.end()) {
            return std::static_pointer_cast<T>(it->second);
        }
        return nullptr;
    }

    // Convenience methods for common services
    void setEngine(std::shared_ptr<Engine> engine) {
        registerService("engine", engine);
        // Extract and register analysis service from engine
        if (engine && engine->getAnalysisService()) {
            registerService("analysisService", 
                std::shared_ptr<AudioAnalysisService>(engine->getAnalysisService(), 
                [](AudioAnalysisService*){})); // Non-owning shared_ptr
        }
    }

    std::shared_ptr<Engine> getEngine() const {
        return getService<Engine>("engine");
    }

    std::shared_ptr<AudioAnalysisService> getAnalysisService() const {
        return getService<AudioAnalysisService>("analysisService");
    }

    void setBrowserModel(std::shared_ptr<BrowserModel> model) {
        registerService("browserModel", model);
    }

    std::shared_ptr<BrowserModel> getBrowserModel() const {
        return getService<BrowserModel>("browserModel");
    }

    // Clear all services (for shutdown)
    void clear() {
        services.clear();
    }

private:
    GrokServiceRegistry() = default;
    ~GrokServiceRegistry() = default;

    std::unordered_map<std::string, std::shared_ptr<void>> services;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrokServiceRegistry)
};

} // namespace zenith
