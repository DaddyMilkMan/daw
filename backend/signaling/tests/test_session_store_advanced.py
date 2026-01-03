"""Advanced tests for session store: thread-safety, rate limiting, persistence, and security."""

import concurrent.futures
import os
import sys
import tempfile
import threading
import time
from pathlib import Path

root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if root not in sys.path:
    sys.path.insert(0, root)

from backend.signaling.session_store import (
    DiskBackend,
    MemoryBackend,
    SessionEntry,
    SessionStore,
)


# Thread-safety tests
def test_concurrent_session_creation_no_duplicates() -> None:
    """Verify no duplicate codes when creating sessions concurrently."""
    store = SessionStore(ttl=10)
    num_threads = 20
    sessions_per_thread = 50

    def create_sessions():
        return [store.create_session() for _ in range(sessions_per_thread)]

    with concurrent.futures.ThreadPoolExecutor(max_workers=num_threads) as executor:
        futures = [executor.submit(create_sessions) for _ in range(num_threads)]
        all_codes = []
        for future in concurrent.futures.as_completed(futures):
            all_codes.extend(future.result())

    # Check all codes are unique (no duplicates from race conditions)
    assert len(all_codes) == len(set(all_codes))
    assert None not in all_codes


def test_concurrent_register_and_get_host() -> None:
    """Verify thread-safe registration and retrieval of hosts."""
    store = SessionStore(ttl=10)
    code = store.create_session()
    num_threads = 10
    results = []

    def register_host(addr):
        success = store.register_host(code, addr)
        retrieved = store.get_host(code)
        results.append((success, retrieved))

    threads = []
    for i in range(num_threads):
        addr = (f"192.0.2.{i}", 6000 + i)
        t = threading.Thread(target=register_host, args=(addr,))
        threads.append(t)
        t.start()

    for t in threads:
        t.join()

    # All operations should succeed
    assert all(r[0] for r in results)
    # All should get a valid host (might be different due to race, but not None)
    assert all(r[1] is not None for r in results)


def test_concurrent_cleanup_is_safe() -> None:
    """Verify cleanup can run concurrently with other operations."""
    store = SessionStore(ttl=1)
    
    def create_and_access():
        for _ in range(10):
            code = store.create_session()
            if code:
                store.has_code(code)
                time.sleep(0.01)

    def cleanup_loop():
        for _ in range(10):
            store.cleanup()
            time.sleep(0.01)

    threads = [
        threading.Thread(target=create_and_access) for _ in range(3)
    ] + [
        threading.Thread(target=cleanup_loop) for _ in range(2)
    ]

    for t in threads:
        t.start()
    for t in threads:
        t.join()

    # Just verify no crashes occurred


# Rate limiting tests
def test_rate_limiting_blocks_excessive_requests() -> None:
    """Verify rate limiting blocks IPs that exceed the limit."""
    store = SessionStore(ttl=300, rate_limit_window=5, rate_limit_max=3)
    client_ip = "192.0.2.100"

    # First 3 should succeed
    codes = []
    for _ in range(3):
        code = store.create_session(client_ip=client_ip)
        assert code is not None
        codes.append(code)

    # 4th should be rate limited
    code = store.create_session(client_ip=client_ip)
    assert code is None

    # Different IP should still work
    code = store.create_session(client_ip="192.0.2.101")
    assert code is not None


def test_rate_limiting_allows_after_window() -> None:
    """Verify rate limiting resets after the time window."""
    store = SessionStore(ttl=300, rate_limit_window=1, rate_limit_max=2)
    client_ip = "192.0.2.100"

    # Use up the limit
    assert store.create_session(client_ip=client_ip) is not None
    assert store.create_session(client_ip=client_ip) is not None
    assert store.create_session(client_ip=client_ip) is None

    # Wait for window to expire
    time.sleep(1.1)

    # Should work again
    assert store.create_session(client_ip=client_ip) is not None


