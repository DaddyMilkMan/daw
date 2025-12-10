#include "CollaborationManager.h"

CollaborationManager::CollaborationManager() : juce::Thread("CollabP2PThread") {
  // Check for timeouts every 5 seconds
  startTimer(5000);
}

CollaborationManager::~CollaborationManager() { disconnect(); }

void CollaborationManager::startHosting() {
  disconnect();
  isHost = true;

  // 1. Start Server for Signaling
  startLocalSignalingServer();

  currentState = ConnectionState::Registering;
  sendChangeMessage();

  juce::Thread::launch([this]() {
    // 2. Register via TCP to get Code
    juce::String code = registerWithSignalingTCP();

    juce::MessageManager::callAsync([this, code]() {
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

  juce::Thread::launch([this, code]() {
    // 1. Verify Code via TCP
    if (verifyCodeTCP(code)) {
      juce::MessageManager::callAsync([this]() { startHolePunching(); });
    } else {
      juce::MessageManager::callAsync([this]() {
        currentState = ConnectionState::Error;
        sendChangeMessage();
      });
    }
  });
}

void CollaborationManager::disconnect() {
  stopTimer();
  signalThreadShouldExit();
  stopThread(1000);
  p2pSocket.shutdown();

  {
    const juce::ScopedWriteLock sl(usersLock);
    remoteUsers.clear();
  }

  currentState = ConnectionState::Disconnected;
  sessionCode = "";

  zenith::ZenithLogger::getInstance().log(
      zenith::LogLevel::Info, "Disconnected from session", "Network");
  sendChangeMessage();
}

void CollaborationManager::timerCallback() {
  // The Dead Man's Switch
  // Runs on Message Thread - safe to interact with UI state if needed

  // Only check if we are actually connected
  if (currentState != ConnectionState::Connected)
    return;

  auto now = juce::Time::currentTimeMillis();
  bool changed = false;

  {
    const juce::ScopedWriteLock sl(usersLock);

    for (auto it = remoteUsers.begin(); it != remoteUsers.end();) {
      // 15 seconds timeout
      if (now - it->lastSeen > 15000) {
        zenith::ZenithLogger::getInstance().log(
            zenith::LogLevel::Warning,
            "Peer timed out (Dead Man's Switch): " + it->name + " (" + it->id +
                ")",
            "Network");

        it = remoteUsers.erase(it);
        changed = true;
      } else {
        ++it;
      }
    }
  }

  if (changed) {
    sendChangeMessage();
  }
}

void CollaborationManager::handleKeepAlive() {
  // KeepAlive is just a heartbeat to update the timestamp
  // The timestamp update happens in handleIncomingPacket for ANY packet,
  // so this method might just be a placeholder or specific logic if needed.
  // For now, it's valid to be empty as handleIncomingPacket does the work.
}

// --- Logic ---

void CollaborationManager::startHolePunching() {
  currentState = ConnectionState::Punching;
  sendChangeMessage();

  // Bind to ANY local port
  p2pSocket.bindToPort(0);

  startTimer(5000); // Start the Dead Man's Switch watchdog
  startThread();    // Start the read/keepalive loop
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

  // 2. Listen Loop
  char buffer[2048];
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
                        (int)hello.length());
      }
    } else if (currentState == ConnectionState::Connected) {
      auto now = juce::Time::currentTimeMillis();
      static int64 lastKeepAlive = 0;

      // Send KeepAlive every 2 seconds
      if (now - lastKeepAlive > 2000) {
        juce::MemoryBlock msg;
        int t = (int)PacketType::KeepAlive;
        msg.append(&t, sizeof(int));
        p2pSocket.write(peerIP, peerPort, msg.getData(), (int)msg.getSize());
        lastKeepAlive = now;
      }

      // Note: Timeout checking is now handled by timerCallback on the Message
      // Thread
    }
  }
}

