#include "CollaborationManager.h"
#include "../ui/framework/ConfigurationManager.h"
#include <juce_cryptography/juce_cryptography.h>

namespace zenith {

CollaborationManager::CollaborationManager()
    : juce::Thread("CollabP2PThread") {
  signalingServerIP = zenith::config::ConfigurationManager::getInstance().getString(
      zenith::config::keys::COLLAB_SERVER_IP, "216.126.231.46");
}

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
  // Timeout tracking
  juce::int64 punchingStartTime = juce::Time::currentTimeMillis();
  constexpr int PUNCHING_TIMEOUT_MS = 30000;
  constexpr int PEER_TIMEOUT_MS = 10000;

  // --- Phase 1: UDP Punching and Peer Discovery ---
  juce::String punchMsg = (isHost ? "REGISTER:" : "JOIN:") + sessionCode;
  while (!threadShouldExit() && currentState == ConnectionState::Punching) {
    auto now = juce::Time::currentTimeMillis();
    if (now - punchingStartTime > PUNCHING_TIMEOUT_MS) {
        juce::MessageManager::callAsync([this]() { reportError("Connection timeout - did not receive peer info from server."); });
        return;
    }

    // Send punch to signaling server
    p2pSocket.getInternalSocket().write(signalingServerIP, SIGNALING_UDP_PORT, punchMsg.toRawUTF8(), (int)punchMsg.length());

    // Check for response from server
    char buffer[2048];
    if (p2pSocket.getInternalSocket().waitUntilReady(true, 500) == 1) {
      juce::String senderIP;
      int senderPort;
      int bytes = p2pSocket.getInternalSocket().read(buffer, 2048, false, senderIP, senderPort);
      if (bytes > 0 && senderIP == signalingServerIP) {
          juce::String msg = juce::String::createStringFromData(buffer, bytes);
          if (msg.startsWith("PEER:")) {
              auto parts = juce::StringArray::fromTokens(msg, ":", "");
              if (parts.size() >= 3) {
                  peerIP = parts[1];
                  peerPort = parts[2].getIntValue();
                  DBG("Collab: Got peer info: " + peerIP + ":" + juce::String(peerPort));
                  currentState.store(ConnectionState::Handshaking, std::memory_order_release);
                  sendChangeMessage();
                  break; // Exit discovery loop
              }
          }
      }
    }
  }

  // --- Phase 2: DTLS Handshake ---
  if (currentState != ConnectionState::Handshaking) {
    reportError("Failed to discover peer.");
    return;
  }
  
  if (!p2pSocket.connect(peerIP, peerPort)) {
      reportError("DTLS connection failed to start.");
      return;
  }
  
  juce::int64 handshakeStartTime = juce::Time::currentTimeMillis();
  constexpr int HANDSHAKE_TIMEOUT_MS = 15000;

  while(!threadShouldExit() && currentState == ConnectionState::Handshaking) {
      if (juce::Time::currentTimeMillis() - handshakeStartTime > HANDSHAKE_TIMEOUT_MS) {
          reportError("DTLS handshake timed out.");
          return;
      }
      if (p2pSocket.performHandshake()) {
          currentState.store(ConnectionState::Connected, std::memory_order_release);
          sendChangeMessage();
          DBG("Collab: DTLS Handshake complete. Connection is now secure.");

          // Send initial Hello packet
          juce::MemoryBlock m;
          int t = (int)PacketType::Hello;
          m.append(&t, sizeof(int));
          m.append(localUserName.toRawUTF8(), localUserName.length());
          p2pSocket.write(m.getData(), (int)m.getSize());

          break; // Exit handshake loop
      }
      wait(50); // Small wait to prevent busy-looping
  }

  // --- Phase 3: Connected Operation ---
  if (currentState != ConnectionState::Connected) {
    reportError("Handshake failed.");
    return;
  }

  char decryptedBuffer[2048];
  juce::int64 lastKeepAlive = 0;

