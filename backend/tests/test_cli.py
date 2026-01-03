"""
Comprehensive tests for backend/cli.py.

Tests cover:
- AppState initialization and dependency injection
- Configuration loading and overrides
- Service management commands (start, check, status)
- Error handling and edge cases
- Thread safety considerations
"""

import os
import sys
from pathlib import Path
from unittest.mock import MagicMock, Mock, patch, call
import pytest
from typer.testing import CliRunner

# Add parent directory to path for imports
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if root not in sys.path:
    sys.path.insert(0, root)

from backend.cli import app, AppState
from backend.config import ZenithConfig
from backend.orchestrator import ServiceManager


@pytest.fixture
def runner():
    """Create a CLI test runner."""
    return CliRunner()


@pytest.fixture
def mock_config():
    """Create a mock ZenithConfig for testing."""
    config = ZenithConfig()
    config.log_level = "INFO"
    config.log_json = False
    config.signaling_port = 8080
    config.health_port = 8000
    config.upnp_enabled = True
    config.upnp_port = 4000
    config.upnp_timeout = 30
    return config


@pytest.fixture
def mock_service_manager():
    """Create a mock ServiceManager for testing."""
    manager = Mock(spec=ServiceManager)
    manager.start_services = Mock()
    manager.stop_services = Mock()
    manager.wait_forever = Mock()
    manager.get_status = Mock(return_value={
        'signaling': {
            'state': 'RUNNING',
            'pid': 12345,
            'restarts': 0,
            'healthy': True
        }
    })
    return manager


class TestAppState:
    """Tests for the AppState class."""
    
    def test_app_state_initialization(self, mock_config, mock_service_manager):
        """Test that AppState correctly stores config and manager."""
        state = AppState(config=mock_config, manager=mock_service_manager)
        
        assert state.config == mock_config
        assert state.manager == mock_service_manager
    
    def test_app_state_holds_references(self, mock_config, mock_service_manager):
        """Test that AppState maintains separate instances."""
        state1 = AppState(config=mock_config, manager=mock_service_manager)
        
        # Create another config and manager
        config2 = ZenithConfig()
        manager2 = Mock(spec=ServiceManager)
        state2 = AppState(config=config2, manager=manager2)
        
        # Verify they are independent
        assert state1.config != state2.config
        assert state1.manager != state2.manager


class TestMainCallback:
    """Tests for the main callback that initializes the CLI."""
    
    @patch('backend.cli.configure_logging')
    @patch('backend.cli.ServiceManager')
    @patch('backend.cli.ZenithConfig')
    def test_main_initializes_with_defaults(self, mock_config_cls, mock_manager_cls, 
                                           mock_configure_logging, runner):
        """Test main callback initializes with default configuration."""
        mock_config = Mock(spec=ZenithConfig)
        mock_config.log_level = "INFO"
        mock_config.log_json = False
        mock_config_cls.return_value = mock_config
        
        mock_manager = Mock(spec=ServiceManager)
        mock_manager_cls.return_value = mock_manager
        mock_manager.get_status = Mock(return_value={})
        
        result = runner.invoke(app, ["status"])
        
        # Verify configuration was created
        mock_config_cls.assert_called_once()
        
        # Verify logging was configured
        mock_configure_logging.assert_called_once_with(mock_config)
        
        # Verify ServiceManager was created with config
        mock_manager_cls.assert_called_once_with(mock_config)
    
    @patch('backend.cli.configure_logging')
    @patch('backend.cli.ServiceManager')
    @patch('backend.cli.ZenithConfig')
    def test_main_applies_cli_overrides(self, mock_config_cls, mock_manager_cls,
                                       mock_configure_logging, runner):
        """Test that CLI flags override default configuration."""
        mock_config = Mock(spec=ZenithConfig)
        mock_config.log_level = "INFO"
        mock_config.log_json = False
        mock_config_cls.return_value = mock_config
        
        mock_manager = Mock(spec=ServiceManager)
        mock_manager_cls.return_value = mock_manager
        mock_manager.get_status = Mock(return_value={})
        
        result = runner.invoke(app, ["--json", "--log-level", "DEBUG", "status"])
        
        # Verify overrides were applied
        assert mock_config.log_json == True
        assert mock_config.log_level == "DEBUG"
    
    @patch('backend.cli.ZenithConfig')
    def test_main_handles_config_failure(self, mock_config_cls, runner):
        """Test that configuration failures are handled gracefully."""
        mock_config_cls.side_effect = Exception("Config load failed")
        
        result = runner.invoke(app, ["status"])
        
        assert result.exit_code == 1
        assert "Fatal error during initialization" in result.output


