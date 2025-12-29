
//==============================================================================
// PDC
//==============================================================================

bool Engine::isPDCEnabled() const {
  return audioRenderer_ ? audioRenderer_->isPDCEnabled() : false;
}

int Engine::getMaxTrackLatency() const {
  return audioRenderer_ ? audioRenderer_->getMasterLatency() : 0;
}

void Engine::recalculatePDC() {
  if (audioRenderer_ && liveContext_) {
    // Build raw pointer vector for AudioRenderer
    std::vector<Track *> trackPtrs;
    trackPtrs.reserve(tracks_.size());
    for (const auto &t : tracks_)
      trackPtrs.push_back(t.get());

    audioRenderer_->calculatePDC(*liveContext_, trackPtrs);
  }
}

//==============================================================================
// Master Plugin Management
//==============================================================================

void Engine::clearMasterPlugins() {
  const juce::ScopedLock lock(masterPluginLock_);
  masterPlugins_.clear();
}

int Engine::getNumMasterPlugins() const {
  const juce::ScopedLock lock(masterPluginLock_);
  return static_cast<int>(masterPlugins_.size());
}

juce::AudioPluginInstance* Engine::getMasterPlugin(int index) const {
  const juce::ScopedLock lock(masterPluginLock_);
  if (index >= 0 && index < static_cast<int>(masterPlugins_.size())) {
    return masterPlugins_[index].get();
  }
  return nullptr;
}

void Engine::addMasterPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin) {
  if (!plugin) return;
  addMasterPlugin(std::shared_ptr<juce::AudioPluginInstance>(plugin.release()));
}

void Engine::addMasterPlugin(std::shared_ptr<juce::AudioPluginInstance> plugin) {
  if (!plugin) return;
  
  const juce::ScopedLock lock(masterPluginLock_);
  plugin->prepareToPlay(currentSampleRate.load(), currentBufferSize.load());
  masterPlugins_.push_back(std::move(plugin));
}

void Engine::removeMasterPlugin(int index) {
  const juce::ScopedLock lock(masterPluginLock_);
  if (index >= 0 && index < static_cast<int>(masterPlugins_.size())) {
    masterPlugins_.erase(masterPlugins_.begin() + index);
  }
}

void Engine::updateSoloState() {
  bool anySolo = false;
  for (const auto &track : tracks_) {
    if (track && track->isSolo()) {
      anySolo = true;
      break;
    }
  }

  for (const auto &track : tracks_) {
    if (track) {
      if (anySolo) {
        // If any track is soloed, mute this track unless it is also soloed
        track->setSilencedBySolo(!track->isSolo());
      } else {
        // No solo active, unmute everyone (from solo perspective)
        track->setSilencedBySolo(false);
      }
    }
  }
}

//==============================================================================
// Aux Bus Management
//==============================================================================

int Engine::createAuxBus(const juce::String &name) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  // Create new bus
  auto bus = std::make_shared<zenith::AuxBus>(name);

  // Initialize if engine is running
  if (currentSampleRate.load() > 0) {
    bus->prepareToPlay(currentBufferSize.load(), currentSampleRate.load());
  }

  auxBuses_.push_back(bus);
  int index = static_cast<int>(auxBuses_.size()) - 1;

  // Set ID (Use monotonic counter)
  juce::String id = "aux_" + juce::String(auxBusIdCounter.fetch_add(1));
  bus->setId(id);

  // Register with RoutingGraph
  RoutingGraph::Node node;
  node.id = id;
  node.name = name;
  node.type = RoutingGraph::NodeType::Bus;
  routingGraph_.addNode(node);

  if (audioRenderer_ && liveContext_) {
    liveContext_->prepare(currentSampleRate.load(), currentBufferSize.load(),
                            tracks_.size(), auxBuses_.size());
  }

  updateTrackSnapshot();

  DBG("Engine: Created Aux Bus '" + name + "' (ID: " + node.id + ")");
  return index;
}

void Engine::removeAuxBus(int auxIndex) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (auxIndex >= 0 && auxIndex < static_cast<int>(auxBuses_.size())) {
    auto bus = auxBuses_[auxIndex];
    if (bus) {
      juce::String id = bus->getId();
      routingGraph_.removeNode(id);
      bus->releaseResources();
    }

    auxBuses_.erase(auxBuses_.begin() + auxIndex);

    updateTrackSnapshot();
  }
}

