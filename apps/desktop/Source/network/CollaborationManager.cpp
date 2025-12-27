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

  juce::Thread::launch([this]() {
    // 2. Register via TCP to get Code
    juce::String code = registerWithSignalingTCP();

    juce::MessageManager::callAsync([this, code]() {
      if (code != "ERR" && code.isNotEmpty()) {
        sessionCode = code;
        startHolePunching(); // Move to UDP phase
      } else {
        currentState.store(ConnectionState::Error, std::memory_order_release);
        sendChangeMessage();
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

  juce::Thread::launch([this, code]() {
    // 1. Verify Code via TCP
    if (verifyCodeTCP(code)) {
      juce::MessageManager::callAsync([this]() { startHolePunching(); });
    } else {
      juce::MessageManager::callAsync([this]() {
        currentState.store(ConnectionState::Error, std::memory_order_release);
        sendChangeMessage();
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
        juce::MessageManager::callAsync([this]() {
          reportError("Connection timeout - could not establish P2P connection");
        });
        return;
      }
    }
    
    // Handshaking timeout
    if (state == ConnectionState::Handshaking) {
      if (handshakingStartTime == 0) {
        handshakingStartTime = now;
      } else if (now - handshakingStartTime > HANDSHAKING_TIMEOUT_MS) {
        juce::MessageManager::callAsync([this]() {
          reportError("Handshake timeout - peer authentication failed");
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
          juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
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

        // Send Challenge (if we haven't already for this specific peer, but for now we re-use sentChallenge)
        if (sentChallenge == 0) {
            juce::Random rng;
            sentChallenge = rng.nextInt();
        }
        sendPacket(PacketType::Challenge, &sentChallenge, sizeof(int), senderIP, senderPort);
      }
    }

    if (size < 4)
      return;

    // Handle Real Data
    int typeInt = 0;
    memcpy(&typeInt, data, sizeof(int));
    PacketType type = (PacketType)typeInt;

    int headerSize = sizeof(int);
    char *payloadPtr = (char *)data + headerSize;
    int payloadSize = size - headerSize;

    // --- Authentication Flow ---
    if (type == PacketType::Challenge) {
      if (payloadSize == sizeof(int)) {
        int challenge = 0;
        memcpy(&challenge, payloadPtr, sizeof(int));
        
        // SECURITY NOTE: MD5 is cryptographically weak but juce_cryptography is not linked.
        // For proper security, add juce_cryptography to CMakeLists.txt and switch to SHA256.
        // This authentication is sufficient for casual collaboration but NOT for high-security use.
        juce::String salt = zenith::config::ConfigurationManager::getInstance()
                               .getString(zenith::config::keys::COLLAB_SALT, "ZENITH_SALT_2025");
        // Add session code AND current time to prevent replay attacks
        juce::String secret = juce::String(challenge) + sessionCode + salt + 
                              juce::String(juce::Time::currentTimeMillis() / 30000); // 30-second window
        
        // Compute MD5 hash - use FULL 16-byte hash, not truncated
        juce::MD5 hasher((const juce::uint8*)secret.toRawUTF8(), (size_t)secret.length());
        auto hashBlock = hasher.getRawChecksumData();
        
        // Send full 16-byte hash as response
        sendPacket(PacketType::ChallengeResponse, hashBlock.getData(), 16);
      }
    } else if (type == PacketType::ChallengeResponse) {
      if (payloadSize >= 16) { // Expect full 16-byte hash now
        // SECURITY NOTE: Same MD5 limitation applies here
        juce::String salt = zenith::config::ConfigurationManager::getInstance()
                               .getString(zenith::config::keys::COLLAB_SALT, "ZENITH_SALT_2025");
        juce::String expectedSecret = juce::String(sentChallenge) + sessionCode + salt +
                                      juce::String(juce::Time::currentTimeMillis() / 30000);
        
        juce::MD5 hasher((const juce::uint8*)expectedSecret.toRawUTF8(), (size_t)expectedSecret.length());
        auto expectedHash = hasher.getRawChecksumData();

        // Compare full 16-byte hash using constant-time comparison to prevent timing attacks
        bool match = true;
        const juce::uint8* received = static_cast<const juce::uint8*>(static_cast<const void*>(payloadPtr));
        const juce::uint8* expected = static_cast<const juce::uint8*>(expectedHash.getData());
        for (int i = 0; i < 16; ++i) {
          if (received[i] != expected[i]) match = false;
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
          DBG("Collab: Auth Failed for " + senderIP + "! Hash mismatch.");
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
                [this, cmdData]() { onEditReceived(cmdData); });
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
