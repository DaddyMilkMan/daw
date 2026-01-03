"""
Comprehensive test suite for the ServiceSentinel orchestrator.

These tests validate thread safety, error handling, restart logic,
and health checking functionality.
"""

import os
import sys
import time
import threading
import subprocess
from unittest.mock import Mock, patch, MagicMock

# Add backend to path for imports
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if root not in sys.path:
    sys.path.insert(0, root)

from backend.orchestrator import (
    ServiceSentinel,
    ServiceDefinition,
    ServiceState,
    RuntimeState,
    check_tcp_port
)


class TestServiceDefinition:
    """Test ServiceDefinition configuration."""
    
    def test_minimal_service_definition(self):
        """Test creating a service with minimal configuration."""
        svc = ServiceDefinition(
            name="test_service",
            command=["echo", "hello"]
        )
        assert svc.name == "test_service"
        assert svc.command == ["echo", "hello"]
        assert svc.env == {}
        assert svc.cwd is None
        assert svc.health_check is None
        assert svc.max_retries == -1
        assert svc.backoff_base_sec == 1.0
        assert svc.backoff_max_sec == 30.0
        assert svc.backoff_jitter == 0.1
    
    def test_full_service_definition(self):
        """Test creating a service with all configuration options."""
        health_fn = lambda: True
        svc = ServiceDefinition(
            name="full_service",
            command=["python", "-m", "http.server"],
            env={"PORT": "8000"},
            cwd="/tmp",
            health_check=health_fn,
            max_retries=5,
            backoff_base_sec=2.0,
            backoff_max_sec=60.0,
            backoff_jitter=0.2
        )
        assert svc.name == "full_service"
        assert svc.env == {"PORT": "8000"}
        assert svc.cwd == "/tmp"
        assert svc.health_check == health_fn
        assert svc.max_retries == 5
        assert svc.backoff_jitter == 0.2


class TestRuntimeState:
    """Test RuntimeState tracking."""
    
    def test_default_runtime_state(self):
        """Test default runtime state initialization."""
        rt = RuntimeState()
        assert rt.process is None
        assert rt.state == ServiceState.STOPPED
        assert rt.restart_count == 0
        assert rt.next_restart_time == 0.0
        assert rt.last_exit_code is None
        assert rt.last_health_status is False


