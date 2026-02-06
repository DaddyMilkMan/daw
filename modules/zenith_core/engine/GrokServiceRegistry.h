/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

    ==============================================================================
    GrokServiceRegistry.h
    Dependency injection container for Grok AI services
    Replaces singleton anti-pattern with proper DI
  ==============================================================================


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
        engineRef = engine;
        // Extract and register analysis service from engine
        // Note: Analysis service is currently not directly accessible from Engine in this branch,
        // but we'll keep the logic if it's added later or use the proper accessor.
    }

    std::shared_ptr<Engine> getEngine() const {
        return engineRef.lock();
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
    std::weak_ptr<Engine> engineRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrokServiceRegistry)
};

} // namespace zenith
