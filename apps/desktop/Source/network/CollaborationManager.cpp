#include "CollaborationManager.h"
#include "../ui/framework/ConfigurationManager.h"
#include <juce_cryptography/juce_cryptography.h>

namespace zenith {

CollaborationManager::CollaborationManager()
    : juce::Thread("CollabP2PThread") {}

CollaborationManager::~CollaborationManager() { disconnect(); }

void CollaborationManager::startHosting() {
  disconnect();
  isHost = true;

  // 1. Start Server for Signaling
  startLocalSignalingServer();

  currentState.store(ConnectionState::Registering, std::memory_order_release);
  sendChangeMessage();

  juce::Thread::launch([weakThis = juce::WeakReference<CollaborationManager>(this)]() {
    if (!weakThis) return;
    
    // 2. Register via TCP to get Code
    juce::String code = weakThis->registerWithSignalingTCP();

    juce::MessageManager::callAsync([weakThis, code]() {
      if (weakThis) {
          if (code != "ERR" && code.isNotEmpty()) {
            weakThis->sessionCode = code;
            weakThis->startHolePunching(); // Move to UDP phase
          } else {
            // Can't access direct atomic with weak ptr easily without accessor or friend, 
            // but we are inside the class scope effectively? No, lambda.
            // But we can call methods.
            weakThis->reportError("Registration failed");
          }
      }
    });
  });
}

void CollaborationManager::joinSession(const juce::String &code) {
  disconnect();
  
  // Strict Validation
  if (!validateSessionCode(code)) {
      juce::MessageManager::callAsync([this]() { reportError("Invalid session code format (must be 6-12 upper-case alphanumeric)"); });
      return;
  }

  isHost = false;
  sessionCode = code;

  currentState.store(ConnectionState::Registering, std::memory_order_release);
  sendChangeMessage();

  juce::Thread::launch([weakThis = juce::WeakReference<CollaborationManager>(this), code]() {
    if (!weakThis) return;

    // 1. Verify Code via TCP
    if (weakThis->verifyCodeTCP(code)) {
      juce::MessageManager::callAsync([weakThis]() { 
          if (weakThis) weakThis->startHolePunching(); 
      });
    } else {
      juce::MessageManager::callAsync([weakThis]() {
        if (weakThis) weakThis->reportError("Invalid session code");
      });
    }
  });
}

void CollaborationManager::disconnect() {
  signalThreadShouldExit();
  stopThread(1000);
  p2pSocket.shutdown();

  currentState.store(ConnectionState::Disconnected, std::memory_order_release);
  {
    const juce::ScopedLock sl(peersLock);
    activePeers.clear();
  }
  remoteUsers.clear();
  sessionCode = "";
  sendChangeMessage();
}

// --- Logic ---

void CollaborationManager::startHolePunching() {
  currentState.store(ConnectionState::Punching, std::memory_order_release);
  sendChangeMessage();

  // Bind to ANY local port
  p2pSocket.bindToPort(0);

  startThread(); // Start the read/keepalive loop
}

void CollaborationManager::run() {
  // 1. Punch Phase: Spam the Server
  juce::String punchMsg = (isHost ? "REGISTER:" : "JOIN:") + sessionCode;

  // Send initial punches to server to register our public port
  for (int i = 0; i < 5; ++i) {
    if (threadShouldExit())
      return;
    p2pSocket.write(SIGNALING_SERVER_IP, SIGNALING_UDP_PORT,
                    punchMsg.toRawUTF8(), (int)punchMsg.length());
    wait(200);
  }

  // Timeout tracking
  juce::int64 punchingStartTime = juce::Time::currentTimeMillis();
  juce::int64 handshakingStartTime = 0;
  constexpr int PUNCHING_TIMEOUT_MS = 30000;  // 30 seconds for hole punching
  constexpr int HANDSHAKING_TIMEOUT_MS = 15000;  // 15 seconds for handshake
  constexpr int PEER_TIMEOUT_MS = 10000;  // 10 seconds peer timeout

  // 2. Listen Loop
  char buffer[2048];
  juce::int64 lastKeepAlive = 0;
  
  while (!threadShouldExit()) {
    auto now = juce::Time::currentTimeMillis();
    
    // Read
    if (p2pSocket.waitUntilReady(true, 100) == 1) {
      juce::String senderIP;
      int senderPort;
      int bytes = p2pSocket.read(buffer, 2048, false, senderIP, senderPort);

      if (bytes > 0) {
        handleIncomingPacket(buffer, bytes, senderIP, senderPort);
      }
    }

    // === Timeout Checks ===
    auto state = currentState.load(std::memory_order_acquire);
    
    // Punching timeout
    if (state == ConnectionState::Punching) {
      if (now - punchingStartTime > PUNCHING_TIMEOUT_MS) {
        juce::MessageManager::callAsync([weakThis = juce::WeakReference<CollaborationManager>(this)]() {
          if (weakThis) weakThis->reportError("Connection timeout - could not establish P2P connection");
        });
        return;
      }
    }
    
    // Handshaking timeout
    if (state == ConnectionState::Handshaking) {
      if (handshakingStartTime == 0) {
        handshakingStartTime = now;
      } else if (now - handshakingStartTime > HANDSHAKING_TIMEOUT_MS) {
        juce::MessageManager::callAsync([weakThis = juce::WeakReference<CollaborationManager>(this)]() {
          if (weakThis) weakThis->reportError("Handshake timeout - peer authentication failed");
        });
        return;
      }
    }
    
    // === Peer Cleanup ===
    // Remove stale peers (no keepalive for PEER_TIMEOUT_MS)
    if (state == ConnectionState::Connected) {
      const juce::ScopedLock sl(peersLock);
      for (auto it = activePeers.begin(); it != activePeers.end();) {
        if (it->authenticated && (now - (juce::int64)it->lastSeen) > PEER_TIMEOUT_MS) {
          DBG("Collab: Peer timed out: " + it->ip + ":" + juce::String(it->port));
          
          // Remove from remoteUsers
          juce::String peerId = it->ip + ":" + juce::String(it->port);
          {
            const juce::ScopedLock sl(usersLock);
            remoteUsers.erase(
              std::remove_if(remoteUsers.begin(), remoteUsers.end(),
                [&peerId](const RemoteUser& u) { return u.id == peerId; }),
              remoteUsers.end());
          }
          
          it = activePeers.erase(it);
          juce::MessageManager::callAsync([weakThis = juce::WeakReference<CollaborationManager>(this)]() { 
              if (weakThis) weakThis->sendChangeMessage(); 
          });
        } else {
          ++it;
        }
      }
      
      // If no authenticated peers remain, go back to hosting/disconnected
      bool anyAuthenticated = false;
      for (const auto& peer : activePeers) {
        if (peer.authenticated) { anyAuthenticated = true; break; }
      }
      if (!anyAuthenticated && !activePeers.empty()) {
        // Could implement reconnection logic here
      }
    }

    // === KeepAlive / Punch Continuation ===
    state = currentState.load(std::memory_order_acquire); // Re-read after cleanup
    if (state == ConnectionState::Punching || state == ConnectionState::Handshaking) {
      // Keep telling server we are here until we get a PEER
      p2pSocket.write(SIGNALING_SERVER_IP, SIGNALING_UDP_PORT,
                      punchMsg.toRawUTF8(), (int)punchMsg.length());

      // If we have Peer Info (from handleIncomingPacket), punch THEM
      const juce::ScopedLock sl(peersLock);
      for (const auto& peer : activePeers) {
        juce::String hello = "HELLO_PEER";
        p2pSocket.write(peer.ip, peer.port, hello.toRawUTF8(), (int)hello.length());
      }
    } else if (state == ConnectionState::Connected) {
      if (now - lastKeepAlive > 2000) {
        const juce::ScopedLock sl(peersLock);
        for (const auto& peer : activePeers) {
          if (peer.authenticated) {
            juce::MemoryBlock msg;
            int t = (int)PacketType::KeepAlive;
            msg.append(&t, sizeof(int));
            p2pSocket.write(peer.ip, peer.port, msg.getData(), (int)msg.getSize());
          }
        }
        lastKeepAlive = now;
      }
    }
  }
}

void CollaborationManager::handleIncomingPacket(const void *data, int size,
                                                const juce::String &senderIP,
                                                int senderPort) {
  juce::String msg = juce::String::createStringFromData(data, size);

  // Case A: Message from Signaling Server
  if (msg.startsWith("PEER:")) {
    // Format: PEER:IP:PORT
    auto parts = juce::StringArray::fromTokens(msg, ":", "");
    if (parts.size() >= 3) {
      juce::String pIP = parts[1];
      int pPort = parts[2].getIntValue();
      
      // Add to potential peers if not already there
      const juce::ScopedLock sl(peersLock);
      bool found = false;
      for (auto& p : activePeers) {
          if (p.ip == pIP && p.port == pPort) { found = true; break; }
      }
      if (!found) {
          activePeers.push_back({ pIP, pPort, false, (juce::uint64)juce::Time::currentTimeMillis() });
          DBG("Collab: Added Potential Peer: " + pIP + ":" + juce::String(pPort));
      }
    }
    return;
  }

  // Case B: Message from Peer (Hole Punch Success!)
  if (msg == "HELLO_PEER" || size >= 4) {
    // Find the peer - MUST hold lock for entire operation
    const juce::ScopedLock sl(peersLock);
    PeerConnection* peer = nullptr;
    for (auto& p : activePeers) {
        if (p.ip == senderIP && p.port == senderPort) { peer = &p; break; }
    }
    
    if (!peer) {
        // New spontaneous peer (might happen with some NATs or if signaling missed it)
        activePeers.push_back({ senderIP, senderPort, false, (juce::uint64)juce::Time::currentTimeMillis() });
        peer = &activePeers.back();
    }
    
    peer->lastSeen = juce::Time::currentTimeMillis();

    // If we were punching, we are now handshaking!
    auto state = currentState.load(std::memory_order_acquire);
    if (state == ConnectionState::Punching || state == ConnectionState::Handshaking) {
      if (!peer->authenticated) {
        if (state == ConnectionState::Punching) {
            currentState.store(ConnectionState::Handshaking, std::memory_order_release);
            DBG("Collab: Hole Punch Successful with " + senderIP + "! Starting Handshake...");
            sendChangeMessage();
        }

        // Send Challenge with Salt if not sent yet for this peer
        if (peer->challenge == 0) {
            juce::Random rng;
            peer->challenge = rng.nextInt();
            
            // Generate robust 16-byte random salt
            juce::MemoryBlock saltMB(16, true);
            for(int i=0; i<4; ++i) { // Fill with random ints
                int r = rng.nextInt();
                saltMB.copyFrom(&r, i*4, 4);
            }
            peer->salt = juce::String::toHexString(saltMB.getData(), (int)saltMB.getSize(), 0);
            
            // Payload: [INT Challenge] [16 bytes SALT]
            // We'll send salt as raw bytes for efficiency
            juce::MemoryBlock payload;
            payload.append(&peer->challenge, sizeof(int));
            payload.append(saltMB.getData(), 16);
            
            sendPacket(PacketType::Challenge, payload.getData(), payload.getSize(), senderIP, senderPort);
        }
      }
    }

    if (size < 4)
      return;

    // Handle Real Data
    int typeInt = 0;
    memcpy(&typeInt, data, sizeof(int));
    PacketType type = (PacketType)typeInt;

    int headerSize = sizeof(int);
    const char *payloadPtr = (const char *)data + headerSize;
    int payloadSize = size - headerSize;

    // --- Authentication Flow ---
    if (type == PacketType::Challenge) {
      // Expect: [INT Challenge] [16 bytes SALT]
      if (payloadSize >= (int)(sizeof(int) + 16)) {
        int challenge = 0;
        memcpy(&challenge, payloadPtr, sizeof(int));
        
        juce::String saltHex;
        {
             juce::MemoryBlock s(payloadPtr + sizeof(int), 16);
             saltHex = juce::String::toHexString(s.getData(), (int)s.getSize(), 0);
        }
        
        // Derive session key using PBKDF2
        juce::MemoryBlock sessionKey = deriveSessionKey(sessionCode, saltHex);
        juce::String secret = juce::String::toHexString(sessionKey.getData(), (int)sessionKey.getSize(), 0);
        
        // Compute HMAC
        // Nonce? We use challenge as nonce.
        juce::String message = juce::String(challenge);
        juce::String signature = calculateHMAC(message, secret);
        
        // Send signature
        sendPacket(PacketType::ChallengeResponse, signature.toRawUTF8(), signature.length(), senderIP, senderPort);
      }
    } else if (type == PacketType::ChallengeResponse) {
        // We expect a hex string of the HMAC-SHA256 (64 chars)
      if (payloadSize >= 64 && peer->challenge != 0) { 
        juce::String receivedSignature = juce::String::fromUTF8(payloadPtr, payloadSize);

        // Re-derive key using the salt WE sent
        juce::MemoryBlock sessionKey = deriveSessionKey(sessionCode, peer->salt);
        juce::String secret = juce::String::toHexString(sessionKey.getData(), (int)sessionKey.getSize(), 0);
        
        juce::String message = juce::String(peer->challenge);
        juce::String expectedSignature = calculateHMAC(message, secret);
        
        // Constant-time comparison
        bool match = (receivedSignature.length() == expectedSignature.length());
        if (match) {
            const char* a = receivedSignature.toRawUTF8();
            const char* b = expectedSignature.toRawUTF8();
            volatile int result = 0;
            for (int i = 0; i < receivedSignature.length(); ++i) {
                result |= (a[i] ^ b[i]);
            }
            match = (result == 0);
        }

        if (match) {
          DBG("Collab: Auth Successful for " + senderIP + "!");
          peer->authenticated = true;
          currentState.store(ConnectionState::Connected, std::memory_order_release);
          sendChangeMessage();

          // Auth complete, send our User Info
          juce::MemoryBlock m;
          int t = (int)PacketType::Hello;
          m.append(&t, sizeof(int));
          m.append(localUserName.toRawUTF8(), localUserName.length());
          p2pSocket.write(senderIP, senderPort, m.getData(), (int)m.getSize());
        } else {
          DBG("Collab: Auth Failed for " + senderIP + "! Signature mismatch.");
          DBG("Expected: " + expectedSignature);
          DBG("Received: " + receivedSignature);
        }
      }
    }

    // --- Application Data ---
    else if (type == PacketType::Hello && payloadSize > 0) {
      // Received remote user's name
      juce::String remoteName = juce::String::fromUTF8(payloadPtr, payloadSize);
      juce::String remoteId = senderIP + ":" + juce::String(senderPort);
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
            RemoteUser newUser;
            newUser.id = remoteId;
            newUser.name = remoteName;
            newUser.isOnline = true;
            // Assign a color based on name hash
            int hash = remoteName.hashCode();
            newUser.color = juce::Colour::fromHSV((hash & 0xFF) / 255.0f, 0.7f, 0.9f, 1.0f);
            remoteUsers.push_back(newUser);
        }
      }
      DBG("Collab: Remote user joined: " + remoteName);
      sendChangeMessage();
    } else if (type == PacketType::CursorMove &&
               payloadSize == sizeof(float) * 2) {
      float pos[2];
      memcpy(pos, payloadPtr, sizeof(pos));
      juce::String remoteId = senderIP + ":" + juce::String(senderPort);
      {
        const juce::ScopedLock sl(usersLock);
        for (auto& user : remoteUsers) {
            if (user.id == remoteId) {
                user.mousePosition = {pos[0], pos[1]};
                user.isOnline = true;
                break;
            }
        }
      }
      sendChangeMessage();
    } else if (type == PacketType::SelectionUpdate) {
        juce::String selectionsStr = juce::String::fromUTF8(payloadPtr, payloadSize);
        juce::StringArray selections;
        selections.addLines(selectionsStr);
        juce::String remoteId = senderIP + ":" + juce::String(senderPort);
        {
            const juce::ScopedLock sl(usersLock);
            for (auto& user : remoteUsers) {
                if (user.id == remoteId) {
                    user.selectedIds = selections;
                    user.isOnline = true;
                    break;
                }
            }
        }
        sendChangeMessage();
    }
 else if (type == PacketType::EditCommand) {
      if (allowRemoteEditing) {
        if (payloadSize > 0) {
          juce::String cmdData =
              juce::String::fromUTF8(payloadPtr, payloadSize);
          if (onEditReceived) {
            juce::MessageManager::callAsync(
                [weakThis = juce::WeakReference<CollaborationManager>(this), cmdData]() { 
                    if (weakThis && weakThis->onEditReceived) weakThis->onEditReceived(cmdData); 
                });
          }
        }
      } else {
        DBG("Collab: Security blocked remote edit command.");
      }
    } else if (type == PacketType::CRDTUpdate) {
      if (crdtBridge && payloadSize > 0) {
        juce::MemoryBlock updates(payloadPtr, (size_t)payloadSize);
        crdtBridge->applyRemoteUpdates(updates);
      }
    }
  }
}

