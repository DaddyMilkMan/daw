/*
  ==============================================================================
    CollaborativeSession.h
    Real-time collaborative editing system
    Phase 5: Advanced Features
  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_network/juce_network.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace zenith {
namespace collaboration {

// User information
struct UserInfo {
    juce::String id;
    juce::String name;
    juce::String email;
    juce::Colour avatarColor;
    bool isOnline = false;
    bool isHost = false;
    juce::Time lastSeen;
    juce::String currentTrack;
    juce::String currentAction;
};

// Operation types for collaborative editing
enum class OperationType {
    AddTrack,
    RemoveTrack,
    MoveTrack,
    AddEffect,
    RemoveEffect,
    MoveEffect,
    ChangeParameter,
    ChangeVolume,
    ChangePan,
    MuteTrack,
    SoloTrack,
    ArmTrack,
    EditAudio,
    EditMidi,
    AddMarker,
    RemoveMarker,
    TempoChange,
    TimeSignatureChange,
    Custom
};

// Collaborative operation
struct Operation {
    juce::String id;
    OperationType type;
    juce::String userId;
    juce::Time timestamp;
    juce::var data;
    juce::String targetId;  // track/effect ID
    int version;
    bool isUndoable = true;
    juce::String description;
};

// Conflict resolution strategies
enum class ConflictResolution {
    LastWriterWins,
    FirstWriterWins,
    Merge,
    Manual,
    OperationalTransform
};

// Session configuration
struct SessionConfig {
    juce::String sessionId;
    juce::String name;
    juce::String description;
    UserInfo host;
    int maxUsers = 10;
    bool autoSync = true;
    int syncInterval = 100;  // ms
    ConflictResolution conflictResolution = ConflictResolution::OperationalTransform;
    bool allowAudioStreaming = false;
    int audioQuality = 128;  // kbps
    bool enableChat = true;
    bool enableVideo = false;
    bool requirePassword = false;
    juce::String password;
};

// Audio stream data
struct AudioStreamData {
    juce::String userId;
    juce::AudioBuffer<float> audioBuffer;
    double sampleRate;
    int bitDepth;
    bool isMuted;
    float volume;
    juce::Time timestamp;
};

// Chat message
struct ChatMessage {
    juce::String id;
    juce::String userId;
    juce::String userName;
    juce::String message;
    juce::Time timestamp;
    bool isSystemMessage = false;
};

// Collaborative session manager
class CollaborativeSession : public juce::Timer,
                            public juce::InterprocessConnection,
                            public juce::InterprocessConnectionServer {
public:
    CollaborativeSession();
    ~CollaborativeSession() override;
    
    // Session management
    bool createSession(const SessionConfig& config);
    bool joinSession(const juce::String& sessionId, const UserInfo& user);
    void leaveSession();
    bool isSessionActive() const;
    SessionConfig getSessionConfig() const;
    
    // User management
    void addUser(const UserInfo& user);
    void removeUser(const juce::String& userId);
    std::vector<UserInfo> getConnectedUsers() const;
    UserInfo getCurrentUser() const;
    UserInfo getHost() const;
    
    // Operations
    void sendOperation(const Operation& operation);
    void applyOperation(const Operation& operation);
    std::vector<Operation> getOperationHistory() const;
    void clearOperationHistory();
    
    // Conflict resolution
    void setConflictResolution(ConflictResolution strategy);
    ConflictResolution getConflictResolution() const;
    bool resolveConflict(const Operation& op1, const Operation& op2);
    
    // Audio streaming
    void enableAudioStreaming(bool enabled);
    bool isAudioStreamingEnabled() const;
    void sendAudioStream(const AudioStreamData& audioData);
    void receiveAudioStream(const AudioStreamData& audioData);
    
    // Chat
    void sendChatMessage(const juce::String& message);
    std::vector<ChatMessage> getChatHistory() const;
    void clearChatHistory();
    
    // Synchronization
    void syncWithServer();
    void forceSync();
    bool isSyncing() const;
    juce::Time getLastSyncTime() const;
    
    // Timer callback for sync
    void timerCallback() override;
    
    // Network callbacks
    void connectionMade() override;
    void connectionLost() override;
    void messageReceived(const juce::MemoryBlock& message) override;
    
    // Listeners
    struct Listener {
        virtual ~Listener() = default;
        virtual void sessionCreated(const SessionConfig& config) {}
        virtual void sessionJoined(const UserInfo& user) {}
        virtual void sessionLeft(const UserInfo& user) {}
        virtual void userConnected(const UserInfo& user) {}
        virtual void userDisconnected(const UserInfo& user) {}
        virtual void operationReceived(const Operation& operation) {}
        virtual void operationApplied(const Operation& operation) {}
        virtual void conflictDetected(const Operation& op1, const Operation& op2) {}
        virtual void audioStreamReceived(const AudioStreamData& audioData) {}
        virtual void chatMessageReceived(const ChatMessage& message) {}
        virtual void syncStarted() {}
        virtual void syncCompleted() {}
        virtual void syncFailed(const juce::String& error) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    // Session state
    std::unique_ptr<SessionConfig> sessionConfig;
    UserInfo currentUser;
    std::vector<UserInfo> connectedUsers;
    bool isHost = false;
    bool isSessionActive = false;
    
    // Operations
    std::vector<Operation> operationHistory;
    std::unordered_map<juce::String, int> operationVersions;
    std::mutex operationMutex;
    
    // Audio streaming
    bool audioStreamingEnabled = false;
    std::unordered_map<juce::String, AudioStreamData> audioStreams;
    std::mutex audioMutex;
    
    // Chat
    std::vector<ChatMessage> chatHistory;
    std::mutex chatMutex;
    
    // Synchronization
    bool isCurrentlySyncing = false;
    juce::Time lastSyncTime;
    ConflictResolution conflictStrategy = ConflictResolution::OperationalTransform;
    
    // Network
    std::unique_ptr<juce::WebSocketClient> webSocketClient;
    std::unique_ptr<juce::WebSocketServer> webSocketServer;
    juce::String serverUrl;
    int serverPort = 8080;
    
    // Listeners
    std::vector<Listener*> listeners;
    
    // Internal methods
    void initializeNetworking();
    void connectToServer();
    void startServer();
    void disconnectFromServer();
    
    // Operation handling
    juce::String generateOperationId() const;
    void incrementOperationVersion(const juce::String& targetId);
    bool validateOperation(const Operation& operation) const;
    Operation transformOperation(const Operation& op1, const Operation& op2) const;
    
    // Message serialization
    juce::MemoryBlock serializeOperation(const Operation& operation) const;
    Operation deserializeOperation(const juce::MemoryBlock& data) const;
    
    juce::MemoryBlock serializeUser(const UserInfo& user) const;
    UserInfo deserializeUser(const juce::MemoryBlock& data) const;
    
    juce::MemoryBlock serializeAudioData(const AudioStreamData& audioData) const;
    AudioStreamData deserializeAudioData(const juce::MemoryBlock& data) const;
    
    juce::MemoryBlock serializeChatMessage(const ChatMessage& message) const;
    ChatMessage deserializeChatMessage(const juce::MemoryBlock& data) const;
    
    // Conflict resolution
    bool operationsConflict(const Operation& op1, const Operation& op2) const;
    Operation mergeOperations(const Operation& op1, const Operation& op2) const;
    
    // Notification
    void notifySessionCreated(const SessionConfig& config);
    void notifySessionJoined(const UserInfo& user);
    void notifySessionLeft(const UserInfo& user);
    void notifyUserConnected(const UserInfo& user);
    void notifyUserDisconnected(const UserInfo& user);
    void notifyOperationReceived(const Operation& operation);
    void notifyOperationApplied(const Operation& operation);
    void notifyConflictDetected(const Operation& op1, const Operation& op2);
    void notifyAudioStreamReceived(const AudioStreamData& audioData);
    void notifyChatMessageReceived(const ChatMessage& message);
    void notifySyncStarted();
    void notifySyncCompleted();
    void notifySyncFailed(const juce::String& error);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CollaborativeSession)
};

// Collaborative editor component
class CollaborativeEditor : public juce::Component,
                          public CollaborativeSession::Listener {
public:
    CollaborativeEditor();
    ~CollaborativeEditor() override;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Session management
    void setSession(CollaborativeSession* session);
    CollaborativeSession* getSession() const { return session; }
    
    // User interface
    void showUserList(bool show);
    void showChat(bool show);
    void showOperationHistory(bool show);
    
    // Collaboration features
    void highlightUserActions(const juce::String& userId, bool highlight);
    void showUserCursors(bool show);
    void enableRealTimeUpdates(bool enabled);
    
    // CollaborativeSession::Listener overrides
    void userConnected(const UserInfo& user) override;
    void userDisconnected(const UserInfo& user) override;
    void operationReceived(const Operation& operation) override;
    void operationApplied(const Operation& operation) override;
    void chatMessageReceived(const ChatMessage& message) override;
    
private:
    CollaborativeSession* session = nullptr;
    
    // UI components
    std::unique_ptr<juce::Viewport> mainViewport;
    std::unique_ptr<juce::Component> mainComponent;
    
    // User list
    std::unique_ptr<juce::ListBox> userListBox;
    std::unique_ptr<juce::TextButton> inviteButton;
    
    // Chat
    std::unique_ptr<juce::Viewport> chatViewport;
    std::unique_ptr<juce::Component> chatComponent;
    std::unique_ptr<juce::ListBox> chatListBox;
    std::unique_ptr<juce::TextEditor> messageEditor;
    std::unique_ptr<juce::TextButton> sendButton;
    
    // Operation history
    std::unique_ptr<juce::ListBox> operationListBox;
    std::unique_ptr<juce::TextButton> clearHistoryButton;
    
    // Settings
    std::unique_ptr<juce::ToggleButton> realTimeToggle;
    std::unique_ptr<juce::ToggleButton> highlightToggle;
    std::unique_ptr<juce::ToggleButton> cursorsToggle;
    
    // UI creation
    void createUserInterface();
    void createChatInterface();
    void createOperationHistory();
    void createSettingsInterface();
    
    // Updates
    void updateUserList();
    void updateChatDisplay();
    void updateOperationHistory();
    
    // User list model
    class UserListModel : public juce::ListBoxModel {
    public:
        UserListModel(const std::vector<UserInfo>& users);
        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height) override;
        juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
        
        void setUsers(const std::vector<UserInfo>& users);
        
    private:
        std::vector<UserInfo> users;
    };
    
    // Chat model
    class ChatModel : public juce::ListBoxModel {
    public:
        ChatModel(const std::vector<ChatMessage>& messages);
        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height) override;
        
        void setMessages(const std::vector<ChatMessage>& messages);
        
    private:
        std::vector<ChatMessage> messages;
    };
    
    std::unique_ptr<UserListModel> userListModel;
    std::unique_ptr<ChatModel> chatModel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CollaborativeEditor)
};

// Operational transform implementation
class OperationalTransform {
public:
    // Transform operations based on type
    static Operation transform(const Operation& op1, const Operation& op2);
    
    // Specific transform methods
    static Operation transformAddTrack(const Operation& op1, const Operation& op2);
    static Operation transformRemoveTrack(const Operation& op1, const Operation& op2);
    static Operation transformMoveTrack(const Operation& op1, const Operation& op2);
    static Operation transformAddEffect(const Operation& op1, const Operation& op2);
    static Operation transformRemoveEffect(const Operation& op1, const Operation& op2);
    static Operation transformMoveEffect(const Operation& op1, const Operation& op2);
    static Operation transformChangeParameter(const Operation& op1, const Operation& op2);
    
    // Utility methods
    static bool canTransform(const Operation& op1, const Operation& op2);
    static bool areOperationsConcurrent(const Operation& op1, const Operation& op2);
    static Operation compose(const Operation& op1, const Operation& op2);
    static Operation invert(const Operation& operation);
    
private:
    OperationalTransform() = delete;
};

// Session discovery service
class SessionDiscovery {
public:
    struct DiscoveredSession {
        juce::String id;
        juce::String name;
        juce::String host;
        int port;
        int currentUsers;
        int maxUsers;
        bool hasPassword;
        juce::String description;
        juce::Time discovered;
    };
    
    SessionDiscovery();
    ~SessionDiscovery() = default;
    
    // Discovery
    void startDiscovery();
    void stopDiscovery();
    bool isDiscovering() const;
    
    // Sessions
    std::vector<DiscoveredSession> getDiscoveredSessions() const;
    void clearDiscoveredSessions();
    
    // Broadcasting (for hosts)
    void startBroadcasting(const SessionConfig& config);
    void stopBroadcasting();
    bool isBroadcasting() const;
    
    // Listeners
    struct Listener {
        virtual ~Listener() = default;
        virtual void sessionDiscovered(const DiscoveredSession& session) {}
        virtual void sessionLost(const DiscoveredSession& session) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    bool isCurrentlyDiscovering = false;
    bool isCurrentlyBroadcasting = false;
    
    std::vector<DiscoveredSession> discoveredSessions;
    std::mutex sessionsMutex;
    
    std::unique_ptr<juce::DatagramSocket> discoverySocket;
    std::unique_ptr<juce::DatagramSocket> broadcastSocket;
    
    std::vector<Listener*> listeners;
    
    static constexpr int DISCOVERY_PORT = 19000;
    static constexpr int BROADCAST_INTERVAL = 5000;  // 5 seconds
    
    void sendDiscoveryRequest();
    void sendBroadcastMessage();
    void handleDiscoveryResponse(const juce::MemoryBlock& data, const juce::String& senderAddress);
    void handleBroadcastMessage(const juce::MemoryBlock& data, const juce::String& senderAddress);
    
    void notifySessionDiscovered(const DiscoveredSession& session);
    void notifySessionLost(const DiscoveredSession& session);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionDiscovery)
};

} // namespace collaboration
} // namespace zenith
