"""
Tests for WebRTCGatewayAgent.

Tests cover:
- Agent instantiation
- Basic run() method execution
- Configuration handling
"""

import os
import sys
from pathlib import Path

# Ensure agents module is importable in test environment
root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
agents_path = os.path.join(root, 'agents')
if agents_path not in sys.path:
    sys.path.insert(0, agents_path)

from WebRTCGatewayAgent.WebRTCGatewayAgent import WebRTCGatewayAgent


def test_agent_instantiation():
    """Test that WebRTCGatewayAgent can be instantiated."""
    agent = WebRTCGatewayAgent()
    assert agent is not None
    assert isinstance(agent, WebRTCGatewayAgent)


def test_agent_with_config():
    """Test that WebRTCGatewayAgent can be instantiated with config."""
    config = {"test_key": "test_value"}
    agent = WebRTCGatewayAgent(config=config)
    assert agent.config == config


def test_run_is_callable():
    """Test that run() method is callable."""
    agent = WebRTCGatewayAgent()
    assert callable(agent.run)


def test_run_executes():
    """Test that run() method executes successfully."""
    agent = WebRTCGatewayAgent()
    result = agent.run()
    
    assert result is not None
    assert isinstance(result, dict)
    assert "status" in result
    assert result["status"] == "success"


def test_run_sets_running_flag():
    """Test that run() sets the running flag."""
    agent = WebRTCGatewayAgent()
    assert agent.running is False
    
    agent.run()
    assert agent.running is True


if __name__ == "__main__":
    # Run tests without pytest
    test_agent_instantiation()
    test_agent_with_config()
    test_run_is_callable()
    test_run_executes()
    test_run_sets_running_flag()
    print("All tests passed!")
