#include "CollaborationManager.h"
#include "../ui/framework/ConfigurationManager.h"

CollaborationManager::CollaborationManager()
    : juce::Thread("CollabP2PThread") {}

CollaborationManager::~CollaborationManager() {
  isShuttingDown_->store(true);
  disconnect();
}

void CollaborationManager::startHosting() {
  disconnect();
  isHost = true;

  // 1. Start Server for Signaling
  startLocalSignalingServer();

  currentState = ConnectionState::Registering;
  sendChangeMessage();

  auto shutdownFlag = isShuttingDown_;
  juce::Thread::launch([this, shutdownFlag]() {
    // 2. Register via TCP to get Code
    juce::String code = registerWithSignalingTCP();

    juce::MessageManager::callAsync([this, code, shutdownFlag]() {
      if (shutdownFlag->load()) return;
      if (code != "ERR" && code.isNotEmpty()) {
        sessionCode = code;
        startHolePunching(); // Move to UDP phase
      } else {
        currentState = ConnectionState::Error;
        sendChangeMessage();
      }
    });
  });
}

void CollaborationManager::joinSession(const juce::String &code) {
  disconnect();
  isHost = false;
  sessionCode = code;

  currentState = ConnectionState::Registering;
  sendChangeMessage();

  auto shutdownFlag = isShuttingDown_;
  juce::Thread::launch([this, code, shutdownFlag]() {
    // 1. Verify Code via TCP
    if (verifyCodeTCP(code)) {
      juce::MessageManager::callAsync([this, shutdownFlag]() {
        if (shutdownFlag->load()) return;
        startHolePunching();
      });
    } else {
      juce::MessageManager::callAsync([this, shutdownFlag]() {
        if (shutdownFlag->load()) return;
        currentState = ConnectionState::Error;
        sendChangeMessage();
      });
    }
  });
}

void CollaborationManager::disconnect() {
  signalThreadShouldExit();
  stopThread(1000);
  p2pSocket.shutdown();

  currentState = ConnectionState::Disconnected;
  remoteUsers.clear();
  sessionCode = "";
  sendChangeMessage();
}

// --- Logic ---

void CollaborationManager::startHolePunching() {
  currentState = ConnectionState::Punching;
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
                    punchMsg.toRawUTF8(), (int)punchMsg.getNumBytesAsUTF8());
    wait(200);
  }

  // 2. Listen Loop
  char buffer[2048];
  juce::int64 lastKeepAlive = 0;
  
  while (!threadShouldExit()) {
    // Read
    if (p2pSocket.waitUntilReady(true, 100) == 1) {
      juce::String senderIP;
      int senderPort;
      int bytes = p2pSocket.read(buffer, 2048, false, senderIP, senderPort);

      if (bytes > 0) {
        handleIncomingPacket(buffer, bytes, senderIP, senderPort);
      }
    }

    // KeepAlive / Punch Continuation
    if (currentState == ConnectionState::Punching) {
      // Keep telling server we are here until we get a PEER
      p2pSocket.write(SIGNALING_SERVER_IP, SIGNALING_UDP_PORT,
                      punchMsg.toRawUTF8(), (int)punchMsg.length());

      // If we have Peer Info (from handleIncomingPacket), punch THEM
      if (peerIP.isNotEmpty()) {
        juce::String hello = "HELLO_PEER";
        p2pSocket.write(peerIP, peerPort, hello.toRawUTF8(),
                        (int)hello.getNumBytesAsUTF8());
      }
    } else if (currentState == ConnectionState::Connected) {
      auto now = juce::Time::currentTimeMillis();
      if (now - lastKeepAlive > 2000) {
        juce::MemoryBlock msg;
        int t = (int)PacketType::KeepAlive;
        msg.append(&t, sizeof(int));
        p2pSocket.write(peerIP, peerPort, msg.getData(), (int)msg.getSize());
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
      peerIP = parts[1];
      peerPort = parts[2].getIntValue();
      DBG("Collab: Received Peer Info: " + peerIP + ":" +
          juce::String(peerPort));
    }
    return;
  }

  // Case B: Message from Peer (Hole Punch Success!)
  if (msg == "HELLO_PEER" || size >= 4) {
    // If we were punching, we are now handshaking!
    if (currentState == ConnectionState::Punching) {
      if (peerIP.isNotEmpty()) {
        currentState = ConnectionState::Handshaking;
        DBG("Collab: Hole Punch Successful! Starting Handshake...");
        sendChangeMessage();

        // Send Challenge
        juce::Random rng;
        sentChallenge = rng.nextInt();
        sendPacket(PacketType::Challenge, &sentChallenge, sizeof(int));
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
        
        // A+ Security: Response = Hash(Challenge + SessionCode + Salt)
        // We use string hashing as a robust mechanism since simple XOR is reversible.
        juce::String salt = zenith::config::ConfigurationManager::getInstance()
                               .getString(zenith::config::keys::COLLAB_SALT, "ZENITH_SALT_2025");
        juce::String secret = juce::String(challenge) + sessionCode + salt;
        int response = secret.hashCode(); 
        
        sendPacket(PacketType::ChallengeResponse, &response, sizeof(int));
      }
    } else if (type == PacketType::ChallengeResponse) {
      if (payloadSize == sizeof(int)) {
        int receivedResponse = 0;
        memcpy(&receivedResponse, payloadPtr, sizeof(int));
        
        juce::String salt = zenith::config::ConfigurationManager::getInstance()
                               .getString(zenith::config::keys::COLLAB_SALT, "ZENITH_SALT_2025");
        juce::String expectedSecret = juce::String(sentChallenge) + sessionCode + salt;
        int expectedResponse = expectedSecret.hashCode();

        if (receivedResponse == expectedResponse) {
          DBG("Collab: Auth Successful (Hash Verified)!");
          currentState = ConnectionState::Connected;
          sendChangeMessage();

          // Auth complete, send our User Info
          juce::MemoryBlock m;
          int t = (int)PacketType::Hello;
          m.append(&t, sizeof(int));
          m.append(localUserName.toRawUTF8(), localUserName.getNumBytesAsUTF8());
          p2pSocket.write(peerIP, peerPort, m.getData(), (int)m.getSize());
        } else {
          DBG("Collab: Auth Failed! Response mismatch.");
          disconnect();
        }
      }
    }
    // --- Application Data ---
    else if (type == PacketType::Hello && payloadSize > 0) {
      // Received remote user's name
      juce::String remoteName = juce::String::fromUTF8(payloadPtr, payloadSize);
      {
        const juce::ScopedLock sl(usersLock);
        if (remoteUsers.empty()) {
          remoteUsers.push_back({});
        }
        remoteUsers[0].name = remoteName;
        remoteUsers[0].isOnline = true;
        remoteUsers[0].id = senderIP + ":" + juce::String(senderPort);
        // Assign a color based on name hash
        int hash = remoteName.hashCode();
        remoteUsers[0].color =
            juce::Colour::fromHSV((hash & 0xFF) / 255.0f, 0.7f, 0.9f, 1.0f);
      }
      DBG("Collab: Remote user joined: " + remoteName);
      sendChangeMessage();
    } else if (type == PacketType::CursorMove &&
               payloadSize == sizeof(float) * 2) {
      float pos[2];
      memcpy(pos, payloadPtr, sizeof(pos));
      {
        const juce::ScopedLock sl(usersLock);
        if (remoteUsers.empty())
          remoteUsers.push_back({});
        remoteUsers[0].mousePosition = {pos[0], pos[1]};
        remoteUsers[0].isOnline = true;
      }
      sendChangeMessage();
    } else if (type == PacketType::EditCommand) {
      if (allowRemoteEditing) {
        if (payloadSize > 0) {
          juce::String cmdData =
              juce::String::fromUTF8(payloadPtr, payloadSize);
          if (onEditReceived) {
            auto shutdownFlag = isShuttingDown_;
            auto callback = onEditReceived;
            juce::MessageManager::callAsync(
                [shutdownFlag, callback, cmdData]() {
                  if (shutdownFlag->load()) return;
                  callback(cmdData);
                });
          }
        }
      } else {
        DBG("Collab: Security blocked remote edit command.");
      }
    }
  }
}

