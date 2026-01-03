"""
Thread-safe session storage with TTL management for signaling server.

This module provides a thread-safe data store for managing signaling sessions
with automatic expiration based on time-to-live (TTL) settings.
"""

import random
import threading
import time
from dataclasses import dataclass, field
from typing import Dict, Optional, Tuple

Address = Tuple[str, int]


@dataclass
class SessionEntry:
    """
    Represents a single signaling session.
    
    Attributes:
        timestamp: Unix timestamp of last activity for TTL calculation.
        host: Optional address tuple (IP, port) of the session host.
    """
    timestamp: float = field(default_factory=time.time)
    host: Optional[Address] = None


class SessionStore:
    """
    Thread-safe TTL-backed store for signaling sessions.
    
    Manages signaling sessions with automatic expiration and thread-safe access.
    Sessions are identified by unique 4-digit codes and can be associated with
    host addresses for peer-to-peer connection coordination.
    
    Attributes:
        _ttl: Time-to-live in seconds for sessions.
        _sessions: Internal dictionary mapping codes to session entries.
        _lock: Reentrant lock for thread-safe operations.
    """

    def __init__(self, ttl: int = 300) -> None:
        """
        Initialize a new SessionStore.
        
        Args:
            ttl: Time-to-live in seconds for sessions (default: 300).
        """
        self._ttl = ttl
        self._sessions: Dict[str, SessionEntry] = {}
        self._lock = threading.RLock()

    def create_session(self) -> str:
        """
        Create a new session with a unique code.
        
        Generates a unique 4-digit code and creates a session entry.
        
        Returns:
            str: The unique 4-digit session code.
        """
        with self._lock:
            code = self._generate_code()
            self._sessions[code] = SessionEntry()
            return code

    def register_host(self, code: str, addr: Address) -> bool:
        """
        Register a host address for an existing session.
        
        Updates the session's host address and refreshes the timestamp.
        
        Args:
            code: The session code to update.
            addr: Address tuple (IP, port) of the host.
            
        Returns:
            bool: True if registration succeeded, False if code doesn't exist.
        """
        with self._lock:
            session = self._sessions.get(code)
            if session is None:
                return False
            session.host = addr
            session.timestamp = time.time()
            return True

    def get_host(self, code: str) -> Optional[Address]:
        """
        Retrieve the host address for a session.
        
        Also refreshes the session timestamp on access.
        
        Args:
            code: The session code to look up.
            
        Returns:
            Optional[Address]: The host address if found, None otherwise.
        """
        with self._lock:
            session = self._sessions.get(code)
            if session is None:
                return None
            session.timestamp = time.time()
            return session.host

    def has_code(self, code: str) -> bool:
        """
        Check if a session code exists.
        
        Args:
            code: The session code to check.
            
        Returns:
            bool: True if the code exists, False otherwise.
        """
        with self._lock:
            return code in self._sessions

    def cleanup(self) -> int:
        """
        Remove expired sessions based on TTL.
        
        Should be called periodically to free memory from stale sessions.
        
        Returns:
            int: Number of sessions removed.
        """
        with self._lock:
            now = time.time()
            expired = [k for k, v in self._sessions.items() if now - v.timestamp > self._ttl]
            for code in expired:
                self._sessions.pop(code, None)
            return len(expired)

    def _generate_code(self) -> str:
        """
        Generate a unique 4-digit session code.
        
        Ensures the generated code doesn't already exist in the store.
        
        Returns:
            str: A unique 4-digit code as a string.
        """
        while True:
            candidate = str(random.randint(1000, 9999))
            if candidate not in self._sessions:
                return candidate


__all__ = ["SessionStore"]