void CollaborationManager::sendPacket(PacketType type, const void *data,
                                      size_t size, const juce::String& targetIP, int targetPort) {
  if (currentState != ConnectionState::Connected &&
      currentState != ConnectionState::Handshaking &&
      type != PacketType::Challenge && type != PacketType::ChallengeResponse)
    return;

  juce::MemoryBlock msg;
  int t = (int)type;
  msg.append(&t, sizeof(int));
  msg.append(data, size);
  
  if (targetIP.isNotEmpty()) {
      p2pSocket.write(targetIP, targetPort, msg.getData(), (int)msg.getSize());
  } else {
      broadcastPacket(type, data, size);
  }
}

void CollaborationManager::broadcastPacket(PacketType type, const void *data, size_t size) {
    juce::MemoryBlock msg;
    int t = (int)type;
    msg.append(&t, sizeof(int));
    msg.append(data, size);
    
    for (const auto& peer : activePeers) {
        if (peer.authenticated) {
            p2pSocket.write(peer.ip, peer.port, msg.getData(), (int)msg.getSize());
        }
    }
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

void CollaborationManager::initializeCRDT(juce::ValueTree& projectTree) {
  crdtDoc = std::make_unique<Zenith::LoroDoc>();
  crdtBridge = std::make_unique<Zenith::ValueTreeCRDTBridge>(projectTree, *crdtDoc);
  
  crdtBridge->onLocalChange = [this]() {
    syncCRDT();
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

// --- Helpers ---

juce::String CollaborationManager::registerWithSignalingTCP() {
  juce::StreamingSocket sock;
  if (sock.connect(SIGNALING_SERVER_IP, SIGNALING_TCP_PORT, 2000)) {
    juce::String req = "{\"action\": \"REGISTER\"}";
    sock.write(req.toRawUTF8(), req.length());

    char buffer[1024];
    int bytes = sock.read(buffer, 1024, true);
    if (bytes > 0) {
      auto r = juce::JSON::parse(juce::String(buffer, bytes));
      if (r["status"] == "OK")
        return r["code"];
    }
  }
  return "ERR";
}

bool CollaborationManager::verifyCodeTCP(const juce::String &code) {
  juce::StreamingSocket sock;
  if (sock.connect(SIGNALING_SERVER_IP, SIGNALING_TCP_PORT, 2000)) {
    juce::String req = "{\"action\": \"LOOKUP\", \"code\": \"" + code + "\"}";
    sock.write(req.toRawUTF8(), req.length());

    char buffer[1024];
    int bytes = sock.read(buffer, 1024, true);
    if (bytes > 0) {
      auto r = juce::JSON::parse(juce::String(buffer, bytes));
      return r["status"] == "OK";
    }
  }
  return false;
}

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

void CollaborationManager::reportError(const juce::String& error) {
    DBG("Collaboration Error: " + error);
    currentState = ConnectionState::Error;
    sendChangeMessage();
}

} // namespace zenith

// --- Security Helpers ---

// --- Security Helpers ---

bool zenith::CollaborationManager::validateSessionCode(const juce::String& code) {
    // Strict validation: 6-12 chars, alphanumeric only
    if (code.length() < 6 || code.length() > 12) return false;
    return code.containsOnly("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
}

juce::MemoryBlock hmacSha256(const juce::MemoryBlock& key, const juce::MemoryBlock& data) {
    const int blockSize = 64;
    juce::MemoryBlock k = key;
    
    // Hash key if longer than block size
    if (k.getSize() > blockSize) {
        juce::SHA256 h(k.getData(), k.getSize());
        auto hash = h.getRawData(); 
        k = hash;
    }
    
    // Pad key if shorter
    if (k.getSize() < blockSize) k.ensureSize(blockSize, true);
    
    juce::uint8* kData = static_cast<juce::uint8*>(k.getData());
    juce::MemoryBlock ipad(blockSize, true);
    juce::MemoryBlock opad(blockSize, true);
    juce::uint8* iData = static_cast<juce::uint8*>(ipad.getData());
    juce::uint8* oData = static_cast<juce::uint8*>(opad.getData());
    
    for (int i = 0; i < blockSize; ++i) {
        iData[i] = kData[i] ^ 0x36;
        oData[i] = kData[i] ^ 0x5c;
    }
    
    // Inner
    juce::MemoryBlock inner;
    inner.append(ipad.getData(), ipad.getSize());
    inner.append(data.getData(), data.getSize());
    juce::SHA256 hInner(inner.getData(), inner.getSize());
    
    // Outer
    juce::MemoryBlock outer;
    outer.append(opad.getData(), opad.getSize());
    auto innerHash = hInner.getRawData();
    outer.append(innerHash.getData(), innerHash.getSize());
    
    juce::SHA256 hOuter(outer.getData(), outer.getSize());
    auto outerHash = hOuter.getRawData();
    return juce::MemoryBlock(outerHash.getData(), outerHash.getSize());
}

juce::String zenith::CollaborationManager::calculateHMAC(const juce::String& message, const juce::String& secret) {
    juce::MemoryBlock k; k.append(secret.toRawUTF8(), secret.length());
    juce::MemoryBlock m; m.append(message.toRawUTF8(), message.length());
    
    juce::MemoryBlock result = hmacSha256(k, m);
    return juce::String::toHexString(result.getData(), (int)result.getSize(), 0);
}

juce::MemoryBlock zenith::CollaborationManager::deriveSessionKey(const juce::String& password, const juce::String& salt) {
    // PBKDF2-HMAC-SHA256 Implementation
    // Iterations: 10000
    // DKLen: 32 bytes
    
    const int iterations = 10000;
    
    // Key is the password, Data is Salt + INT(1)
    juce::MemoryBlock P; P.append(password.toRawUTF8(), password.length());
    
    juce::MemoryBlock S; S.append(salt.toRawUTF8(), salt.length());
    juce::uint8 blockIndex[4] = {0, 0, 0, 1}; // Big Endian 1
    S.append(blockIndex, 4);
    
    // U1 = PRF(P, S || 1)
    juce::MemoryBlock U = hmacSha256(P, S);
    juce::MemoryBlock T = U; // T = U1
    
    // Loop
    for (int i = 1; i < iterations; ++i) {
        // U_i = PRF(P, U_{i-1})
        U = hmacSha256(P, U);
        
        // T ^= U_i
        juce::uint8* tData = static_cast<juce::uint8*>(T.getData());
        juce::uint8* uData = static_cast<juce::uint8*>(U.getData());
        for (size_t b = 0; b < 32; ++b) {
            tData[b] ^= uData[b];
        }
    }
    
    return T;
}
