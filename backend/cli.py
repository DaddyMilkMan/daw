import sys
from pathlib import Path
from typing import Optional
import socket
import http.client
import json

import typer

from backend.config import ZenithConfig
from backend.logger import configure_logging, get_logger
from backend.orchestrator import ServiceManager

app = typer.Typer(
    help="Gemini Backend CLI for Zenith DAW",
    add_completion=False,
    no_args_is_help=True,
)

log = get_logger()


class AppState:
    """
    Application state container for dependency injection via Typer context.
    
    This class holds the configuration and service manager instances,
    providing thread-safe access through Typer's context mechanism.
    Eliminates the need for global state variables.
    """
    
    def __init__(self, config: ZenithConfig, manager: ServiceManager):
        """
        Initialize application state.
        
        Args:
            config: ZenithConfig instance with application configuration
            manager: ServiceManager instance for service orchestration
        """
        self.config = config
        self.manager = manager


@app.callback()
def main(
    ctx: typer.Context,
    json_logs: bool = typer.Option(False, "--json", help="Emit structured JSON logs"),
    log_level: str = typer.Option("INFO", help="Log level (DEBUG, INFO, ERROR)"),
    config_file: Optional[Path] = typer.Option(None, envvar="ZENITH_CONFIG", help="Path to config file"),
):
    """
    Main entry point for Zenith Backend CLI.
    
    Initializes configuration, logging, and service management infrastructure.
    State is passed to subcommands via Typer's context object for dependency injection.
    """
    try:
        # Load configuration from environment or config file
        if config_file and config_file.exists():
            # Future enhancement: Load from YAML/JSON config file
            # For now, Pydantic handles environment variables and .env files
            log.debug("Config file specified but custom loading not yet implemented", 
                     path=str(config_file))
        
        config = ZenithConfig()
        
        # Apply CLI overrides
        if json_logs:
            config.log_json = True
        if log_level:
            config.log_level = log_level
            
        configure_logging(config)
        
        # Initialize service manager
        manager = ServiceManager(config)
        
        # Store state in Typer context for dependency injection
        ctx.obj = AppState(config=config, manager=manager)
        
        log.debug("CLI initialized successfully", 
                 log_level=config.log_level,
                 json_logs=config.log_json)
        
    except Exception as e:
        # Configuration or initialization failure is fatal
        # Use stderr since logging may not be configured
        typer.echo(f"Fatal error during initialization: {e}", err=True)
        log.error("Initialization failed", error=str(e), exc_info=True)
        raise typer.Exit(code=1)


@app.command()
def start(
    ctx: typer.Context,
    dry_run: bool = typer.Option(False, "--dry-run", help="Validate config but do not start services"),
    disable_upnp: bool = typer.Option(False, help="Disable UPnP port mapping"),
):
    """
    Start the Zenith Backend services (Signaling, UPnP).
    
    This command initializes and starts all configured backend services.
    In dry-run mode, it validates the configuration without starting services.
    
    Args:
        ctx: Typer context containing AppState
        dry_run: If True, validate configuration only
        disable_upnp: If True, disable UPnP port mapping service
    
    Raises:
        Exit(1): If service startup fails
    """
    state: AppState = ctx.obj
    
    if disable_upnp:
        state.config.upnp_enabled = False
        log.info("UPnP disabled via CLI flag")

    log.info("Starting Zenith backend", 
            dry_run=dry_run,
            upnp_enabled=state.config.upnp_enabled)

    try:
        state.manager.start_services(dry_run=dry_run)
        if not dry_run:
            log.info("Services started, entering main loop")
            state.manager.wait_forever()
    except KeyboardInterrupt:
        log.info("Interrupted by user")
        state.manager.stop_services()
        raise typer.Exit(code=0)
    except Exception as e:
        log.error("Backend service failed", error=str(e), exc_info=True)
        typer.echo(f"Error: {e}", err=True)
        raise typer.Exit(code=1)


