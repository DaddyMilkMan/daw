"""Tests for the signaling TCP handler routines."""

import json
import os
import socket
import sys

root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if root not in sys.path:
    sys.path.insert(0, root)

from backend.signaling.signaling_server import handle_tcp_client
from backend.signaling.session_store import SessionStore


def _run_handler(store: SessionStore, payload: bytes) -> dict:
    server, client = socket.socketpair()
    client.send(payload)
    handle_tcp_client(server, ("127.0.0.1", 0), store)
    data = client.recv(2048)
    client.close()
    server.close()
    return json.loads(data.decode("utf-8"))


def test_handle_tcp_register_returns_code() -> None:
    store = SessionStore()
    payload = json.dumps({"action": "REGISTER"}).encode("utf-8")

    response = _run_handler(store, payload)

    assert response["status"] == "OK"
    assert "code" in response
    assert store.has_code(response["code"])


def test_handle_tcp_lookup_existing_code() -> None:
    store = SessionStore()
    code = store.create_session()
    payload = json.dumps({"action": "LOOKUP", "code": code}).encode("utf-8")

    response = _run_handler(store, payload)

    assert response["status"] == "OK"


def test_handle_tcp_invalid_json_returns_error() -> None:
    store = SessionStore()
    response = _run_handler(store, b"not a json")

    assert response["status"] == "ERROR"
