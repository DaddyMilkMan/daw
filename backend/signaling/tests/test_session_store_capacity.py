
import os
import sys
import unittest
import time
from backend.signaling.session_store import SessionStore

class TestSessionStoreCapacity(unittest.TestCase):
    def test_store_capacity_exception(self):
        """Verify that RuntimeError is raised when store is full."""
        # Create a small pool manually for testing if possible,
        # but since we can't easily mock init without patching, we fill it up.
        # Actually, 9000 is small enough to fill quickly.
        store = SessionStore(ttl=999)

        # Fill it
        codes = []
        for _ in range(9000):
            codes.append(store.create_session())

        self.assertEqual(len(store._sessions), 9000)
        self.assertEqual(len(store._available_codes), 0)

        # Next one should raise
        with self.assertRaises(RuntimeError) as cm:
            store.create_session()
        self.assertEqual(str(cm.exception), "Session store full")

    def test_recycling(self):
        """Verify that expired codes are returned to the pool."""
        store = SessionStore(ttl=0.1) # Short TTL
        code = store.create_session()
        self.assertFalse(code in store._available_codes)

        time.sleep(0.2)
        store.cleanup()

        self.assertTrue(code in store._available_codes)
        self.assertFalse(store.has_code(code))

if __name__ == "__main__":
    unittest.main()
