import socket
import json

HOST = '216.126.231.46'
PORT = 54320

print(f"Connecting to {HOST}:{PORT}...")
try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(3)
    s.connect((HOST, PORT))
    print("TCP Connection Successful!")
    
    msg = json.dumps({"action": "REGISTER"})
    s.sendall(msg.encode())
    
    data = s.recv(1024)
    print(f"Server Replied: {data.decode()}")
    s.close()
    print("TEST PASSED: Server is online and logic is working.")
except Exception as e:
    print(f"TEST FAILED: {e}")
