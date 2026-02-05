/*
  ==============================================================================
    CollaborativeSession.cpp
    Real-time collaborative editing implementation
  ==============================================================================
*/

#include "CollaborativeSession.h"
#include <algorithm>
#include <random>

namespace zenith {
namespace collaboration {

// CollaborativeSession Implementation
CollaborativeSession::CollaborativeSession() {
    initializeNetworking();
}

CollaborativeSession::~CollaborativeSession() {
    leaveSession();
    stopTimer();
}

bool CollaborativeSession::createSession(const SessionConfig& config) {
    sessionConfig = std::make_unique<SessionConfig>(config);
    currentUser = config.host;
    isHost = true;
    isSessionActive = true;
    
    // Start server
    startServer();
    
    // Add host to connected users
    connectedUsers.push_back(currentUser);
    
    // Start sync timer
    startTimer(config.syncInterval);
    
    notifySessionCreated(config);
    return true;
}

bool CollaborativeSession::joinSession(const juce::String& sessionId, const UserInfo& user) {
    if (isSessionActive) {
        return false;  // Already in a session
    }
    
    currentUser = user;
    isHost = false;
    
    // Connect to server
    serverUrl = "ws://localhost:" + juce::String(serverPort);
    connectToServer();
    
    // Send join request
    juce::DynamicObject::Ptr joinRequest = new juce::DynamicObject();
    joinRequest->setProperty("type", "join");
    joinRequest->setProperty("sessionId", sessionId);
    joinRequest->setProperty("user", serializeUser(user).toString());
    
    if (webSocketClient && webSocketClient->isConnected()) {
        webSocketClient->send(juce::JSON::toString(joinRequest));
    }
    
    isSessionActive = true;
    notifySessionJoined(user);
    
    return true;
}

void CollaborativeSession::leaveSession() {
    if (!isSessionActive) {
        return;
    }
    
    // Send leave notification
    if (webSocketClient && webSocketClient->isConnected()) {
        juce::DynamicObject::Ptr leaveRequest = new juce::DynamicObject();
        leaveRequest->setProperty("type", "leave");
        leaveRequest->setProperty("userId", currentUser.id);
        webSocketClient->send(juce::JSON::toString(leaveRequest));
    }
    
    // Disconnect
    disconnectFromServer();
    
    // Clear state
    isSessionActive = false;
    connectedUsers.clear();
    operationHistory.clear();
    audioStreams.clear();
    chatHistory.clear();
    
    stopTimer();
    
    notifySessionLeft(currentUser);
}

bool CollaborativeSession::isSessionActive() const {
    return isSessionActive;
}

SessionConfig CollaborativeSession::getSessionConfig() const {
    if (sessionConfig) {
        return *sessionConfig;
    }
    return SessionConfig{};
}

void CollaborativeSession::addUser(const UserInfo& user) {
    std::lock_guard<std::mutex> lock(operationMutex);
    
    // Check if user already exists
    auto it = std::find_if(connectedUsers.begin(), connectedUsers.end(),
                          [&user](const UserInfo& u) { return u.id == user.id; });
    
    if (it == connectedUsers.end()) {
        connectedUsers.push_back(user);
        notifyUserConnected(user);
    }
}

void CollaborativeSession::removeUser(const juce::String& userId) {
    std::lock_guard<std::mutex> lock(operationMutex);
    
    auto it = std::find_if(connectedUsers.begin(), connectedUsers.end(),
                          [&userId](const UserInfo& u) { return u.id == userId; });
    
    if (it != connectedUsers.end()) {
        UserInfo user = *it;
        connectedUsers.erase(it);
        
        // Remove user's audio stream
        auto audioIt = audioStreams.find(userId);
        if (audioIt != audioStreams.end()) {
            audioStreams.erase(audioIt);
        }
        
        notifyUserDisconnected(user);
    }
}

std::vector<UserInfo> CollaborativeSession::getConnectedUsers() const {
    std::lock_guard<std::mutex> lock(operationMutex);
    return connectedUsers;
}

UserInfo CollaborativeSession::getCurrentUser() const {
    return currentUser;
}

UserInfo CollaborativeSession::getHost() const {
    if (sessionConfig) {
        return sessionConfig->host;
    }
    return UserInfo{};
}

void CollaborativeSession::sendOperation(const Operation& operation) {
    if (!isSessionActive || !webSocketClient || !webSocketClient->isConnected()) {
        return;
    }
    
    // Add to local history
    {
        std::lock_guard<std::mutex> lock(operationMutex);
        operationHistory.push_back(operation);
        incrementOperationVersion(operation.targetId);
    }
    
    // Send to server
    juce::MemoryBlock data = serializeOperation(operation);
    webSocketClient->send(data.toString());
    
    notifyOperationReceived(operation);
}

void CollaborativeSession::applyOperation(const Operation& operation) {
    std::lock_guard<std::mutex> lock(operationMutex);
    
    // Check for conflicts
    for (const auto& existingOp : operationHistory) {
        if (operationsConflict(existingOp, operation)) {
            notifyConflictDetected(existingOp, operation);
            
            // Resolve conflict
            Operation resolvedOp = resolveConflict(existingOp, operation);
            
            // Apply resolved operation
            operationHistory.push_back(resolvedOp);
            notifyOperationApplied(resolvedOp);
            return;
        }
    }
    
    // No conflict, apply directly
    operationHistory.push_back(operation);
    incrementOperationVersion(operation.targetId);
    notifyOperationApplied(operation);
}

std::vector<Operation> CollaborativeSession::getOperationHistory() const {
    std::lock_guard<std::mutex> lock(operationMutex);
    return operationHistory;
}

void CollaborativeSession::clearOperationHistory() {
    std::lock_guard<std::mutex> lock(operationMutex);
    operationHistory.clear();
    operationVersions.clear();
}

void CollaborativeSession::setConflictResolution(ConflictResolution strategy) {
    conflictStrategy = strategy;
}

ConflictResolution CollaborativeSession::getConflictResolution() const {
    return conflictStrategy;
}

bool CollaborativeSession::resolveConflict(const Operation& op1, const Operation& op2) {
    switch (conflictStrategy) {
        case ConflictResolution::LastWriterWins:
            // Use the operation with the later timestamp
            return op2.timestamp > op1.timestamp;
            
        case ConflictResolution::FirstWriterWins:
            // Use the operation with the earlier timestamp
            return op1.timestamp < op2.timestamp;
            
        case ConflictResolution::Merge:
            // Attempt to merge operations
            if (OperationalTransform::canTransform(op1, op2)) {
                Operation merged = OperationalTransform::compose(op1, op2);
                operationHistory.push_back(merged);
                return true;
            }
            return false;
            
        case ConflictResolution::OperationalTransform:
            // Use operational transform
            if (OperationalTransform::canTransform(op1, op2)) {
                Operation transformed = OperationalTransform::transform(op1, op2);
                operationHistory.push_back(transformed);
                return true;
            }
            return false;
            
        case ConflictResolution::Manual:
            // Signal that manual intervention is needed
            return false;
    }
    
    return false;
}

void CollaborativeSession::enableAudioStreaming(bool enabled) {
    audioStreamingEnabled = enabled;
    
    if (enabled && sessionConfig) {
        // Configure audio streaming
        juce::DynamicObject::Ptr config = new juce::DynamicObject();
        config->setProperty("type", "audio_config");
        config->setProperty("enabled", true);
        config->setProperty("quality", sessionConfig->audioQuality);
        
        if (webSocketClient && webSocketClient->isConnected()) {
            webSocketClient->send(juce::JSON::toString(config));
        }
    }
}

bool CollaborativeSession::isAudioStreamingEnabled() const {
    return audioStreamingEnabled;
}

void CollaborativeSession::sendAudioStream(const AudioStreamData& audioData) {
    if (!audioStreamingEnabled || !webSocketClient || !webSocketClient->isConnected()) {
        return;
    }
    
    juce::MemoryBlock data = serializeAudioData(audioData);
    webSocketClient->send(data.toString());
}

void CollaborativeSession::receiveAudioStream(const AudioStreamData& audioData) {
    std::lock_guard<std::mutex> lock(audioMutex);
    audioStreams[audioData.userId] = audioData;
    notifyAudioStreamReceived(audioData);
}

void CollaborativeSession::sendChatMessage(const juce::String& message) {
    if (!isSessionActive || !webSocketClient || !webSocketClient->isConnected()) {
        return;
    }
    
    ChatMessage chatMsg;
    chatMsg.id = juce::Uuid().toString();
    chatMsg.userId = currentUser.id;
    chatMsg.userName = currentUser.name;
    chatMsg.message = message;
    chatMsg.timestamp = juce::Time::getCurrentTime();
    
    juce::MemoryBlock data = serializeChatMessage(chatMsg);
    webSocketClient->send(data.toString());
    
    // Add to local history
    {
        std::lock_guard<std::mutex> lock(chatMutex);
        chatHistory.push_back(chatMsg);
    }
    
    notifyChatMessageReceived(chatMsg);
}

std::vector<ChatMessage> CollaborativeSession::getChatHistory() const {
    std::lock_guard<std::mutex> lock(chatMutex);
    return chatHistory;
}

void CollaborativeSession::clearChatHistory() {
    std::lock_guard<std::mutex> lock(chatMutex);
    chatHistory.clear();
}

void CollaborativeSession::syncWithServer() {
    if (!isSessionActive || isCurrentlySyncing) {
        return;
    }
    
    isCurrentlySyncing = true;
    notifySyncStarted();
    
    // Send sync request
    juce::DynamicObject::Ptr syncRequest = new juce::DynamicObject();
    syncRequest->setProperty("type", "sync");
    syncRequest->setProperty("lastSync", lastSyncTime.toMilliseconds());
    
    if (webSocketClient && webSocketClient->isConnected()) {
        webSocketClient->send(juce::JSON::toString(syncRequest));
    }
    
    // Simulate sync completion (in real implementation, wait for response)
    juce::Timer::callAfterDelay(1000, [this]() {
        isCurrentlySyncing = false;
        lastSyncTime = juce::Time::getCurrentTime();
        notifySyncCompleted();
    });
}

void CollaborativeSession::forceSync() {
    syncWithServer();
}

bool CollaborativeSession::isSyncing() const {
    return isCurrentlySyncing;
}

juce::Time CollaborativeSession::getLastSyncTime() const {
    return lastSyncTime;
}

void CollaborativeSession::timerCallback() {
    if (sessionConfig && sessionConfig->autoSync) {
        syncWithServer();
    }
}

void CollaborativeSession::connectionMade() {
    // Connection established
    DBG("Collaborative session connected");
}

void CollaborativeSession::connectionLost() {
    // Connection lost
    DBG("Collaborative session disconnected");
    
    // Attempt to reconnect
    juce::Timer::callAfterDelay(1000, [this]() {
        if (isSessionActive) {
            connectToServer();
        }
    });
}

void CollaborativeSession::messageReceived(const juce::MemoryBlock& message) {
    juce::var data = juce::JSON::parse(message.toString());
    
    if (!data.isObject()) {
        return;
    }
    
    juce::String type = data.getProperty("type", "");
    
    if (type == "operation") {
        Operation op = deserializeOperation(message);
        applyOperation(op);
    } else if (type == "user_joined") {
        UserInfo user = deserializeUser(data.getProperty("user", "").toString());
        addUser(user);
    } else if (type == "user_left") {
        juce::String userId = data.getProperty("userId", "");
        removeUser(userId);
    } else if (type == "audio_stream") {
        AudioStreamData audioData = deserializeAudioData(message);
        receiveAudioStream(audioData);
    } else if (type == "chat_message") {
        ChatMessage chatMsg = deserializeChatMessage(message);
        {
            std::lock_guard<std::mutex> lock(chatMutex);
            chatHistory.push_back(chatMsg);
        }
        notifyChatMessageReceived(chatMsg);
    } else if (type == "sync_response") {
        isCurrentlySyncing = false;
        lastSyncTime = juce::Time::getCurrentTime();
        notifySyncCompleted();
    }
}

void CollaborativeSession::addListener(Listener* listener) {
    listeners.push_back(listener);
}

void CollaborativeSession::removeListener(Listener* listener) {
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

void CollaborativeSession::initializeNetworking() {
    // Initialize WebSocket components
    webSocketClient = std::make_unique<juce::WebSocketClient>();
    webSocketServer = std::make_unique<juce::WebSocketServer>();
}

void CollaborativeSession::connectToServer() {
    if (webSocketClient) {
        webSocketClient->connect(serverUrl);
    }
}

void CollaborativeSession::startServer() {
    if (webSocketServer) {
        webSocketServer->start(serverPort);
    }
}

void CollaborativeSession::disconnectFromServer() {
    if (webSocketClient) {
        webSocketClient->disconnect();
    }
    
    if (webSocketServer) {
        webSocketServer->stop();
    }
}

juce::String CollaborativeSession::generateOperationId() const {
    return juce::Uuid().toString();
}

void CollaborativeSession::incrementOperationVersion(const juce::String& targetId) {
    operationVersions[targetId]++;
}

bool CollaborativeSession::validateOperation(const Operation& operation) const {
    // Check if operation has required fields
    if (operation.id.isEmpty() || operation.userId.isEmpty()) {
        return false;
    }
    
    // Check timestamp
    if (!operationTimestamp.isValid()) {
        return false;
    }
    
    // Check version
    auto it = operationVersions.find(operation.targetId);
    if (it != operationVersions.end() && operation.version <= it->second) {
        return false;  // Outdated operation
    }
    
    return true;
}

Operation CollaborativeSession::transformOperation(const Operation& op1, const Operation& op2) const {
    return OperationalTransform::transform(op1, op2);
}

juce::MemoryBlock CollaborativeSession::serializeOperation(const Operation& operation) const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    
    obj->setProperty("id", operation.id);
    obj->setProperty("type", static_cast<int>(operation.type));
    obj->setProperty("userId", operation.userId);
    obj->setProperty("timestamp", operation.timestamp.toMilliseconds());
    obj->setProperty("data", juce::JSON::toString(operation.data));
    obj->setProperty("targetId", operation.targetId);
    obj->setProperty("version", operation.version);
    obj->setProperty("isUndoable", operation.isUndoable);
    obj->setProperty("description", operation.description);
    
    return juce::MemoryBlock(juce::JSON::toString(obj).toUTF8(), strlen(juce::JSON::toString(obj).toUTF8()));
}

Operation CollaborativeSession::deserializeOperation(const juce::MemoryBlock& data) const {
    juce::var parsed = juce::JSON::parse(data.toString());
    juce::DynamicObject* obj = parsed.getDynamicObject();
    
    if (!obj) {
        return Operation{};
    }
    
    Operation operation;
    operation.id = obj->getProperty("id");
    operation.type = static_cast<OperationType>(static_cast<int>(obj->getProperty("type")));
    operation.userId = obj->getProperty("userId");
    operation.timestamp = juce::Time(obj->getProperty("timestamp"));
    operation.data = juce::JSON::parse(obj->getProperty("data"));
    operation.targetId = obj->getProperty("targetId");
    operation.version = obj->getProperty("version");
    operation.isUndoable = obj->getProperty("isUndoable");
    operation.description = obj->getProperty("description");
    
    return operation;
}

juce::MemoryBlock CollaborativeSession::serializeUser(const UserInfo& user) const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    
    obj->setProperty("id", user.id);
    obj->setProperty("name", user.name);
    obj->setProperty("email", user.email);
    obj->setProperty("avatarColor", user.avatarColor.toString());
    obj->setProperty("isOnline", user.isOnline);
    obj->setProperty("isHost", user.isHost);
    obj->setProperty("lastSeen", user.lastSeen.toMilliseconds());
    obj->setProperty("currentTrack", user.currentTrack);
    obj->setProperty("currentAction", user.currentAction);
    
