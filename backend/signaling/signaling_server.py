"""
Minimal TLS signaling server with UDP hole-punch coordination.

This module implements a dual-protocol signaling server for peer-to-peer
connection coordination:
- TCP with TLS for secure session registration and lookup
- UDP for NAT hole-punching coordination

Security Considerations:
- All TCP connections use TLS encryption (requires cert.pem/key.pem)
- Session codes are 4-digit numbers (10,000 possibilities) with TTL expiration
- No authentication beyond session codes - suitable for ephemeral connections
- Input validation on JSON payloads to prevent injection attacks
- Error handling prevents information disclosure

Thread Safety: Each TCP connection is handled in a separate thread.
UDP listener runs in a dedicated thread. The SessionStore provides
thread-safe access to session data.
"""

import json
import logging
import os
import socket
import ssl
import sys
import threading
import time
from typing import Optional, Tuple

LOG_FORMAT = "%(asctime)s %(levelname)s %(message)s"
logger = logging.getLogger("zenith.signaling")

if __package__ is None:
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    if root not in sys.path:
        sys.path.insert(0, root)

try:
    from .session_store import SessionStore
except ImportError:  # pragma: no cover - script run outside package
    from session_store import SessionStore

HOST = os.environ.get("ZENITH_SIGNALING_HOST", "0.0.0.0")
PORT = int(os.environ.get("ZENITH_SIGNALING_PORT", "54320"))
UDP_PORT = int(os.environ.get("ZENITH_SIGNALING_UDP_PORT", "54321"))
SESSION_TTL = int(os.environ.get("ZENITH_SIGNALING_SESSION_TTL", "300"))
SESSION_CLEAN_INTERVAL = int(os.environ.get("ZENITH_SIGNALING_CLEAN_FREQ", "60"))


def configure_logging(level: int = logging.INFO) -> None:
    """
    Configure logging for the signaling server.
    
    Args:
        level: Logging level (default: logging.INFO).
    """
    logging.basicConfig(level=level, format=LOG_FORMAT)


def cleanup_loop(store: SessionStore) -> None:
    """
    Periodic cleanup of expired sessions.
    
    Runs in a background thread to remove expired sessions based on TTL.
    
    Args:
        store: SessionStore instance to clean up.
    """
    while True:
        time.sleep(SESSION_CLEAN_INTERVAL)
        cleaned = store.cleanup()
        if cleaned:
            logger.info("Cleaned up %d expired sessions", cleaned)


def udp_listener(store: SessionStore) -> None:
    """
    UDP hole-punching listener and coordinator.
    
    Listens for UDP messages to coordinate peer-to-peer connections:
    - REGISTER:<code> - Register host UDP endpoint for a session
    - JOIN:<code> - Join session and exchange endpoints with host
    
    Thread Safety: This function is designed to run in a dedicated thread.
    
    Args:
        store: SessionStore instance for session management.
    """
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.bind((HOST, UDP_PORT))
        logger.info("Zenith UDP Hole Punch Listener on %d", UDP_PORT)

        while True:
            try:
                data, addr = sock.recvfrom(1024)
                msg = data.decode("utf-8", errors="ignore").strip()

                if msg.startswith("REGISTER:"):
                    code = msg.split(":", 1)[1]
                    # Validate code format to prevent injection
                    if not code.isdigit() or len(code) != 4:
                        logger.warning("Invalid code format from %s", addr)
                        continue
                        
                    if store.register_host(code, addr):
                        logger.info("Host %s UDP registered at %s", code, addr)
                    else:
                        logger.warning("REGISTER failed: code %s not found", code)

                elif msg.startswith("JOIN:"):
                    code = msg.split(":", 1)[1]
                    # Validate code format
                    if not code.isdigit() or len(code) != 4:
                        logger.warning("Invalid code format from %s", addr)
                        continue
                        
                    host_addr = store.get_host(code)
                    client_addr = addr

                    if host_addr:
                        sock.sendto(
                            f"PEER:{client_addr[0]}:{client_addr[1]}".encode(), host_addr
                        )
                        sock.sendto(
                            f"PEER:{host_addr[0]}:{host_addr[1]}".encode(), client_addr
                        )
                        logger.info(
                            "Hole punch initiated for %s: %s <-> %s",
                            code,
                            host_addr,
                            client_addr,
                        )
                    else:
                        logger.warning("JOIN failed: code %s has no host recorded", code)
            except Exception as exc:
                logger.exception("UDP listener error: %s", exc)