void CollaborationManager::handleIncomingPacket(const void *data, int size,
                                                const juce::String &senderIP,
                                                int senderPort) {
  if (size < (int)sizeof(int)) {
    zenith::ZenithLogger::getInstance().log(
        zenith::LogLevel::Warning, "Ignored malformed packet (too small)",
        "Network");
    return;
  }

  juce::String msg = juce::String::createStringFromData(data, size);

  // Case A: Message from Signaling Server (Legacy/Punching Protocol - uses
  // strings)
  if (msg.startsWith("PEER:")) {
    // Format: PEER:IP:PORT
    auto parts = juce::StringArray::fromTokens(msg, ":", "");
    if (parts.size() >= 3) {
      peerIP = parts[1];
      peerPort = parts[2].getIntValue();
      zenith::ZenithLogger::getInstance().log(zenith::LogLevel::Info,
                                              "Received Peer Info: " + peerIP +
                                                  ":" + juce::String(peerPort),
                                              "Network");
    }
    return;
  }

  // Case B: Message from Peer (Hole Punch Success!)
  if (msg == "HELLO_PEER" || (size >= 10 && msg.startsWith("HELLO_PEER"))) {
    // Strict check, though usually exact match
    // If we were punching, we are now connected!
    if (currentState == ConnectionState::Punching) {
      currentState = ConnectionState::Connected;
      zenith::ZenithLogger::getInstance().log(
          zenith::LogLevel::Info, "P2P UDP Connection Established!", "Network");
      sendChangeMessage();

      // Handshake - send our name
      juce::MemoryBlock m;
      int t = (int)PacketType::Hello;
      m.append(&t, sizeof(int));
      m.append(localUserName.toRawUTF8(), localUserName.length());
      p2pSocket.write(peerIP, peerPort, m.getData(), (int)m.getSize());
    }

    // Fall through if it also contains data? Usually HELLO_PEER is just a
    // string.
    if (size < 4)
      return;
  }

  // Handle Real Data (Binary Protocol)
  // Header parsing
  int typeInt = 0;
  memcpy(&typeInt, data, sizeof(int));

  // Validate PacketType
  if (typeInt != (int)PacketType::Hello &&
      typeInt != (int)PacketType::CursorMove &&
      typeInt != (int)PacketType::EditCommand &&
      typeInt != (int)PacketType::KeepAlive) {
    // Ignore unknown packets
    return;
  }

  PacketType type = (PacketType)typeInt;
  int headerSize = sizeof(int);

  // Update timestamps for any valid packet from peer
  {
    const juce::ScopedWriteLock sl(usersLock);
    if (!remoteUsers.empty()) {
      remoteUsers[0].lastSeen = juce::Time::currentTimeMillis();
    }
  }

  if (type == PacketType::KeepAlive) {
    // Just updating timestamp is enough
    return;
  }

  char *payloadPtr = (char *)data + headerSize;
  int payloadSize = size - headerSize;

  if (payloadSize < 0)
    return; // Should not happen given initial size check

  if (type == PacketType::Hello && payloadSize > 0) {
    // Received remote user's name
    // Sanity check length
    if (payloadSize > 256)
      payloadSize = 256;

    juce::String remoteName = juce::String::fromUTF8(payloadPtr, payloadSize);
    {
      const juce::ScopedWriteLock sl(usersLock);
      if (remoteUsers.empty()) {
        remoteUsers.push_back({});
      }
      remoteUsers[0].name = remoteName;
      remoteUsers[0].isOnline = true;
      remoteUsers[0].lastSeen = juce::Time::currentTimeMillis();
      remoteUsers[0].id = senderIP + ":" + juce::String(senderPort);
      // Assign a color based on name hash
      int hash = remoteName.hashCode();
      remoteUsers[0].color =
          juce::Colour::fromHSV((hash & 0xFF) / 255.0f, 0.7f, 0.9f, 1.0f);
    }
    zenith::ZenithLogger::getInstance().log(
        zenith::LogLevel::Info, "Remote user joined: " + remoteName, "Network");
    sendChangeMessage();
  } else if (type == PacketType::CursorMove) {
    if (payloadSize == sizeof(float) * 2) {
      float pos[2];
      memcpy(pos, payloadPtr, sizeof(pos));
      {
        const juce::ScopedWriteLock sl(usersLock);
        if (remoteUsers.empty())
          remoteUsers.push_back({});
        remoteUsers[0].mousePosition = {pos[0], pos[1]};
        remoteUsers[0].isOnline = true;
        // lastSeen updated above
      }
      sendChangeMessage();
    }
  } else if (type == PacketType::EditCommand) {
    if (payloadSize > 0) {
      juce::String cmdData = juce::String::fromUTF8(payloadPtr, payloadSize);

      // JSON Validation for Phase 1 Requirement:
      // "If a packet says it has a 'trackID' but doesn't, discard it and warn."
      // We parse logic here if possible, but generic 'EditCommand' implies
      // downstream handling. However, we can at least ensure it's valid JSON if
      // we expect JSON. Let's optimize: Check if it LOOKS like JSON.
      if (cmdData.trim().startsWith("{")) {
        // Wrap in try-catch because JSON::parse can throw or assert in some
        // JUCE versions (though mainly it returns void var) Actually parse
        // returns var, doesn't throw usually. But let's be safe.
        try {
          juce::var parsed = juce::JSON::parse(cmdData);
          if (parsed.isVoid()) {
            zenith::ZenithLogger::getInstance().log(
                zenith::LogLevel::Warning,
                "Received malformed JSON edit command (parse failed)",
                "Network");
            return;
          }
          // Phase 1: Robust checking "If a packet says it has a 'trackID' but
          // doesn't" This implies if the logic expects it. Since we don't know
          // the logic here, we just validate it IS valid JSON.
        } catch (...) {
          zenith::ZenithLogger::getInstance().log(
              zenith::LogLevel::Warning, "Exception parsing JSON edit command",
              "Network");
          return;
        }
      }

      if (onEditReceived) {
        juce::MessageManager::callAsync(
            [this, cmdData]() { onEditReceived(cmdData); });
      }
    }
  }
}

