#pragma once
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <functional>
#include <vector>
#include <atomic>
#include "ZenithCRDT.h"
#include "LoroCRDTBridge.h"

namespace zenith {

struct RemoteUser {
  juce::String id;
  juce::String name;
  juce::Colour color;
  juce::Point<float> mousePosition;
  juce::StringArray selectedIds;
  bool isOnline;
};

enum class PacketType {
  Hello = 0,
  CursorMove = 1,
  EditCommand = 2,
  Challenge = 3,
  ChallengeResponse = 4,
  CRDTUpdate = 5,
  SelectionUpdate = 6,
  KeepAlive = 99
};

class CollaborationManager : public juce::ChangeBroadcaster,
                             private juce::Thread {
  JUCE_DECLARE_WEAK_REFERENCEABLE(CollaborationManager)
  friend class CollaborationSecurityTest;
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
    Handshaking, // Challenge-Response Auth
    Hosting,     // Acting as session host
    Connected,   // P2P UDP Stream Active
    Error
  };

  void startHosting();
  void joinSession(const juce::String &code);
  void disconnect();

  ConnectionState getState() const { return currentState.load(std::memory_order_acquire); }
  juce::String getCurrentCode() const { return sessionCode; }

  // --- Real-time Sync ---
  void updateLocalCursor(float x, float y);
  std::vector<RemoteUser> getRemoteUsers() const {
    const juce::ScopedLock sl(usersLock);
    return remoteUsers;
  }
  void broadcastEdit(const juce::String &commandData);
  void broadcastSelection(const juce::StringArray& selectedIds);

  // --- User Identity ---
  void setLocalUserName(const juce::String &name) { localUserName = name; }
  juce::String getLocalUserName() const { return localUserName; }

  // --- Security ---
  void setAllowRemoteEditing(bool allow) { allowRemoteEditing = allow; }
  bool isRemoteEditingAllowed() const { return allowRemoteEditing; }

  std::function<void(const juce::String &)> onEditReceived;

  // --- CRDT Integration ---
  void initializeCRDT(juce::ValueTree& projectTree);
  void syncCRDT();
  void shutdownCRDT(); // Release CRDT bridge (must be called before ProjectState is destroyed)
  Zenith::ValueTreeCRDTBridge* getCRDTBridge() const { return crdtBridge.get(); }

private:
  CollaborationManager();
  ~CollaborationManager();

  // Loop
  void run() override;

  // CRITIC FIX: currentState MUST be atomic - accessed from message thread and network thread
  std::atomic<ConnectionState> currentState{ConnectionState::Disconnected};
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
  bool allowRemoteEditing = false;



  // TCP Helper
  juce::String registerWithSignalingTCP();
  bool verifyCodeTCP(const juce::String &code);

  // UDP Packet Handling
  void sendPacket(PacketType type, const void *data, size_t size, const juce::String& targetIP = {}, int targetPort = 0);
  void broadcastPacket(PacketType type, const void *data, size_t size);
  void handleIncomingPacket(const void *data, int size,
                            const juce::String &senderIP, int senderPort);

  // Hole Punching Logic
  void startHolePunching();
  void reportError(const juce::String& error);

  std::unique_ptr<Zenith::LoroDoc> crdtDoc;
  std::unique_ptr<Zenith::ValueTreeCRDTBridge> crdtBridge;

  struct PeerConnection {
      juce::String ip;
      int port;
      bool authenticated = false;
      juce::uint64 lastSeen = 0;
      int challenge = 0;
      juce::String salt;
  };
  std::vector<PeerConnection> activePeers;
  mutable juce::CriticalSection peersLock; 
  
  // Security Helper
  static juce::String calculateHMAC(const juce::String& message, const juce::String& secret);
  static bool validateSessionCode(const juce::String& code);
  static juce::MemoryBlock deriveSessionKey(const juce::String& password, const juce::String& salt);
};

} // namespace zenith