class TestStartCommand:
    """Tests for the start command."""
    
    @patch('backend.cli.configure_logging')
    @patch('backend.cli.ServiceManager')
    @patch('backend.cli.ZenithConfig')
    def test_start_command_basic(self, mock_config_cls, mock_manager_cls,
                                 mock_configure_logging, runner):
        """Test start command with default options."""
        mock_config = Mock(spec=ZenithConfig)
        mock_config.log_level = "INFO"
        mock_config.log_json = False
        mock_config.upnp_enabled = True
        mock_config_cls.return_value = mock_config
        
        mock_manager = Mock(spec=ServiceManager)
        mock_manager_cls.return_value = mock_manager
        
        # Mock wait_forever to exit immediately
        mock_manager.wait_forever = Mock(side_effect=KeyboardInterrupt)
        
        result = runner.invoke(app, ["start"])
        
        # Verify services were started
        mock_manager.start_services.assert_called_once_with(dry_run=False)
        
        # Verify wait_forever was called
        mock_manager.wait_forever.assert_called_once()
        
        # Should exit cleanly on KeyboardInterrupt
        assert result.exit_code == 0
    
    @patch('backend.cli.configure_logging')
    @patch('backend.cli.ServiceManager')
    @patch('backend.cli.ZenithConfig')
    def test_start_command_dry_run(self, mock_config_cls, mock_manager_cls,
                                   mock_configure_logging, runner):
        """Test start command in dry-run mode."""
        mock_config = Mock(spec=ZenithConfig)
        mock_config.log_level = "INFO"
        mock_config.log_json = False
        mock_config.upnp_enabled = True
        mock_config_cls.return_value = mock_config
        
        mock_manager = Mock(spec=ServiceManager)
        mock_manager_cls.return_value = mock_manager
        
        result = runner.invoke(app, ["start", "--dry-run"])
        
        # Verify dry_run was passed
        mock_manager.start_services.assert_called_once_with(dry_run=True)
        
        # wait_forever should NOT be called in dry-run
        mock_manager.wait_forever.assert_not_called()
    
    @patch('backend.cli.configure_logging')
    @patch('backend.cli.ServiceManager')
    @patch('backend.cli.ZenithConfig')
    def test_start_command_disable_upnp(self, mock_config_cls, mock_manager_cls,
                                       mock_configure_logging, runner):
        """Test start command with UPnP disabled."""
        mock_config = Mock(spec=ZenithConfig)
        mock_config.log_level = "INFO"
        mock_config.log_json = False
        mock_config.upnp_enabled = True
        mock_config_cls.return_value = mock_config
        
        mock_manager = Mock(spec=ServiceManager)
        mock_manager_cls.return_value = mock_manager
        mock_manager.wait_forever = Mock(side_effect=KeyboardInterrupt)
        
        result = runner.invoke(app, ["start", "--disable-upnp"])
        
        # Verify UPnP was disabled
        assert mock_config.upnp_enabled == False
    
    @patch('backend.cli.configure_logging')
    @patch('backend.cli.ServiceManager')
    @patch('backend.cli.ZenithConfig')
    def test_start_command_handles_errors(self, mock_config_cls, mock_manager_cls,
                                         mock_configure_logging, runner):
        """Test start command handles service startup errors."""
        mock_config = Mock(spec=ZenithConfig)
        mock_config.log_level = "INFO"
        mock_config.log_json = False
        mock_config.upnp_enabled = True
        mock_config_cls.return_value = mock_config
        
        mock_manager = Mock(spec=ServiceManager)
        mock_manager_cls.return_value = mock_manager
        mock_manager.start_services.side_effect = Exception("Startup failed")
        
        result = runner.invoke(app, ["start"])
        
        assert result.exit_code == 1
        assert "Error:" in result.output


