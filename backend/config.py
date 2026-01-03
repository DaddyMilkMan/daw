from pathlib import Path
from typing import Optional

from pydantic_settings import BaseSettings, SettingsConfigDict


class ZenithConfig(BaseSettings):
    """
    Central configuration for Zenith DAW Backend.
    Reads from environment variables (prefix ZENITH_) or automatic .env file.
    """
    model_config = SettingsConfigDict(
        env_prefix="ZENITH_",
        env_file=".env",
        env_file_encoding="utf-8",
        extra="ignore"
    )

    # Logging
    log_level: str = "INFO"
    log_file: Optional[Path] = None
    log_json: bool = False

    # Networking
    signaling_host: str = "0.0.0.0"
    signaling_port: int = 8080
    
    health_port: int = 8000
    
    # UPnP
    upnp_enabled: bool = True
    upnp_port: int = 4000
    upnp_timeout: int = 30

    def get_service_env(self) -> dict[str, str]:
        """
        Get environment variables for child services.
        
        Returns:
            dict[str, str]: Dictionary of environment variables to pass to spawned services.
                          Includes signaling host/port and log level configuration.
        """
        return {
            "ZENITH_SIGNALING_HOST": self.signaling_host,
            "ZENITH_SIGNALING_PORT": str(self.signaling_port),
            "ZENITH_LOG_LEVEL": self.log_level,
        }