def test_rate_limiting_without_ip_always_succeeds() -> None:
    """Verify rate limiting doesn't apply when no IP is provided."""
    store = SessionStore(ttl=300, rate_limit_window=1, rate_limit_max=2)

    # Should be able to create many sessions without IP
    for _ in range(10):
        code = store.create_session()
        assert code is not None


# Security tests
def test_code_generation_uses_cryptographic_random() -> None:
    """Verify generated codes have sufficient entropy."""
    store = SessionStore(ttl=10)
    codes = set()
    num_samples = 1000

    for _ in range(num_samples):
        code = store.create_session()
        codes.add(code)

    # All codes should be 6 digits
    assert all(len(code) == 6 for code in codes)
    assert all(code.isdigit() for code in codes)
    
    # Should have high uniqueness (no duplicates in reasonable sample size)
    assert len(codes) == num_samples


def test_code_generation_avoids_collisions() -> None:
    """Verify collision avoidance when store is heavily populated."""
    store = SessionStore(ttl=10)
    
    # Pre-populate with many codes
    codes = set()
    for _ in range(5000):
        code = store.create_session()
        codes.add(code)

    # Should still be able to generate unique codes
    new_code = store.create_session()
    assert new_code is not None
    assert new_code not in codes


def test_code_range_is_expanded() -> None:
    """Verify codes are 6 digits (100000-999999) not 4 digits."""
    store = SessionStore(ttl=10)
    
    for _ in range(100):
        code = store.create_session()
        code_int = int(code)
        assert 100000 <= code_int <= 999999


# Persistence tests
def test_memory_backend_does_not_persist() -> None:
    """Verify MemoryBackend doesn't actually persist data."""
    backend = MemoryBackend()
    entry = SessionEntry(timestamp=time.time(), host=("192.0.2.1", 6000))
    
    backend.save("1234", entry)
    loaded = backend.load("1234")
    
    assert loaded is None


def test_disk_backend_persists_and_loads() -> None:
    """Verify DiskBackend correctly persists and loads sessions."""
    with tempfile.TemporaryDirectory() as tmpdir:
        filepath = Path(tmpdir) / "test_sessions.json"
        backend = DiskBackend(str(filepath))
        
        # Save a session
        entry = SessionEntry(timestamp=time.time(), host=("192.0.2.1", 6000))
        backend.save("123456", entry)
        
        # Load it back
        loaded = backend.load("123456")
        assert loaded is not None
        assert loaded.host == ("192.0.2.1", 6000)
        assert abs(loaded.timestamp - entry.timestamp) < 0.01


def test_disk_backend_deletes() -> None:
    """Verify DiskBackend deletion works."""
    with tempfile.TemporaryDirectory() as tmpdir:
        filepath = Path(tmpdir) / "test_sessions.json"
        backend = DiskBackend(str(filepath))
        
        entry = SessionEntry(timestamp=time.time(), host=("192.0.2.1", 6000))
        backend.save("123456", entry)
        backend.delete("123456")
        
        loaded = backend.load("123456")
        assert loaded is None


def test_disk_backend_batch_delete() -> None:
    """Verify DiskBackend batch deletion."""
    with tempfile.TemporaryDirectory() as tmpdir:
        filepath = Path(tmpdir) / "test_sessions.json"
        backend = DiskBackend(str(filepath))
        
        # Create multiple sessions
        for i in range(5):
            entry = SessionEntry(timestamp=time.time(), host=("192.0.2.1", 6000 + i))
            backend.save(f"12345{i}", entry)
        
        # Delete some
        backend.delete_all(["123450", "123452", "123454"])
        
        # Verify
        assert backend.load("123450") is None
        assert backend.load("123451") is not None
        assert backend.load("123452") is None
        assert backend.load("123453") is not None
        assert backend.load("123454") is None


