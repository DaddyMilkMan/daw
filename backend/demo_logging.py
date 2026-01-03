#!/usr/bin/env python3
"""
Demo script showing the standardized logging approach.
This demonstrates the consistent logging patterns across the backend.
"""

import sys
import os

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from backend.config import ZenithConfig
from backend.logger import configure_logging, get_logger


def demo_console_logging():
    """Demonstrate console-style logging (development mode)."""
    print("\n" + "="*60)
    print("DEMO: Console Logging (Development Mode)")
    print("="*60 + "\n")
    
    config = ZenithConfig()
    config.log_level = "DEBUG"
    config.log_json = False
    configure_logging(config)
    
    log = get_logger("demo.module")
    
    log.debug("Debug message", component="initialization", value=42)
    log.info("Service started", port=8080, protocol="TCP")
    log.warning("Resource usage high", memory_percent=85, threshold=80)
    
    try:
        # Simulate an error
        raise ValueError("Connection timeout")
    except Exception as e:
        log.error("Operation failed", operation="connect", error=str(e))


def demo_json_logging():
    """Demonstrate JSON logging (production mode)."""
    print("\n" + "="*60)
    print("DEMO: JSON Logging (Production Mode)")
    print("="*60 + "\n")
    
    config = ZenithConfig()
    config.log_level = "INFO"
    config.log_json = True
    configure_logging(config)
    
    log = get_logger("demo.service")
    
    log.info("Service initialized", service_name="signaling", version="1.0.0")
    log.info("Connection accepted", remote_addr="192.168.1.100", port=54320)
    log.warning("Retry attempt", attempt=2, max_attempts=5)


if __name__ == "__main__":
    print("\nZenith Backend Logging Demonstration")
    print("This script demonstrates the standardized logging approach.")
    
    demo_console_logging()
    demo_json_logging()
    
    print("\n" + "="*60)
    print("See backend/LOGGING.md for complete documentation")
    print("="*60 + "\n")
