import logging
import sys
from typing import Any

import structlog
from structlog.types import Processor

from backend.config import ZenithConfig


def configure_logging(config: ZenithConfig) -> None:
    """Configures structlog and standard logging based on the configuration."""
    
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
    return structlog.get_logger(name)
