"""Minimal TLS signaling server with UDP hole-punch coordination."""

import json
import logging
import os
import socket
import ssl
import sys
import threading
import time

LOG_FORMAT = "%(asctime)s %(levelname)s %(message)s"
logger = logging.getLogger("zenith.signaling")

if __package__ is None:
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    if root not in sys.path:
        sys.path.insert(0, root)

try:
    from .session_store import SessionStore, DiskBackend, MemoryBackend, RedisBackend
except ImportError:  # pragma: no cover - script run outside package
    from session_store import SessionStore, DiskBackend, MemoryBackend, RedisBackend

HOST = os.environ.get("ZENITH_SIGNALING_HOST", "0.0.0.0")
PORT = int(os.environ.get("ZENITH_SIGNALING_PORT", "54320"))
UDP_PORT = int(os.environ.get("ZENITH_SIGNALING_UDP_PORT", "54321"))
SESSION_TTL = int(os.environ.get("ZENITH_SIGNALING_SESSION_TTL", "300"))
SESSION_CLEAN_INTERVAL = int(os.environ.get("ZENITH_SIGNALING_CLEAN_FREQ", "60"))

# Persistence configuration
PERSISTENCE_BACKEND = os.environ.get("ZENITH_PERSISTENCE_BACKEND", "memory")  # memory, disk, redis
PERSISTENCE_DISK_PATH = os.environ.get("ZENITH_PERSISTENCE_DISK_PATH", "/tmp/zenith_sessions.json")
PERSISTENCE_REDIS_URL = os.environ.get("ZENITH_PERSISTENCE_REDIS_URL", "redis://localhost:6379/0")

# Rate limiting configuration
RATE_LIMIT_ENABLED = os.environ.get("ZENITH_RATE_LIMIT_ENABLED", "true").lower() == "true"
RATE_LIMIT_WINDOW = int(os.environ.get("ZENITH_RATE_LIMIT_WINDOW", "60"))
RATE_LIMIT_MAX = int(os.environ.get("ZENITH_RATE_LIMIT_MAX", "10"))


def configure_logging(level: int = logging.INFO) -> None:
    logging.basicConfig(level=level, format=LOG_FORMAT)


def cleanup_loop(store: SessionStore) -> None:
    while True:
        time.sleep(SESSION_CLEAN_INTERVAL)
        cleaned = store.cleanup()
        if cleaned:
            logger.info("Cleaned up %d expired sessions", cleaned)


def udp_listener(store: SessionStore) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.bind((HOST, UDP_PORT))
        logger.info("Zenith UDP Hole Punch Listener on %d", UDP_PORT)

        while True:
            try:
                data, addr = sock.recvfrom(1024)
                msg = data.decode("utf-8", errors="ignore").strip()

                if msg.startswith("REGISTER:"):
                    code = msg.split(":", 1)[1]
                    if store.register_host(code, addr):
                        logger.info("Host %s UDP registered at %s", code, addr)
                    else:
                        logger.warning("REGISTER failed: code %s not found", code)

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
                            "Hole punch initiated for %s: %s <-> %s",
                            code,
                            host_addr,
                            client_addr,
                        )
                    else:
                        logger.warning("JOIN failed: code %s has no host recorded", code)
            except Exception as exc:
                logger.exception("UDP listener error: %s", exc)


def handle_tcp_client(conn: socket.socket, addr: tuple, store: SessionStore) -> None:
    response = {"status": "ERROR", "msg": "Unhandled error"}
    try:
        payload = conn.recv(2048)
        if not payload:
            logger.debug("Empty payload from %s", addr)
            return

        request = json.loads(payload.decode("utf-8"))
        action = request.get("action")

        if action == "REGISTER":
            client_ip = addr[0] if RATE_LIMIT_ENABLED else None
            code = store.create_session(client_ip=client_ip)
            if code is None:
                response = {"status": "ERROR", "msg": "Rate limit exceeded"}
                logger.warning("TCP: Rate limit exceeded for %s", addr)
            else:
                response = {"status": "OK", "code": code}
                logger.info("TCP: Registered session %s from %s", code, addr)

        elif action == "LOOKUP":
            code = request.get("code")
            if code and store.has_code(code):
                response = {"status": "OK"}
            else:
                response = {"status": "ERROR", "msg": "Code not found"}
            logger.debug("TCP: LOOKUP %s -> %s", code, response["status"])

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
    base_dir = os.path.dirname(os.path.abspath(__file__))
    cert_path = os.path.join(base_dir, "cert.pem")
    key_path = os.path.join(base_dir, "key.pem")

    context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    context.load_cert_chain(certfile=cert_path, keyfile=key_path)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
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
    # Configure persistence backend
    backend = None
    if PERSISTENCE_BACKEND == "disk":
        backend = DiskBackend(PERSISTENCE_DISK_PATH)
        logger.info("Using disk persistence: %s", PERSISTENCE_DISK_PATH)
    elif PERSISTENCE_BACKEND == "redis":
        try:
            backend = RedisBackend(PERSISTENCE_REDIS_URL)
            logger.info("Using Redis persistence: %s", PERSISTENCE_REDIS_URL)
        except ImportError:
            logger.error("Redis backend requested but redis-py not installed. Falling back to memory-only.")
            backend = MemoryBackend()
        except Exception as e:
            logger.error("Failed to initialize Redis backend: %s. Falling back to memory-only.", e)
            backend = MemoryBackend()
    else:
        backend = MemoryBackend()
        logger.info("Using memory-only persistence (sessions will be lost on restart)")

    # Create session store with configured backend and rate limiting
    store = SessionStore(
        ttl=SESSION_TTL,
        backend=backend,
        rate_limit_window=RATE_LIMIT_WINDOW if RATE_LIMIT_ENABLED else 60,
        rate_limit_max=RATE_LIMIT_MAX if RATE_LIMIT_ENABLED else 10,
    )

    if RATE_LIMIT_ENABLED:
        logger.info("Rate limiting enabled: max %d requests per %d seconds per IP", RATE_LIMIT_MAX, RATE_LIMIT_WINDOW)
    else:
        logger.info("Rate limiting disabled")

    threading.Thread(target=cleanup_loop, args=(store,), daemon=True).start()
    threading.Thread(target=udp_listener, args=(store,), daemon=True).start()

    tcp_server(store)


if __name__ == "__main__":
    configure_logging()
    run_signaling()
