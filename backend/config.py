"""Configuration module for Zenith DAW Backend.

This module provides a robust, type-safe configuration system using Pydantic.
All settings can be configured via environment variables with the ZENITH_ prefix
or through a .env file in the working directory.

Configuration Categories:
    - Logging: Log level, file output, and structured logging
    - Networking: Host bindings, port assignments, and service endpoints
    - UPnP: Universal Plug and Play for NAT traversal

Environment Variable Examples:
    ZENITH_LOG_LEVEL=DEBUG
    ZENITH_SIGNALING_PORT=54320
    ZENITH_HEALTH_PORT=8000
    ZENITH_UPNP_ENABLED=true
    ZENITH_UPNP_PORT=4000
    ZENITH_UPNP_TIMEOUT=30

Default Values:
    All settings have safe, production-ready defaults. Critical services use
    non-privileged ports (>1024) to avoid requiring root privileges.
"""

import ipaddress
from collections import Counter
from pathlib import Path
from typing import Any, Dict, Optional

from pydantic import Field, field_validator, model_validator
from pydantic_settings import BaseSettings, SettingsConfigDict


class ZenithConfig(BaseSettings):
    """Central configuration for Zenith DAW Backend.
    
    This configuration class uses Pydantic for automatic validation, type safety,
    and environment variable parsing. All fields have validators to ensure
    correctness and prevent misconfiguration.
    
    The configuration supports:
        - Automatic environment variable loading (ZENITH_* prefix)
        - .env file support for local development
        - Type validation and coercion
        - Range validation for ports and timeouts
        - Host validation for network bindings
    
    Attributes:
        log_level: Logging verbosity level (DEBUG, INFO, WARNING, ERROR, CRITICAL).
                   Default: "INFO"
        log_file: Optional path to log file. If None, logs to stdout only.
                  Default: None (stdout only)
        log_json: Enable structured JSON logging for production environments.
                  Default: False (human-readable console format)
        
        signaling_host: Network interface to bind signaling server.
                        "0.0.0.0" binds to all interfaces.
                        Default: "0.0.0.0"
        signaling_port: TCP port for signaling server (TLS).
                        Must be 1024-65535 (non-privileged).
                        Default: 8080
        
        health_port: TCP port for health check endpoint.
                     Must be 1024-65535 (non-privileged).
                     Default: 8000
        
        upnp_enabled: Enable/disable UPnP port mapping for NAT traversal.
                      Default: True
        upnp_port: TCP port to map via UPnP.
                   Must be 1024-65535 (non-privileged).
                   Default: 4000
        upnp_timeout: UPnP discovery and SOAP timeout in seconds.
                      Must be 1-300 seconds.
                      Default: 30
    
    Example:
        >>> config = ZenithConfig()
        >>> print(f"Signaling on {config.signaling_host}:{config.signaling_port}")
        >>> config = ZenithConfig(signaling_port=9000, upnp_enabled=False)
    """
    
    model_config = SettingsConfigDict(
        env_prefix="ZENITH_",
        env_file=".env",
        env_file_encoding="utf-8",
        extra="ignore",  # Ignore unknown environment variables
        case_sensitive=False,  # Allow lowercase env vars
        validate_default=True,  # Validate default values
    )

    # ========================================================================
    # Logging Configuration
    # ========================================================================
    
    log_level: str = Field(
        default="INFO",
        description=(
            "Logging level for the backend services. "
            "Valid values: DEBUG, INFO, WARNING, ERROR, CRITICAL. "
            "DEBUG provides verbose output for development, "
            "INFO is recommended for production."
        )
    )
    
    log_file: Optional[Path] = Field(
        default=None,
        description=(
            "Path to log file for persistent logging. "
            "If None, logs are written to stdout only. "
            "Use absolute paths or paths relative to working directory."
        )
    )
    
    log_json: bool = Field(
        default=False,
        description=(
            "Enable structured JSON logging for machine parsing. "
            "Recommended for production deployments with log aggregation. "
            "When False, uses human-readable console format."
        )
    )

    # ========================================================================
    # Networking Configuration
    # ========================================================================
    
    signaling_host: str = Field(
        default="0.0.0.0",
        description=(
            "Network interface to bind the signaling server. "
            "Use '0.0.0.0' to bind to all interfaces (recommended), "
            "'127.0.0.1' for localhost only, or a specific IP address."
        )
    )
    
    signaling_port: int = Field(
        default=8080,
        ge=1024,
        le=65535,
        description=(
            "TCP port for the signaling server (TLS). "
            "Must be in range 1024-65535 (non-privileged port). "
            "Default: 8080. Common alternatives: 8443, 54320."
        )
    )
    
    health_port: int = Field(
        default=8000,
        ge=1024,
        le=65535,
        description=(
            "TCP port for the health check HTTP endpoint. "
            "Must be in range 1024-65535 (non-privileged port). "
            "Exposes /health for service monitoring and orchestration."
        )
    )
    
    # ========================================================================
    # UPnP Configuration
    # ========================================================================
    
    upnp_enabled: bool = Field(
        default=True,
        description=(
            "Enable UPnP (Universal Plug and Play) port mapping. "
            "Automatically configures NAT routers for peer-to-peer connectivity. "
            "Disable if behind corporate firewall or for security."
        )
    )
    
    upnp_port: int = Field(
        default=4000,
        ge=1024,
        le=65535,
        description=(
            "TCP port to map via UPnP for peer-to-peer connections. "
            "Must be in range 1024-65535 (non-privileged port). "
            "This port is opened on the router's external interface."
        )
    )
    
    upnp_timeout: int = Field(
        default=30,
        ge=1,
        le=300,
        description=(
            "Timeout for UPnP discovery and SOAP requests in seconds. "
            "Must be 1-300 seconds. Lower values (3-10) for quick detection, "
            "higher values (30+) for slower networks. Default: 30."
        )
    )

    # ========================================================================
    # Validators
    # ========================================================================
    
    @field_validator('log_level')
    @classmethod
    def validate_log_level(cls, v: str) -> str:
        """Validate log level is one of the standard Python logging levels.
        
        Args:
            v: The log level string to validate.
            
        Returns:
            The uppercase log level string.
            
        Raises:
            ValueError: If log level is not valid.
        """
        valid_levels = {'DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'}
        v_upper = v.upper()
        if v_upper not in valid_levels:
            raise ValueError(
                f"log_level must be one of {valid_levels}, got '{v}'"
            )
        return v_upper
    
    @field_validator('log_file')
    @classmethod
    def validate_log_file(cls, v: Optional[Path]) -> Optional[Path]:
        """Validate log file path and ensure parent directory exists if possible.
        
        Args:
            v: The log file path to validate.
            
        Returns:
            The validated Path object or None.
            
        Note:
            This validator does not create the directory, but checks if the
            parent is accessible when it exists.
        """
        if v is None:
            return v
        
        # Convert to absolute path if relative
        if not v.is_absolute():
            v = Path.cwd() / v
        
        # Check if parent directory exists (don't create it)
        parent = v.parent
        if parent.exists() and not parent.is_dir():
            raise ValueError(
                f"log_file parent path exists but is not a directory: {parent}"
            )
        
        return v
    
    @field_validator('signaling_host')
    @classmethod
    def validate_host(cls, v: str) -> str:
        """Validate host binding address format.
        
        Uses the ipaddress module for proper IPv4 and IPv6 validation.
        Also accepts 'localhost' as a special case.
        
        Args:
            v: The host string to validate.
            
        Returns:
            The validated host string.
            
        Raises:
            ValueError: If host format is invalid.
        """
        if not v or not isinstance(v, str):
            raise ValueError("signaling_host must be a non-empty string")
        
        # Allow 'localhost' as special case
        if v == 'localhost':
            return v
        
        # Validate as IPv4 or IPv6 address
        try:
            ipaddress.ip_address(v)
            return v
        except ValueError:
            raise ValueError(
                f"signaling_host must be 'localhost' or a valid IP address, got '{v}'. "
                "Examples: '0.0.0.0', '127.0.0.1', '::1', '192.168.1.100'"
            )
    
    @model_validator(mode='after')
    def validate_port_conflicts(self) -> 'ZenithConfig':
        """Validate that different services don't use the same port.
        
        Uses Counter for efficient O(n) conflict detection.
        
        Returns:
            The validated config instance.
            
        Raises:
            ValueError: If ports conflict.
        """
        ports: Dict[str, int] = {
            'signaling_port': self.signaling_port,
            'health_port': self.health_port,
        }
        
        if self.upnp_enabled:
            ports['upnp_port'] = self.upnp_port
        
        # Use Counter for efficient duplicate detection
        port_counts = Counter(ports.values())
        duplicates = {port for port, count in port_counts.items() if count > 1}
        
        if duplicates:
            # Find which port names conflict
            conflicts = [f"{name}={port}" for name, port in ports.items() if port in duplicates]
            raise ValueError(
                f"Port conflict detected: {', '.join(conflicts)}. "
                "Each service must use a unique port."
            )
        
        return self

    # ========================================================================
    # Methods
    # ========================================================================

    def get_service_env(self) -> Dict[str, str]:
        """Build environment variables dictionary for child services.
        
        This method creates a dictionary of environment variables that can be
        passed to subprocess calls or child services. All values are converted
        to strings as required by environment variables.
        
        Returns:
            Dictionary mapping environment variable names to string values.
            Contains ZENITH_* prefixed variables for signaling and logging.
            
        Example:
            >>> config = ZenithConfig()
            >>> env = config.get_service_env()
            >>> # Pass to subprocess: subprocess.Popen(..., env=env)
        """
        return {
            "ZENITH_SIGNALING_HOST": self.signaling_host,
            "ZENITH_SIGNALING_PORT": str(self.signaling_port),
            "ZENITH_LOG_LEVEL": self.log_level,
        }
    
    def validate_config(self) -> bool:
        """Perform additional runtime validation checks.
        
        This method can be called after configuration to perform checks that
        require system state (e.g., port availability, file permissions).
        
        Returns:
            True if all validation checks pass.
            
        Raises:
            ValueError: If validation fails with details.
            
        Note:
            This is an optional validation method. The Pydantic validators
            run automatically during initialization.
        """
        # Check log file is writable if specified
        if self.log_file:
            try:
                # Try to create parent directory if it doesn't exist
                self.log_file.parent.mkdir(parents=True, exist_ok=True)
                # Test if we can write (append mode, don't truncate)
                with open(self.log_file, 'a'):
                    pass
            except (OSError, PermissionError) as e:
                raise ValueError(
                    f"Cannot write to log file {self.log_file}: {e}"
                )
        
        return True
