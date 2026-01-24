import random
import threading
import time
from collections import OrderedDict, deque
from dataclasses import dataclass, field
from typing import Dict, Optional, Tuple

Address = Tuple[str, int]


@dataclass
class SessionEntry:
    timestamp: float = field(default_factory=time.time)
    host: Optional[Address] = None


class SessionStore:
    """Thread-safe TTL-backed store for signaling sessions."""

    def __init__(self, ttl: int = 300) -> None:
        self._ttl = ttl
        self._sessions: OrderedDict[str, SessionEntry] = OrderedDict()
        self._lock = threading.RLock()

        # Pre-allocate and shuffle all possible 4-digit codes (1000-9999)
        # O(N) initialization
        all_codes = [str(i) for i in range(1000, 10000)]
        random.shuffle(all_codes)
        self._available_codes = deque(all_codes)

    def create_session(self) -> str:
        """Create a new session and return its unique 4-digit code.
        
        Returns:
            A unique session code (string of 4 digits).
            
        Raises:
            RuntimeError: If the session store is full (all 9000 codes in use).
        """
        with self._lock:
            code = self._generate_code()
            self._sessions[code] = SessionEntry()
            return code

    def register_host(self, code: str, addr: Address) -> bool:
        """Register a host address for a given session code.
        
        Args:
            code: The session code.
            addr: The host address tuple (ip, port).
            
        Returns:
            True if successful, False if the code doesn't exist.
        """
        with self._lock:
            session = self._sessions.get(code)
            if session is None:
                return False
            session.host = addr
            session.timestamp = time.time()
            self._sessions.move_to_end(code)
            return True

    def get_host(self, code: str) -> Optional[Address]:
        """Get the host address for a session code.
        
        Args:
            code: The session code.
            
        Returns:
            The host address tuple (ip, port) if found, None otherwise.
        """
        with self._lock:
            session = self._sessions.get(code)
            if session is None:
                return None
            session.timestamp = time.time()
            self._sessions.move_to_end(code)
            return session.host

    def has_code(self, code: str) -> bool:
        """Check if a session code exists.
        
        Args:
            code: The session code to check.
            
        Returns:
            True if the code exists, False otherwise.
        """
        with self._lock:
            return code in self._sessions

    def cleanup(self) -> int:
        """Remove expired sessions and recycle their codes.
        
        Returns:
            The number of sessions removed.
        """
        with self._lock:
            now = time.time()
            expired_count = 0
            while self._sessions:
                code = next(iter(self._sessions))
                session = self._sessions[code]
                if now - session.timestamp > self._ttl:
                    self._sessions.popitem(last=False)
                    self._available_codes.append(code) # Recycle the code
                    expired_count += 1
                else:
                    break
            return expired_count

    def _generate_code(self) -> str:
        """Generate a unique session code from the available pool.
        
        Uses FIFO ordering to ensure better distribution of codes.
        """
        if not self._available_codes:
            raise RuntimeError("Session store full")
        return self._available_codes.popleft()


__all__ = ["SessionStore"]
