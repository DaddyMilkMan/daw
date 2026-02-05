#pragma once
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <functional>
#include <vector>
#include <atomic>
#include <array>
#include "ZenithCRDT.h"
#include "DTLSSocket.h"
#include "AudioStreamingManager.h"

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
  AudioData = 10,
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
    Handshaking, // DTLS Handshake in progress
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
#ifdef ZENITH_ENABLE_COLLAB
  void initializeCRDT(juce::ValueTree& projectTree);
  void syncCRDT();
  void shutdownCRDT(); // Release CRDT bridge (must be called before ProjectState is destroyed)
  Zenith::ValueTreeCRDTBridge* getCRDTBridge() const { return crdtBridge.get(); }
#endif

  // --- Audio Streaming ---
  // Called from audio thread - lock-free, real-time safe
  void broadcastAudio(const juce::AudioBuffer<float>& buffer);
  
  // Called from audio thread to get remote audio - lock-free, real-time safe
  void getRemoteAudio(juce::AudioBuffer<float>& outputBuffer) {
      if (audioStreamer) {
          audioStreamer->getNextAudioBlock(outputBuffer);
      }
  }

private:
  CollaborationManager();
  ~CollaborationManager();

  // Loop
  void run() override;

  // CRITICAL FIX: currentState MUST be atomic - accessed from message thread and network thread
  std::atomic<ConnectionState> currentState{ConnectionState::Disconnected};
  juce::String sessionCode;
  juce::String localUserName = "User";
  std::vector<RemoteUser> remoteUsers;
  mutable juce::CriticalSection usersLock;

  // --- Networking ---
  juce::String signalingServerIP;
  const int SIGNALING_TCP_PORT = 54320;
  const int SIGNALING_UDP_PORT = 54321;

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

  // Hole Punching Logic
  void startHolePunching();
  void reportError(const juce::String& error);

#ifdef ZENITH_ENABLE_COLLAB
  std::unique_ptr<Zenith::LoroDoc> crdtDoc;
  std::unique_ptr<Zenith::ValueTreeCRDTBridge> crdtBridge;
#endif

  // --- Audio Streaming ---
  static constexpr int AUDIO_PACKET_QUEUE_SIZE = 64;
  static constexpr int MAX_AUDIO_PACKET_SIZE = 4000;
  
  struct AudioPacket {
      std::array<juce::uint8, MAX_AUDIO_PACKET_SIZE> data;
      size_t size = 0;
      std::atomic<bool> ready{false};
  };
  
  // Lock-free audio packet queue (single producer: audio thread, single consumer: network thread)
  std::array<AudioPacket, AUDIO_PACKET_QUEUE_SIZE> audioPacketQueue;
  std::atomic<int> audioQueueWriteIdx{0};
  std::atomic<int> audioQueueReadIdx{0};
  
  std::unique_ptr<network::AudioStreamingManager> audioStreamer;
  
  // Internal Helper to process audio
  void processAudioPacket(const void* data, int size, const juce::String& senderIP, int senderPort);
  
  // Network thread: drain audio packet queue
  void sendQueuedAudioPackets();
};

} // namespace zenith
