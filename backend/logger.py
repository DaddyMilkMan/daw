"""
Centralized logging configuration for Zenith DAW Backend.

This module provides a standardized logging approach using structlog for
application code and configures standard logging for third-party libraries.

USAGE:
    1. Call configure_logging() once at application startup
    2. Use get_logger() to obtain a logger instance in each module
    3. Always use the returned logger for all logging operations

EXAMPLES:
    # In main entry point (e.g., run_backend.py, cli.py):
    from backend.logger import configure_logging, get_logger
    from backend.config import ZenithConfig
    
    config = ZenithConfig()
    configure_logging(config)
    log = get_logger(__name__)
    log.info("Application started")
    
    # In other modules:
    from backend.logger import get_logger
    
    log = get_logger(__name__)
    log.debug("Debug message", extra_context="value")
    log.info("Info message", key="value")
    log.warning("Warning message")
    log.error("Error occurred", exc_info=True)
"""

import logging
import sys
from typing import Any

import structlog
from structlog.types import Processor

from backend.config import ZenithConfig


def configure_logging(config: ZenithConfig) -> None:
    """
    Configure structlog and standard logging based on the configuration.
    
    This function should be called once at application startup before any
    logging occurs. It sets up both structlog (for application code) and
    the standard logging module (for third-party libraries).
    
    Args:
        config: ZenithConfig instance with logging settings
            - log_level: Logging level (DEBUG, INFO, WARNING, ERROR)
            - log_json: If True, emit JSON logs; otherwise use console format
    
    Note:
        All modules should use get_logger() from this module rather than
        directly calling logging.getLogger() or structlog.get_logger().
    """
    # Configure standard logging first (for libraries that use it)
    logging.basicConfig(
        level=config.log_level,
        format="%(asctime)s [%(name)s] %(levelname)s: %(message)s",
        stream=sys.stdout
    )
    
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


def get_logger(name: str = "zenith") -> structlog.stdlib.BoundLogger:
    """
    Get a configured structlog logger instance.
    
    This is the standard way to obtain a logger in Zenith backend code.
    Use __name__ as the logger name to identify the module in logs.
    
    Args:
        name: Logger name, typically __name__ to identify the module
        
    Returns:
        A configured structlog BoundLogger instance
        
    Example:
        log = get_logger(__name__)
        log.info("Operation completed", items_processed=42)
    """
    return structlog.get_logger(name)
