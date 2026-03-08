// TURN INTEGRATION FOR CollaborationManager
// Add these methods to CollaborationManager.cpp

//==============================================================================
// CONSTRUCTOR UPDATE
//==============================================================================

// In CollaborationManager constructor, add:
//   turnClient(std::make_unique<TURNClient>())

// And initialize TURN servers:
//   turnServers = { "turn.your-server.com:3478" };
//   turnUsername = "your-username";
//   turnPassword = "your-password";

//==============================================================================
// METHOD: allocateTURNRelay()
// Allocates a relay address from TURN server as fallback
//==============================================================================

void CollaborationManager::allocateTURNRelay() {
    if (turnServers.isEmpty()) {
        DBG("TURN: No TURN servers configured");
        return;
    }

    DBG("TURN: Allocating relay address");

    juce::WeakReference<CollaborationManager> weakRef(this);

    juce::Thread::launch([this, weakRef]() {
        for (auto& turnServerStr : turnServers) {
            auto parts = juce::StringArray::fromTokens(turnServerStr, ":", "");
            if (parts.size() != 2) continue;

            auto result = turnClient->allocateRelay(
                parts[0],  // TURN server host
                parts[1].getIntValue(),  // TURN server port
                turnUsername,
                turnPassword,
                10000  // 10 second timeout
            );

            juce::MessageManager::callAsync([this, weakRef, result]() {
                if (auto* cm = weakRef.get()) {
                    if (result.success) {
                        cm->turnAllocation = result;
                        cm->turnAllocated = true;

                        // Create relayed ICE candidate
                        ICECandidate relayCandidate = TURNClient::createRelayedCandidate(
                            result,
                            cm->p2pSocket.getInternalSocket().getBoundPort()
                        );

                        const juce::ScopedLock sl(cm->iceCandidatesLock);
                        cm->localICECandidates.push_back(relayCandidate);

                        DBG("TURN: Successfully allocated relay: " +
                            result.relayedAddress + ":" + juce::String(result.relayedPort));
                    } else {
                        DBG("TURN: Allocation failed: " + result.errorMessage);
                    }
                }
            });

            if (result.success) {
                break;  // Got a working TURN server
            }
        }
    });
}

//==============================================================================
// METHOD: deallocateTURNRelay()
// Releases TURN relay allocation when done
//==============================================================================

void CollaborationManager::deallocateTURNRelay() {
    if (!turnAllocated) {
        return;
    }

    DBG("TURN: Deallocating relay");

    if (turnClient->deallocate(turnAllocation, turnUsername, turnPassword)) {
        DBG("TURN: Successfully deallocated relay");
    } else {
        DBG("TURN: Failed to deallocate relay");
    }

    turnAllocated = false;
}

//==============================================================================
// UPDATED: gatherICECandidates()
// Now includes TURN relay candidate as fallback
//==============================================================================

void CollaborationManager::gatherICECandidates() {
    currentState.store(ConnectionState::GatheringICE, std::memory_order_release);
    sendChangeMessage();

    // Clear previous candidates
    {
        const juce::ScopedLock sl(iceCandidatesLock);
        localICECandidates.clear();
        remoteICECandidates.clear();
    }

    // Use WeakReference for safety
    juce::WeakReference<CollaborationManager> weakRef(this);

    juce::Thread::launch([this, weakRef]() {
        // 1. Gather host candidates (local interfaces)
        juce::Array<juce::IPAddress> localAddresses;
        juce::IPAddress::findAllAddresses(localAddresses, true);

        {
            const juce::ScopedLock sl(iceCandidatesLock);
            for (auto& addr : localAddresses) {
                if (addr == juce::IPAddress::local()) continue;
                if (!addr.isIPv4()) continue;

                ICECandidate candidate;
                candidate.foundation = ICECandidate::generateFoundation(
                    addr.toString(), 0, CandidateType::Host
                );
                candidate.componentId = 1;
                candidate.transport = TransportProtocol::UDP;
                candidate.priority = ICECandidate::calculatePriority(CandidateType::Host);
                candidate.connectionAddress = addr.toString();
                candidate.port = p2pSocket.getInternalSocket().getBoundPort();
                candidate.type = CandidateType::Host;

                localICECandidates.push_back(candidate);
            }
        }

        // 2. Gather server reflexive candidates via STUN
        for (auto& stunServerStr : stunServers) {
            auto parts = juce::StringArray::fromTokens(stunServerStr, ":", "");
            if (parts.size() != 2) continue;

            auto stunResult = stunClient->performBindingRequest(
                parts[0],
                parts[1].getIntValue(),
                3000
            );

            if (stunResult.success) {
                ICECandidate srflxCandidate = STUNClient::createServerReflexiveCandidate(
                    stunResult,
                    p2pSocket.getInternalSocket().getBoundPort()
                );

                const juce::ScopedLock sl(iceCandidatesLock);
                localICECandidates.push_back(srflxCandidate);

                DBG("ICE: Discovered server reflexive candidate: " +
                    srflxCandidate.connectionAddress + ":" + juce::String(srflxCandidate.port));

                break;
            }
        }

        // 3. Allocate TURN relay as fallback (for symmetric NAT)
        // Do this asynchronously so it doesn't block candidate exchange
        juce::MessageManager::callAsync([weakRef]() {
            if (auto* cm = weakRef.get()) {
                cm->allocateTURNRelay();
            }
        });

        // Update UI
        juce::MessageManager::callAsync([weakRef]() {
            if (auto* cm = weakRef.get()) {
                DBG("ICE: Gathered " + juce::String(cm->localICECandidates.size()) +
                    " local candidates (relay allocation pending)");

                // Exchange candidates now (don't wait for TURN)
                cm->exchangeICECandidates();
            }
        });
    });
}

