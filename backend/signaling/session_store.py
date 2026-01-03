"""
Session store for signaling server with optional persistent backend.

This module provides a thread-safe session store that can optionally persist
sessions to Redis or disk to prevent data loss on crash or restart.

Security Considerations:
    - Uses secrets module for cryptographically secure code generation
    - Implements rate limiting per IP to prevent brute-force attacks
    - Session codes are 6 digits (1M possibilities) with collision avoidance
    - Rate limit: max 10 session creations per IP per minute (configurable)

Persistence Backends:
    - Memory-only: Default, fast but volatile
    - Redis: Recommended for production, requires redis-py
    - Disk: JSON-based fallback, suitable for single-instance deployments

Implementation Notes:
    - RLock used for thread-safety with reentrant capabilities
    - All operations are atomic within lock scope
    - Persistence operations are synchronous for data consistency
    - Rate limiting uses sliding window with per-IP counters
"""

import json
import secrets
import threading
import time
from abc import ABC, abstractmethod
from collections import defaultdict, deque
from dataclasses import dataclass, field
from pathlib import Path
from typing import Deque, Dict, Optional, Tuple

Address = Tuple[str, int]


@dataclass
class SessionEntry:
    """Represents a signaling session with timestamp and optional host address."""

    timestamp: float = field(default_factory=time.time)
    host: Optional[Address] = None

    def to_dict(self) -> dict:
        """Convert to dict for serialization."""
        return {
            "timestamp": self.timestamp,
            "host": list(self.host) if self.host else None,
        }

    @classmethod
    def from_dict(cls, data: dict) -> "SessionEntry":
        """Create from dict after deserialization."""
        host = tuple(data["host"]) if data.get("host") else None
        return cls(timestamp=data["timestamp"], host=host)


class PersistenceBackend(ABC):
    """Abstract base for session persistence backends."""

    @abstractmethod
    def save(self, code: str, entry: SessionEntry) -> None:
        """Persist a session entry."""
        pass

    @abstractmethod
    def load(self, code: str) -> Optional[SessionEntry]:
        """Load a session entry."""
        pass

    @abstractmethod
    def delete(self, code: str) -> None:
        """Remove a session entry."""
        pass

    @abstractmethod
    def load_all(self) -> Dict[str, SessionEntry]:
        """Load all sessions (used for initialization)."""
        pass

    @abstractmethod
    def delete_all(self, codes: list) -> None:
        """Batch delete sessions."""
        pass


class MemoryBackend(PersistenceBackend):
    """No-op backend for memory-only storage (default)."""

    def save(self, code: str, entry: SessionEntry) -> None:
        pass

    def load(self, code: str) -> Optional[SessionEntry]:
        return None

    def delete(self, code: str) -> None:
        pass

    def load_all(self) -> Dict[str, SessionEntry]:
        return {}

    def delete_all(self, codes: list) -> None:
        pass


class DiskBackend(PersistenceBackend):
    """JSON-based disk persistence for session recovery."""

    def __init__(self, filepath: str = "/tmp/zenith_sessions.json") -> None:
        self.filepath = Path(filepath)
        self._lock = threading.Lock()

    def save(self, code: str, entry: SessionEntry) -> None:
        with self._lock:
            data = self._load_file()
            data[code] = entry.to_dict()
            self._save_file(data)

    def load(self, code: str) -> Optional[SessionEntry]:
        with self._lock:
            data = self._load_file()
            entry_data = data.get(code)
            return SessionEntry.from_dict(entry_data) if entry_data else None

    def delete(self, code: str) -> None:
        with self._lock:
            data = self._load_file()
            data.pop(code, None)
            self._save_file(data)

    def load_all(self) -> Dict[str, SessionEntry]:
        with self._lock:
            data = self._load_file()
            return {k: SessionEntry.from_dict(v) for k, v in data.items()}

    def delete_all(self, codes: list) -> None:
        with self._lock:
            data = self._load_file()
            for code in codes:
                data.pop(code, None)
            self._save_file(data)

    def _load_file(self) -> dict:
        if not self.filepath.exists():
            return {}
        try:
            with open(self.filepath, "r") as f:
                return json.load(f)
        except (json.JSONDecodeError, IOError):
            return {}

    def _save_file(self, data: dict) -> None:
        try:
            self.filepath.parent.mkdir(parents=True, exist_ok=True)
            with open(self.filepath, "w") as f:
                json.dump(data, f)
        except IOError as e:
            # Log at module level if available, otherwise pass silently
            # This prevents server crashes while allowing debugging
            import logging
            logging.getLogger(__name__).warning(
                "Failed to persist sessions to disk: %s", e
            )