  while (!threadShouldExit()) {
    juce::String senderIP;
    int senderPort;
    int bytes = p2pSocket.read(decryptedBuffer, sizeof(decryptedBuffer), senderIP, senderPort);
    if (bytes > 0) {
        handleIncomingPacket(decryptedBuffer, bytes, senderIP, senderPort);
    }

    // Keepalive
    auto now = juce::Time::currentTimeMillis();
    if (now - lastKeepAlive > 2000) {
      int t = (int)PacketType::KeepAlive;
      p2pSocket.write(&t, sizeof(int));
      lastKeepAlive = now;
    }
    
    // TODO: Peer timeout logic
    wait(10);
  }
}

void CollaborationManager::handleIncomingPacket(const void *data, int size,
                                                const juce::String &senderIP,
                                                int senderPort) {
    if (size < 4) return;

    // Data is now DECRYPTED by DTLSSocket
    int typeInt = 0;
    memcpy(&typeInt, data, sizeof(int));
    PacketType type = (PacketType)typeInt;

    int headerSize = sizeof(int);
    char *payloadPtr = (char *)data + headerSize;
    int payloadSize = size - headerSize;

    // --- Application Data ---
    if (type == PacketType::Hello && payloadSize > 0) {
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
      juce::String remoteId = senderIP + ":" + juce::String(senderPort);
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
        juce::String remoteId = senderIP + ":" + juce::String(senderPort);
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
      if (crdtBridge && payloadSize > 0) {
        juce::MemoryBlock updates(payloadPtr, (size_t)payloadSize);
        crdtBridge->applyRemoteUpdates(updates);
      }
    }
#endif
}

void CollaborationManager::sendPacket(PacketType type, const void *data,
                                      size_t size, const juce::String&, int) {
  if (currentState != ConnectionState::Connected)
    return;

  juce::MemoryBlock msg;
  int t = (int)type;
  msg.append(&t, sizeof(int));
  msg.append(data, size);
  
  p2pSocket.write(msg.getData(), (int)msg.getSize());
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
#endif

// --- Helpers ---

#ifdef ZENITH_ENABLE_COLLAB
juce::String CollaborationManager::registerWithSignalingTCP() {
  juce::TLSOptions tlsOptions;
  tlsOptions.withAllowSelfSignedCerts();

  juce::StreamingSocket sock;
  if (sock.connect(signalingServerIP, SIGNALING_TCP_PORT, 2000)) {
    juce::TLSSocket securedSocket(&sock, tlsOptions);
    
    juce::String req = "{\"action\": \"REGISTER\"}";
    securedSocket.write(req.toRawUTF8(), req.length());

    char buffer[1024];
    int bytes = securedSocket.read(buffer, 1024, true);
    if (bytes > 0) {
      auto r = juce::JSON::parse(juce::String(buffer, bytes));
      if (r["status"] == "OK")
        return r["code"];
    }
  }
  return "ERR";
}

bool CollaborationManager::verifyCodeTCP(const juce::String &code) {
  juce::TLSOptions tlsOptions;
  tlsOptions.withAllowSelfSignedCerts();

  juce::StreamingSocket sock;
  if (sock.connect(signalingServerIP, SIGNALING_TCP_PORT, 2000)) {
    juce::TLSSocket securedSocket(&sock, tlsOptions);

    juce::String req = "{\"action\": \"LOOKUP\", \"code\": \"" + code + "\"}";
    securedSocket.write(req.toRawUTF8(), req.length());

    char buffer[1024];
    int bytes = securedSocket.read(buffer, 1024, true);
    if (bytes > 0) {
      auto r = juce::JSON::parse(juce::String(buffer, bytes));
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

void CollaborationManager::reportError(const juce::String& error) {
    DBG("Collaboration Error: " + error);
    currentState = ConnectionState::Error;
    sendChangeMessage();
}

} // namespace zenith
