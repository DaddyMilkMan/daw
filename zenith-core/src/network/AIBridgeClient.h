/*
  ==============================================================================

    AIBridgeClient.h
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 7: Wingman AI Integration

    WebSocket/HTTP client for AI bridge server communication

    Responsibilities:
    - Send natural language + session graph to ai-bridge-server
    - Receive AI-generated command sequences
    - Queue responses for UI consumption
    - Notify listeners when responses arrive

    Protocol v1:
    - Request: POST to http://localhost:8765/wingman
      {
        "type": "wingman_nl_request",
        "requestId": "uuid-1234",
        "text": "create a drum loop",
        "sessionGraph": { ...full graph... },
        "source": "zenith-core"
      }
    - Response:
      {
        "type": "wingman_nl_response",
        "requestId": "uuid-1234",
        "status": "ok",
        "thought": "I'll create...",
        "commands": [ {command, params}, ... ]
      }

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include <queue>
#include <atomic>

namespace zenith {

//==============================================================================
/**
    HTTP client for communicating with AI bridge server

    All public methods are thread-safe and can be called from the message thread.
    Network I/O happens on a background thread.
*/
class AIBridgeClient : public juce::ChangeBroadcaster,
                       private juce::Thread
{
public:
    //==============================================================================
    /**
        Response structure from AI bridge server
    */
    struct Response
    {
        juce::String requestId;     // Matches request
        juce::String status;        // "ok" | "error"
        juce::String errorMessage;  // Populated if status == "error"
        juce::String thought;       // AI explanation/reasoning
        juce::Array<juce::var> commands; // Array of {command, params} objects
    };

    //==============================================================================
    AIBridgeClient();
    ~AIBridgeClient() override;

    //==============================================================================
    /**
        Set server URL (default: http://localhost:8765)

        @param url Full URL including protocol and port
    */
    void setServerUrl(const juce::String& url);

    /**
        Get current server URL
    */
    juce::String getServerUrl() const;

    //==============================================================================
    /**
        Send natural language request to AI bridge server

        This method queues the request for background processing and returns immediately.
        When the response arrives, sendChangeMessage() is called.

        @param naturalLanguage User's natural language input
        @param sessionGraph Full session graph from SessionGraph::generateGraph()
        @param sourceTag Source identifier (default: "zenith-core")
    */
    void sendRequest(const juce::String& naturalLanguage,
                    const juce::var& sessionGraph,
                    const juce::String& sourceTag = "zenith-core");

    /**
        Check if any responses are pending

        @return true if responses are available via popNextResponse()
    */
    bool hasPendingResponse() const;

    /**
        Retrieve and remove the next response from the queue

        @return Response object (check status field for success/error)
    */
    Response popNextResponse();

    //==============================================================================
    /**
        Check if last request succeeded (simple connectivity check)

        @return true if server is responding
    */
    bool isConnected() const;

    /**
        Get status message for debugging

        @return Human-readable status string
    */
    juce::String getStatusMessage() const;

private:
    //==============================================================================
    // Thread callbacks
    void run() override;

    //==============================================================================
    // Request structure for background thread
    struct PendingRequest
    {
        juce::String requestId;
        juce::String jsonPayload;
    };

    // Internal helpers
    void performRequest(const PendingRequest& request);
    void handleResponse(const juce::String& requestId, const juce::String& responseJson);
    Response parseResponse(const juce::var& responseVar);

    //==============================================================================
    // Member variables

    juce::String serverUrl{"http://localhost:8765"};

    // Thread-safe queue for responses
    juce::CriticalSection responseLock;
    std::queue<Response> responseQueue;

    // Thread-safe queue for requests
    juce::CriticalSection requestLock;
    std::queue<PendingRequest> requestQueue;

    // Status
    std::atomic<bool> connected{false};
    juce::CriticalSection statusLock;
    juce::String statusMessage;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIBridgeClient)
};

} // namespace zenith

