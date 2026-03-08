// REAL ICE CONNECTIVITY CHECKS IMPLEMENTATION
// This replaces the placeholder selectBestICECandidate() and performICEConnectivityChecks()

void CollaborationManager::performICEConnectivityChecks() {
  currentState.store(ConnectionState::ICEConnecting, std::memory_order_release);
  sendChangeMessage();

  // Generate all candidate pair combinations
  {
    const juce::ScopedLock sl(iceCandidatesLock);
    const juce::ScopedLock sl2(icePairsLock);

    iceCandidatePairs.clear();

    // Determine role based on who's hosting
    iceAgent.role = isHost ? ICERole::Controlling : ICERole::Controlled;

    // Create all possible pairs
    for (const auto& local : localICECandidates) {
      for (const auto& remote : remoteICECandidates) {
        ICECandidatePair pair;
        pair.local = local;
        pair.remote = remote;
        pair.priority = ICECandidatePair::calculatePairPriority(
          local, remote, iceAgent.role == ICERole::Controlling
        );
        pair.state = ICECandidatePair::Frozen;

        // Generate transaction ID for STUN binding request
        juce::Random rng;
        for (int i = 0; i < 12; ++i) {
          pair.transactionId[i] = (uint8_t)rng.nextInt(256);
        }

        iceCandidatePairs.push_back(pair);
      }
    }

    // Sort by priority (highest first)
    std::sort(iceCandidatePairs.begin(), iceCandidatePairs.end(),
      [](const ICECandidatePair& a, const ICECandidatePair& b) {
        return a.priority > b.priority;
      });

    DBG("ICE: Created " + juce::String(iceCandidatePairs.size()) + " candidate pairs");

    // Unfreeze top N pairs (start checking highest priority first)
    constexpr int MAX_CONCURRENT_CHECKS = 5;
    int unfrozen = 0;
    for (auto& pair : iceCandidatePairs) {
      if (unfrozen < MAX_CONCURRENT_CHECKS) {
        pair.state = ICECandidatePair::Waiting;
        unfrozen++;
      } else {
        break;
      }
    }
  }

  // Start the ICE check thread
  if (!isThreadRunning()) {
    startThread();
  }
}

void CollaborationManager::selectBestICECandidate() {
  const juce::ScopedLock sl(icePairsLock);

  // Find the first pair with succeeded state
  for (auto& pair : iceCandidatePairs) {
    if (pair.state == ICECandidatePair::Succeeded) {
      selectedLocalCandidate = pair.local;
      selectedRemoteCandidate = pair.remote;
      iceCandidatesSelected = true;

      DBG("ICE: Selected WORKING candidate pair:");
      DBG("  Local: " + selectedLocalCandidate.connectionAddress + ":" +
          juce::String(selectedLocalCandidate.port));
      DBG("  Remote: " + selectedRemoteCandidate.connectionAddress + ":" +
          juce::String(selectedRemoteCandidate.port));

      // Mark as nominated
      pair.nominated = true;
      return;
    }
  }

  // If all pairs have failed, signal error
  bool allFailed = true;
  for (const auto& pair : iceCandidatePairs) {
    if (pair.state != ICECandidatePair::Failed) {
      allFailed = false;
      break;
    }
  }

  if (allFailed && !iceCandidatePairs.empty()) {
    reportError("ICE connectivity checks failed - no working candidate pairs");
    return;
  }

  // No succeeded pair yet, but some checks are still in progress
  DBG("ICE: No succeeded candidate pairs yet, " +
      juce::String(std::count_if(iceCandidatePairs.begin(), iceCandidatePairs.end(),
          [](const ICECandidatePair& p) { return p.state == ICECandidatePair::Succeeded; })) +
      " succeeded out of " + juce::String(iceCandidatePairs.size()));
}

