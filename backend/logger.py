"""
Centralized logging configuration for Zenith DAW Backend.

This module provides a consistent logging interface across all backend modules
using structlog for structured logging with support for both development and
production environments.

Usage:
    from backend.logger import configure_logging, get_logger
    
    # Configure logging once at application startup
    configure_logging(config)
    
    # Get a logger instance in each module
    logger = get_logger("module_name")
    
    # Use the logger
    logger.info("Operation completed", user_id=123, duration_ms=45)
    logger.error("Failed to connect", host="localhost", port=8080)

Features:
    - Structured logging with key-value context
    - JSON output for production (machine-readable)
    - Human-friendly console output for development
    - Configurable log levels (DEBUG, INFO, WARNING, ERROR)
    - ISO timestamp formatting
    - Exception info and stack traces
    - Context variable merging for request tracking
"""

import logging
import sys

import structlog
from structlog.types import Processor

from backend.config import ZenithConfig


def configure_logging(config: ZenithConfig) -> None:
    """
    Configures structlog for the entire backend application.
    
    This function should be called once at application startup, before any
    logging occurs. It sets up structlog with appropriate processors for
    structured logging and configures the underlying standard library logging
    to work seamlessly with structlog.
    
    Args:
        config: ZenithConfig instance containing logging preferences including
                log_level (e.g., "DEBUG", "INFO") and log_json (bool)
    
    Example:
        >>> from backend.config import ZenithConfig
        >>> config = ZenithConfig(log_level="DEBUG", log_json=False)
        >>> configure_logging(config)
        >>> logger = get_logger("myapp")
        >>> logger.info("Application started")
    """
    # Configure standard logging as a foundation for third-party libraries
    # that use standard logging. Set to WARNING to reduce noise from libraries.
    logging.basicConfig(
        level=logging.WARNING,
        format="%(message)s",
        stream=sys.stdout
    )
    
    # Build the structlog processor pipeline
    processors: list[Processor] = [
        # Merge in context variables from contextvars
        structlog.contextvars.merge_contextvars,
        # Add log level to the event dict
        structlog.processors.add_log_level,
        # Render stack traces
        structlog.processors.StackInfoRenderer(),
        # Set exception info if present
        structlog.dev.set_exc_info,
        # Add ISO8601 timestamps
        structlog.processors.TimeStamper(fmt="iso"),
    ]

    # Choose renderer based on configuration
    if config.log_json:
        # JSON output for production (structured, machine-readable)
        processors.append(structlog.processors.JSONRenderer())
    else:
        # Console output for development (colored, human-readable)
        processors.append(structlog.dev.ConsoleRenderer())

    # Configure structlog globally
    structlog.configure(
        processors=processors,
        logger_factory=structlog.PrintLoggerFactory(),
        wrapper_class=structlog.make_filtering_bound_logger(
            logging.getLevelName(config.log_level)
        ),
        cache_logger_on_first_use=True,
    )


def get_logger(name: str = "zenith") -> structlog.stdlib.BoundLogger:
    """
    Get a configured structlog logger instance.
    
    This function returns a structlog logger that can be used for structured
    logging throughout the application. Each module should get its own logger
    with a descriptive name for better log traceability.
    
    Args:
        name: The name of the logger, typically the module name or component
              name (e.g., "zenith.orchestrator", "zenith.signaling")
    
    Returns:
        A bound logger instance configured with the application's processors
    
    Example:
        >>> logger = get_logger("zenith.mymodule")
        >>> logger.info("Processing started", task_id="abc123")
        >>> logger.error("Connection failed", host="example.com", error="timeout")
    """
    return structlog.get_logger(name)