class TestServiceSentinel:
    """Test ServiceSentinel orchestrator."""
    
    def test_sentinel_initialization(self):
        """Test creating a sentinel."""
        sentinel = ServiceSentinel(check_interval=0.5)
        assert sentinel.check_interval == 0.5
        assert len(sentinel.services) == 0
        assert len(sentinel.state) == 0
    
    def test_add_service(self):
        """Test adding a service to the sentinel."""
        sentinel = ServiceSentinel()
        service = ServiceDefinition(
            name="test_service",
            command=["echo", "test"]
        )
        sentinel.add_service(service)
        
        assert "test_service" in sentinel.services
        assert "test_service" in sentinel.state
        assert sentinel.state["test_service"].state == ServiceState.STOPPED
    
    def test_add_duplicate_service_raises(self):
        """Test that adding a duplicate service raises ValueError."""
        sentinel = ServiceSentinel()
        service = ServiceDefinition(name="test", command=["echo"])
        sentinel.add_service(service)
        
        try:
            sentinel.add_service(service)
            assert False, "Should have raised ValueError"
        except ValueError as e:
            assert "already registered" in str(e)
    
    def test_start_all_requires_services(self):
        """Test that start_all raises if no services registered."""
        sentinel = ServiceSentinel()
        try:
            sentinel.start_all()
            assert False, "Should have raised RuntimeError"
        except RuntimeError as e:
            assert "No services registered" in str(e)
    
    def test_get_status(self):
        """Test getting service status."""
        sentinel = ServiceSentinel()
        service = ServiceDefinition(
            name="status_test",
            command=["echo", "test"]
        )
        sentinel.add_service(service)
        
        status = sentinel.get_status()
        assert "status_test" in status
        assert status["status_test"]["state"] == "STOPPED"
        assert status["status_test"]["pid"] is None
        assert status["status_test"]["restarts"] == 0
        assert status["status_test"]["healthy"] is False
    
    def test_spawn_service_success(self):
        """Test successfully spawning a service."""
        sentinel = ServiceSentinel()
        service = ServiceDefinition(
            name="spawn_test",
            command=["sleep", "0.1"]
        )
        sentinel.add_service(service)
        
        with sentinel._acquire_lock():
            sentinel._spawn_service("spawn_test")
        
        rt = sentinel.state["spawn_test"]
        assert rt.state == ServiceState.RUNNING
        assert rt.process is not None
        assert rt.process.pid > 0
        
        # Clean up
        if rt.process and rt.process.poll() is None:
            rt.process.terminate()
            rt.process.wait()
    
    def test_spawn_service_command_not_found(self):
        """Test spawning with non-existent command."""
        sentinel = ServiceSentinel()
        service = ServiceDefinition(
            name="bad_command",
            command=["this_command_does_not_exist_xyz"]
        )
        sentinel.add_service(service)
        
        with sentinel._acquire_lock():
            sentinel._spawn_service("bad_command")
        
        rt = sentinel.state["bad_command"]
        assert rt.state == ServiceState.BACKOFF
        assert rt.last_exit_code == -1
        assert rt.restart_count == 1
    
    def test_stop_service(self):
        """Test stopping a running service."""
        sentinel = ServiceSentinel()
        service = ServiceDefinition(
            name="stop_test",
            command=["sleep", "60"]
        )
        sentinel.add_service(service)
        
        with sentinel._acquire_lock():
            sentinel._spawn_service("stop_test")
        
        rt = sentinel.state["stop_test"]
        assert rt.state == ServiceState.RUNNING
        pid = rt.process.pid
        
        # Stop the service
        sentinel._stop_service("stop_test")
        
        assert rt.state == ServiceState.STOPPED
        assert rt.process is None
        
        # Verify process is actually dead
        time.sleep(0.1)
        try:
            os.kill(pid, 0)
            assert False, "Process should be dead"
        except OSError:
            pass  # Process is dead, as expected
    
    def test_schedule_backoff(self):
        """Test backoff scheduling with exponential delay."""
        sentinel = ServiceSentinel()
        service = ServiceDefinition(
            name="backoff_test",
            command=["echo"],
            backoff_base_sec=1.0,
            backoff_max_sec=10.0,
            backoff_jitter=0.0  # No jitter for predictable testing
        )
        sentinel.add_service(service)
        
        with sentinel._acquire_lock():
            rt = sentinel.state["backoff_test"]
            
            # First backoff: 1 * 2^0 = 1 second
            sentinel._schedule_backoff("backoff_test")
            assert rt.state == ServiceState.BACKOFF
            assert rt.restart_count == 1
            delay1 = rt.next_restart_time - time.time()
            assert 0.9 <= delay1 <= 1.1
            
            # Second backoff: 1 * 2^1 = 2 seconds
            sentinel._schedule_backoff("backoff_test")
            assert rt.restart_count == 2
            delay2 = rt.next_restart_time - time.time()
            assert 1.9 <= delay2 <= 2.1
            
            # Third backoff: 1 * 2^2 = 4 seconds
            sentinel._schedule_backoff("backoff_test")
            assert rt.restart_count == 3
            delay3 = rt.next_restart_time - time.time()
            assert 3.9 <= delay3 <= 4.1
    
    def test_max_retries_enforced(self):
        """Test that max_retries is enforced."""
        sentinel = ServiceSentinel()
        service = ServiceDefinition(
            name="retry_test",
            command=["false"],  # Always fails
            max_retries=3,
            backoff_base_sec=0.1
        )
        sentinel.add_service(service)
        
        with sentinel._acquire_lock():
            rt = sentinel.state["retry_test"]
            
            # Simulate 3 failures
            for i in range(3):
                sentinel._spawn_service("retry_test")
                assert rt.state == ServiceState.BACKOFF
            
            # 4th attempt should fail permanently
            sentinel._spawn_service("retry_test")
            assert rt.state == ServiceState.FAILED
            assert rt.restart_count == 3
    
    def test_backoff_with_jitter(self):
        """Test that jitter is applied to backoff delays."""
        sentinel = ServiceSentinel()
        service = ServiceDefinition(
            name="jitter_test",
            command=["echo"],
            backoff_base_sec=10.0,
            backoff_jitter=0.5  # 50% jitter
        )
        sentinel.add_service(service)
        
        delays = []
        with sentinel._acquire_lock():
            for _ in range(5):
                sentinel.state["jitter_test"].restart_count = 0
                sentinel._schedule_backoff("jitter_test")
                delay = sentinel.state["jitter_test"].next_restart_time - time.time()
                delays.append(delay)
        
        # With jitter, delays should vary
        assert len(set(delays)) > 1, "Jitter should produce different delays"
        # All delays should be between base and base*(1+jitter)
        for delay in delays:
            assert 10.0 <= delay <= 15.0
    
    def test_health_check_integration(self):
        """Test health check integration in service lifecycle."""
        health_status = {"healthy": True}
        
        def health_check():
            return health_status["healthy"]
        
        sentinel = ServiceSentinel()
        service = ServiceDefinition(
            name="health_test",
            command=["sleep", "10"],
            health_check=health_check
        )
        sentinel.add_service(service)
        
        with sentinel._acquire_lock():
            sentinel._spawn_service("health_test")
            rt = sentinel.state["health_test"]
            assert rt.last_health_status is True
        
        # Clean up
        sentinel._stop_service("health_test")
    
    def test_thread_safety(self):
        """Test concurrent access to sentinel."""
        sentinel = ServiceSentinel()
        service = ServiceDefinition(
            name="thread_test",
            command=["sleep", "0.1"]
        )
        sentinel.add_service(service)
        
        errors = []
        
        def get_status_repeatedly():
            try:
                for _ in range(50):
                    sentinel.get_status()
                    time.sleep(0.001)
            except Exception as e:
                errors.append(e)
        
        def add_service_repeatedly():
            try:
                for i in range(50):
                    try:
                        sentinel.add_service(ServiceDefinition(
                            name=f"service_{i}",
                            command=["echo"]
                        ))
                    except ValueError:
                        pass  # Duplicate is OK
                    time.sleep(0.001)
            except Exception as e:
                errors.append(e)
        
        threads = [
            threading.Thread(target=get_status_repeatedly),
            threading.Thread(target=get_status_repeatedly),
            threading.Thread(target=add_service_repeatedly),
        ]
        
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        
        assert len(errors) == 0, f"Thread safety errors: {errors}"


