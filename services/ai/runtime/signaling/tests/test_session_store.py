"""Pytest coverage for the signaling session store."""

import os
import sys
import time

root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if root not in sys.path:
    sys.path.insert(0, root)

from backend.signaling.session_store import SessionStore


def test_create_session_generates_unique_code_and_records_it() -> None:
    store = SessionStore(ttl=10)
    first = store.create_session()
    second = store.create_session()

    assert first != second
    assert store.has_code(first)
    assert store.has_code(second)


def test_register_host_returns_false_for_unknown_code() -> None:
    store = SessionStore(ttl=10)
    assert not store.register_host("9999", ("127.0.0.1", 1234))


def test_register_and_get_host_updates_timestamp() -> None:
    store = SessionStore(ttl=10)
    code = store.create_session()
    host = ("192.0.2.1", 6000)

    assert store.register_host(code, host)
    assert store.get_host(code) == host


def test_cleanup_removes_stale_sessions() -> None:
    store = SessionStore(ttl=1)
    code = store.create_session()
    now = time.time()
    store._sessions[code].timestamp = now - 2

    removed = store.cleanup()

    assert removed == 1
    assert not store.has_code(code)
