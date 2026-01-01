import sys
from pathlib import Path
from typing import Optional

import typer
import structlog
from typing_extensions import Annotated

from backend.config import ZenithConfig
from backend.logger import configure_logging, get_logger
from backend.orchestrator import ServiceManager

app = typer.Typer(
    help="Gemini Backend CLI for Zenith DAW",
    add_completion=False,
    no_args_is_help=True,
)

log = get_logger()

# Global config state
state = {"config": None, "manager": None}


@app.callback()
def main(
    ctx: typer.Context,
    json_logs: bool = typer.Option(False, "--json", help="Emit structured JSON logs"),
    log_level: str = typer.Option("INFO", help="Log level (DEBUG, INFO, ERROR)"),
    config_file: Optional[Path] = typer.Option(None, envvar="ZENITH_CONFIG", help="Path to config file"),
):
    """
    Main entry point for Gemini CLI.
    """
    # Load configuration
    try:
        # In a real app we'd load from config_file if provided
        # For now, we rely on Pydantic's automatic env/file loading
        config = ZenithConfig()
        
        # Overrides from CLI
        if json_logs:
            config.log_json = True
        if log_level:
            config.log_level = log_level
            
        configure_logging(config)
        
        state["config"] = config
        state["manager"] = ServiceManager(config)
        
        log.debug("CLI initialized", config=config.model_dump(mode='json'))
        
    except Exception as e:
        # If config fails, we must print to stderr and exit
        # We can't use the logger if it hasn't been configured, but let's try
        print(f"Failed to initialize configuration: {e}", file=sys.stderr)
        raise typer.Exit(code=1)


@app.command()
def start(
    dry_run: bool = typer.Option(False, "--dry-run", help="Validate config but do not start services"),
    disable_upnp: bool = typer.Option(False, help="Disable UPnP port mapping"),
):
    """
    Start the Zenith Backend services (Signaling, UPnP).
    """
    manager: ServiceManager = state["manager"]
    config: ZenithConfig = state["config"]

    if disable_upnp:
        config.upnp_enabled = False

    log.info("Starting Gemini backend...", dry_run=dry_run)

    try:
        manager.start_services(dry_run=dry_run)
        if not dry_run:
            manager.wait_forever()
    except Exception as e:
        log.error("Backend dispatcher failed", error=str(e))
        raise typer.Exit(code=1)


@app.command()
def check():
    """
    Validate environment, dependencies, and configuration.
    """
    config: ZenithConfig = state["config"]
    log.info("Running health/environment check")
    
    issues = []
    
    # 1. Check ports
    # This is a stub for actual port availability check
    if config.signaling_port < 1024:
        issues.append("Signaling port is privileged (<1024)")

    # 2. Check dependencies (Stub)
    # We are here, so python deps are likely okay.

    if issues:
        log.error("Validation failed", issues=issues)
        raise typer.Exit(code=1)
        
    log.info("Environment validates successfully", config=config.model_dump(exclude={'api_key'} if hasattr(config, 'api_key') else None))


@app.command()
def status():
    """
    Check the status of running services.
    """
    # This would talk to the health endpoint or check PIDs
    log.info("Checking service status...")
    # TODO: Implement actual HTTP check to config.health_port
    print("Services: UNKNOWN (Not implemented)")

@app.command()
def hotfix():
    """
    Apply hotfixes to the running environment (Stub).
    """
    log.info("No hotfixes available.")


if __name__ == "__main__":
    app()