@app.command()
def check(ctx: typer.Context):
    """
    Validate environment, dependencies, and configuration.
    
    Performs comprehensive checks including:
    - Port availability for signaling and health endpoints
    - Configuration validation
    - Python dependencies (implicitly validated by import success)
    
    Args:
        ctx: Typer context containing AppState
        
    Raises:
        Exit(1): If validation fails
    """
    state: AppState = ctx.obj
    config = state.config
    
    log.info("Running environment and configuration validation")
    
    issues = []
    warnings = []
    
    # 1. Check if ports are privileged
    if config.signaling_port < 1024:
        warnings.append(f"Signaling port {config.signaling_port} is privileged (<1024), may require root")
    
    if config.health_port < 1024:
        warnings.append(f"Health port {config.health_port} is privileged (<1024), may require root")
    
    # 2. Check port availability
    def is_port_available(port: int) -> bool:
        """Check if a port is available for binding."""
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                sock.bind(('', port))
                return True
        except OSError as e:
            log.debug(f"Port {port} check failed", error=str(e))
            return False
    
    if not is_port_available(config.signaling_port):
        issues.append(f"Signaling port {config.signaling_port} is not available (already in use)")
    
    if not is_port_available(config.health_port):
        issues.append(f"Health port {config.health_port} is not available (already in use)")
    
    # 3. Check UPnP configuration if enabled
    if config.upnp_enabled:
        if config.upnp_port < 1024:
            warnings.append(f"UPnP port {config.upnp_port} is privileged (<1024)")
        if config.upnp_timeout < 1:
            issues.append(f"UPnP timeout {config.upnp_timeout} is too low (minimum 1 second)")
    
    # 4. Python dependencies are implicitly validated (we imported successfully)
    log.debug("Python dependencies validated (imports successful)")
    
    # Report findings
    if warnings:
        for warning in warnings:
            log.warning(warning)
            typer.echo(f"⚠️  WARNING: {warning}", err=True)
    
    if issues:
        for issue in issues:
            log.error("Validation issue", issue=issue)
            typer.echo(f"❌ ERROR: {issue}", err=True)
        
        log.error("Environment validation failed", 
                 issue_count=len(issues),
                 warning_count=len(warnings))
        raise typer.Exit(code=1)
    
    # Success
    typer.echo("✅ Environment validation passed")
    if warnings:
        typer.echo(f"   ({len(warnings)} warning(s) - see above)")
    
    log.info("Environment validates successfully", 
            warning_count=len(warnings),
            config_summary={
                "signaling_port": config.signaling_port,
                "health_port": config.health_port,
                "upnp_enabled": config.upnp_enabled,
                "log_level": config.log_level,
            })


@app.command()
def status(ctx: typer.Context):
    """
    Check the status of running services.
    
    Queries the service manager for current service states and attempts
    to connect to the health endpoint if configured.
    
    Args:
        ctx: Typer context containing AppState
    """
    state: AppState = ctx.obj
    config = state.config
    
    log.info("Checking service status")
    
    # Get status from service manager
    service_status = state.manager.get_status()
    
    if not service_status:
        typer.echo("No services are currently managed")
        log.info("No services registered with manager")
        return
    
    # Display service status
    typer.echo("\n📊 Service Status:")
    typer.echo("=" * 50)
    
    all_running = True
    for service_name, status_info in service_status.items():
        state_name = status_info.get('state', 'UNKNOWN')
        pid = status_info.get('pid')
        restarts = status_info.get('restarts', 0)
        healthy = status_info.get('healthy', False)
        
        # Status icon
        if state_name == 'RUNNING' and healthy:
            icon = "✅"
        elif state_name == 'RUNNING':
            icon = "⚠️"
        else:
            icon = "❌"
            all_running = False
        
        typer.echo(f"{icon} {service_name.upper()}: {state_name}")
        if pid:
            typer.echo(f"   PID: {pid}")
        if restarts > 0:
            typer.echo(f"   Restarts: {restarts}")
        if state_name == 'RUNNING':
            health_str = "healthy" if healthy else "unhealthy"
            typer.echo(f"   Health: {health_str}")
    
    typer.echo("=" * 50)
    
    # Try to query health endpoint if available
    try:
        conn = http.client.HTTPConnection("127.0.0.1", config.health_port, timeout=2)
        conn.request("GET", "/health")
        response = conn.getresponse()
        
        if response.status == 200:
            data = json.loads(response.read().decode())
            typer.echo(f"\n🏥 Health endpoint (:{config.health_port}): REACHABLE")
            typer.echo(f"   Response: {json.dumps(data, indent=2)}")
            log.info("Health endpoint reachable", data=data)
        else:
            typer.echo(f"\n⚠️  Health endpoint returned status {response.status}")
            log.warning("Health endpoint returned non-200", status=response.status)
            
        conn.close()
        
    except (ConnectionRefusedError, OSError) as e:
        typer.echo(f"\n❌ Health endpoint (:{config.health_port}): NOT REACHABLE")
        typer.echo(f"   Error: {e}")
        log.info("Health endpoint not reachable", error=str(e))
    except Exception as e:
        typer.echo(f"\n⚠️  Could not query health endpoint: {e}")
        log.warning("Health endpoint query failed", error=str(e))
    
    typer.echo()
    
    if all_running:
        log.info("All services running")
    else:
        log.warning("Some services are not running")


if __name__ == "__main__":
    app()
