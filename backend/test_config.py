"""Tests for backend configuration module.

This test suite validates the configuration system including:
- Default value loading
- Environment variable parsing
- Input validation (ports, timeouts, hosts)
- Port conflict detection
- Type safety and coercion
"""

import os
import tempfile
from pathlib import Path

import pytest

from backend.config import ZenithConfig


class TestZenithConfigDefaults:
    """Test default configuration values."""
    
    def test_default_values(self):
        """Test all default configuration values are correct."""
        config = ZenithConfig()
        
        # Logging defaults
        assert config.log_level == "INFO"
        assert config.log_file is None
        assert config.log_json is False
        
        # Networking defaults
        assert config.signaling_host == "0.0.0.0"
        assert config.signaling_port == 8080
        assert config.health_port == 8000
        
        # UPnP defaults
        assert config.upnp_enabled is True
        assert config.upnp_port == 4000
        assert config.upnp_timeout == 30


class TestLogLevelValidation:
    """Test log level validation."""
    
    def test_valid_log_levels(self):
        """Test that valid log levels are accepted."""
        valid_levels = ["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"]
        for level in valid_levels:
            config = ZenithConfig(log_level=level)
            assert config.log_level == level.upper()
    
    def test_lowercase_log_level(self):
        """Test that lowercase log levels are normalized to uppercase."""
        config = ZenithConfig(log_level="debug")
        assert config.log_level == "DEBUG"
    
    def test_invalid_log_level(self):
        """Test that invalid log levels raise ValueError."""
        with pytest.raises(ValueError, match="log_level must be one of"):
            ZenithConfig(log_level="INVALID")


class TestPortValidation:
    """Test port number validation."""
    
    def test_valid_ports(self):
        """Test that valid port numbers are accepted."""
        config = ZenithConfig(
            signaling_port=8080,
            health_port=8000,
            upnp_port=4000
        )
        assert config.signaling_port == 8080
        assert config.health_port == 8000
        assert config.upnp_port == 4000
    
    def test_port_minimum_1024(self):
        """Test that ports below 1024 are rejected (privileged ports)."""
        with pytest.raises(ValueError):
            ZenithConfig(signaling_port=80)
        
        with pytest.raises(ValueError):
            ZenithConfig(health_port=443)
        
        with pytest.raises(ValueError):
            ZenithConfig(upnp_port=22)
    
    def test_port_maximum_65535(self):
        """Test that ports above 65535 are rejected."""
        with pytest.raises(ValueError):
            ZenithConfig(signaling_port=70000)
        
        with pytest.raises(ValueError):
            ZenithConfig(health_port=99999)
    
    def test_edge_case_ports(self):
        """Test edge case port values."""
        # Minimum valid port (1024)
        config = ZenithConfig(signaling_port=1024)
        assert config.signaling_port == 1024
        
        # Maximum valid port (65535)
        config = ZenithConfig(health_port=65535)
        assert config.health_port == 65535


class TestPortConflictValidation:
    """Test port conflict detection."""
    
    def test_no_port_conflicts(self):
        """Test that different ports are accepted."""
        config = ZenithConfig(
            signaling_port=8080,
            health_port=8000,
            upnp_port=4000
        )
        assert config.signaling_port != config.health_port
        assert config.signaling_port != config.upnp_port
        assert config.health_port != config.upnp_port
    
    def test_signaling_health_port_conflict(self):
        """Test that signaling and health ports must differ."""
        with pytest.raises(ValueError, match="Port conflict detected"):
            ZenithConfig(signaling_port=8080, health_port=8080)
    
    def test_signaling_upnp_port_conflict(self):
        """Test that signaling and UPnP ports must differ."""
        with pytest.raises(ValueError, match="Port conflict detected"):
            ZenithConfig(signaling_port=8080, upnp_port=8080, upnp_enabled=True)
    
    def test_upnp_disabled_no_conflict_check(self):
        """Test that UPnP port is not checked when UPnP is disabled."""
        # This should work because upnp_enabled=False
        config = ZenithConfig(
            signaling_port=8080,
            upnp_port=8080,  # Same as signaling but UPnP disabled
            upnp_enabled=False
        )
        assert config.signaling_port == 8080
        assert config.upnp_port == 8080


class TestHostValidation:
    """Test host binding address validation."""
    
    def test_valid_hosts(self):
        """Test that valid host addresses are accepted."""
        valid_hosts = ["0.0.0.0", "127.0.0.1", "localhost", "192.168.1.100"]
        for host in valid_hosts:
            config = ZenithConfig(signaling_host=host)
            assert config.signaling_host == host
    
    def test_valid_ipv6_hosts(self):
        """Test that valid IPv6 addresses are accepted."""
        valid_ipv6 = ["::1", "::", "::ffff:192.0.2.1", "2001:db8::1"]
        for host in valid_ipv6:
            config = ZenithConfig(signaling_host=host)
            assert config.signaling_host == host
    
    def test_invalid_host_rejected(self):
        """Test that invalid host addresses are rejected."""
        with pytest.raises(ValueError, match="valid IP address"):
            ZenithConfig(signaling_host="invalid.host.name")
    
    def test_empty_host_rejected(self):
        """Test that empty host string is rejected."""
        with pytest.raises(ValueError, match="signaling_host must be a non-empty string"):
            ZenithConfig(signaling_host="")