void CollaborationManager::sendPacket(PacketType type, const void *data,
                                      size_t size) {
  if (currentState != ConnectionState::Connected &&
      currentState != ConnectionState::Handshaking)
    return;

  juce::MemoryBlock msg;
  int t = (int)type;
  msg.append(&t, sizeof(int));
  msg.append(data, size);
  p2pSocket.write(peerIP, peerPort, msg.getData(), (int)msg.getSize());
}

void CollaborationManager::updateLocalCursor(float x, float y) {
  float pos[2] = {x, y};
  sendPacket(PacketType::CursorMove, pos, sizeof(pos));
}

void CollaborationManager::broadcastEdit(const juce::String &commandData) {
  sendPacket(PacketType::EditCommand, commandData.toRawUTF8(),
             commandData.getNumBytesAsUTF8());
}

// --- Helpers ---

juce::String CollaborationManager::registerWithSignalingTCP() {
  juce::StreamingSocket sock;
  if (sock.connect(SIGNALING_SERVER_IP, SIGNALING_TCP_PORT, 2000)) {
    juce::String req = "{\"action\": \"REGISTER\"}";
    sock.write(req.toRawUTF8(), req.getNumBytesAsUTF8());

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
    sock.write(req.toRawUTF8(), req.getNumBytesAsUTF8());

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
  juce::File scriptFile("c:/zenith/daw/backend/signaling/signaling_server.py");
  if (scriptFile.existsAsFile()) {
    juce::StringArray args;
    args.add("python");
    args.add(scriptFile.getFullPathName());

    if (signalingProcess.start(args)) {
      DBG("Collab: Started Local Signaling Server (python)");
    } else {
      args.set(0, "python3");
      if (signalingProcess.start(args)) {
        DBG("Collab: Started Local Signaling Server (python3)");
      } else {
        // Try 'py' launcher for Windows
        args.set(0, "py");
        if (signalingProcess.start(args)) {
          DBG("Collab: Started Local Signaling Server (py)");
        } else {
          DBG("Collab: FAILED to start Signaling Server. Ensure Python is "
              "installed.");
          // We could alert the user here, but for now we rely on the connection
          // failing logic.
        }
      }
    }
  }
}

void CollaborationManager::reportError(const juce::String& error) {
    DBG("Collaboration Error: " + error);
    currentState = ConnectionState::Error;
    sendChangeMessage();
}
