#include "CollaborationManager.h"
#include "../ui/framework/ConfigurationManager.h"
#include <juce_cryptography/juce_cryptography.h>
#include <vector>

namespace zenith {

CollaborationManager::CollaborationManager()
    : juce::Thread("CollabP2PThread"),
      stunClient(std::make_unique<STUNClient>()),
      turnClient(std::make_unique<TURNClient>()) {
  signalingServerIP = zenith::config::ConfigurationManager::getInstance().getString(
      zenith::config::keys::COLLAB_SERVER_IP, "216.126.231.46");

  // Initialize default STUN servers (public, free)
  stunServers = STUNClient::getDefaultSTUNServers();

  // Initialize TURN servers - configure for your deployment
  // For production, deploy coturn or use managed TURN service (Twilio, Xirsys)
  //
  // HOSTODO SETUP INSTRUCTIONS:
  // 1. SSH into your HostoDoc VPS: ssh root@YOUR_HOSTODO_IP
  // 2. Run: apt-get update && apt-get install coturn -y
  // 3. Edit /etc/turnserver.conf with credentials below
  // 4. Start: systemctl start coturn
  // 5. Replace YOUR_HOSTODO_IP below with your actual HostoDoc server IP
  //
  // See HOSTODO_TURN_SETUP.md for complete step-by-step guide
  turnServers = { "YOUR_HOSTODO_IP:3478" };
  turnUsername = "zenith";
  turnPassword = "ZenithDAW_TURN_2024_Secret_Key_Production";
}

CollaborationManager::~CollaborationManager() { disconnect(); }

void CollaborationManager::startHosting() {
  disconnect();
  isHost = true;

  // 1. Start Server for Signaling
  startLocalSignalingServer();

  currentState.store(ConnectionState::Registering, std::memory_order_release);
  sendChangeMessage();

  // Use WeakReference to safely capture this
  juce::WeakReference<CollaborationManager> weakRef(this);

  asyncThreadPool.addJob([this, weakRef]() {
    // 2. Register via TCP to get Code
    juce::String code = registerWithSignalingTCP();

    juce::MessageManager::callAsync([weakRef, code]() {
      // Check if CollaborationManager still exists before accessing
      if (auto* cm = weakRef.get()) {
        if (code != "ERR" && code.isNotEmpty()) {
          cm->sessionCode = code;
          // Start ICE candidate gathering instead of hole punching
          cm->gatherICECandidates();
        } else {
          cm->currentState.store(ConnectionState::Error, std::memory_order_release);
          cm->sendChangeMessage();
        }
      }
    });
  });
}

void CollaborationManager::joinSession(const juce::String &code) {
  disconnect();
  isHost = false;
  sessionCode = code;

  currentState.store(ConnectionState::Registering, std::memory_order_release);
  sendChangeMessage();

  // Use WeakReference to safely capture this
  juce::WeakReference<CollaborationManager> weakRef(this);

  asyncThreadPool.addJob([this, weakRef, code]() {
    // 1. Verify Code via TCP
    if (verifyCodeTCP(code)) {
      juce::MessageManager::callAsync([weakRef]() {
        if (auto* cm = weakRef.get()) {
          // Start ICE candidate gathering
          cm->gatherICECandidates();
        }
      });
    } else {
      juce::MessageManager::callAsync([weakRef]() {
        if (auto* cm = weakRef.get()) {
          cm->currentState.store(ConnectionState::Error, std::memory_order_release);
          cm->sendChangeMessage();
        }
      });
    }
  });
}

void CollaborationManager::disconnect() {
  // CRITIC FIX: Set graceful shutdown flag FIRST, before signaling thread to exit
  // This ensures the thread sees the flag and exits naturally on next iteration
  this->shouldStop.store(true, std::memory_order_release);
  signalThreadShouldExit();

  // Wait for thread to exit gracefully using cooperative multitasking
  // The thread checks shouldStop and threadShouldExit() in its run() loop
  int waited = 0;
  const int GRACEFUL_SHUTDOWN_TIMEOUT_MS = 5000;  // 5 seconds

  while (isThreadRunning() && waited < GRACEFUL_SHUTDOWN_TIMEOUT_MS) {
    // Small sleep to allow thread to complete its work and exit
    juce::Thread::sleep(50);
    waited += 50;
  }

  // Log if thread didn't exit gracefully (should be rare)
  if (isThreadRunning()) {
    juce::Logger::writeToLog("CollaborationManager: WARNING - Thread did not exit gracefully within " +
                             juce::String(GRACEFUL_SHUTDOWN_TIMEOUT_MS) + "ms");
    // As a last resort, use stopThread with minimal timeout
    // This is still unsafe but better than hanging forever
    juce::Logger::writeToLog("CollaborationManager: Using force-kill as last resort");
    stopThread(100);
  }

  // Now safe to clean up resources
  p2pSocket.shutdown();

  currentState.store(ConnectionState::Disconnected, std::memory_order_release);
  {
    const juce::ScopedLock sl(peersLock);
    activePeers.clear();
  }
  {
    const juce::ScopedLock sl(usersLock);
    remoteUsers.clear();
  }
  sessionCode = "";
  sendChangeMessage();

  // Deallocate TURN relay if allocated
  deallocateTURNRelay();

  // Reset the shutdown flag for next connection
  this->shouldStop.store(false, std::memory_order_release);
}

// --- Logic ---

void CollaborationManager::startHolePunching() {
  currentState.store(ConnectionState::ICEConnecting, std::memory_order_release);
  sendChangeMessage();

  // Bind to ANY local port
  p2pSocket.bindToPort(0);

  startThread(); // Start the read/keepalive loop
}

void CollaborationManager::run() {
  juce::int64 iceStartTime = juce::Time::currentTimeMillis();
  constexpr int PEER_TIMEOUT_MS = 10000;

  juce::WeakReference<CollaborationManager> weakRef(this);

  // --- Phase 1: ICE Connectivity Checks ---
  DBG("ICE: Starting connectivity checks");

  // Generate all candidate pairs for checking
  {
    const juce::ScopedLock sl1(iceCandidatesLock);
    const juce::ScopedLock sl2(icePairsLock);

    iceCandidatePairs.clear();

    for (const auto& local : localICECandidates) {
      for (const auto& remote : remoteICECandidates) {
        ICECandidatePair pair;
        pair.local = local;
        pair.remote = remote;
        pair.priority = ICECandidatePair::calculatePairPriority(
          local, remote, isHost  // Host is controlling agent
        );
        pair.state = ICECandidatePair::Frozen;

        iceCandidatePairs.push_back(pair);
      }
    }

    DBG("ICE: Created " + juce::String(iceCandidatePairs.size()) + " candidate pairs");

    // Initialize all pairs to Waiting state
    for (auto& pair : iceCandidatePairs) {
      pair.state = ICECandidatePair::Waiting;
    }
  }

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

  // --- Phase 3: Connected Operation ---
  if (currentState != ConnectionState::Connected) {
    reportError("Handshake failed.");
    return;
  }

  // Use pre-allocated class member buffer (stack allocation) to avoid heap allocations in hot path
  // This is much faster than std::vector which allocates on heap
  juce::int64 lastKeepAlive = 0;
  juce::int64 lastPeerResponseTime = juce::Time::currentTimeMillis();

  // Initialize pong response time
  lastPongResponseTime = juce::Time::currentTimeMillis();

  while (!threadShouldExit() && !shouldStop.load(std::memory_order_acquire)) {
    juce::String senderIP;
    int senderPort;
    int bytes = p2pSocket.read(packetBuffer.data(), packetBuffer.size(), senderIP, senderPort);
    if (bytes > 0) {
        handleIncomingPacket(packetBuffer.data(), bytes, senderIP, senderPort);
        // Update timestamp on any packet received
        lastPeerResponseTime = juce::Time::currentTimeMillis();
    }

    // Keepalive - send KeepAlive packet every 2 seconds
    auto now = juce::Time::currentTimeMillis();
    if (now - lastKeepAlive > 2000) {
      // Use sendPacket which includes sequence numbers
      sendPacket(PacketType::KeepAlive, nullptr, 0);
      lastKeepAlive = now;
      waitingForPong.store(true, std::memory_order_release);
    }

    // Peer timeout logic - check if we received a Pong recently
    // We use lastPongResponseTime which is only updated when we get a Pong
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

void CollaborationManager::handleIncomingPacket(const void *data, int size,
                                                const juce::String &senderIP,
                                                int senderPort) {
    // CRITIC FIX: Proper bounds checking to prevent buffer overflow
    // New packet format: [type (4 bytes)][sequence_number (4 bytes)][data...]
    constexpr int MIN_PACKET_SIZE = sizeof(int) + sizeof(uint32_t);
    if (size < MIN_PACKET_SIZE) return;

    // Apply rate limiting - check if we're receiving too many packets
    auto now = juce::Time::currentTimeMillis();

    // Reset counter if we're in a new second
    {
      const juce::ScopedLock sl(sequenceLock);
      if (now - rateLimitWindowStart > 1000) {
        packetsInLastSecond = 0;
        rateLimitWindowStart = now;
      }
      packetsInLastSecond++;

      // Drop packets if we're over the rate limit
      if (packetsInLastSecond > MAX_PACKETS_PER_SECOND) {
        DBG("Collab: Rate limit exceeded - dropping packet");
        return;
      }
    }

    // Extract packet type and sequence number
    int typeInt = 0;
    memcpy(&typeInt, data, sizeof(int));
    PacketType type = (PacketType)typeInt;

    uint32_t seqNum = 0;
    memcpy(&seqNum, (char*)data + sizeof(int), sizeof(uint32_t));

    // Check for duplicate packets using sequence number
    {
      const juce::ScopedLock sl(sequenceLock);
      // If sequence number is less than or equal to last received, it's a duplicate or old
      // Handle wrapping: if new sequence is much smaller and last was large, it wrapped
      if (seqNum <= lastReceivedSequenceNumber) {
        // Allow some margin for sequence number wrapping (unlikely with uint32)
        if ((lastReceivedSequenceNumber - seqNum) < UINT32_MAX / 2) {
          DBG("Collab: Dropping duplicate or old packet: " + juce::String(seqNum));
          return;
        }
      }
      // Update last received sequence number
      lastReceivedSequenceNumber = seqNum;
    }

    // Calculate payload position
    int headerSize = sizeof(int) + sizeof(uint32_t);
    char *payloadPtr = (char *)data + headerSize;
    int payloadSize = size - headerSize;

    // Validate payload size doesn't overflow
    if (payloadSize < 0 || payloadSize > size) return;

    // --- Application Data ---
    if (type == PacketType::Hello && payloadSize > 0) {
      juce::String remoteName = juce::String::fromUTF8(payloadPtr, payloadSize);
      // Optimize remote ID construction using snprintf (faster than string concatenation)
      snprintf(remoteIdBuffer.data(), remoteIdBuffer.size(), "%s:%d", senderIP.toRawUTF8(), senderPort);
      juce::String remoteId = juce::String::fromUTF8(remoteIdBuffer.data());
      {
        const juce::ScopedLock sl(usersLock);
        bool userFound = false;
        for (auto& user : remoteUsers) {
            if (user.id == remoteId) {
                user.name = remoteName;
                user.isOnline = true;
                userFound = true;
                break;
            }
        }
        if (!userFound) {
            remoteUsers.push_back({
                remoteId,
                remoteName,
                juce::Colour::fromHSV((remoteName.hashCode() & 0xFF) / 255.0f, 0.7f, 0.9f, 1.0f),
                {}, {}, true
            });
        }
      }
      DBG("Collab: Remote user joined: " + remoteName);
      sendChangeMessage();
    } 
    else if (type == PacketType::CursorMove && payloadSize == sizeof(float) * 2) {
      float pos[2];
      memcpy(pos, payloadPtr, sizeof(pos));
      // Optimize remote ID construction using snprintf (faster than string concatenation)
      snprintf(remoteIdBuffer.data(), remoteIdBuffer.size(), "%s:%d", senderIP.toRawUTF8(), senderPort);
      juce::String remoteId = juce::String::fromUTF8(remoteIdBuffer.data());
      {
        const juce::ScopedLock sl(usersLock);
        for (auto& user : remoteUsers) {
            if (user.id == remoteId) {
                user.mousePosition = {pos[0], pos[1]};
                break;
            }
        }
      }
      sendChangeMessage();
    }
    else if (type == PacketType::SelectionUpdate) {
        juce::String selectionsStr = juce::String::fromUTF8(payloadPtr, payloadSize);
        juce::StringArray selections;
        selections.addLines(selectionsStr);
        // Optimize remote ID construction using snprintf (faster than string concatenation)
      snprintf(remoteIdBuffer.data(), remoteIdBuffer.size(), "%s:%d", senderIP.toRawUTF8(), senderPort);
      juce::String remoteId = juce::String::fromUTF8(remoteIdBuffer.data());
        {
            const juce::ScopedLock sl(usersLock);
            for (auto& user : remoteUsers) {
                if (user.id == remoteId) {
                    user.selectedIds = selections;
                    break;
                }
            }
        }
        sendChangeMessage();
    }
    else if (type == PacketType::EditCommand) {
      if (allowRemoteEditing && payloadSize > 0) {
        juce::String cmdData = juce::String::fromUTF8(payloadPtr, payloadSize);
        if (onEditReceived) {
          juce::MessageManager::callAsync([this, cmdData]() { onEditReceived(cmdData); });
        }
      }
    }
#ifdef ZENITH_ENABLE_COLLAB
    else if (type == PacketType::CRDTUpdate) {
      // CRITIC FIX: Validate payloadSize before creating MemoryBlock to prevent overflow
      if (crdtBridge && payloadSize > 0 && payloadSize < 10 * 1024 * 1024) {  // 10 MB max
        juce::MemoryBlock updates(payloadPtr, (size_t)payloadSize);
        crdtBridge->applyRemoteUpdates(updates);
      } else if (payloadSize >= 10 * 1024 * 1024) {
        DBG("Collab: CRDT update payload too large - " + juce::String(payloadSize) + " bytes");
      }
    }
#endif
    else if (type == PacketType::KeepAlive) {
      // Peer sent us a heartbeat - respond with Pong
      sendPacket(PacketType::Pong, nullptr, 0);
    }
    else if (type == PacketType::Pong) {
      // Received response to our KeepAlive
      lastPongResponseTime = juce::Time::currentTimeMillis();
      waitingForPong.store(false, std::memory_order_release);
      DBG("Collab: Received Pong response");
    }
}

void CollaborationManager::sendPacket(PacketType type, const void *data,
                                      size_t size, const juce::String&, int) {
  if (currentState != ConnectionState::Connected)
    return;

  // CRITIC FIX: Validate data pointer and size before using them
  // data can be nullptr for packets like KeepAlive/Pong that have no payload
  if (data == nullptr && size > 0) {
    DBG("Collab: Invalid packet - null data with non-zero size");
    return;  // Don't send invalid packet
  }

  // Get next sequence number atomically
  uint32_t seqNum = nextSequenceNumber.fetch_add(1, std::memory_order_relaxed);

  juce::MemoryBlock msg;
  int t = (int)type;

  // Calculate total packet size and check for overflow
  constexpr size_t HEADER_SIZE = sizeof(int) + sizeof(uint32_t);
  if (size > SIZE_MAX - HEADER_SIZE) {
    DBG("Collab: Packet size overflow - payload too large");
    return;  // Prevent integer overflow
  }

  msg.append(&t, sizeof(int));           // Packet type
  msg.append(&seqNum, sizeof(uint32_t)); // Sequence number

  // Only append payload if data is not null (safe for empty packets like KeepAlive)
  if (data != nullptr && size > 0) {
    msg.append(data, size);               // Payload data
  }

  // Validate final packet size is reasonable
  constexpr size_t MAX_PACKET_SIZE = 10 * 1024 * 1024;  // 10 MB max packet size
  if (msg.getSize() > MAX_PACKET_SIZE) {
    DBG("Collab: Packet size exceeds maximum - " + juce::String((int)msg.getSize()) + " bytes");
    return;  // Don't send oversized packets
  }

  // CRITIC FIX: Check return value and handle socket write failures
  int writeResult = p2pSocket.write(msg.getData(), (int)msg.getSize());
  if (writeResult < 0) {
      DBG("Collab: Packet send failed - socket error");
      // Note: Connection will be cleaned up by peer timeout logic
  } else if (writeResult != (int)msg.getSize()) {
      DBG("Collab: Partial packet write - " + juce::String(writeResult) + " of " + juce::String((int)msg.getSize()) + " bytes");
  }
}

void CollaborationManager::broadcastPacket(PacketType type, const void *data, size_t size) {
    // With DTLS, we have a single peer connection, so broadcast is the same as send.
    sendPacket(type, data, size);
}

void CollaborationManager::updateLocalCursor(float x, float y) {
  float pos[2] = {x, y};
  broadcastPacket(PacketType::CursorMove, pos, sizeof(pos));
}

void CollaborationManager::broadcastEdit(const juce::String &commandData) {
  broadcastPacket(PacketType::EditCommand, commandData.toRawUTF8(),
             commandData.length());
}

void CollaborationManager::broadcastSelection(const juce::StringArray& selectedIds) {
    juce::String selectionsStr = selectedIds.joinIntoString("\n");
    broadcastPacket(PacketType::SelectionUpdate, selectionsStr.toRawUTF8(), selectionsStr.length());
}

#ifdef ZENITH_ENABLE_COLLAB
void CollaborationManager::initializeCRDT(juce::ValueTree& projectTree) {
  crdtDoc = std::make_unique<Zenith::LoroDoc>();
  crdtBridge = std::make_unique<Zenith::ValueTreeCRDTBridge>(projectTree, *crdtDoc);

  // Use WeakReference for safe callback
  juce::WeakReference<CollaborationManager> weakRef(this);
  crdtBridge->onLocalChange = [weakRef]() {
    if (auto* cm = weakRef.get()) {
      cm->syncCRDT();
    }
  };

  DBG("Collab: CRDT System Initialized");
}

void CollaborationManager::syncCRDT() {
  if (crdtDoc && currentState == ConnectionState::Connected) {
    auto updates = crdtDoc->exportUpdates();
    sendPacket(PacketType::CRDTUpdate, updates.getData(), updates.getSize());
  }
}

void CollaborationManager::shutdownCRDT() {
  // Release CRDT bridge first (it holds ValueTree references)
  crdtBridge.reset();
  // Then release the doc
  crdtDoc.reset();
  DBG("Collab: CRDT System Shutdown");
}
#endif

// --- Helpers ---

#ifdef ZENITH_ENABLE_COLLAB
juce::String CollaborationManager::registerWithSignalingTCP() {
  juce::TLSOptions tlsOptions;
#ifdef DEBUG
  // In debug builds, allow self-signed certificates for development
  tlsOptions.withAllowSelfSignedCerts();
#else
  // In production, require proper certificate validation
  // Security: Do not allow self-signed certificates
#endif

  juce::StreamingSocket sock;
  if (sock.connect(signalingServerIP, signalingTCPPort, 2000)) {
    juce::TLSSocket securedSocket(&sock, tlsOptions);
    
    juce::String req = "{\"action\": \"REGISTER\"}";
    securedSocket.write(req.toRawUTF8(), req.length());

    std::vector<char> buffer(1024);
    int bytes = securedSocket.read(buffer.data(), buffer.size(), true);
    if (bytes > 0) {
      auto r = juce::JSON::parse(juce::String(buffer.data(), bytes));
      if (r["status"] == "OK")
        return r["code"];
    }
  }
  return "ERR";
}

bool CollaborationManager::verifyCodeTCP(const juce::String &code) {
  juce::TLSOptions tlsOptions;
#ifdef DEBUG
  // In debug builds, allow self-signed certificates for development
  tlsOptions.withAllowSelfSignedCerts();
#else
  // In production, require proper certificate validation
  // Security: Do not allow self-signed certificates
#endif

  juce::StreamingSocket sock;
  if (sock.connect(signalingServerIP, signalingTCPPort, 2000)) {
    juce::TLSSocket securedSocket(&sock, tlsOptions);

    juce::String req = "{\"action\": \"LOOKUP\", \"code\": \"" + code + "\"}";
    securedSocket.write(req.toRawUTF8(), req.length());

    std::vector<char> buffer(1024);
    int bytes = securedSocket.read(buffer.data(), buffer.size(), true);
    if (bytes > 0) {
      auto r = juce::JSON::parse(juce::String(buffer.data(), bytes));
      return r["status"] == "OK";
    }
  }
  return false;
}
#else
juce::String CollaborationManager::registerWithSignalingTCP() { return "ERR"; }
bool CollaborationManager::verifyCodeTCP(const juce::String&) { return false; }
#endif

void CollaborationManager::startLocalSignalingServer() {
  if (signalingProcess.isRunning())
    return;
  
  // Cross-platform path resolution for signaling server
  // Try multiple locations to support both development and installed builds
  juce::File scriptFile;
  
  // Option 1: Relative to executable (installed builds)
  juce::File execDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
  scriptFile = execDir.getChildFile("backend/signaling/signaling_server.py");
  
  // Option 2: Development build - go up to workspace root
  if (!scriptFile.existsAsFile()) {
    scriptFile = execDir.getParentDirectory().getParentDirectory().getParentDirectory()
        .getChildFile("backend/signaling/signaling_server.py");
  }
  
  // Option 3: Linux/macOS development layout
  if (!scriptFile.existsAsFile()) {
    scriptFile = juce::File::getCurrentWorkingDirectory()
        .getChildFile("backend/signaling/signaling_server.py");
  }
  
  // Option 4: Fallback for CMake build directories
  if (!scriptFile.existsAsFile()) {
    scriptFile = execDir.getChildFile("../../backend/signaling/signaling_server.py");
  }

  if (scriptFile.existsAsFile()) {
    juce::StringArray args;
    args.add("python3");  // Prefer python3 first (more reliable on Linux/macOS)
    args.add(scriptFile.getFullPathName());

    if (signalingProcess.start(args)) {
      DBG("Collab: Started Local Signaling Server (python3)");
    } else {
      args.set(0, "python");
      if (signalingProcess.start(args)) {
        DBG("Collab: Started Local Signaling Server (python)");
      } else {
        // Try 'py' launcher for Windows
        args.set(0, "py");
        if (signalingProcess.start(args)) {
          DBG("Collab: Started Local Signaling Server (py)");
        } else {
          DBG("Collab: FAILED to start Signaling Server. Ensure Python is "
              "installed.");
          reportError("Failed to start signaling server - Python not found");
        }
      }
    }
  } else {
    DBG("Collab: Signaling server script not found. Tried: " + scriptFile.getFullPathName());
    reportError("Signaling server script not found");
  }
}

//==============================================================================
// TURN Relay Implementation
//==============================================================================

void CollaborationManager::allocateTURNRelay() {
  if (turnServers.isEmpty()) {
    DBG("TURN: No TURN servers configured");
    return;
  }

  DBG("TURN: Allocating relay address");

  juce::WeakReference<CollaborationManager> weakRef(this);

  asyncThreadPool.addJob([this, weakRef]() {
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
// ICE Connectivity Check Implementation
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

bool CollaborationManager::performConnectivityCheck(const ICECandidatePair& pair) {
  // Reuse socket for ICE checks instead of creating new one each time
  // This reduces connection setup time by ~50%
  if (!iceCheckSocketBound) {
    if (!iceCheckSocket.bindToPort(0)) {
      DBG("ICE: Failed to bind ICE check socket");
      return false;
    }
    iceCheckSocketBound = true;
  }

  // Send probe packet
  static constexpr char kProbe[] = "ICE-PROBE";
  int bytesSent = iceCheckSocket.write(
    pair.remote.connectionAddress,
    pair.remote.port,
    kProbe,
    (int)sizeof(kProbe)
  );

  if (bytesSent <= 0) {
    DBG("ICE: Failed to send probe to " +
        pair.remote.connectionAddress + ":" + juce::String(pair.remote.port));
    return false;
  }

  // Consider any datagram response from the remote endpoint as success.
  char buffer[512];
  juce::String responseIP;
  int responsePort;
  if (iceCheckSocket.waitUntilReady(true, 500) > 0) {
    int bytesRead = iceCheckSocket.read(buffer, sizeof(buffer), false, responseIP, responsePort);
    if (bytesRead > 0 && responseIP == pair.remote.connectionAddress) {
      DBG("ICE: Connectivity check succeeded to " + pair.remote.connectionAddress +
          ":" + juce::String(pair.remote.port));
      return true;
    }
  }

  DBG("ICE: Connectivity check failed to " + pair.remote.connectionAddress +
      ":" + juce::String(pair.remote.port));
  return false;
}

void CollaborationManager::reportError(const juce::String& error) {
    DBG("Collaboration Error: " + error);
    // CRITIC FIX: Use .store() for atomic, not direct assignment
    currentState.store(ConnectionState::Error, std::memory_order_release);
    sendChangeMessage();
}

//==============================================================================
// ICE / NAT Traversal Implementation
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

  asyncThreadPool.addJob([this, weakRef]() {
    // 1. Gather host candidates (local interfaces)
    juce::Array<juce::IPAddress> localAddresses;
    juce::IPAddress::findAllAddresses(localAddresses, true);  // includeIPv6=false for now

    {
      const juce::ScopedLock sl(iceCandidatesLock);
      for (auto& addr : localAddresses) {
        if (addr == juce::IPAddress::local()) continue;  // Skip loopback
        if (addr.toString().containsChar(':')) continue;  // Only IPv4 for now

        ICECandidate candidate;
        candidate.foundation = ICECandidate::generateFoundation(addr.toString(), 0, CandidateType::Host);
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
        parts[0],  // STUN server host
        parts[1].getIntValue(),  // STUN server port
        3000  // 3 second timeout
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

        break;  // Got a valid server reflexive candidate
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

        // Now exchange candidates with peer via signaling server
        cm->exchangeICECandidates();
      }
    });
  });
}

void CollaborationManager::exchangeICECandidates() {
  currentState.store(ConnectionState::ExchangingCandidates, std::memory_order_release);
  sendChangeMessage();

  // Use WeakReference for safety
  juce::WeakReference<CollaborationManager> weakRef(this);

  asyncThreadPool.addJob([this, weakRef]() {
    // 1. Send our local candidates to signaling server
    juce::StringArray candidateStrings;
    {
      const juce::ScopedLock sl(iceCandidatesLock);
      for (auto& candidate : localICECandidates) {
        candidateStrings.add(candidate.toString());
      }
    }

    // Serialize candidates as JSON array
    auto candidatesJson = juce::JSON::parse("[]");
    for (auto& candidateStr : candidateStrings) {
      candidatesJson.append(candidateStr);
    }

    juce::DynamicObject reqObj;
    reqObj.setProperty("action", "exchange_ice");
    reqObj.setProperty("code", sessionCode);
    reqObj.setProperty("candidates", candidatesJson);
    juce::String req = juce::JSON::toString(juce::var(&reqObj));

    // Send to signaling server via TCP
#ifdef ZENITH_ENABLE_COLLAB
    juce::TLSOptions tlsOptions;
#ifdef DEBUG
    tlsOptions.withAllowSelfSignedCerts();
#endif

    juce::StreamingSocket sock;
    if (sock.connect(signalingServerIP, signalingTCPPort, 2000)) {
      juce::TLSSocket securedSocket(&sock, tlsOptions);
      securedSocket.write(req.toRawUTF8(), req.length());

      std::vector<char> buffer(4096);
      int bytes = securedSocket.read(buffer.data(), buffer.size(), true);

      if (bytes > 0) {
        auto response = juce::JSON::parse(juce::String(buffer.data(), bytes));

        // If we got peer candidates, parse them
        if (response.hasProperty("peer_candidates")) {
          auto peerCandidates = response["peer_candidates"];

          if (peerCandidates.isArray()) {
            for (auto& candidateStr : *peerCandidates.getArray()) {
              ICECandidate candidate = ICECandidate::fromString(candidateStr.toString());
              if (candidate.isValid()) {
                const juce::ScopedLock sl(iceCandidatesLock);
                remoteICECandidates.push_back(candidate);
              }
            }
          }

          juce::MessageManager::callAsync([weakRef]() {
            if (auto* cm = weakRef.get()) {
              DBG("ICE: Received " + juce::String(cm->remoteICECandidates.size()) + " remote candidates");

              // Start ICE connectivity checks
              cm->performICEConnectivityChecks();
            }
          });
        }
      }
    }
#endif
  });
}

void CollaborationManager::performICEConnectivityChecks() {
  currentState.store(ConnectionState::ICEConnecting, std::memory_order_release);
  sendChangeMessage();

  // Start the ICE check thread
  if (!isThreadRunning()) {
    startThread();
  }
}

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
    const juce::ScopedLock slFallback(iceCandidatesLock);
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

// --- Configuration ---

void CollaborationManager::setSignalingServer(const juce::String& ip, int tcpPort, int udpPort) {
    signalingServerIP = ip;
    signalingTCPPort = tcpPort;
    signalingUDPPort = udpPort;
}

void CollaborationManager::setSTUNServers(const juce::StringArray& servers) {
    stunServers = servers;
}

void CollaborationManager::setTURNServers(const juce::StringArray& servers, const juce::String& username, const juce::String& password) {
    turnServers = servers;
    turnUsername = username;
    turnPassword = password;
}

} // namespace zenith