int Engine::getNumAuxBuses() const noexcept {
  return static_cast<int>(auxBuses_.size());
}

AuxBus *Engine::getAuxBus(int auxIndex) noexcept {
  if (auxIndex >= 0 && auxIndex < static_cast<int>(auxBuses_.size()))
    return auxBuses_[auxIndex].get();
  return nullptr;
}

float Engine::getAuxBusLevel(int auxIndex) const {
  if (auxIndex >= 0 && auxIndex < static_cast<int>(auxBuses_.size()))
    return auxBuses_[auxIndex]->getCurrentLevel();
  return 0.0f;
}

float Engine::getAuxBusPeakLevel(int auxIndex) const {
  if (auxIndex >= 0 && auxIndex < static_cast<int>(auxBuses_.size()))
    return auxBuses_[auxIndex]->getPeakLevel();
  return 0.0f;
}

//==============================================================================
// Master Limiter API
//==============================================================================

void Engine::setMasterLimiterEnabled(bool enabled) {
  masterLimiter_.setEnabled(enabled);
  DBG("Engine: Master limiter " +
      juce::String(enabled ? "enabled" : "disabled"));
}

bool Engine::isMasterLimiterEnabled() const {
  return masterLimiter_.isEnabled();
}

void Engine::setMasterLimiterCeiling(float ceilingDb) {
  masterLimiter_.setCeiling(ceilingDb);
  DBG("Engine: Master limiter ceiling set to " + juce::String(ceilingDb, 1) +
      " dB");
}

float Engine::getMasterLimiterGainReduction() const {
  return masterLimiter_.getGainReductionDb();
}

int Engine::getMasterLimiterLatency() const {
  return masterLimiter_.getLatency();
}

//==============================================================================
// Track Freeze API (CPU Optimization)
//==============================================================================

bool Engine::freezeTrack(
    int trackIndex, std::function<void(float, const juce::String &)> progress) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks_.size())) {
    DBG("Engine: freezeTrack - Invalid track index: " +
        juce::String(trackIndex));
    return false;
  }

  if (!freezeManager_) {
    DBG("Engine: freezeTrack - FreezeManager not initialized");
    return false;
  }

  // Determine freeze directory
  juce::File freezeDir;
  if (projectState_ && projectState_->getProjectFile().existsAsFile()) {
    freezeDir = projectState_->getProjectFile().getSiblingFile("Freeze Files");
  } else {
    freezeDir =
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("ZenithDAW/Freeze");
  }

  return freezeManager_->freezeTrack(*tracks_[trackIndex], *this, freezeDir,
                                     progress);
}

bool Engine::unfreezeTrack(int trackIndex) {
  jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks_.size())) {
    DBG("Engine: unfreezeTrack - Invalid track index: " +
        juce::String(trackIndex));
    return false;
  }

  if (!freezeManager_) {
    DBG("Engine: unfreezeTrack - FreezeManager not initialized");
    return false;
  }

  return freezeManager_->unfreezeTrack(*tracks_[trackIndex]);
}

bool Engine::isTrackFrozen(int trackIndex) const {
  if (trackIndex < 0 || trackIndex >= static_cast<int>(tracks_.size())) {
    return false;
  }
  return tracks_[trackIndex]->isFrozen();
}

void Engine::cancelFreeze() {
  if (freezeManager_) {
    freezeManager_->cancelFreeze();
  }
}

//==============================================================================
// Metronome
//==============================================================================

void Engine::toggleMetronome() {
  if (transportController_) {
    transportController_->setMetronomeEnabled(
        !transportController_->isMetronomeEnabled());
    
    // Sync internal state if needed
    if (metronome_) {
        metronome_->setEnabled(transportController_->isMetronomeEnabled());
    }
  }
}

bool Engine::isMetronomeEnabled() const {
  return transportController_ ? transportController_->isMetronomeEnabled()
                              : false;
}

void Engine::setMetronomeLevel(float level) {
  if (transportController_) {
    transportController_->setMetronomeLevel(level);
    if (metronome_) {
        metronome_->setGain(level);
    }
  }
}

juce::ThreadPool &Engine::getThreadPool() { return threadPool; }

} // namespace zenith