//==============================================================================
// UPDATED: selectBestICECandidate()
// Now prioritizes relay candidates when direct connection fails
//==============================================================================

void CollaborationManager::selectBestICECandidate() {
    const juce::ScopedLock sl(icePairsLock);

    // Priority 1: Find succeeded direct connection pair (host, srflx)
    for (auto& pair : iceCandidatePairs) {
        if (pair.state == ICECandidatePair::Succeeded) {
            // Only select direct connections here (not relay)
            if (pair.local.type != CandidateType::Relayed &&
                pair.remote.type != CandidateType::Relayed) {
                selectedLocalCandidate = pair.local;
                selectedRemoteCandidate = pair.remote;
                iceCandidatesSelected = true;

                DBG("ICE: Selected WORKING direct candidate pair:");
                DBG("  Local: " + selectedLocalCandidate.connectionAddress + ":" +
                    juce::String(selectedLocalCandidate.port));
                DBG("  Remote: " + selectedRemoteCandidate.connectionAddress + ":" +
                    juce::String(selectedRemoteCandidate.port));

                pair.nominated = true;
                return;
            }
        }
    }

    // Priority 2: If no direct connection succeeded, use TURN relay
    if (turnAllocated && turnAllocation.success) {
        const juce::ScopedLock sl2(iceCandidatesLock);

        // Find relay candidate
        for (const auto& local : localICECandidates) {
            if (local.type == CandidateType::Relayed) {
                // Find best remote candidate (preferably also relay, but can be srflx)
                const juce::ScopedLock sl3(icePairsLock);

                // Get remote candidates from the first pair
                if (!iceCandidatePairs.empty()) {
                    selectedLocalCandidate = local;
                    selectedRemoteCandidate = iceCandidatePairs[0].remote;
                    iceCandidatesSelected = true;

                    DBG("ICE: Selected TURN RELAY candidate pair:");
                    DBG("  Local (Relay): " + selectedLocalCandidate.connectionAddress + ":" +
                        juce::String(selectedLocalCandidate.port));
                    DBG("  Remote: " + selectedRemoteCandidate.connectionAddress + ":" +
                        juce::String(selectedRemoteCandidate.port));

                    DBG("ICE: Using TURN relay - connection will be via " +
                        turnAllocation.relayedAddress);

                    return;
                }
            }
        }
    }

    // Priority 3: Last resort - try host-to-host even if not explicitly checked
    if (!iceCandidatesSelected && !localICECandidates.empty() && !remoteICECandidates.empty()) {
        for (const auto& local : localICECandidates) {
            if (local.type == CandidateType::Host) {
                for (const auto& remote : remoteICECandidates) {
                    if (remote.type == CandidateType::Host) {
                        selectedLocalCandidate = local;
                        selectedRemoteCandidate = remote;
                        iceCandidatesSelected = true;

                        DBG("ICE: Fallback to host-host candidate (may not work)");
                        return;
                    }
                }
                break;
            }
        }
    }

    DBG("ICE: No valid candidate pairs available");
}

//==============================================================================
// ADD TO DISCONNECT() METHOD
//==============================================================================

// At the end of CollaborationManager::disconnect(), add:
//   deallocateTURNRelay();

//==============================================================================
// DEPLOYMENT INSTRUCTIONS
//==============================================================================

/*
1. DEPLOY TURN SERVER (coturn):

   # Install coturn
   sudo apt-get install coturn  # Ubuntu/Debian
   # OR compile from source: https://github.com/coturn/coturn

   # Configure /etc/turnserver.conf
   listening-port=3478
   fingerprint
   lt-cred-mech
   user=username:password
   realm=zenithdaw

   # Start TURN server
   sudo turnserver -o -c /etc/turnserver.conf

2. UPDATE COLLABORATIONMANAGER:

   In constructor, set:
   turnServers = { "your-turn-server.com:3478" };
   turnUsername = "username";
   turnPassword = "password";

3. TEST TURN:

   - Verify TURN server is running:
     turnserver --server-name=your-turn-server.com -o -v

   - Test allocation:
     stunclient --mode turn --username=username --password=password
                your-turn-server.com:3478

4. PRODUCTION DEPLOYMENT:

   - Deploy TURN server on geographically distributed servers
   - Use TLS/DTLS for TURN connections
   - Monitor TURN server usage (relays cost bandwidth)
   - Set appropriate lifetime limits
   - Implement rate limiting per user

5. TURN PROVIDER OPTIONS:

   Self-hosted:
   - coturn (open source, free)
   - restund (open source)
   - turnerator (open source)

   Managed services:
   - Twilio Network Traversal Service
   - Xirsys TURN service
   - Nexmo TURN service
*/
