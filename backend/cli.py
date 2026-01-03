"""
Command-line interface for Zenith DAW backend services.

This module provides a Typer-based CLI for managing Zenith backend services.
It offers commands for starting services, checking configuration, and monitoring
status.

Note: This CLI is currently a work-in-progress. For production use, consider
using backend.run_backend directly.

Commands:
    start: Start backend services with optional configuration
    check: Validate environment and configuration
    status: Check status of running services (stub)
    hotfix: Apply hotfixes (stub)
"""

import sys
from pathlib import Path
from typing import Optional, Any

import typer
import structlog
from typing_extensions import Annotated

from backend.config import ZenithConfig
from backend.logger import configure_logging, get_logger

app = typer.Typer(
    help="Backend CLI for Zenith DAW",
    add_completion=False,
    no_args_is_help=True,
)

log = get_logger()

# Global config state
state: dict[str, Any] = {"config": None}


@app.callback()
def main(
    ctx: typer.Context,
    json_logs: bool = typer.Option(False, "--json", help="Emit structured JSON logs"),
    log_level: str = typer.Option("INFO", help="Log level (DEBUG, INFO, ERROR)"),
    config_file: Optional[Path] = typer.Option(None, envvar="ZENITH_CONFIG", help="Path to config file"),
) -> None:
    """
    Initialize the CLI with configuration and logging.
    
    This callback runs before any command and sets up the global configuration
    and logging system.
    
    Args:
        ctx: Typer context object.
        json_logs: Enable JSON structured logging output.
        log_level: Logging level (DEBUG, INFO, WARNING, ERROR).
        config_file: Optional path to configuration file.
        
    Raises:
        typer.Exit: If configuration initialization fails.
    """
    try:
        # Load configuration from environment/file
        config = ZenithConfig()
        
        # Apply CLI overrides
        if json_logs:
            config.log_json = True
        if log_level:
            config.log_level = log_level
            
        configure_logging(config)
        
        state["config"] = config
        
        log.debug("CLI initialized", config=config.model_dump(mode='json'))
        
    except Exception as e:
        # If config fails, print to stderr and exit
        print(f"Failed to initialize configuration: {e}", file=sys.stderr)
        raise typer.Exit(code=1)


@app.command()
def start(
    dry_run: bool = typer.Option(False, "--dry-run", help="Validate config but do not start services"),
    disable_upnp: bool = typer.Option(False, help="Disable UPnP port mapping"),
) -> None:
    """
    Start the Zenith Backend services.
    
    Note: This command is currently a stub. Use 'python -m backend.run_backend'
    for the full implementation with ServiceSentinel orchestration.
    
    Args:
        dry_run: Validate configuration without starting services.
        disable_upnp: Skip UPnP port mapping service.
        
    Raises:
        typer.Exit: If service startup fails.
    """
    config: Optional[ZenithConfig] = state.get("config")
    if not config:
        log.error("Configuration not initialized")
        raise typer.Exit(code=1)

    if disable_upnp:
        config.upnp_enabled = False

    log.info("Backend start requested", dry_run=dry_run, upnp_enabled=config.upnp_enabled)

    if dry_run:
        log.info("Dry run mode - configuration validated successfully")
        return

    # TODO: Implement actual service startup
    # For now, direct users to run_backend.py
    log.warning("This command is a stub. Please use 'python -m backend.run_backend' instead")
    print("\nTo start the backend, run:")
    print("  python -m backend.run_backend")
    print("\nFor help, run:")
    print("  python -m backend.run_backend --help")


@app.command()
def check() -> None:
    """
    Validate environment, dependencies, and configuration.
    
    Performs basic validation checks including port configuration and
    environment setup.
    
    Raises:
        typer.Exit: If validation fails.
    """
    config: Optional[ZenithConfig] = state.get("config")
    if not config:
        log.error("Configuration not initialized")
        raise typer.Exit(code=1)
        
    log.info("Running health/environment check")
    
    issues = []
    
    # Check for privileged port usage
    if config.signaling_port < 1024:
        issues.append(f"Signaling port {config.signaling_port} is privileged (<1024) and may require root")
    
    if config.health_port < 1024:
        issues.append(f"Health port {config.health_port} is privileged (<1024) and may require root")

    # Validate port ranges
    if not (1 <= config.signaling_port <= 65535):
        issues.append(f"Invalid signaling port: {config.signaling_port}")
    
    if not (1 <= config.upnp_port <= 65535):
        issues.append(f"Invalid UPnP port: {config.upnp_port}")

    if issues:
        log.error("Validation failed", issues=issues)
        for issue in issues:
            print(f"  ⚠ {issue}")
        raise typer.Exit(code=1)
        
    log.info("✓ Environment validates successfully")
    print("\n✓ Configuration is valid")
    print(f"  Signaling port: {config.signaling_port}")
    print(f"  Health port: {config.health_port}")
    print(f"  UPnP enabled: {config.upnp_enabled}")
    print(f"  Log level: {config.log_level}")


@app.command()
def status() -> None:
    """
    Check the status of running services.
    
    Note: This command is currently a stub. Status checking should be
    implemented via the health endpoint (GET /health on the health port).
    """
    config: Optional[ZenithConfig] = state.get("config")
    if not config:
        log.error("Configuration not initialized")
        raise typer.Exit(code=1)
        
    log.info("Checking service status...")
    print(f"\nTo check service status, query the health endpoint:")
    print(f"  curl http://localhost:{config.health_port}/health")
    print("\nOr use this command:")
    print(f"  curl -s http://localhost:{config.health_port}/health | python -m json.tool")


@app.command()
def hotfix() -> None:
    """
    Apply hotfixes to the running environment.
    
    Note: This command is currently a stub for future hotfix functionality.
    """
    log.info("No hotfixes available")
    print("No hotfixes are currently available.")


if __name__ == "__main__":
    app()
