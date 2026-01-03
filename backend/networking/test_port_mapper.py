"""
Tests for UPnP Port Mapper

These tests verify the refactored port_mapper module functionality,
including error handling, retry logic, and fallback mode.
"""

import unittest
from unittest.mock import Mock, patch, MagicMock
import socket

from backend.networking.port_mapper import (
    UPnPPortMapper,
    RouterDiscoveryError,
    ControlURLError,
    PortMappingError,
    NetworkError,
    DEFAULT_PORT,
    DEFAULT_TIMEOUT,
)


class TestUPnPPortMapperConfiguration(unittest.TestCase):
    """Test configuration and initialization."""
    
    def test_default_initialization(self):
        """Test that mapper initializes with default values."""
        mapper = UPnPPortMapper()
        self.assertEqual(mapper.port, DEFAULT_PORT)
        self.assertEqual(mapper.timeout, DEFAULT_TIMEOUT)
        self.assertFalse(mapper.fallback_mode)
    
    def test_custom_initialization(self):
        """Test that mapper accepts custom configuration."""
        mapper = UPnPPortMapper(
            port=8080,
            timeout=5,
            max_retries=5,
            backoff_base=2.0,
            backoff_max=60.0
        )
        self.assertEqual(mapper.port, 8080)
        self.assertEqual(mapper.timeout, 5)
        self.assertEqual(mapper.max_retries, 5)
        self.assertEqual(mapper.backoff_base, 2.0)
        self.assertEqual(mapper.backoff_max, 60.0)


class TestBackoffCalculation(unittest.TestCase):
    """Test exponential backoff calculation."""
    
    def test_exponential_backoff(self):
        """Test that backoff increases exponentially."""
        mapper = UPnPPortMapper(backoff_base=1.0, backoff_max=30.0)
        
        # First few attempts should double
        self.assertEqual(mapper._calculate_backoff(0), 1.0)
        self.assertEqual(mapper._calculate_backoff(1), 2.0)
        self.assertEqual(mapper._calculate_backoff(2), 4.0)
        self.assertEqual(mapper._calculate_backoff(3), 8.0)
    
    def test_backoff_respects_maximum(self):
        """Test that backoff doesn't exceed maximum."""
        mapper = UPnPPortMapper(backoff_base=1.0, backoff_max=10.0)
        
        # Should cap at max
        self.assertEqual(mapper._calculate_backoff(10), 10.0)
        self.assertEqual(mapper._calculate_backoff(20), 10.0)


class TestLocalIPDetection(unittest.TestCase):
    """Test local IP address detection."""
    
    def test_get_local_ip_success(self):
        """Test successful local IP detection."""
        mapper = UPnPPortMapper()
        local_ip = mapper._get_local_ip()
        
        # Should return a valid IP address (not just fallback)
        self.assertIsInstance(local_ip, str)
        self.assertRegex(local_ip, r'\d+\.\d+\.\d+\.\d+')
    
    @patch('socket.socket')
    def test_get_local_ip_fallback(self, mock_socket):
        """Test that fallback IP is returned on error."""
        # Simulate socket error
        mock_socket.return_value.__enter__.return_value.connect.side_effect = OSError()
        
        mapper = UPnPPortMapper()
        local_ip = mapper._get_local_ip()
        
        # Should return fallback
        self.assertEqual(local_ip, "127.0.0.1")


class TestLocationExtraction(unittest.TestCase):
    """Test SSDP response parsing."""
    
    def test_extract_location_success(self):
        """Test extracting LOCATION from SSDP response."""
        mapper = UPnPPortMapper()
        ssdp_response = """HTTP/1.1 200 OK
CACHE-CONTROL: max-age=1800
LOCATION: http://192.168.1.1:5000/rootDesc.xml
SERVER: Linux/3.14, UPnP/1.0, Test Router
ST: urn:schemas-upnp-org:service:WANIPConnection:1
"""
        location = mapper._extract_location(ssdp_response)
        self.assertEqual(location, "http://192.168.1.1:5000/rootDesc.xml")
    
    def test_extract_location_missing(self):
        """Test handling of missing LOCATION header."""
        mapper = UPnPPortMapper()
        ssdp_response = """HTTP/1.1 200 OK
CACHE-CONTROL: max-age=1800
SERVER: Test Router
"""
        location = mapper._extract_location(ssdp_response)
        self.assertIsNone(location)


class TestFallbackMode(unittest.TestCase):
    """Test fallback mode activation."""
    
    def test_enable_fallback_mode(self):
        """Test that fallback mode is properly enabled."""
        mapper = UPnPPortMapper()
        self.assertFalse(mapper.fallback_mode)
        
        mapper._enable_fallback_mode("Test reason")
        self.assertTrue(mapper.fallback_mode)
    
    @patch('backend.networking.port_mapper.UPnPPortMapper._discover_router')
    def test_run_enters_fallback_on_no_router(self, mock_discover):
        """Test that run() enters fallback mode when no router is found."""
        mock_discover.return_value = (None, None)
        
        mapper = UPnPPortMapper(max_retries=0)
        result = mapper.run()
        
        self.assertFalse(result)
        self.assertTrue(mapper.fallback_mode)


class TestRetryLogic(unittest.TestCase):
    """Test retry mechanism with exponential backoff."""
    
    @patch('time.sleep')  # Mock sleep to speed up tests
    def test_retry_operation_success_on_retry(self, mock_sleep):
        """Test that operations succeed after retries."""
        mapper = UPnPPortMapper(max_retries=3)
        
        # Mock function that fails twice, then succeeds
        mock_func = Mock(side_effect=[
            RouterDiscoveryError("Attempt 1"),
            RouterDiscoveryError("Attempt 2"),
            ("data", "192.168.1.1")  # Success
        ])
        
        result = mapper._retry_operation("Test operation", mock_func)
        
        # Should succeed on third attempt
        self.assertEqual(result, ("data", "192.168.1.1"))
        self.assertEqual(mock_func.call_count, 3)
        
        # Should have slept twice (before retries)
        self.assertEqual(mock_sleep.call_count, 2)
    
    @patch('time.sleep')
    def test_retry_operation_exhausts_retries(self, mock_sleep):
        """Test that operations fail after exhausting retries."""
        mapper = UPnPPortMapper(max_retries=2)
        
        # Mock function that always fails
        mock_func = Mock(side_effect=RouterDiscoveryError("Always fails"))
        
        with self.assertRaises(RouterDiscoveryError):
            mapper._retry_operation("Test operation", mock_func)
        
        # Should try max_retries + 1 times
        self.assertEqual(mock_func.call_count, 3)


class TestCustomExceptions(unittest.TestCase):
    """Test custom exception hierarchy."""
    
    def test_exception_inheritance(self):
        """Test that custom exceptions inherit from UPnPError."""
        from backend.networking.port_mapper import UPnPError
        
        self.assertTrue(issubclass(RouterDiscoveryError, UPnPError))
        self.assertTrue(issubclass(ControlURLError, UPnPError))
        self.assertTrue(issubclass(PortMappingError, UPnPError))
        self.assertTrue(issubclass(NetworkError, UPnPError))


if __name__ == '__main__':
    unittest.main()