class TestCheckTcpPort:
    """Test TCP port health check helper."""
    
    def test_check_nonexistent_port(self):
        """Test checking a port that's not listening."""
        # Port 54321 is unlikely to be in use
        result = check_tcp_port("127.0.0.1", 54321, timeout=0.1)
        assert result is False
    
    def test_check_invalid_host(self):
        """Test checking an invalid hostname."""
        result = check_tcp_port("invalid.host.local", 80, timeout=0.1)
        assert result is False
    
    @patch('socket.create_connection')
    def test_check_successful_connection(self, mock_connect):
        """Test successful connection."""
        mock_socket = MagicMock()
        mock_connect.return_value.__enter__ = Mock(return_value=mock_socket)
        mock_connect.return_value.__exit__ = Mock(return_value=False)
        
        result = check_tcp_port("localhost", 8080)
        assert result is True
        mock_connect.assert_called_once_with(("localhost", 8080), timeout=1.0)
    
    @patch('socket.create_connection')
    def test_check_timeout(self, mock_connect):
        """Test connection timeout."""
        import socket as sock_module
        mock_connect.side_effect = sock_module.timeout()
        
        result = check_tcp_port("slow.host.local", 80, timeout=0.1)
        assert result is False


class TestEndToEndScenarios:
    """End-to-end integration tests."""
    
    def test_service_lifecycle(self):
        """Test complete service lifecycle."""
        sentinel = ServiceSentinel(check_interval=0.1)
        
        # Add a service that runs briefly
        service = ServiceDefinition(
            name="lifecycle_test",
            command=["sleep", "0.2"]
        )
        sentinel.add_service(service)
        
        # Start services
        sentinel.start_all()
        
        # Verify it's running
        time.sleep(0.05)
        status = sentinel.get_status()
        assert status["lifecycle_test"]["state"] == "RUNNING"
        
        # Wait for it to complete
        time.sleep(0.3)
        
        # Should be in backoff after exit
        status = sentinel.get_status()
        assert status["lifecycle_test"]["state"] in ["BACKOFF", "RUNNING"]
        
        # Stop everything
        sentinel.stop_all()
        
        status = sentinel.get_status()
        assert status["lifecycle_test"]["state"] == "STOPPED"
    
    def test_auto_restart_on_crash(self):
        """Test that services are auto-restarted on crash."""
        sentinel = ServiceSentinel(check_interval=0.1)
        
        # Service that exits immediately
        service = ServiceDefinition(
            name="crash_test",
            command=["false"],  # Exits with code 1
            backoff_base_sec=0.2,
            max_retries=2
        )
        sentinel.add_service(service)
        
        sentinel.start_all()
        
        # Wait for initial crash and backoff
        time.sleep(0.15)
        status = sentinel.get_status()
        assert status["crash_test"]["state"] in ["BACKOFF", "FAILED"]
        assert status["crash_test"]["restarts"] >= 1
        
        sentinel.stop_all()


if __name__ == "__main__":
    # Basic smoke test
    print("Running basic smoke tests...")
    
    # Test ServiceDefinition
    print("✓ ServiceDefinition")
    svc = ServiceDefinition(name="test", command=["echo", "hello"])
    assert svc.name == "test"
    
    # Test ServiceSentinel
    print("✓ ServiceSentinel initialization")
    sentinel = ServiceSentinel()
    sentinel.add_service(svc)
    assert "test" in sentinel.services
    
    # Test check_tcp_port
    print("✓ check_tcp_port")
    result = check_tcp_port("127.0.0.1", 54321, timeout=0.1)
    assert result is False
    
    print("\nAll smoke tests passed! Run with pytest for full test suite.")