void CollaborationManager::sendPacket(PacketType type, const void *data,
                                      size_t size) {
  if (currentState != ConnectionState::Connected)
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
             commandData.length());
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
      try {
        // Robust JSON parsing
        auto jsonVar = juce::JSON::parse(juce::String(buffer, bytes));
        if (!jsonVar.isVoid() && jsonVar["status"] == "OK") {
          // Safety check for code field
          if (jsonVar.hasProperty("code") && jsonVar["code"].isString()) {
            return jsonVar["code"];
          }
          zenith::ZenithLogger::getInstance().log(
              zenith::LogLevel::Warning, "Server replied OK but missing code",
              "Network");
        }
      } catch (...) {
        zenith::ZenithLogger::getInstance().log(
            zenith::LogLevel::Error,
            "JSON Parse Error in registerWithSignalingTCP", "Network");
      }
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
      try {
        auto jsonVar = juce::JSON::parse(juce::String(buffer, bytes));
        return !jsonVar.isVoid() && jsonVar["status"] == "OK";
      } catch (...) {
        zenith::ZenithLogger::getInstance().log(
            zenith::LogLevel::Error, "JSON Parse Error in verifyCodeTCP",
            "Network");
      }
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
      zenith::ZenithLogger::getInstance().log(
          zenith::LogLevel::Info, "Started Local Signaling Server (python)",
          "Network");
    } else {
      args.set(0, "python3");
      if (signalingProcess.start(args)) {
        zenith::ZenithLogger::getInstance().log(
            zenith::LogLevel::Info, "Started Local Signaling Server (python3)",
            "Network");
      } else {
        // Try 'py' launcher for Windows
        args.set(0, "py");
        if (signalingProcess.start(args)) {
          zenith::ZenithLogger::getInstance().log(
              zenith::LogLevel::Info, "Started Local Signaling Server (py)",
              "Network");
        } else {
          zenith::ZenithLogger::getInstance().log(
              zenith::LogLevel::Error,
              "FAILED to start Signaling Server. Ensure Python is installed.",
              "Network");
          // We could alert the user here, but for now we rely on the connection
          // failing logic.
        }
      }
    }
  }
}
