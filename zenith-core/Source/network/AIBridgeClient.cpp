/*
  ==============================================================================

    AIBridgeClient.cpp
    Created: 2025-11-14
    Author:  Zenith DAW - Phase 7: Wingman AI Integration

    AI bridge client implementation

  ==============================================================================
*/

#include "AIBridgeClient.h"

namespace zenith {

//==============================================================================
AIBridgeClient::AIBridgeClient()
    : Thread("AIBridgeClient")
{
    DBG("AIBridgeClient: Initialized");
    startThread(juce::Thread::Priority::normal);
}

AIBridgeClient::~AIBridgeClient()
{
    DBG("AIBridgeClient: Shutting down");
    signalThreadShouldExit();
    notify();  // Wake up thread if waiting
    waitForThreadToExit(5000);
}

//==============================================================================
void AIBridgeClient::setServerUrl(const juce::String& url)
{
    serverUrl = url;
    DBG("AIBridgeClient: Server URL set to " + url);
}

juce::String AIBridgeClient::getServerUrl() const
{
    return serverUrl;
}

//==============================================================================
void AIBridgeClient::sendRequest(const juce::String& naturalLanguage,
                                  const juce::var& sessionGraph,
                                  const juce::String& sourceTag)
{
    // Generate unique request ID
    juce::String requestId = juce::Uuid().toDashedString();

    // Build request JSON
    auto* requestObj = new juce::DynamicObject();
    requestObj->setProperty("type", "wingman_nl_request");
    requestObj->setProperty("requestId", requestId);
    requestObj->setProperty("text", naturalLanguage);
    requestObj->setProperty("sessionGraph", sessionGraph);
    requestObj->setProperty("source", sourceTag);

    juce::String jsonPayload = juce::JSON::toString(juce::var(requestObj));

    // Queue request for background thread
    {
        const juce::ScopedLock lock(requestLock);
        requestQueue.push({requestId, jsonPayload});
    }

    // Wake up background thread
    notify();

    DBG("AIBridgeClient: Queued request " + requestId);
}

bool AIBridgeClient::hasPendingResponse() const
{
    const juce::ScopedLock lock(responseLock);
    return !responseQueue.empty();
}

AIBridgeClient::Response AIBridgeClient::popNextResponse()
{
    const juce::ScopedLock lock(responseLock);

    if (responseQueue.empty())
        return Response{}; // Return empty response

    Response response = responseQueue.front();
    responseQueue.pop();
    return response;
}

bool AIBridgeClient::isConnected() const
{
    return connected.load();
}

juce::String AIBridgeClient::getStatusMessage() const
{
    const juce::ScopedLock lock(statusLock);
    return statusMessage;
}

//==============================================================================
// Background Thread
//==============================================================================

void AIBridgeClient::run()
{
    DBG("AIBridgeClient: Background thread started");

    while (!threadShouldExit())
    {
        // Check for pending requests
        PendingRequest request;
        bool hasRequest = false;

        {
            const juce::ScopedLock lock(requestLock);
            if (!requestQueue.empty())
            {
                request = requestQueue.front();
                requestQueue.pop();
                hasRequest = true;
            }
        }

        if (hasRequest)
        {
            performRequest(request);
        }
        else
        {
            // Wait for notification (with timeout to allow clean shutdown)
            wait(1000);
        }
    }

    DBG("AIBridgeClient: Background thread stopped");
}

void AIBridgeClient::performRequest(const PendingRequest& request)
{
    DBG("AIBridgeClient: Sending request " + request.requestId);

    try
    {
        // Construct URL
        juce::URL url(serverUrl + "/wingman");

        // Set up POST request
        auto stream = url.createInputStream(
            juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                .withConnectionTimeoutMs(10000)
                .withPostData(request.jsonPayload)
                .withExtraHeaders("Content-Type: application/json")
                .withNumRedirectsToFollow(0)
        );

        if (stream == nullptr)
        {
            // Connection failed
            connected = false;

            {
                const juce::ScopedLock lock(statusLock);
                statusMessage = "Failed to connect to " + serverUrl;
            }

            // Queue error response
            Response errorResponse;
            errorResponse.requestId = request.requestId;
            errorResponse.status = "error";
            errorResponse.errorMessage = "Failed to connect to AI bridge server at " + serverUrl;

            {
                const juce::ScopedLock lock(responseLock);
                responseQueue.push(errorResponse);
            }

            sendChangeMessage();

            DBG("AIBridgeClient: Connection failed");
            return;
        }

        // Read response
        juce::String responseBody = stream->readEntireStreamAsString();

        if (responseBody.isEmpty())
        {
            // Empty response
            connected = false;

            Response errorResponse;
            errorResponse.requestId = request.requestId;
            errorResponse.status = "error";
            errorResponse.errorMessage = "Empty response from AI bridge server";

            {
                const juce::ScopedLock lock(responseLock);
                responseQueue.push(errorResponse);
            }

            sendChangeMessage();

            DBG("AIBridgeClient: Empty response");
            return;
        }

        // Connection succeeded
        connected = true;

        {
            const juce::ScopedLock lock(statusLock);
            statusMessage = "Connected";
        }

        // Handle response
        handleResponse(request.requestId, responseBody);

        DBG("AIBridgeClient: Request " + request.requestId + " completed");
    }
    catch (const std::exception& e)
    {
        connected = false;

        Response errorResponse;
        errorResponse.requestId = request.requestId;
        errorResponse.status = "error";
        errorResponse.errorMessage = "Exception during request: " + juce::String(e.what());

        {
            const juce::ScopedLock lock(responseLock);
            responseQueue.push(errorResponse);
        }

        sendChangeMessage();

        DBG("AIBridgeClient: Exception - " + juce::String(e.what()));
    }
}

void AIBridgeClient::handleResponse(const juce::String& requestId, const juce::String& responseJson)
{
    // Parse JSON response
    juce::var responseVar;
    auto parseResult = juce::JSON::parse(responseJson, responseVar);

    if (parseResult.failed())
    {
        // JSON parse error
        Response errorResponse;
        errorResponse.requestId = requestId;
        errorResponse.status = "error";
        errorResponse.errorMessage = "Failed to parse JSON response: " + parseResult.getErrorMessage();

        {
            const juce::ScopedLock lock(responseLock);
            responseQueue.push(errorResponse);
        }

        sendChangeMessage();

        DBG("AIBridgeClient: JSON parse error - " + parseResult.getErrorMessage());
        return;
    }

    // Parse response object
    Response response = parseResponse(responseVar);
    response.requestId = requestId; // Ensure requestId matches

    // Queue response
    {
        const juce::ScopedLock lock(responseLock);
        responseQueue.push(response);
    }

    // Notify listeners
    sendChangeMessage();

    DBG("AIBridgeClient: Response queued for " + requestId + " (status: " + response.status + ")");
}

AIBridgeClient::Response AIBridgeClient::parseResponse(const juce::var& responseVar)
{
    Response response;

    if (!responseVar.isObject())
    {
        response.status = "error";
        response.errorMessage = "Response is not a JSON object";
        return response;
    }

    // Extract status
    response.status = responseVar.getProperty("status", "error").toString();

    if (response.status == "error")
    {
        // Error response
        response.errorMessage = responseVar.getProperty("error", "Unknown error").toString();
    }
    else if (response.status == "ok")
    {
        // Success response
        response.thought = responseVar.getProperty("thought", "").toString();

        // Extract commands array
        juce::var commandsVar = responseVar.getProperty("commands", juce::var());

        if (commandsVar.isArray())
        {
            auto* commandsArray = commandsVar.getArray();
            for (const auto& cmdVar : *commandsArray)
            {
                response.commands.add(cmdVar);
            }
        }
        else
        {
            response.status = "error";
            response.errorMessage = "Response 'commands' field is not an array";
        }
    }
    else
    {
        response.status = "error";
        response.errorMessage = "Unknown status: " + response.status;
    }

    return response;
}

} // namespace zenith
