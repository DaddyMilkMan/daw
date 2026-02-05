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

/*
    ==============================================================================
    Original file header:
*/

  ==============================================================================

    TrackPluginManager.cpp
    Created: 2026-01-31
    Author:  Zenith DAW

  ==============================================================================
*/


#include "TrackPluginManager.h"
#include "TrackProcessor.h"

namespace zenith {

TrackPluginManager::TrackPluginManager(TrackProcessor &processor,
                                       double &sampleRate, int &blockSize)
    : processor_(processor), sampleRate_(sampleRate), blockSize_(blockSize) {}

void TrackPluginManager::addPlugin(
    std::unique_ptr<juce::AudioPluginInstance> plugin) {
  processor_.getPluginChain().addPlugin(std::move(plugin), sampleRate_,
                                        blockSize_);
}

void TrackPluginManager::removePlugin(int pluginIndex) {
  processor_.getPluginChain().removePlugin(pluginIndex);
}

void TrackPluginManager::clearPlugins() {
  processor_.getPluginChain().clearPlugins();
}

int TrackPluginManager::getNumPlugins() const {
  return processor_.getPluginChain().getNumPlugins();
}

juce::AudioPluginInstance *TrackPluginManager::getPlugin(int index) const {
  return processor_.getPluginChain().getPlugin(index);
}

std::vector<PluginChain::ParameterInfo>
TrackPluginManager::getPluginParameters(int pluginIndex) const {
  return processor_.getPluginChain().getAutomatableParameters(pluginIndex);
}

std::vector<PluginChain::ParameterInfo>
TrackPluginManager::getAllPluginParameters() const {
  return processor_.getPluginChain().getAllAutomatableParameters();
}

void TrackPluginManager::setPluginParameterValue(int pluginIndex, int paramIndex,
                                                 float normalizedValue) {
  processor_.getPluginChain().setParameterValue(pluginIndex, paramIndex,
                                                normalizedValue);
}

int TrackPluginManager::getPluginNumParameters(int pluginIndex) const {
  return processor_.getPluginChain().getNumParameters(pluginIndex);
}

juce::String TrackPluginManager::getPluginParameterName(int pluginIndex,
                                                        int paramIndex) const {
  return processor_.getPluginChain().getParameterName(pluginIndex, paramIndex);
}

} // namespace zenith