class TestTimeoutValidation:
    """Test timeout value validation."""
    
    def test_valid_timeouts(self):
        """Test that valid timeout values are accepted."""
        config = ZenithConfig(upnp_timeout=30)
        assert config.upnp_timeout == 30
    
    def test_timeout_minimum(self):
        """Test that timeout below 1 second is rejected."""
        with pytest.raises(ValueError):
            ZenithConfig(upnp_timeout=0)
    
    def test_timeout_maximum(self):
        """Test that timeout above 300 seconds is rejected."""
        with pytest.raises(ValueError):
            ZenithConfig(upnp_timeout=301)
    
    def test_edge_case_timeouts(self):
        """Test edge case timeout values."""
        # Minimum (1 second)
        config = ZenithConfig(upnp_timeout=1)
        assert config.upnp_timeout == 1
        
        # Maximum (300 seconds)
        config = ZenithConfig(upnp_timeout=300)
        assert config.upnp_timeout == 300


class TestLogFileValidation:
    """Test log file path validation."""
    
    def test_none_log_file(self):
        """Test that None log file is accepted (stdout only)."""
        config = ZenithConfig(log_file=None)
        assert config.log_file is None
    
    def test_valid_log_file_path(self):
        """Test that valid log file path is accepted."""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.log"
            config = ZenithConfig(log_file=log_path)
            assert config.log_file == log_path
    
    def test_relative_log_file_path(self):
        """Test that relative paths are converted to absolute."""
        config = ZenithConfig(log_file=Path("logs/test.log"))
        assert config.log_file.is_absolute()
    
    def test_invalid_parent_directory(self):
        """Test that log file with non-directory parent is rejected."""
        with tempfile.NamedTemporaryFile(delete=False) as tmpfile:
            # Try to use a file as parent directory (invalid)
            invalid_path = Path(tmpfile.name) / "test.log"
            try:
                with pytest.raises(ValueError, match="is not a directory"):
                    ZenithConfig(log_file=invalid_path)
            finally:
                os.unlink(tmpfile.name)


class TestServiceEnvironment:
    """Test service environment variable generation."""
    
    def test_get_service_env(self):
        """Test that service environment variables are correctly generated."""
        config = ZenithConfig(
            signaling_host="192.168.1.100",
            signaling_port=9000,
            log_level="DEBUG"
        )
        env = config.get_service_env()
        
        assert env["ZENITH_SIGNALING_HOST"] == "192.168.1.100"
        assert env["ZENITH_SIGNALING_PORT"] == "9000"
        assert env["ZENITH_LOG_LEVEL"] == "DEBUG"
    
    def test_service_env_values_are_strings(self):
        """Test that all environment values are strings."""
        config = ZenithConfig()
        env = config.get_service_env()
        
        for key, value in env.items():
            assert isinstance(value, str), f"{key} value must be string, got {type(value)}"


class TestValidateConfig:
    """Test the validate_config runtime method."""
    
    def test_validate_config_succeeds(self):
        """Test that validate_config returns True for valid config."""
        config = ZenithConfig()
        assert config.validate_config() is True
    
    def test_validate_config_with_writable_log_file(self):
        """Test validate_config with a writable log file."""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "test.log"
            config = ZenithConfig(log_file=log_path)
            assert config.validate_config() is True
            # Check that log file can be written
            assert log_path.exists()
    
    def test_validate_config_creates_log_directory(self):
        """Test that validate_config creates parent directories if needed."""
        with tempfile.TemporaryDirectory() as tmpdir:
            log_path = Path(tmpdir) / "subdir" / "logs" / "test.log"
            config = ZenithConfig(log_file=log_path)
            assert config.validate_config() is True
            # Check that parent directory was created
            assert log_path.parent.exists()
            assert log_path.parent.is_dir()


class TestEnvironmentVariableLoading:
    """Test environment variable loading."""
    
    def test_env_var_loading(self, monkeypatch):
        """Test that environment variables are loaded with ZENITH_ prefix."""
        monkeypatch.setenv("ZENITH_LOG_LEVEL", "DEBUG")
        monkeypatch.setenv("ZENITH_SIGNALING_PORT", "9000")
        monkeypatch.setenv("ZENITH_UPNP_ENABLED", "false")
        
        config = ZenithConfig()
        
        assert config.log_level == "DEBUG"
        assert config.signaling_port == 9000
        assert config.upnp_enabled is False
    
    def test_env_var_type_coercion(self, monkeypatch):
        """Test that environment variables are properly coerced to types."""
        monkeypatch.setenv("ZENITH_SIGNALING_PORT", "8888")
        monkeypatch.setenv("ZENITH_LOG_JSON", "true")
        
        config = ZenithConfig()
        
        assert isinstance(config.signaling_port, int)
        assert config.signaling_port == 8888
        assert isinstance(config.log_json, bool)
        assert config.log_json is True


class TestConfigurationDocumentation:
    """Test that configuration is well-documented."""
    
    def test_class_has_docstring(self):
        """Test that ZenithConfig class has comprehensive docstring."""
        assert ZenithConfig.__doc__ is not None
        assert len(ZenithConfig.__doc__) > 100
        assert "Pydantic" in ZenithConfig.__doc__
    
    def test_fields_have_descriptions(self):
        """Test that all fields have descriptions."""
        config = ZenithConfig()
        schema = config.model_json_schema()
        
        for field_name, field_info in schema.get("properties", {}).items():
            assert "description" in field_info, f"Field {field_name} missing description"
            assert len(field_info["description"]) > 10, f"Field {field_name} has short description"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
