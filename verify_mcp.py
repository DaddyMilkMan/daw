import subprocess, json, time, sys, select, os

default_path = "./build/ZenithDAW_artefacts/Debug/Zenith DAW"
exe = os.environ.get('ZENITH_DAW_PATH', default_path)
print(f"Launching {exe}...")

try:
    p = subprocess.Popen([exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=1)
except Exception as e:
    print(f"Fail: {e}")
    sys.exit(1)

def send(method, params=None, id=1):
    req = {"jsonrpc": "2.0", "method": method, "id": id}
    if params: req["params"] = params
    msg = json.dumps(req)
    print(f"Sending: {msg}")
    p.stdin.write(msg + "\n")
    p.stdin.flush()

def read(timeout=10):
    start = time.time()
    while time.time() - start < timeout:
        if p.poll() is not None: return None
        if select.select([p.stdout], [], [], 0.5)[0]:
            line = p.stdout.readline()
            if not line: return None
            l = line.strip()
            print(f"[RAW] {repr(l)}")
            if l.startswith("{"):
                try: return json.loads(l)
                except: pass
    return None

try:
    send("initialize", {"protocolVersion": "2024-11-05"}, 1)
    res = read()
    if res:
        print("Init OK")
        send("initialized", {}, 2)
        send("tools/call", {"name": "get_ui_tree", "arguments": {"depth": 1}}, 3)
        res = read(timeout=5)
        if res and "result" in res:
            print("UI Tree Received")
            print(str(res["result"])[:200])
    else:
        print("No Init Response")
except Exception as e:
    print(f"Err: {e}")
finally:
    p.terminate()
    try: p.wait(2)
    except: p.kill()
    print("\nSTDERR last 10 lines:")
    print("\n".join(p.stderr.read().splitlines()[-10:]))
