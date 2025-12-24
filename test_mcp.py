import subprocess
import json
import time
import os
import signal

# Path to executable
exe_path = "./build/ZenithDAW_artefacts/Debug/Zenith DAW"

# Start process
print(f"Launching {exe_path}...")
try:
    process = subprocess.Popen(
        [exe_path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1 # Line buffered
    )
except Exception as e:
    print(f"Failed to launch: {e}")
    exit(1)

def send_request(method, params=None, req_id=1):
    req = {
        "jsonrpc": "2.0",
        "method": method,
        "id": req_id
    }
    if params:
        req["params"] = params
    
    print(f"Sending: {json.dumps(req)}")
    process.stdin.write(json.dumps(req) + "\n")
    process.stdin.flush()

def read_response(timeout=5):
    print("Waiting for response...")
    # Simple timeout handling using poll
    start_time = time.time()
    while time.time() - start_time < timeout:
        if process.poll() is not None:
            print("Process exited prematurely.")
            return None
        line = process.stdout.readline()
        if line:
            return json.loads(line)
        time.sleep(0.1)
    print("Timeout waiting for response.")
    return None

try:
    # 1. Initialize
    send_request("initialize", {"protocolVersion": "2024-11-05"}, 1)
    res = read_response()
    if res:
        print("Received:", json.dumps(res, indent=2))
        send_request("initialized", {}, 2) # Notification
        
        # 2. List Tools
        send_request("tools/list", {}, 3)
        res = read_response()
        print("Received Tools List (summary):")
        if res and "result" in res:
            tools = res["result"]["tools"]
            print(f"Found {len(tools)} tools.")
            for t in tools[:5]: # Show first 5
                print(f"- {t['name']}")
        
        # 3. Get UI Tree (Vision Test)
        send_request("tools/call", {"name": "get_ui_tree", "arguments": {"depth": 2}}, 4)
        res = read_response()
        print("Received UI Tree:")
        if res and "result" in res and "content" in res["result"]:
            content = res["result"]["content"][0]["text"]
            print(content[:500] + "...") # Truncate
    else:
        print("No response to initialize.")

except Exception as e:
    print(f"Error: {e}")
finally:
    print("Terminating...")
    process.terminate()
    try:
        process.wait(timeout=2)
    except:
        process.kill()
    
    # Print stderr for debug
    stderr_out = process.stderr.read()
    if stderr_out:
        print("\nSTDERR Output:")
        print(stderr_out)
