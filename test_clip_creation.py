#!/usr/bin/env python3
"""
Test script to verify clip creation via MCP/CommandAPI
Connects to ZenithDAW's embedded MCP server via subprocess
"""
import subprocess
import json
import sys
import time

def send_mcp_request(process, method, params=None, request_id=1):
    """Send a JSON-RPC request to the MCP server"""
    request = {
        "jsonrpc": "2.0",
        "id": request_id,
        "method": method
    }
    if params:
        request["params"] = params
    
    msg = json.dumps(request)
    process.stdin.write(msg + "\n")
    process.stdin.flush()
    
    # Read response
    response_line = process.stdout.readline()
    if response_line:
        return json.loads(response_line.strip())
    return None

def main():
    # Launch the DAW in MCP mode
    import os
    default_path = "/home/micah/Desktop/zenith/daw/build/ZenithDAW_artefacts/Release/Zenith DAW"
    daw_path = os.environ.get('ZENITH_DAW_PATH', default_path)
    
    print("Starting Zenith DAW in MCP mode...")
    
    process = subprocess.Popen(
        [daw_path, "--mcp-server"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1
    )
    
    time.sleep(1)  # Give it time to start
    
    try:
        # Step 1: Initialize
        print("1. Sending initialize...")
        response = send_mcp_request(process, "initialize", {
            "protocolVersion": "2024-11-05",
            "clientInfo": {"name": "clip-test", "version": "1.0"}
        })
        print(f"   Initialize response: {response}")
        
        # Step 2: Send initialized notification
        print("2. Sending initialized notification...")
        process.stdin.write(json.dumps({"jsonrpc": "2.0", "method": "notifications/initialized"}) + "\n")
        process.stdin.flush()
        
        # Step 3: Get current project state
        print("3. Getting project state...")
        response = send_mcp_request(process, "tools/call", {
            "name": "get_project_state",
            "arguments": {}
        }, request_id=2)
        print(f"   Project state: {response}")
        
        # Step 4: Create a track if none exist
        print("4. Creating test track...")
        response = send_mcp_request(process, "tools/call", {
            "name": "create_track",
            "arguments": {
                "name": "Test MIDI Track",
                "type": "instrument"
            }
        }, request_id=3)
        print(f"   Create track response: {response}")
        
        # Step 5: Create a clip (simulating double-click behavior)
        print("5. Creating clip (like double-click would)...")
        response = send_mcp_request(process, "tools/call", {
            "name": "create_clip",
            "arguments": {
                "trackIndex": 0,
                "startBeats": 4.0,
                "lengthBeats": 4.0,
                "clipType": "midi",
                "name": "Test Clip"
            }
        }, request_id=4)
        print(f"   Create clip response: {response}")
        
        # Step 6: Verify by getting project state again
        print("6. Verifying clip exists...")
        response = send_mcp_request(process, "tools/call", {
            "name": "get_project_state",
            "arguments": {}
        }, request_id=5)
        print(f"   Final project state: {response}")
        
        print("\n=== TEST COMPLETE ===")
        print("If create_clip returned a clipId, clip creation is working!")
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        process.terminate()
        process.wait()

if __name__ == "__main__":
    main()