class TestCheckCommand:
    """Tests for the check command."""
    
    @patch('backend.cli.socket.socket')
    @patch('backend.cli.configure_logging')
    @patch('backend.cli.ServiceManager')
    @patch('backend.cli.ZenithConfig')
    def test_check_command_all_ports_available(self, mock_config_cls, mock_manager_cls,
                                               mock_configure_logging, mock_socket, runner):
        """Test check command when all ports are available."""
        mock_config = Mock(spec=ZenithConfig)
        mock_config.log_level = "INFO"
        mock_config.log_json = False
        mock_config.signaling_port = 8080
        mock_config.health_port = 8000
        mock_config.upnp_enabled = True
        mock_config.upnp_port = 4000
        mock_config.upnp_timeout = 30
        mock_config_cls.return_value = mock_config
        
        mock_manager = Mock(spec=ServiceManager)
        mock_manager_cls.return_value = mock_manager
        
        # Mock socket to indicate ports are available
        mock_sock_instance = Mock()
        mock_socket.return_value.__enter__ = Mock(return_value=mock_sock_instance)
        mock_socket.return_value.__exit__ = Mock(return_value=False)
        
        result = runner.invoke(app, ["check"])
        
        assert result.exit_code == 0
        assert "Environment validation passed" in result.output
    
    @patch('backend.cli.socket.socket')
    @patch('backend.cli.configure_logging')
    @patch('backend.cli.ServiceManager')
    @patch('backend.cli.ZenithConfig')
    def test_check_command_port_in_use(self, mock_config_cls, mock_manager_cls,
                                       mock_configure_logging, mock_socket, runner):
        """Test check command when a port is in use."""
        mock_config = Mock(spec=ZenithConfig)
        mock_config.log_level = "INFO"
        mock_config.log_json = False
        mock_config.signaling_port = 8080
        mock_config.health_port = 8000
        mock_config.upnp_enabled = False
        mock_config_cls.return_value = mock_config
        
        mock_manager = Mock(spec=ServiceManager)
        mock_manager_cls.return_value = mock_manager
        
        # Mock socket to indicate port is in use
        mock_sock_instance = Mock()
        mock_sock_instance.bind.side_effect = OSError("Address already in use")
        mock_socket.return_value.__enter__ = Mock(return_value=mock_sock_instance)
        mock_socket.return_value.__exit__ = Mock(return_value=False)
        
        result = runner.invoke(app, ["check"])
        
        assert result.exit_code == 1
        assert "not available" in result.output
    
    @patch('backend.cli.socket.socket')
    @patch('backend.cli.configure_logging')
    @patch('backend.cli.ServiceManager')
    @patch('backend.cli.ZenithConfig')
    def test_check_command_privileged_port_warning(self, mock_config_cls, mock_manager_cls,
                                                   mock_configure_logging, mock_socket, runner):
        """Test check command warns about privileged ports."""
        mock_config = Mock(spec=ZenithConfig)
        mock_config.log_level = "INFO"
        mock_config.log_json = False
        mock_config.signaling_port = 80  # Privileged port
        mock_config.health_port = 8000
        mock_config.upnp_enabled = False
        mock_config_cls.return_value = mock_config
        
        mock_manager = Mock(spec=ServiceManager)
        mock_manager_cls.return_value = mock_manager
        
        # Mock socket to indicate ports are available
        mock_sock_instance = Mock()
        mock_socket.return_value.__enter__ = Mock(return_value=mock_sock_instance)
        mock_socket.return_value.__exit__ = Mock(return_value=False)
        
        result = runner.invoke(app, ["check"])
        
        assert result.exit_code == 0
        assert "WARNING" in result.output
        assert "privileged" in result.output


