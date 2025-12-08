#include "CollaborationManager.h"

CollaborationManager::CollaborationManager()
    : juce::Thread("CollabP2PThread") {}

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
  signalThreadShouldExit();
  stopThread(1000);
  p2pSocket.close();

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
    p2pSocket.write(punchMsg.toRawUTF8(), punchMsg.length(),
                    SIGNALING_SERVER_IP, SIGNALING_UDP_PORT);
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
      p2pSocket.write(punchMsg.toRawUTF8(), punchMsg.length(),
                      SIGNALING_SERVER_IP, SIGNALING_UDP_PORT);

      // If we have Peer Info (from handleIncomingPacket), punch THEM
      if (peerIP.isNotEmpty()) {
        juce::String hello = "HELLO_PEER";
        p2pSocket.write(hello.toRawUTF8(), hello.length(), peerIP, peerPort);
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
    // If we were punching, we are now connected!
    if (currentState == ConnectionState::Punching) {
      currentState = ConnectionState::Connected;
      DBG("Collab: P2P UDP Connection Established!");
      sendChangeMessage();

      // Handshake
      juce::String myName =
          "User_" + juce::String(juce::Random::getSystemRandom().nextInt(100));
      juce::MemoryBlock m;
      int t = (int)PacketType::Hello;
      m.append(&t, sizeof(int));
      m.append(myName.toRawUTF8(), myName.length());
      p2pSocket.write(m.getData(), m.getSize(), peerIP, peerPort);
    }

    if (size < 4)
      return;

    // Handle Real Data
    // ... (Header parsing logic similar to before) ...
    int typeInt = 0;
    memcpy(&typeInt, data, sizeof(int));
    PacketType type = (PacketType)typeInt;

    int headerSize = sizeof(int);
    char *payloadPtr = (char *)data + headerSize;
    int payloadSize = size - headerSize;

    if (type == PacketType::CursorMove && payloadSize == sizeof(float) * 2) {
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
    }
    // ... Other types
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
  p2pSocket.write(msg.getData(), msg.getSize(), peerIP, peerPort);
}

void CollaborationManager::updateLocalCursor(float x, float y) {
  float pos[2] = {x, y};
  sendPacket(PacketType::CursorMove, pos, sizeof(pos));
}

void CollaborationManager::broadcastEdit(const juce::String &commandData) {
  // ...
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
