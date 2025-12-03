const http = require('http');
const { InstrumentAIAdapter } = require('./InstrumentAIAdapter');
require('dotenv').config();

const PORT = 8765;
const HOST = 'localhost';

// Initialize Adapter with Real API Calls
const adapter = new InstrumentAIAdapter({
    // Function to send commands back to DAW (if needed via separate channel, but mostly we respond to HTTP)
    sendCommand: async (command) => {
        // In the HTTP Request/Response model, we return commands in the response.
        // This helper might be used if we need to query the DAW for state *during* processing.
        // For now, we assume the DAW sends context in the request.
        console.log('Simulating command execution check:', command.command);
        return { status: 'ok', data: {} };
    },

    // Real LLM Caller
    callLLM: async (promptData) => {
        const apiKey = process.env.OPENAI_API_KEY;
        const model = process.env.OPENAI_MODEL || 'gpt-4-turbo-preview';
        
        if (!apiKey) {
            throw new Error("OPENAI_API_KEY not found in environment variables.");
        }

        console.log(`[LLM] Sending request to OpenAI (${model})...`);
        
        const response = await fetch('https://api.openai.com/v1/chat/completions', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Authorization': `Bearer ${apiKey}`
            },
            body: JSON.stringify({
                model: model,
                messages: [
                    { role: 'system', content: promptData.systemPrompt },
                    { role: 'user', content: promptData.userPrompt }
                ],
                response_format: { type: "json_object" }
            })
        });

        if (!response.ok) {
            const err = await response.text();
            throw new Error(`OpenAI API Error: ${response.status} - ${err}`);
        }

        const data = await response.json();
        const content = data.choices[0].message.content;
        console.log('[LLM] Received response');
        return content;
    }
});

// Create HTTP Server
const server = http.createServer(async (req, res) => {
    // Set CORS headers
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

    // Handle Preflight
    if (req.method === 'OPTIONS') {
        res.writeHead(204);
        res.end();
        return;
    }

    // Only accept POST to /wingman
    if (req.method === 'POST' && req.url === '/wingman') {
        let body = '';
        
        req.on('data', chunk => {
            body += chunk.toString();
        });

        req.on('end', async () => {
            try {
                const request = JSON.parse(body);
                console.log(`[Server] Received Request ID: ${request.requestId}`);
                console.log(`[Server] Input: "${request.text}"`);

                // Process with Adapter
                // We determine intent based on text (naive routing for now, ideally LLM does this too)
                let result;

                // Check for explicit mode or infer from text
                if (request.text.toLowerCase().includes('preset')) {
                     // Assume Preset Tweak/Select context
                     // In a real implementation, we'd use the LLM to classify intent first.
                     // For this "working contents" replacement, we'll do a direct generic LLM pass
                     // that returns the expected "wingman_nl_response" format directly.
                     result = await handleGeneralRequest(request);
                } else {
                     result = await handleGeneralRequest(request);
                }

                // Send Response
                res.writeHead(200, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify(result));
                console.log(`[Server] Sent Response ID: ${request.requestId}`);

            } catch (e) {
                console.error('[Server] Error processing request:', e);
                res.writeHead(500, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({
                    status: 'error',
                    error: e.message
                }));
            }
        });
    } else {
        res.writeHead(404);
        res.end('Not Found');
    }
});

// General Request Handler using the Adapter's LLM capability
async function handleGeneralRequest(request) {
    // We construct a prompt that asks the LLM to output the specific command JSON
    // expected by the C++ client.
    
    const systemPrompt = `You are Wingman, the AI assistant for Zenith DAW.
You translate natural language requests into JSON commands for the DAW.

AVAILABLE COMMANDS:
- create_track(type: "audio"|"midi", name: string)
- create_clip(trackId: string, length: number)
- set_tempo(bpm: number)
- play()
- stop()
- set_parameter(trackId: string, paramId: string, value: number)

RESPONSE FORMAT:
You must respond with a JSON object matching this structure:
{
    "thought": "Brief explanation of what you are doing",
    "commands": [
        { "command": "command_name", "params": { ... } }
    ]
}

Example User: "Make a new midi track called Drums and set tempo to 140"
Example Response:
{
    "thought": "Creating a MIDI track named 'Drums' and setting tempo to 140 BPM.",
    "commands": [
        { "command": "create_track", "params": { "type": "midi", "name": "Drums" } },
        { "command": "set_tempo", "params": { "bpm": 140 } }
    ]
}
`;

    const userPrompt = `Request: "${request.text}"
Context: ${JSON.stringify(request.sessionGraph || {})}
`;

    try {
        const jsonStr = await adapter.callLLM({ systemPrompt, userPrompt });
        const parsed = JSON.parse(jsonStr);
        
        return {
            type: 'wingman_nl_response',
            requestId: request.requestId,
            status: 'ok',
            thought: parsed.thought,
            commands: parsed.commands
        };
    } catch (e) {
        console.error("LLM Parsing Error or Failure:", e);
        return {
            type: 'wingman_nl_response',
            requestId: request.requestId,
            status: 'error',
            error: "Failed to generate response: " + e.message
        };
    }
}

server.listen(PORT, HOST, () => {
    console.log(`Zenith AI Bridge (HTTP) running on http://${HOST}:${PORT}`);
    console.log(`Ready for POST /wingman requests.`);
});