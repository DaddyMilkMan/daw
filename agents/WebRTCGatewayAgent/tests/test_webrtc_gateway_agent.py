import sys
import unittest
from unittest.mock import MagicMock, patch

# Mock aiortc before importing the module under test
# This simulates the library being installed for the purpose of the test logic
mock_aiortc = MagicMock()
sys.modules["aiortc"] = mock_aiortc

from agents.WebRTCGatewayAgent.webrtc_gateway_agent import WebRTCGatewayAgent

class TestWebRTCGatewayAgent(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.agent = WebRTCGatewayAgent()

    async def test_start_initializes_webrtc(self):
        """Test that start() initializes the WebRTC backend and ICE config."""
        await self.agent.start()

        # Verify running state
        self.assertTrue(self.agent._running)

        # Verify RTCConfiguration was created
        # We expect self.rtc_config to store the configuration
        self.assertTrue(hasattr(self.agent, "rtc_config"))
        mock_aiortc.RTCConfiguration.assert_called()

        # Verify ICE servers were processed
        # We expect RTCIceServer to be called for the default stun server
        mock_aiortc.RTCIceServer.assert_called()

        # Check args passed to RTCConfiguration
        call_args = mock_aiortc.RTCConfiguration.call_args
        self.assertIsNotNone(call_args)
        # We expect 'iceServers' in kwargs or as first arg
        if 'iceServers' in call_args.kwargs:
            self.assertTrue(len(call_args.kwargs['iceServers']) > 0)

    async def test_create_peer_connection_uses_config(self):
        """Test that create_peer_connection uses the shared ICE config."""
        await self.agent.start()

        # Reset mock to clear calls from start()
        mock_aiortc.RTCPeerConnection.reset_mock()

        # Callback mock
        callback = MagicMock()

        # Create connection
        peer_id = "test_peer"
        await self.agent.create_peer_connection(peer_id, callback)

        # Verify RTCPeerConnection initialized with config
        mock_aiortc.RTCPeerConnection.assert_called_once()
        call_args = mock_aiortc.RTCPeerConnection.call_args
        self.assertEqual(call_args.kwargs['configuration'], self.agent.rtc_config)

        # Verify peer stored
        self.assertIn(peer_id, self.agent.peers)

    async def test_create_peer_connection_when_not_running(self):
        """Test behavior when creating connection before start."""
        # Ensure not running initially
        self.assertFalse(self.agent._running)

        # Mock callback
        callback = MagicMock()

        # Create connection - should auto-start
        await self.agent.create_peer_connection("peer_auto", callback)

        # Verify it started
        self.assertTrue(self.agent._running)
        self.assertIsNotNone(self.agent.rtc_config)

        # Verify connection created
        self.assertIn("peer_auto", self.agent.peers)

if __name__ == "__main__":
    unittest.main()