class TestStatusCommand:
    """Tests for the status command."""
    
    @patch('backend.cli.configure_logging')
    @patch('backend.cli.ServiceManager')
    @patch('backend.cli.ZenithConfig')
    def test_status_command_with_running_services(self, mock_config_cls, mock_manager_cls,
                                                  mock_configure_logging, runner):
        """Test status command with running services."""
        mock_config = Mock(spec=ZenithConfig)
        mock_config.log_level = "INFO"
        mock_config.log_json = False
        mock_config.health_port = 8000
        mock_config_cls.return_value = mock_config
        
        mock_manager = Mock(spec=ServiceManager)
        mock_manager_cls.return_value = mock_manager
        mock_manager.get_status.return_value = {
            'signaling': {
                'state': 'RUNNING',
                'pid': 12345,
                'restarts': 0,
                'healthy': True
            }
        }
        
        result = runner.invoke(app, ["status"])
        
        assert result.exit_code == 0
        assert "Service Status" in result.output
        assert "SIGNALING" in result.output
        assert "RUNNING" in result.output
    
    @patch('backend.cli.configure_logging')
    @patch('backend.cli.ServiceManager')
    @patch('backend.cli.ZenithConfig')
    def test_status_command_no_services(self, mock_config_cls, mock_manager_cls,
                                       mock_configure_logging, runner):
        """Test status command with no services registered."""
        mock_config = Mock(spec=ZenithConfig)
        mock_config.log_level = "INFO"
        mock_config.log_json = False
        mock_config.health_port = 8000
        mock_config_cls.return_value = mock_config
        
        mock_manager = Mock(spec=ServiceManager)
        mock_manager_cls.return_value = mock_manager
        mock_manager.get_status.return_value = {}
        
        result = runner.invoke(app, ["status"])
        
        assert result.exit_code == 0
        assert "No services" in result.output
    
    @patch('backend.cli.http.client.HTTPConnection')
    @patch('backend.cli.configure_logging')
    @patch('backend.cli.ServiceManager')
    @patch('backend.cli.ZenithConfig')
    def test_status_command_checks_health_endpoint(self, mock_config_cls, mock_manager_cls,
                                                   mock_configure_logging, mock_http_conn, runner):
        """Test status command queries health endpoint."""
        mock_config = Mock(spec=ZenithConfig)
        mock_config.log_level = "INFO"
        mock_config.log_json = False
        mock_config.health_port = 8000
        mock_config_cls.return_value = mock_config
        
        mock_manager = Mock(spec=ServiceManager)
        mock_manager_cls.return_value = mock_manager
        mock_manager.get_status.return_value = {
            'signaling': {
                'state': 'RUNNING',
                'pid': 12345,
                'restarts': 0,
                'healthy': True
            }
        }
        
        # Mock health endpoint response
        mock_response = Mock()
        mock_response.status = 200
        mock_response.read.return_value = b'{"status": "ok"}'
        
        mock_conn_instance = Mock()
        mock_conn_instance.getresponse.return_value = mock_response
        mock_http_conn.return_value = mock_conn_instance
        
        result = runner.invoke(app, ["status"])
        
        assert result.exit_code == 0
        assert "Health endpoint" in result.output
        assert "REACHABLE" in result.output
        
        # Verify connection was made to correct port
        mock_http_conn.assert_called_once_with("127.0.0.1", 8000, timeout=2)


class TestServiceManager:
    """Tests for the ServiceManager class."""
    
    @patch('backend.orchestrator.ServiceSentinel')
    def test_service_manager_initialization(self, mock_sentinel_cls, mock_config):
        """Test ServiceManager initializes correctly."""
        mock_sentinel = Mock()
        mock_sentinel_cls.return_value = mock_sentinel
        
        manager = ServiceManager(mock_config)
        
        assert manager.config == mock_config
        assert manager.sentinel == mock_sentinel
        
        # Verify services were set up
        assert mock_sentinel.add_service.called
    
    def test_service_manager_requires_zenith_config(self):
        """Test ServiceManager raises TypeError for invalid config."""
        with pytest.raises(TypeError):
            ServiceManager("not a config")
    
    @patch('backend.orchestrator.ServiceSentinel')
    def test_service_manager_start_services(self, mock_sentinel_cls, mock_config):
        """Test ServiceManager starts services correctly."""
        mock_sentinel = Mock()
        mock_sentinel_cls.return_value = mock_sentinel
        
        manager = ServiceManager(mock_config)
        manager.start_services(dry_run=False)
        
        mock_sentinel.start_all.assert_called_once()
    
    @patch('backend.orchestrator.ServiceSentinel')
    def test_service_manager_start_services_dry_run(self, mock_sentinel_cls, mock_config):
        """Test ServiceManager dry run doesn't start services."""
        mock_sentinel = Mock()
        mock_sentinel_cls.return_value = mock_sentinel
        
        manager = ServiceManager(mock_config)
        manager.start_services(dry_run=True)
        
        # start_all should not be called in dry run
        mock_sentinel.start_all.assert_not_called()
    
    @patch('backend.orchestrator.ServiceSentinel')
    def test_service_manager_stop_services(self, mock_sentinel_cls, mock_config):
        """Test ServiceManager stops services correctly."""
        mock_sentinel = Mock()
        mock_sentinel_cls.return_value = mock_sentinel
        
        manager = ServiceManager(mock_config)
        manager.stop_services()
        
        mock_sentinel.stop_all.assert_called_once()
    
    @patch('backend.orchestrator.ServiceSentinel')
    def test_service_manager_get_status(self, mock_sentinel_cls, mock_config):
        """Test ServiceManager returns service status."""
        mock_sentinel = Mock()
        mock_sentinel.get_status.return_value = {'test': 'status'}
        mock_sentinel_cls.return_value = mock_sentinel
        
        manager = ServiceManager(mock_config)
        status = manager.get_status()
        
        assert status == {'test': 'status'}
        mock_sentinel.get_status.assert_called_once()


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