def handle_tcp_client(conn: socket.socket, addr: Tuple[str, int], store: SessionStore) -> None:
    """
    Handle a single TCP/TLS client connection.
    
    Processes JSON requests for session management:
    - REGISTER: Create a new session and return a code
    - LOOKUP: Check if a session code exists
    
    Security: All errors return generic messages to avoid information disclosure.
    
    Args:
        conn: Secure socket connection from client.
        addr: Client address tuple (IP, port).
        store: SessionStore instance for session management.
    """
    response = {"status": "ERROR", "msg": "Unhandled error"}
    try:
        payload = conn.recv(2048)
        if not payload:
            logger.debug("Empty payload from %s", addr)
            return

        request = json.loads(payload.decode("utf-8"))
        action = request.get("action")

        if action == "REGISTER":
            code = store.create_session()
            response = {"status": "OK", "code": code}
            logger.info("TCP: Registered session %s from %s", code, addr)

        elif action == "LOOKUP":
            code = request.get("code")
            # Validate code format
            if code and isinstance(code, str) and code.isdigit() and len(code) == 4:
                if store.has_code(code):
                    response = {"status": "OK"}
                else:
                    response = {"status": "ERROR", "msg": "Code not found"}
                logger.debug("TCP: LOOKUP %s -> %s", code, response["status"])
            else:
                response = {"status": "ERROR", "msg": "Invalid code format"}
                logger.warning("TCP: invalid code format from %s", addr)

        else:
            response = {"status": "ERROR", "msg": "Unknown action"}
            logger.warning("TCP: unknown action %s from %s", action, addr)

    except json.JSONDecodeError as exc:
        response = {"status": "ERROR", "msg": "Invalid JSON"}
        logger.warning("TCP: invalid payload from %s: %s", addr, exc)
    except Exception:
        response = {"status": "ERROR", "msg": "Internal server error"}
        logger.exception("TCP handler failure for %s", addr)
    finally:
        try:
            conn.send(json.dumps(response).encode("utf-8"))
        except Exception:
            logger.debug("Could not send response to %s", addr)
        finally:
            conn.close()


def tcp_server(store: SessionStore) -> None:
    """
    Run the TCP/TLS signaling server.
    
    Listens for secure connections and handles each in a separate thread.
    Requires cert.pem and key.pem in the signaling directory.
    
    Security: Uses TLS 1.2+ with default secure cipher suites.
    
    Args:
        store: SessionStore instance for session management.
    """
    base_dir = os.path.dirname(os.path.abspath(__file__))
    cert_path = os.path.join(base_dir, "cert.pem")
    key_path = os.path.join(base_dir, "key.pem")

    context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    context.load_cert_chain(certfile=cert_path, keyfile=key_path)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((HOST, PORT))
    sock.listen(10)
    logger.info("Zenith Signaling TCP with TLS on %d", PORT)

    with context.wrap_socket(sock, server_side=True) as secure_sock:
        while True:
            try:
                conn, addr = secure_sock.accept()
                logger.info("Accepted secure connection from %s", addr)
                threading.Thread(
                    target=handle_tcp_client,
                    args=(conn, addr, store),
                    daemon=True,
                ).start()
            except ssl.SSLError as exc:
                logger.error("SSL error on accept: %s", exc)
            except Exception:
                logger.exception("TCP accept loop failure")


def run_signaling() -> None:
    """
    Run the complete signaling server.
    
    Starts the session store, cleanup thread, UDP listener, and TCP server.
    This function blocks indefinitely.
    """
    store = SessionStore(ttl=SESSION_TTL)

    threading.Thread(target=cleanup_loop, args=(store,), daemon=True).start()
    threading.Thread(target=udp_listener, args=(store,), daemon=True).start()

    tcp_server(store)


if __name__ == "__main__":
    configure_logging()
    run_signaling()