    return juce::MemoryBlock(juce::JSON::toString(obj).toUTF8(), strlen(juce::JSON::toString(obj).toUTF8()));
}

UserInfo CollaborativeSession::deserializeUser(const juce::MemoryBlock& data) const {
    juce::var parsed = juce::JSON::parse(data.toString());
    juce::DynamicObject* obj = parsed.getDynamicObject();
    
    if (!obj) {
        return UserInfo{};
    }
    
    UserInfo user;
    user.id = obj->getProperty("id");
    user.name = obj->getProperty("name");
    user.email = obj->getProperty("email");
    user.avatarColor = juce::Colour::fromString(obj->getProperty("avatarColor"));
    user.isOnline = obj->getProperty("isOnline");
    user.isHost = obj->getProperty("isHost");
    user.lastSeen = juce::Time(obj->getProperty("lastSeen"));
    user.currentTrack = obj->getProperty("currentTrack");
    user.currentAction = obj->getProperty("currentAction");
    
    return user;
}

bool CollaborativeSession::operationsConflict(const Operation& op1, const Operation& op2) const {
    // Operations conflict if they target the same object and are concurrent
    if (op1.targetId != op2.targetId) {
        return false;
    }
    
    if (op1.userId == op2.userId) {
        return false;  // Same user, no conflict
    }
    
    // Check if operations are concurrent (neither happened before the other)
    bool op1BeforeOp2 = op1.timestamp < op2.timestamp;
    bool op2BeforeOp1 = op2.timestamp < op1.timestamp;
    
    return !op1BeforeOp2 && !op2BeforeOp1;
}

Operation CollaborativeSession::mergeOperations(const Operation& op1, const Operation& op2) const {
    if (!OperationalTransform::canTransform(op1, op2)) {
        return op1;  // Cannot merge, return first
    }
    
    return OperationalTransform::compose(op1, op2);
}

void CollaborativeSession::notifySessionCreated(const SessionConfig& config) {
    for (auto* listener : listeners) {
        listener->sessionCreated(config);
    }
}

void CollaborativeSession::notifySessionJoined(const UserInfo& user) {
    for (auto* listener : listeners) {
        listener->sessionJoined(user);
    }
}

void CollaborativeSession::notifySessionLeft(const UserInfo& user) {
    for (auto* listener : listeners) {
        listener->sessionLeft(user);
    }
}

void CollaborativeSession::notifyUserConnected(const UserInfo& user) {
    for (auto* listener : listeners) {
        listener->userConnected(user);
    }
}

void CollaborativeSession::notifyUserDisconnected(const UserInfo& user) {
    for (auto* listener : listeners) {
        listener->userDisconnected(user);
    }
}

void CollaborativeSession::notifyOperationReceived(const Operation& operation) {
    for (auto* listener : listeners) {
        listener->operationReceived(operation);
    }
}

void CollaborativeSession::notifyOperationApplied(const Operation& operation) {
    for (auto* listener : listeners) {
        listener->operationApplied(operation);
    }
}

void CollaborativeSession::notifyConflictDetected(const Operation& op1, const Operation& op2) {
    for (auto* listener : listeners) {
        listener->conflictDetected(op1, op2);
    }
}

void CollaborativeSession::notifyAudioStreamReceived(const AudioStreamData& audioData) {
    for (auto* listener : listeners) {
        listener->audioStreamReceived(audioData);
    }
}

void CollaborativeSession::notifyChatMessageReceived(const ChatMessage& message) {
    for (auto* listener : listeners) {
        listener->chatMessageReceived(message);
    }
}

void CollaborativeSession::notifySyncStarted() {
    for (auto* listener : listeners) {
        listener->syncStarted();
    }
}

void CollaborativeSession::notifySyncCompleted() {
    for (auto* listener : listeners) {
        listener->syncCompleted();
    }
}

void CollaborativeSession::notifySyncFailed(const juce::String& error) {
    for (auto* listener : listeners) {
        listener->syncFailed(error);
    }
}

// OperationalTransform Implementation
Operation OperationalTransform::transform(const Operation& op1, const Operation& op2) {
    if (!canTransform(op1, op2)) {
        return op1;  // Cannot transform, return first
    }
    
    switch (op1.type) {
        case OperationType::AddTrack:
            return transformAddTrack(op1, op2);
        case OperationType::RemoveTrack:
            return transformRemoveTrack(op1, op2);
        case OperationType::MoveTrack:
            return transformMoveTrack(op1, op2);
        case OperationType::AddEffect:
            return transformAddEffect(op1, op2);
        case OperationType::RemoveEffect:
            return transformRemoveEffect(op1, op2);
        case OperationType::MoveEffect:
            return transformMoveEffect(op1, op2);
        case OperationType::ChangeParameter:
            return transformChangeParameter(op1, op2);
        default:
            return op1;
    }
}

bool OperationalTransform::canTransform(const Operation& op1, const Operation& op2) {
    // Can transform if operations are of same type or compatible types
    if (op1.type == op2.type) {
        return true;
    }
    
    // Specific compatibility rules
    switch (op1.type) {
        case OperationType::AddTrack:
            return op2.type == OperationType::MoveTrack;
        case OperationType::RemoveTrack:
            return op2.type == OperationType::MoveTrack;
        case OperationType::MoveTrack:
            return op2.type == OperationType::AddTrack || op2.type == OperationType::RemoveTrack || op2.type == OperationType::MoveTrack;
        default:
            return false;
    }
}

bool OperationalTransform::areOperationsConcurrent(const Operation& op1, const Operation& op2) {
    return !(op1.timestamp < op2.timestamp || op2.timestamp < op1.timestamp);
}

Operation OperationalTransform::compose(const Operation& op1, const Operation& op2) {
    Operation composed = op1;
    composed.data = op2.data;  // Simplified composition
    composed.timestamp = op2.timestamp;  // Use later timestamp
    return composed;
}

Operation OperationalTransform::invert(const Operation& operation) {
    Operation inverted = operation;
    inverted.userId = operation.userId;  // Keep same user
    
    // Invert operation based on type
    switch (operation.type) {
        case OperationType::AddTrack:
            inverted.type = OperationType::RemoveTrack;
            break;
        case OperationType::RemoveTrack:
            inverted.type = OperationType::AddTrack;
            break;
        case OperationType::MuteTrack:
            inverted.data = !operation.data.getBool();
            break;
        case OperationType::SoloTrack:
            inverted.data = !operation.data.getBool();
            break;
        case OperationType::ArmTrack:
            inverted.data = !operation.data.getBool();
            break;
        default:
            // For other operations, swap old and new values
            if (operation.data.isObject()) {
                auto* obj = operation.data.getDynamicObject();
                if (obj) {
                    juce::var oldValue = obj->getProperty("oldValue");
                    juce::var newValue = obj->getProperty("newValue");
                    obj->setProperty("oldValue", newValue);
                    obj->setProperty("newValue", oldValue);
                    inverted.data = obj;
                }
            }
            break;
    }
    
    return inverted;
}

// Specific transform implementations
Operation OperationalTransform::transformAddTrack(const Operation& op1, const Operation& op2) {
    Operation transformed = op1;
    
    if (op2.type == OperationType::MoveTrack) {
        // Adjust position if another track was added before
        int op1Position = op1.data.getProperty("position", 0);
        int op2Position = op2.data.getProperty("position", 0);
        
        if (op1Position >= op2Position) {
            transformed.data.setProperty("position", op1Position + 1);
        }
    }
    
    return transformed;
}

Operation OperationalTransform::transformRemoveTrack(const Operation& op1, const Operation& op2) {
    Operation transformed = op1;
    
    if (op2.type == OperationType::MoveTrack) {
        // Adjust position if another track was removed before
        int op1Position = op1.data.getProperty("position", 0);
        int op2Position = op2.data.getProperty("position", 0);
        
        if (op1Position > op2Position) {
            transformed.data.setProperty("position", op1Position - 1);
        }
    }
    
    return transformed;
}

Operation OperationalTransform::transformMoveTrack(const Operation& op1, const Operation& op2) {
    Operation transformed = op1;
    
    if (op2.type == OperationType::AddTrack) {
        // Adjust positions if a track was added
        int op1From = op1.data.getProperty("from", 0);
        int op1To = op1.data.getProperty("to", 0);
        int op2Position = op2.data.getProperty("position", 0);
        
        if (op1From >= op2Position) {
            transformed.data.setProperty("from", op1From + 1);
        }
        if (op1To >= op2Position) {
            transformed.data.setProperty("to", op1To + 1);
        }
    } else if (op2.type == OperationType::RemoveTrack) {
        // Adjust positions if a track was removed
        int op1From = op1.data.getProperty("from", 0);
        int op1To = op1.data.getProperty("to", 0);
        int op2Position = op2.data.getProperty("position", 0);
        
        if (op1From > op2Position) {
            transformed.data.setProperty("from", op1From - 1);
        }
        if (op1To > op2Position) {
            transformed.data.setProperty("to", op1To - 1);
        }
    }
    
    return transformed;
}

Operation OperationalTransform::transformAddEffect(const Operation& op1, const Operation& op2) {
    // Similar to transformAddTrack but for effects
    Operation transformed = op1;
    
    if (op2.type == OperationType::MoveEffect && op1.targetId == op2.targetId) {
        int op1Position = op1.data.getProperty("position", 0);
        int op2Position = op2.data.getProperty("position", 0);
        
        if (op1Position >= op2Position) {
            transformed.data.setProperty("position", op1Position + 1);
        }
    }
    
    return transformed;
}

Operation OperationalTransform::transformRemoveEffect(const Operation& op1, const Operation& op2) {
    // Similar to transformRemoveTrack but for effects
    Operation transformed = op1;
    
    if (op2.type == OperationType::MoveEffect && op1.targetId == op2.targetId) {
        int op1Position = op1.data.getProperty("position", 0);
        int op2Position = op2.data.getProperty("position", 0);
        
        if (op1Position > op2Position) {
            transformed.data.setProperty("position", op1Position - 1);
        }
    }
    
    return transformed;
}

Operation OperationalTransform::transformMoveEffect(const Operation& op1, const Operation& op2) {
    // Similar to transformMoveTrack but for effects
    Operation transformed = op1;
    
    if (op1.targetId == op2.targetId) {
        if (op2.type == OperationType::AddEffect) {
            int op1From = op1.data.getProperty("from", 0);
            int op1To = op1.data.getProperty("to", 0);
            int op2Position = op2.data.getProperty("position", 0);
            
            if (op1From >= op2Position) {
                transformed.data.setProperty("from", op1From + 1);
            }
            if (op1To >= op2Position) {
                transformed.data.setProperty("to", op1To + 1);
            }
        } else if (op2.type == OperationType::RemoveEffect) {
            int op1From = op1.data.getProperty("from", 0);
            int op1To = op1.data.getProperty("to", 0);
            int op2Position = op2.data.getProperty("position", 0);
            
            if (op1From > op2Position) {
                transformed.data.setProperty("from", op1From - 1);
            }
            if (op1To > op2Position) {
                transformed.data.setProperty("to", op1To - 1);
            }
        }
    }
    
    return transformed;
}

Operation OperationalTransform::transformChangeParameter(const Operation& op1, const Operation& op2) {
    // For parameter changes, use last-writer-wins
    if (op2.timestamp > op1.timestamp) {
        return op2;
    }
    return op1;
}

} // namespace collaboration
} // namespace zenith
