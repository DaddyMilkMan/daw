"""Minimal TLS signaling server with UDP hole-punch coordination."""

import json
import os
import socket
import ssl
import sys
import threading
import time

from backend.config import ZenithConfig
from backend.logger import configure_logging, get_logger

logger = get_logger(__name__)

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


def cleanup_loop(store: SessionStore) -> None:
    while True:
        time.sleep(SESSION_CLEAN_INTERVAL)
        cleaned = store.cleanup()
        if cleaned:
            logger.info("Cleaned up expired sessions", count=cleaned)


def udp_listener(store: SessionStore) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.bind((HOST, UDP_PORT))
        logger.info("Zenith UDP Hole Punch Listener started", port=UDP_PORT)

        while True:
            try:
                data, addr = sock.recvfrom(1024)
                msg = data.decode("utf-8", errors="ignore").strip()

                if msg.startswith("REGISTER:"):
                    code = msg.split(":", 1)[1]
                    if store.register_host(code, addr):
                        logger.info("Host UDP registered", code=code, address=str(addr))
                    else:
                        logger.warning("REGISTER failed: code not found", code=code)

                elif msg.startswith("JOIN:"):
                    code = msg.split(":", 1)[1]
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
                            "Hole punch initiated",
                            code=code,
                            host=str(host_addr),
                            client=str(client_addr),
                        )
                    else:
                        logger.warning("JOIN failed: code has no host recorded", code=code)
            except Exception as exc:
                logger.exception("UDP listener error", error=str(exc))


def handle_tcp_client(conn: socket.socket, addr: tuple, store: SessionStore) -> None:
    response = {"status": "ERROR", "msg": "Unhandled error"}
    try:
        payload = conn.recv(2048)
        if not payload:
            logger.debug("Empty payload received", address=str(addr))
            return

        request = json.loads(payload.decode("utf-8"))
        action = request.get("action")

        if action == "REGISTER":
            code = store.create_session()
            response = {"status": "OK", "code": code}
            logger.info("TCP: Registered session", code=code, address=str(addr))

        elif action == "LOOKUP":
            code = request.get("code")
            if code and store.has_code(code):
                response = {"status": "OK"}
            else:
                response = {"status": "ERROR", "msg": "Code not found"}
            logger.debug("TCP: LOOKUP", code=code, status=response["status"])

        else:
            response = {"status": "ERROR", "msg": "Unknown action"}
            logger.warning("TCP: unknown action", action=action, address=str(addr))

    except json.JSONDecodeError as exc:
        response = {"status": "ERROR", "msg": "Invalid JSON"}
        logger.warning("TCP: invalid payload", address=str(addr), error=str(exc))
    except Exception:
        response = {"status": "ERROR", "msg": "Internal server error"}
        logger.exception("TCP handler failure", address=str(addr))
    finally:
        try:
            conn.send(json.dumps(response).encode("utf-8"))
        except Exception:
            logger.debug("Could not send response", address=str(addr))
        finally:
            conn.close()


def tcp_server(store: SessionStore) -> None:
    base_dir = os.path.dirname(os.path.abspath(__file__))
    cert_path = os.path.join(base_dir, "cert.pem")
    key_path = os.path.join(base_dir, "key.pem")

    context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    context.load_cert_chain(certfile=cert_path, keyfile=key_path)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((HOST, PORT))
    sock.listen(10)
    logger.info("Zenith Signaling TCP with TLS started", port=PORT)

    with context.wrap_socket(sock, server_side=True) as secure_sock:
        while True:
            try:
                conn, addr = secure_sock.accept()
                logger.info("Accepted secure connection", address=str(addr))
                threading.Thread(
                    target=handle_tcp_client,
                    args=(conn, addr, store),
                    daemon=True,
                ).start()
            except ssl.SSLError as exc:
                logger.error("SSL error on accept", error=str(exc))
            except Exception:
                logger.exception("TCP accept loop failure")


def run_signaling() -> None:
    store = SessionStore(ttl=SESSION_TTL)

    threading.Thread(target=cleanup_loop, args=(store,), daemon=True).start()
    threading.Thread(target=udp_listener, args=(store,), daemon=True).start()

    tcp_server(store)


if __name__ == "__main__":
    # Configure logging when run as main module
    config = ZenithConfig()
    configure_logging(config)
    run_signaling()
