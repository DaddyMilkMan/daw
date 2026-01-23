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
        with self._lock:
            code = self._generate_code()
            self._sessions[code] = SessionEntry()
            return code

    def register_host(self, code: str, addr: Address) -> bool:
        with self._lock:
            session = self._sessions.get(code)
            if session is None:
                return False
            session.host = addr
            session.timestamp = time.time()
            self._sessions.move_to_end(code)
            return True

    def get_host(self, code: str) -> Optional[Address]:
        with self._lock:
            session = self._sessions.get(code)
            if session is None:
                return None
            session.timestamp = time.time()
            self._sessions.move_to_end(code)
            return session.host

    def has_code(self, code: str) -> bool:
        with self._lock:
            return code in self._sessions

    def cleanup(self) -> int:
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
        if not self._available_codes:
            raise RuntimeError("Session store full")
        return self._available_codes.pop()


__all__ = ["SessionStore"]
