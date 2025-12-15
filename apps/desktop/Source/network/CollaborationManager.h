#pragma once
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <functional>
#include <vector>

struct RemoteUser {
  juce::String id;
  juce::String name;
  juce::Colour color;
  juce::Point<float> mousePosition;
  bool isOnline;
};

enum class PacketType {
  Hello = 0,
  CursorMove = 1,
  EditCommand = 2,
  KeepAlive = 99
};

class CollaborationManager : public juce::ChangeBroadcaster,
                             private juce::Thread {
public:
  static CollaborationManager &getInstance() {
    static CollaborationManager instance;
    return instance;
  }

  enum class ConnectionState {
    Disconnected,
    Registering, // Getting Code from TCP
    Connecting,  // Joiner waiting for connection
    Punching,    // Sending UDP to Server to open ports
    Hosting,     // Acting as session host
    Connected,   // P2P UDP Stream Active
    Error
  };

  void startHosting();
  void joinSession(const juce::String &code);
  void disconnect();

  ConnectionState getState() const { return currentState; }
  juce::String getCurrentCode() const { return sessionCode; }

  // --- Real-time Sync ---
  void updateLocalCursor(float x, float y);
  std::vector<RemoteUser> getRemoteUsers() const {
    const juce::ScopedLock sl(usersLock);
    return remoteUsers;
  }
  void broadcastEdit(const juce::String &commandData);

  // --- User Identity ---
  void setLocalUserName(const juce::String &name) { localUserName = name; }
  juce::String getLocalUserName() const { return localUserName; }

  std::function<void(const juce::String &)> onEditReceived;

private:
  CollaborationManager();
  ~CollaborationManager();

  // Loop
  void run() override;

  ConnectionState currentState = ConnectionState::Disconnected;
  juce::String sessionCode;
  juce::String localUserName = "User";
  std::vector<RemoteUser> remoteUsers;
  mutable juce::CriticalSection usersLock;

  // --- Networking ---
  const juce::String SIGNALING_SERVER_IP = "216.126.231.46"; // Production VPS
  const int SIGNALING_TCP_PORT = 54320;
  const int SIGNALING_UDP_PORT = 54321;

  juce::ChildProcess signalingProcess;
  void startLocalSignalingServer();

  // --- The Magic UDP Socket ---
  // We use ONE socket for both Signaling (Punching) and Peer Communication
  juce::DatagramSocket p2pSocket;

  juce::String peerIP;
  int peerPort = 0;
  bool isHost = false;

  // TCP Helper
  juce::String registerWithSignalingTCP();
  bool verifyCodeTCP(const juce::String &code);

  // UDP Packet Handling
  void sendPacket(PacketType type, const void *data, size_t size);
  void handleIncomingPacket(const void *data, int size,
                            const juce::String &senderIP, int senderPort);

  // Hole Punching Logic
  void startHolePunching();
  void reportError(const juce::String& error);
};
