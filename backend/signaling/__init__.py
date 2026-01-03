"""
Signaling server components for Zenith DAW peer-to-peer connections.

This module provides signaling infrastructure for coordinating peer-to-peer
connections between Zenith DAW clients including:
- Session management with TTL-based expiration
- TCP signaling with TLS for secure session coordination  
- UDP hole-punching for NAT traversal assistance

Main components:
- SessionStore: Thread-safe session storage with TTL management
- signaling_server: TCP/TLS and UDP signaling server implementation

For usage details, see the README.md in the parent directory.
"""