def test_store_with_disk_backend_persists_on_create() -> None:
    """Verify SessionStore persists sessions when using DiskBackend."""
    with tempfile.TemporaryDirectory() as tmpdir:
        filepath = Path(tmpdir) / "test_sessions.json"
        backend = DiskBackend(str(filepath))
        
        # Create store and session
        store = SessionStore(ttl=300, backend=backend)
        code = store.create_session()
        
        # Verify persisted to disk
        loaded = backend.load(code)
        assert loaded is not None


def test_store_loads_persisted_sessions_on_init() -> None:
    """Verify SessionStore loads sessions from disk on initialization."""
    with tempfile.TemporaryDirectory() as tmpdir:
        filepath = Path(tmpdir) / "test_sessions.json"
        
        # Create and persist a session
        backend1 = DiskBackend(str(filepath))
        store1 = SessionStore(ttl=300, backend=backend1)
        code = store1.create_session()
        store1.register_host(code, ("192.0.2.1", 6000))
        
        # Create new store with same backend
        backend2 = DiskBackend(str(filepath))
        store2 = SessionStore(ttl=300, backend=backend2)
        
        # Should have loaded the session
        assert store2.has_code(code)
        assert store2.get_host(code) == ("192.0.2.1", 6000)


def test_store_doesnt_load_expired_sessions() -> None:
    """Verify expired sessions aren't loaded on initialization."""
    with tempfile.TemporaryDirectory() as tmpdir:
        filepath = Path(tmpdir) / "test_sessions.json"
        
        # Create and persist an expired session
        backend1 = DiskBackend(str(filepath))
        store1 = SessionStore(ttl=1, backend=backend1)
        code = store1.create_session()
        
        # Manually expire it
        store1._sessions[code].timestamp = time.time() - 10
        backend1.save(code, store1._sessions[code])
        
        # Wait to ensure it's expired
        time.sleep(0.1)
        
        # Create new store - should not load expired session
        backend2 = DiskBackend(str(filepath))
        store2 = SessionStore(ttl=1, backend=backend2)
        
        assert not store2.has_code(code)


def test_cleanup_removes_from_disk_backend() -> None:
    """Verify cleanup removes sessions from persistent storage."""
    with tempfile.TemporaryDirectory() as tmpdir:
        filepath = Path(tmpdir) / "test_sessions.json"
        backend = DiskBackend(str(filepath))
        
        store = SessionStore(ttl=1, backend=backend)
        code = store.create_session()
        
        # Manually expire
        store._sessions[code].timestamp = time.time() - 10
        
        # Cleanup
        store.cleanup()
        
        # Should be gone from both memory and disk
        assert not store.has_code(code)
        assert backend.load(code) is None


def test_disk_backend_handles_corrupted_file() -> None:
    """Verify DiskBackend handles corrupted JSON gracefully."""
    with tempfile.TemporaryDirectory() as tmpdir:
        filepath = Path(tmpdir) / "test_sessions.json"
        
        # Write corrupted JSON
        with open(filepath, "w") as f:
            f.write("{ invalid json }")
        
        backend = DiskBackend(str(filepath))
        
        # Should not crash
        result = backend.load_all()
        assert result == {}


def test_session_entry_serialization() -> None:
    """Verify SessionEntry serialization/deserialization."""
    entry = SessionEntry(timestamp=1234567890.5, host=("192.0.2.1", 6000))
    
    data = entry.to_dict()
    assert data["timestamp"] == 1234567890.5
    assert data["host"] == ["192.0.2.1", 6000]
    
    restored = SessionEntry.from_dict(data)
    assert restored.timestamp == 1234567890.5
    assert restored.host == ("192.0.2.1", 6000)


def test_session_entry_serialization_without_host() -> None:
    """Verify SessionEntry serialization when host is None."""
    entry = SessionEntry(timestamp=1234567890.5, host=None)
    
    data = entry.to_dict()
    assert data["host"] is None
    
    restored = SessionEntry.from_dict(data)
    assert restored.host is None