class RedisBackend(PersistenceBackend):
    """Redis persistence backend for distributed deployments."""

    def __init__(self, redis_url: str = "redis://localhost:6379/0", prefix: str = "zenith:session:") -> None:
        try:
            import redis
            self.redis = redis.from_url(redis_url, decode_responses=True)
            self.prefix = prefix
        except ImportError:
            raise ImportError("redis-py is required for Redis backend. Install with: pip install redis")

    def save(self, code: str, entry: SessionEntry) -> None:
        key = f"{self.prefix}{code}"
        self.redis.set(key, json.dumps(entry.to_dict()))

    def load(self, code: str) -> Optional[SessionEntry]:
        key = f"{self.prefix}{code}"
        data = self.redis.get(key)
        return SessionEntry.from_dict(json.loads(data)) if data else None

    def delete(self, code: str) -> None:
        key = f"{self.prefix}{code}"
        self.redis.delete(key)

    def load_all(self) -> Dict[str, SessionEntry]:
        pattern = f"{self.prefix}*"
        sessions = {}
        for key in self.redis.scan_iter(match=pattern):
            code = key[len(self.prefix):]
            data = self.redis.get(key)
            if data:
                sessions[code] = SessionEntry.from_dict(json.loads(data))
        return sessions

    def delete_all(self, codes: list) -> None:
        if codes:
            keys = [f"{self.prefix}{code}" for code in codes]
            self.redis.delete(*keys)


