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
    - Thread-safe logging via stdlib integration
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
    to work seamlessly with structlog in a thread-safe manner.
    
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
    # Processors that structlog will use to process log records before passing to stdlib
    processors: list[Processor] = [
        structlog.contextvars.merge_contextvars,
        structlog.stdlib.add_logger_name,
        structlog.stdlib.add_log_level,
        structlog.processors.TimeStamper(fmt="iso"),
        structlog.processors.StackInfoRenderer(),
        structlog.processors.format_exc_info,
        structlog.processors.UnicodeDecoder(),
        # ProcessorFormatter will pick it up from here
        structlog.stdlib.ProcessorFormatter.wrap_for_formatter,
    ]

    # Configure structlog to delegate handling to the standard logging module
    # This ensures thread-safety by using stdlib's thread-safe logging infrastructure
    structlog.configure(
        processors=processors,
        logger_factory=structlog.stdlib.LoggerFactory(),
        wrapper_class=structlog.stdlib.BoundLogger,
        cache_logger_on_first_use=True,
    )

    # Configure the standard logging module to render structlog records
    renderer: Processor
    if config.log_json:
        renderer = structlog.processors.JSONRenderer()
    else:
        renderer = structlog.dev.ConsoleRenderer()

    formatter = structlog.stdlib.ProcessorFormatter(
        processor=renderer,
        # These processors are for logs from standard logging not using structlog
        foreign_pre_chain=[
            structlog.stdlib.add_log_level,
            structlog.stdlib.add_logger_name,
            structlog.processors.TimeStamper(fmt="iso"),
        ],
    )

    handler = logging.StreamHandler(sys.stdout)
    handler.setFormatter(formatter)
    
    root_logger = logging.getLogger()
    # Clear any existing handlers to avoid duplicate logs
    if root_logger.hasHandlers():
        root_logger.handlers.clear()
        
    root_logger.addHandler(handler)
    # Ensure the log level is valid, defaulting to INFO if invalid
    log_level = getattr(logging, config.log_level.upper(), logging.INFO)
    root_logger.setLevel(log_level)


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
