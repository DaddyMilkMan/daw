#!/usr/bin/env python3
"""
Enhanced Signaling Server for Zenith DAW Collaboration
Supports ICE candidate exchange and session management
"""

import socket
import json
import threading
import time
from typing import Dict, List, Optional

class Session:
    def __init__(self, code: str, host_ip: str):
        self.code = code
        self.host_ip = host_ip
        self.peer_ip: Optional[str] = None
        self.host_candidates: List[str] = []
        self.peer_candidates: List[str] = []
        self.created_at = time.time()
        self.lock = threading.Lock()

    def is_expired(self, timeout_seconds: int = 3600) -> bool:
        return (time.time() - self.created_at) > timeout_seconds

class SignalingServer:
    def __init__(self, host: str = "0.0.0.0", tcp_port: int = 54320, udp_port: int = 54321):
        self.host = host
        self.tcp_port = tcp_port
        self.udp_port = udp_port
        self.sessions: Dict[str, Session] = {}
        self.lock = threading.Lock()
        self.running = False

    def start(self):
        """Start both TCP and UDP signaling servers"""
        self.running = True

        # Start TCP server
        tcp_thread = threading.Thread(target=self._tcp_server_loop, daemon=True)
        tcp_thread.start()

        # Start UDP server
        udp_thread = threading.Thread(target=self._udp_server_loop, daemon=True)
        udp_thread.start()

        print(f"Signaling server started on {self.host}:{self.tcp_port} (TCP) and {self.udp_port} (UDP)")

    def stop(self):
        self.running = False

    def _tcp_server_loop(self):
        """Handle TCP signaling (registration, code lookup, ICE exchange)"""
        tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        tcp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        tcp_socket.bind((self.host, self.tcp_port))
        tcp_socket.listen(10)
        tcp_socket.settimeout(1.0)

        while self.running:
            try:
                client_socket, address = tcp_socket.accept()
                threading.Thread(target=self._handle_tcp_client, args=(client_socket, address), daemon=True).start()
            except socket.timeout:
                continue
            except Exception as e:
                if self.running:
                    print(f"TCP server error: {e}")
                break

        tcp_socket.close()

    def _udp_server_loop(self):
        """Handle UDP signaling (hole punching, ICE coordination)"""
        udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        udp_socket.bind((self.host, self.udp_port))
        udp_socket.settimeout(1.0)

        while self.running:
            try:
                data, address = udp_socket.recvfrom(2048)
                threading.Thread(target=self._handle_udp_message, args=(data, address, udp_socket), daemon=True).start()
            except socket.timeout:
                continue
            except Exception as e:
                if self.running:
                    print(f"UDP server error: {e}")
                break

        udp_socket.close()

    def _handle_tcp_client(self, client_socket, address):
        """Handle TCP client connection"""
        try:
            # Set timeout
            client_socket.settimeout(5.0)

            # Receive request
            data = client_socket.recv(4096)
            if not data:
                return

            request = json.loads(data.decode('utf-8'))
            action = request.get('action')

            response = {}

            if action == 'REGISTER':
                # Host registering - generate session code
                session_code = self._generate_session_code()
                session = Session(session_code, address[0])

                with self.lock:
                    self.sessions[session_code] = session

                response = {
                    'status': 'OK',
                    'code': session_code
                }
                print(f"[{address[0]}] Registered session: {session_code}")

            elif action == 'LOOKUP':
                # Peer looking up session
                code = request.get('code')
                session = self.sessions.get(code)

                if session and session.peer_ip is None:
                    # First peer to join
                    with self.lock:
                        session.peer_ip = address[0]

                    response = {
                        'status': 'OK',
                        'peer_info': f"PEER:{session.host_ip}:{self.udp_port}"
                    }
                    print(f"[{address[0]}] Joined session: {code}")
                elif session:
                    # Session already has peers
                    # Return existing peer info for hole punching
                    response = {
                        'status': 'OK',
                        'peer_info': f"PEER:{session.host_ip}:{self.udp_port}"
                    }
                    print(f"[{address[0]}] Session full, returning peer: {code}")
                else:
                    response = {'status': 'ERROR', 'message': 'Session not found'}

            elif action == 'exchange_ice':
                # Exchange ICE candidates
                code = request.get('code')
                session = self.sessions.get(code)

                if not session:
                    response = {'status': 'ERROR', 'message': 'Session not found'}
                else:
                    candidates = request.get('candidates', [])
                    is_host = address[0] == session.host_ip

                    with session.lock:
                        if is_host:
                            session.host_candidates = candidates
                            # Return peer candidates if available
                            if session.peer_candidates:
                                response = {
                                    'status': 'OK',
                                    'peer_candidates': session.peer_candidates
                                }
                                print(f"[{address[0]}] Host candidates stored: {len(candidates)}")
                            else:
                                response = {'status': 'OK', 'message': 'Waiting for peer candidates'}
                        else:
                            session.peer_candidates = candidates
                            # Return host candidates
                            response = {
                                'status': 'OK',
                                'peer_candidates': session.host_candidates
                            }
                            print(f"[{address[0]}] Peer candidates stored: {len(candidates)}")

            else:
                response = {'status': 'ERROR', 'message': 'Unknown action'}

            # Send response
            response_json = json.dumps(response) + '\n'
            client_socket.sendall(response_json.encode('utf-8'))

        except json.JSONDecodeError:
            print(f"[{address[0]}] Invalid JSON request")
        except Exception as e:
            print(f"[{address[0]}] Error handling request: {e}")
        finally:
            client_socket.close()

    def _handle_udp_message(self, data: bytes, address: tuple, udp_socket: socket.socket):
        """Handle UDP message (hole punching, STUN relay)"""
        try:
            message = data.decode('utf-8', errors='ignore')

            if message.startswith('REGISTER:') or message.startswith('JOIN:'):
                # Legacy hole punching message
                parts = message.split(':')
                if len(parts) >= 2:
                    code = parts[1]
                    session = self.sessions.get(code)

                    if session:
                        # Send peer info
                        peer_info = f"PEER:{session.host_ip}:{self.udp_port}"
                        udp_socket.sendto(peer_info.encode('utf-8'), address)
                        print(f"[UDP] Sent peer info to {address} for session {code}")

            elif message.startswith('ICE_CHECK:'):
                # ICE connectivity check relay
                parts = message.split(':', 2)
                if len(parts) >= 3:
                    code = parts[1]
                    stun_data = parts[2]
                    session = self.sessions.get(code)

                    if session:
                        # Relay STUN binding request to other peer
                        target_address = None
                        if address[0] == session.host_ip and session.peer_ip:
                            target_address = (session.peer_ip, self.udp_port)
                        elif session.peer_ip and address[0] != session.host_ip:
                            target_address = (session.host_ip, self.udp_port)

                        if target_address:
                            udp_socket.sendto(stun_data.encode('latin1'), target_address)
                            print(f"[UDP] Relayed ICE check for session {code}")

        except Exception as e:
            print(f"[UDP] Error handling message from {address}: {e}")

    def _generate_session_code(self) -> str:
        """Generate unique 6-character session code"""
        import random
        import string
        while True:
            code = ''.join(random.choices(string.ascii_uppercase + string.digits, k=6))
            if code not in self.sessions:
                return code

    def cleanup_expired_sessions(self):
        """Remove expired sessions"""
        with self.lock:
            expired = [code for code, session in self.sessions.items() if session.is_expired()]
            for code in expired:
                del self.sessions[code]
                print(f"Cleaned up expired session: {code}")

def main():
    import sys

    # Allow command line override of ports
    tcp_port = int(sys.argv[1]) if len(sys.argv) > 1 else 54320
    udp_port = int(sys.argv[2]) if len(sys.argv) > 2 else 54321

    server = SignalingServer(tcp_port=tcp_port, udp_port=udp_port)

    try:
        server.start()
        print("Signaling server running. Press Ctrl+C to stop.")

        # Cleanup thread
        while True:
            time.sleep(60)
            server.cleanup_expired_sessions()

    except KeyboardInterrupt:
        print("\nShutting down...")
        server.stop()

if __name__ == "__main__":
    main()
