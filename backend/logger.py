import logging
import sys
from typing import Any

import structlog
from structlog.types import Processor

from backend.config import ZenithConfig


def configure_logging(config: ZenithConfig) -> None:
    """
    Configure structlog and standard logging based on the configuration.
    
    Sets up logging processors for structured logging with timestamp, log level,
    and stack info. Supports both JSON (production) and console (development)
    output formats.
    
    Args:
        config: ZenithConfig instance containing log level, format preferences.
    
    Returns:
        None
    """
    
    # Configure standard logging first (for libraries that use it)
    logging.basicConfig(level=config.log_level, format="%(message)s", stream=sys.stdout)
    
    processors: list[Processor] = [
        structlog.contextvars.merge_contextvars,
        structlog.processors.add_log_level,
        structlog.processors.StackInfoRenderer(),
        structlog.dev.set_exc_info,
        structlog.processors.TimeStamper(fmt="iso"),
    ]

    # If we want JSON logs (production/parsing) vs Console logs (dev)
    if config.log_json:
        processors.append(structlog.processors.JSONRenderer())
    else:
        processors.append(structlog.dev.ConsoleRenderer())

    structlog.configure(
        processors=processors,
        logger_factory=structlog.PrintLoggerFactory(),
        wrapper_class=structlog.make_filtering_bound_logger(logging.getLevelName(config.log_level)),
        cache_logger_on_first_use=True,
    )

    # Redirect standard logging to structlog if desired, or just keep them separate.
    # For now, we keep standard logging simple and use structlog for our app code.

def get_logger(name: str = "zenith") -> structlog.stdlib.BoundLogger:
    """
    Get a configured structlog logger instance.
    
    Args:
        name: Logger name for identification (default: "zenith").
        
    Returns:
        structlog.stdlib.BoundLogger: Configured logger instance for structured logging.
    """
    return structlog.get_logger(name)
