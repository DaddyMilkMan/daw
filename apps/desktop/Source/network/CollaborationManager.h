#pragma once
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <functional>
#include <vector>
#include <atomic>
#include <cstdint>
#include "ZenithCRDT.h"
#include "DTLSSocket.h"
#include "ICECandidate.h"
#include "STUNClient.h"
#include "TURNClient.h"

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
  CRDTUpdate = 5,
  SelectionUpdate = 6,
  KeepAlive = 99,
  Pong = 100  // Response to KeepAlive
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
    Registering,      // Getting Code from TCP
    GatheringICE,     // Gathering ICE candidates
    ExchangingCandidates, // Exchanging ICE candidates via signaling
    ICEConnecting,    // ICE connectivity checks in progress
    Handshaking,      // DTLS Handshake in progress
    Connected,        // P2P UDP Stream Active
    Failed,           // Connection failed
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

  // --- Configuration ---
  void setSignalingServer(const juce::String& ip, int tcpPort = 54320, int udpPort = 54321);
  void setSTUNServers(const juce::StringArray& servers);
  void setTURNServers(const juce::StringArray& servers, const juce::String& username, const juce::String& password);

  std::function<void(const juce::String &)> onEditReceived;

  // --- CRDT Integration ---
#ifdef ZENITH_ENABLE_COLLAB
  void initializeCRDT(juce::ValueTree& projectTree);
  void syncCRDT();
  void shutdownCRDT(); // Release CRDT bridge (must be called before ProjectState is destroyed)
  Zenith::ValueTreeCRDTBridge* getCRDTBridge() const { return crdtBridge.get(); }
#endif

private:
  CollaborationManager();
  ~CollaborationManager();

  // Loop
  void run() override;

  // CRITIC FIX: currentState MUST be atomic - accessed from message thread and network thread
  std::atomic<ConnectionState> currentState{ConnectionState::Disconnected};
  std::atomic<bool> shouldStop{false};  // CRITIC FIX: Graceful shutdown flag
  juce::String sessionCode;
  juce::String localUserName = "User";
  std::vector<RemoteUser> remoteUsers;
  mutable juce::CriticalSection usersLock;

  // --- Networking ---
  juce::String signalingServerIP;
  int signalingTCPPort = 54320;
  int signalingUDPPort = 54321;

  juce::ChildProcess signalingProcess;
  void startLocalSignalingServer();

  // --- The Magic UDP Socket ---
  // We use ONE socket for both Signaling (Punching) and Peer Communication
  DTLSSocket p2pSocket;

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

  // Hole Punching Logic (Legacy - replaced by ICE)
  void startHolePunching();
  void reportError(const juce::String& error);

  // --- ICE / NAT Traversal ---
  void gatherICECandidates();
  void exchangeICECandidates();
  void performICEConnectivityChecks();
  void selectBestICECandidate();

  // --- TURN Relay Support ---
  void allocateTURNRelay();
  void deallocateTURNRelay();

  // Perform STUN connectivity check through a specific candidate pair
  bool performConnectivityCheck(const ICECandidatePair& pair);

  // Check if any ICE pair has succeeded
  bool hasSucceededICEPair() const;

  // ICE candidate management
  std::vector<ICECandidate> localICECandidates;
  std::vector<ICECandidate> remoteICECandidates;
  juce::CriticalSection iceCandidatesLock;

  // STUN client for NAT discovery
  std::unique_ptr<STUNClient> stunClient;

  // TURN client for relay allocation
  std::unique_ptr<TURNClient> turnClient;

  // TURN allocation result (when using relay)
  TURNAllocationResult turnAllocation;
  bool turnAllocated = false;

  // ICE agent for role and tie-breaker management
  ICEAgent iceAgent;

  // All candidate pairs for connectivity checking
  std::vector<ICECandidatePair> iceCandidatePairs;
  juce::CriticalSection icePairsLock;

  // STUN servers to use
  juce::StringArray stunServers;

  // TURN servers to use (for fallback with symmetric NAT)
  juce::StringArray turnServers;
  juce::String turnUsername;
  juce::String turnPassword;

  // Selected ICE candidates for connection
  ICECandidate selectedLocalCandidate;
  ICECandidate selectedRemoteCandidate;
  bool iceCandidatesSelected = false;

  // ICE connectivity check state
  juce::int64 lastICECheckTime = 0;
  static constexpr int ICE_CHECK_INTERVAL_MS = 100;
  static constexpr int ICE_CHECK_TIMEOUT_MS = 30000;

  // Pre-allocated buffer for packet reception (avoids heap allocations in hot path)
  static constexpr int PACKET_BUFFER_SIZE = 2048;
  std::array<char, PACKET_BUFFER_SIZE> packetBuffer{};

  // Reusable socket for ICE connectivity checks (avoids creating new socket per check)
  juce::DatagramSocket iceCheckSocket;
  bool iceCheckSocketBound = false;

  // Thread pool for async operations (reduces thread creation overhead from ~40MB to ~8MB)
  juce::ThreadPool asyncThreadPool{4};  // 4 threads is optimal for most systems

  // Pre-allocated buffer for remote ID construction (avoids repeated string allocations)
  std::array<char, 64> remoteIdBuffer{};  // "xxx.xxx.xxx.xxx:xxxxx" fits easily

#ifdef ZENITH_ENABLE_COLLAB
  std::unique_ptr<Zenith::LoroDoc> crdtDoc;
  std::unique_ptr<Zenith::ValueTreeCRDTBridge> crdtBridge;
#endif

  struct PeerConnection {
      juce::String ip;
      int port;
      bool authenticated = false;
      juce::uint64 lastSeen = 0;
  };
  std::vector<PeerConnection> activePeers;
  mutable juce::CriticalSection peersLock; // CRITIC FIX: Protect activePeers from race conditions

  // Sequence number tracking for deduplication and ordering
  std::atomic<uint32_t> nextSequenceNumber{1};
  uint32_t lastReceivedSequenceNumber = 0;
  juce::CriticalSection sequenceLock;

  // Rate limiting to prevent packet flood attacks
  juce::int64 lastPacketTime = 0;
  int packetsInLastSecond = 0;
  static constexpr int MAX_PACKETS_PER_SECOND = 1000;  // Rate limit threshold
  juce::int64 rateLimitWindowStart = 0;

  // Heartbeat verification - track when we last received a Pong response
  juce::int64 lastPongResponseTime = 0;
  std::atomic<bool> waitingForPong{false};

  // WeakReference support for safe lambda captures
  JUCE_DECLARE_WEAK_REFERENCEABLE(CollaborationManager)
};

} // namespace zenith