class SessionStore:
    """
    Thread-safe, TTL-backed store for signaling sessions with optional persistence.

    Provides resilient session management with cryptographically secure code generation,
    rate limiting, and optional persistent storage backends.

    Args:
        ttl: Session time-to-live in seconds (default: 300)
        backend: Persistence backend instance (default: MemoryBackend)
        rate_limit_window: Rate limit window in seconds (default: 60)
        rate_limit_max: Maximum creations per window per IP (default: 10)

    Example:
        >>> # Memory-only (default)
        >>> store = SessionStore(ttl=300)
        >>>
        >>> # With disk persistence
        >>> disk_backend = DiskBackend("/var/lib/zenith/sessions.json")
        >>> store = SessionStore(ttl=300, backend=disk_backend)
        >>>
        >>> # With Redis persistence
        >>> redis_backend = RedisBackend("redis://localhost:6379/0")
        >>> store = SessionStore(ttl=300, backend=redis_backend)
    """

    def __init__(
        self,
        ttl: int = 300,
        backend: Optional[PersistenceBackend] = None,
        rate_limit_window: int = 60,
        rate_limit_max: int = 10,
    ) -> None:
        self._ttl = ttl
        self._backend = backend or MemoryBackend()
        self._sessions: Dict[str, SessionEntry] = {}
        self._lock = threading.RLock()

        # Rate limiting: track creation timestamps per IP
        self._rate_limit_window = rate_limit_window
        self._rate_limit_max = rate_limit_max
        self._creation_times: Dict[str, Deque[float]] = defaultdict(deque)

        # Load persisted sessions on initialization
        self._load_persisted_sessions()

    def _load_persisted_sessions(self) -> None:
        """Load sessions from persistent backend on startup."""
        try:
            persisted = self._backend.load_all()
            now = time.time()
            # Only load non-expired sessions
            for code, entry in persisted.items():
                if now - entry.timestamp <= self._ttl:
                    self._sessions[code] = entry
        except Exception as e:
            # Log the error but don't crash on startup
            import logging
            logging.getLogger(__name__).warning(
                "Failed to load persisted sessions: %s", e
            )

    def create_session(self, client_ip: Optional[str] = None) -> Optional[str]:
        """
        Create a new session with a unique code.

        Args:
            client_ip: Optional client IP for rate limiting

        Returns:
            Session code if successful, None if rate limited

        Security:
            - Uses secrets.randbelow for cryptographic randomness
            - 6-digit codes provide 1,000,000 possible values
            - Rate limiting prevents brute-force attacks
        """
        with self._lock:
            # Check rate limit
            if client_ip and not self._check_rate_limit(client_ip):
                return None

            code = self._generate_code()
            entry = SessionEntry()
            self._sessions[code] = entry
            self._backend.save(code, entry)

            # Record creation time for rate limiting
            if client_ip:
                self._creation_times[client_ip].append(time.time())

            return code

    def register_host(self, code: str, addr: Address) -> bool:
        """Register a host address for an existing session."""
        with self._lock:
            session = self._sessions.get(code)
            if session is None:
                return False
            session.host = addr
            session.timestamp = time.time()
            self._backend.save(code, session)
            return True

    def get_host(self, code: str) -> Optional[Address]:
        """Get the host address for a session, updating its timestamp."""
        with self._lock:
            session = self._sessions.get(code)
            if session is None:
                return None
            session.timestamp = time.time()
            self._backend.save(code, session)
            return session.host

    def has_code(self, code: str) -> bool:
        """Check if a session code exists."""
        with self._lock:
            return code in self._sessions

    def cleanup(self) -> int:
        """
        Remove expired sessions from memory and persistent storage.

        Returns:
            Number of sessions cleaned up
        """
        with self._lock:
            now = time.time()
            expired = [k for k, v in self._sessions.items() if now - v.timestamp > self._ttl]

            # Remove from memory
            for code in expired:
                self._sessions.pop(code, None)

            # Remove from persistent storage
            if expired:
                self._backend.delete_all(expired)

            # Clean up old rate limit entries
            self._cleanup_rate_limits(now)

            return len(expired)

    def _check_rate_limit(self, client_ip: str) -> bool:
        """
        Check if client IP has exceeded rate limit.

        Uses sliding window: only count creations within the time window.
        """
        now = time.time()
        timestamps = self._creation_times[client_ip]

        # Remove timestamps outside the window
        while timestamps and now - timestamps[0] > self._rate_limit_window:
            timestamps.popleft()

        # Check if under limit
        return len(timestamps) < self._rate_limit_max

    def _cleanup_rate_limits(self, now: float) -> None:
        """Clean up old rate limit tracking data."""
        expired_ips = []
        for ip, timestamps in self._creation_times.items():
            while timestamps and now - timestamps[0] > self._rate_limit_window:
                timestamps.popleft()
            if not timestamps:
                expired_ips.append(ip)

        for ip in expired_ips:
            del self._creation_times[ip]

    def _generate_code(self) -> str:
        """
        Generate a cryptographically secure unique session code.

        Uses secrets.randbelow for cryptographic randomness instead of random.randint.
        6-digit codes (100000-999999) provide 900,000 possible values, reducing
        collision probability while maintaining usability.

        Returns:
            A unique 6-digit session code as a string
        """
        max_attempts = 1000
        for _ in range(max_attempts):
            # Generate 6-digit code (100000-999999) using cryptographic random
            candidate = str(100000 + secrets.randbelow(900000))
            if candidate not in self._sessions:
                return candidate

        # Fallback: if we can't find a unique code after max_attempts,
        # try cleanup first to free up space, then use longer codes
        self.cleanup()
        
        # Try again with same range after cleanup
        for _ in range(max_attempts):
            candidate = str(100000 + secrets.randbelow(900000))
            if candidate not in self._sessions:
                return candidate
        
        # Final fallback: use longer codes (7-8 digits) to guarantee uniqueness
        fallback_attempts = 10000
        for _ in range(fallback_attempts):
            candidate = str(100000 + secrets.randbelow(10000000))
            if candidate not in self._sessions:
                return candidate
        
        # If we still can't generate a code, something is very wrong
        # This should never happen in practice
        raise RuntimeError("Unable to generate unique session code after extensive attempts")


__all__ = ["SessionStore", "SessionEntry", "PersistenceBackend", "MemoryBackend", "DiskBackend", "RedisBackend"]