// NEW: Perform STUN connectivity check through a candidate pair
bool CollaborationManager::performConnectivityCheck(const ICECandidatePair& pair) {
  // Create STUN binding request
  juce::MemoryBlock stunRequest = stunClient->buildBindingRequest();

  // Override transaction ID with pair's transaction ID
  STUNHeader* header = (STUNHeader*)stunRequest.getData();
  std::memcpy(header->transactionId, pair.transactionId.data(), 12);

  // Add PRIORITY attribute (RFC 5245 7.1.1.1)
  // Priority = (2^24 * type preference) + (2^8 * local preference) + (256 - component ID)
  uint32_t priorityAttr = juce::ByteOrder::swapIfBigEndian(
    static_cast<uint32_t>(pair.local.priority & 0xFFFFFFFF)
  );

  STUNAttributeHeader priorityAttrHeader;
  priorityAttrHeader.type = juce::ByteOrder::swapIfBigEndian((uint16_t)0x0024);  // PRIORITY
  priorityAttrHeader.length = juce::ByteOrder::swapIfBigEndian((uint16_t)4);

  stunRequest.append(&priorityAttrHeader, sizeof(STUNAttributeHeader));
  stunRequest.append(&priorityAttr, sizeof(uint32_t));

  // Add ICE-CONTROLLED attribute if we're controlled agent
  if (iceAgent.role == ICERole::Controlled) {
    uint64_t tieBreaker = juce::ByteOrder::swapIfBigEndian(iceAgent.tieBreaker);

    STUNAttributeHeader iceControlledHeader;
    iceControlledHeader.type = juce::ByteOrder::swapIfBigEndian((uint16_t)0x8029);  // ICE-CONTROLLED
    iceControlledHeader.length = juce::ByteOrder::swapIfBigEndian((uint16_t)8);

    stunRequest.append(&iceControlledHeader, sizeof(STUNAttributeHeader));
    stunRequest.append(&tieBreaker, sizeof(uint64_t));
  }

  // Add CONTROLLING attribute if we're controlling agent
  if (iceAgent.role == ICERole::Controlling) {
    uint64_t tieBreaker = juce::ByteOrder::swapIfBigEndian(iceAgent.tieBreaker);

    STUNAttributeHeader iceControllingHeader;
    iceControllingHeader.type = juce::ByteOrder::swapIfBigEndian((uint16_t)0x802A);  // CONTROLLING
    iceControllingHeader.length = juce::ByteOrder::swapIfBigEndian((uint16_t)8);

    stunRequest.append(&iceControllingHeader, sizeof(STUNAttributeHeader));
    stunRequest.append(&tieBreaker, sizeof(uint64_t));
  }

  // Update message length
  header = (STUNHeader*)stunRequest.getData();
  header->messageLength = juce::ByteOrder::swapIfBigEndian(
    static_cast<uint16_t>(stunRequest.getSize() - sizeof(STUNHeader))
  );

  // Send STUN request to remote candidate
  juce::DatagramSocket testSocket;
  if (!testSocket.bindToPort(0)) {
    DBG("ICE: Failed to bind test socket");
    return false;
  }

  // Send request
  int bytesSent = testSocket.write(
    pair.remote.connectionAddress,
    pair.remote.port,
    stunRequest.getData(),
    static_cast<int>(stunRequest.getSize())
  );

  if (bytesSent <= 0) {
    DBG("ICE: Failed to send STUN request to " +
        pair.remote.connectionAddress + ":" + juce::String(pair.remote.port));
    return false;
  }

  // Wait for response with timeout
  constexpr int CHECK_TIMEOUT_MS = 500;
  auto startTime = juce::Time::currentTimeMillis();

  while ((juce::Time::currentTimeMillis() - startTime) < CHECK_TIMEOUT_MS) {
    if (testSocket.waitUntilReady(true, 100) > 0) {
      char buffer[512];
      juce::String responseIP;
      int responsePort;

      int bytesRead = testSocket.read(buffer, sizeof(buffer), false, responseIP, responsePort);

      if (bytesRead > 0) {
        // Parse STUN response
        auto result = stunClient->parseBindingResponse(buffer, bytesRead, pair.remote.connectionAddress);

        if (result.success) {
          // Check if transaction ID matches
          STUNHeader* responseHeader = (STUNHeader*)buffer;
          bool txIdMatches = std::memcmp(
            responseHeader->transactionId,
            pair.transactionId.data(),
            12
          ) == 0;

          if (txIdMatches) {
            DBG("ICE: Connectivity check SUCCEEDED for " +
                pair.local.connectionAddress + ":" + juce::String(pair.local.port) +
                " -> " + pair.remote.connectionAddress + ":" + juce::String(pair.remote.port));
            return true;
          } else {
            DBG("ICE: Transaction ID mismatch, ignoring response");
          }
        }
      }
    }
  }

  DBG("ICE: Connectivity check timed out for " +
      pair.local.connectionAddress + ":" + juce::String(pair.local.port) +
      " -> " + pair.remote.connectionAddress + ":" + juce::String(pair.remote.port));
  return false;
}
