import socket
import threading
import json
import time

# Configuration
HOST = '0.0.0.0'
PORT = 54320 # Signaling port (UDP now preferred for Hole Punching exchange, but we'll use TCP for reliable signaling)

# However, for Hole Punching, we need the SERVER to see the CLIENT'S UDP address.
# So we need a UDP socket on the server side too.
UDP_PORT = 54321

# Database: { '1234': { 'ip': '...', 'port': 12345, 'timestamp': 1234567890 } }
sessions = {}

def cleanup_loop():
    """Removes sessions older than 5 minutes to free memory"""
    while True:
        time.sleep(60)
        now = time.time()
        expired = [k for k, v in sessions.items() if now - v.get('timestamp', 0) > 300]
        for k in expired:
            del sessions[k]
        if expired:
            print(f"Cleaned up {len(expired)} old sessions")

def udp_listener():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((HOST, UDP_PORT))
    print(f"Zenith UDP Hole Punch Listener on {UDP_PORT}")
    
    while True:
        try:
            data, addr = sock.recvfrom(1024)
            msg = data.decode('utf-8')
            
            # Message format: "REGISTER:1234" or "JOIN:1234"
            if msg.startswith("REGISTER:"):
                # Host is announcing their public UDP port
                code = msg.split(":")[1]
                if code in sessions:
                    sessions[code]['host'] = addr # (ip, port)
                    sessions[code]['timestamp'] = time.time() # Update activity
                    print(f"Host {code} UDP registered at {addr}")
                    
            elif msg.startswith("JOIN:"):
                # Joiner is announcing their public UDP port
                code = msg.split(":")[1]
                if code in sessions:
                    host_addr = sessions[code].get('host')
                    client_addr = addr
                    
                    if host_addr:
                        # MAGIC MOMENT: Perform the Exchange
                        # Tell Host about Client
                        sock.sendto(f"PEER:{client_addr[0]}:{client_addr[1]}".encode(), host_addr)
                        
                        # Tell Client about Host
                        sock.sendto(f"PEER:{host_addr[0]}:{host_addr[1]}".encode(), client_addr)
                        
                        print(f"Hole Punch Initiated: {host_addr} <-> {client_addr}")
        except Exception as e:
            print(f"UDP Error: {e}")

def handle_tcp_client(conn, addr):
    try:
        data = conn.recv(1024).decode('utf-8')
        if not data: return
        
        request = json.loads(data)
        response = {}

        if request['action'] == 'REGISTER':
            # Create a Session
            import random
            code = str(random.randint(1000, 9999))
            sessions[code] = {'timestamp': time.time()} # Initialize with time
            response = {'status': 'OK', 'code': code}
            print(f"TCP: Registered Session {code}")

        elif request['action'] == 'LOOKUP':
            # Client Just validates code exists, actual connection happens via UDP now
            code = request.get('code')
            if code in sessions:
                 response = {'status': 'OK'}
            else:
                 response = {'status': 'ERROR', 'msg': 'Code not found'}

        conn.send(json.dumps(response).encode('utf-8'))
    except Exception as e:
        print(f"TCP Error: {e}")
    finally:
        conn.close()

def tcp_server():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((HOST, PORT))
    s.listen(10)
    print(f"Zenith Signaling TCP on {PORT}")
    while True:
        conn, addr = s.accept()
        threading.Thread(target=handle_tcp_client, args=(conn, addr)).start()

if __name__ == "__main__":
    # Run UDP and TCP in parallel
    threading.Thread(target=cleanup_loop, daemon=True).start()
    threading.Thread(target=udp_listener, daemon=True).start()
    tcp_server()
