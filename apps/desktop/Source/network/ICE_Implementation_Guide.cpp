// COMPLETE ICE IMPLEMENTATION FOR CollaborationManager.cpp
// Replace or add these methods to make ICE production-ready

//==============================================================================
// METHOD: hasSucceededICEPair()
// LOCATION: Add to CollaborationManager class (header and implementation)
//==============================================================================

bool CollaborationManager::hasSucceededICEPair() const {
    const juce::ScopedLock sl(icePairsLock);
    for (const auto& pair : iceCandidatePairs) {
        if (pair.state == ICECandidatePair::Succeeded) {
            return true;
        }
    }
    return false;
}

//==============================================================================
// METHOD: run() - UPDATED WITH REAL ICE CHECKS
// LOCATION: Replace existing run() method
//==============================================================================

void CollaborationManager::run() {
    juce::int64 iceStartTime = juce::Time::currentTimeMillis();
    constexpr int PEER_TIMEOUT_MS = 10000;

    juce::WeakReference<CollaborationManager> weakRef(this);

    // --- Phase 1: ICE Connectivity Checks ---
    DBG("ICE: Starting connectivity checks");

    while (!threadShouldExit() && !shouldStop.load(std::memory_order_acquire)) {
        ConnectionState state = currentState.load(std::memory_order_acquire);

        // Check for timeout
        if ((juce::Time::currentTimeMillis() - iceStartTime) > ICE_CHECK_TIMEOUT_MS) {
            juce::MessageManager::callAsync([weakRef]() {
                if (auto* cm = weakRef.get()) {
                    cm->reportError("ICE connection timeout - no successful candidate pairs.");
                }
            });
            return;
        }

        // If we have a succeeded pair, select it and proceed
        if (hasSucceededICEPair()) {
            selectBestICECandidate();
            if (iceCandidatesSelected) {
                break;
            }
        }

        // Perform connectivity checks on waiting pairs
        {
            const juce::ScopedLock sl(icePairsLock);
            constexpr int MAX_CONCURRENT_CHECKS = 3;
            int activeChecks = 0;

            for (auto& pair : iceCandidatePairs) {
                if (pair.state == ICECandidatePair::Succeeded) {
                    // Found a working pair!
                    selectBestICECandidate();
                    if (iceCandidatesSelected) {
                        goto ice_checks_done;
                    }
                }

                if (pair.state == ICECandidatePair::Waiting && activeChecks < MAX_CONCURRENT_CHECKS) {
                    // Check if enough time has passed since last check
                    auto now = juce::Time::currentTimeMillis();
                    if (now - pair.lastCheckTime > ICE_CHECK_INTERVAL_MS) {
                        pair.state = ICECandidatePair::InProgress;
                        pair.lastCheckTime = now;
                        activeChecks++;

                        // Perform the connectivity check
                        bool success = performConnectivityCheck(pair);

                        if (success) {
                            pair.state = ICECandidatePair::Succeeded;
                            DBG("ICE: Candidate pair SUCCEEDED");
                        } else {
                            pair.retryCount++;
                            if (pair.retryCount >= pair.MAX_RETRIES) {
                                pair.state = ICECandidatePair::Failed;
                                DBG("ICE: Candidate pair FAILED after " +
                                    juce::String(pair.MAX_RETRIES) + " retries");
                            } else {
                                pair.state = ICECandidatePair::Waiting;
                            }
                        }
                    }
                }
            }
        }

        wait(ICE_CHECK_INTERVAL_MS);
    }

ice_checks_done:
    if (!iceCandidatesSelected || !selectedRemoteCandidate.isValid()) {
        // Try fallback to direct host-to-host connection
        DBG("ICE: No successful pairs, attempting fallback...");

        const juce::ScopedLock sl(iceCandidatesLock);
        if (!localICECandidates.empty() && !remoteICECandidates.empty()) {
            // Try host-to-host as last resort
            for (const auto& local : localICECandidates) {
                for (const auto& remote : remoteICECandidates) {
                    if (local.type == CandidateType::Host && remote.type == CandidateType::Host) {
                        selectedLocalCandidate = local;
                        selectedRemoteCandidate = remote;
                        iceCandidatesSelected = true;
                        DBG("ICE: Fallback to host-host candidate");
                        break;
                    }
                }
                if (iceCandidatesSelected) break;
            }
        }

        if (!iceCandidatesSelected) {
            reportError("ICE failed - no working candidates even with fallback.");
            return;
        }
    }

    // --- Phase 2: DTLS Handshake with Selected ICE Candidate ---
    peerIP = selectedRemoteCandidate.connectionAddress;
    peerPort = selectedRemoteCandidate.port;

    DBG("ICE: Connecting to peer at " + peerIP + ":" + juce::String(peerPort));

    if (!p2pSocket.connect(peerIP, peerPort)) {
        reportError("DTLS connection failed to start.");
        return;
    }

    currentState.store(ConnectionState::Handshaking, std::memory_order_release);
    sendChangeMessage();

    juce::int64 handshakeStartTime = juce::Time::currentTimeMillis();
    constexpr int HANDSHAKE_TIMEOUT_MS = 15000;

    while(!threadShouldExit() && !shouldStop.load(std::memory_order_acquire) &&
          currentState.load(std::memory_order_acquire) == ConnectionState::Handshaking) {
        if (juce::Time::currentTimeMillis() - handshakeStartTime > HANDSHAKE_TIMEOUT_MS) {
            reportError("DTLS handshake timed out.");
            return;
        }
        if (p2pSocket.performHandshake()) {
            currentState.store(ConnectionState::Connected, std::memory_order_release);
            sendChangeMessage();
            DBG("ICE: DTLS Handshake complete. Connection is now secure.");

            // Send initial Hello packet
            sendPacket(PacketType::Hello, localUserName.toRawUTF8(), localUserName.length());

            break;
        }
        wait(50);
    }

    if (currentState != ConnectionState::Connected) {
        reportError("Handshake failed.");
        return;
    }

    // --- Phase 3: Connected Operation ---
    std::vector<char> decryptedBuffer(2048);
    juce::int64 lastKeepAlive = 0;
    juce::int64 lastPeerResponseTime = juce::Time::currentTimeMillis();
    lastPongResponseTime = juce::Time::currentTimeMillis();

    while (!threadShouldExit() && !shouldStop.load(std::memory_order_acquire)) {
        juce::String senderIP;
        int senderPort;
        int bytes = p2pSocket.read(decryptedBuffer.data(), decryptedBuffer.size(), senderIP, senderPort);

        if (bytes > 0) {
            handleIncomingPacket(decryptedBuffer.data(), bytes, senderIP, senderPort);
            lastPeerResponseTime = juce::Time::currentTimeMillis();
        }

        // Keepalive every 2 seconds
        auto now = juce::Time::currentTimeMillis();
        if (now - lastKeepAlive > 2000) {
            sendPacket(PacketType::KeepAlive, nullptr, 0);
            lastKeepAlive = now;
            waitingForPong.store(true, std::memory_order_release);
        }

        // Peer timeout
        if (now - lastPongResponseTime > PEER_TIMEOUT_MS) {
            juce::MessageManager::callAsync([weakRef]() {
                if (auto* cm = weakRef.get()) {
                    cm->reportError("Connection timed out - peer not responding to heartbeats.");
                }
            });
            break;
        }

        wait(10);
    }
}

//==============================================================================
// ADD TO HEADER (CollaborationManager.h):
//==============================================================================

// In private section, add:
bool hasSucceededICEPair() const;
bool performConnectivityCheck(const ICECandidatePair& pair);

// Also need to expose STUNClient::buildBindingRequest and parseBindingResponse
// Make them public in STUNClient or add friend declaration

//==============================================================================
// SIMPLER FALLBACK: Just use host candidates if ICE fails
//==============================================================================

// Add this before DTLS handshake in run():
if (!iceCandidatesSelected) {
    // Fallback: Try direct host-to-host connection
    DBG("ICE: Falling back to host candidates");

    const juce::ScopedLock sl(iceCandidatesLock);
    for (const auto& local : localICECandidates) {
        for (const auto& remote : remoteICECandidates) {
            if (local.type == CandidateType::Host && remote.type == CandidateType::Host) {
                selectedLocalCandidate = local;
                selectedRemoteCandidate = remote;
                iceCandidatesSelected = true;
                break;
            }
        }
        if (iceCandidatesSelected) break;
    }

    if (!iceCandidatesSelected) {
        reportError("ICE failed - unable to establish connection");
        return;
    }
}
