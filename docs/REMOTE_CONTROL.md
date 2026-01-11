# Remote Control Setting

## Overview

The **Allow Remote Control** setting enables external agents, AI assistants, or collaboration sessions to control transport operations (play, stop, seek) in Zenith DAW.

## Security Model

This feature follows an **explicit opt-in** security model:

1. **Default State**: Remote control is **disabled** by default
2. **User Consent**: Users must explicitly enable the feature in Preferences
3. **Confirmation Required**: A confirmation dialog explains the implications before enabling
4. **Session-Based**: Remote control only applies to sessions the user explicitly joins
5. **Revocable**: Users can disable this setting at any time

## Use Cases

Remote control is required for:

- **AI-Assisted Workflow**: Allow AI agents to control playback for analysis and mastering
- **Remote Collaboration**: Enable collaborators to control transport during live sessions
- **Automation Scripts**: Allow external automation tools to manage playback

## Enabling Remote Control

### Via Preferences UI

1. Open **Preferences** (Ctrl+, or Cmd+,)
2. Navigate to the **Collaboration** tab
3. Enable **"Allow Remote Control of Playback"**
4. Read and confirm the security warning
5. Click **Enable**

### Programmatic Access

```cpp
// Check if remote control is allowed
zenith::Settings& settings = zenith::Settings::getInstance();
bool allowed = settings.getAllowRemoteControl();

// Enable remote control (will trigger save)
settings.setAllowRemoteControl(true);

// Access via Engine
zenith::Engine& engine = /* ... */;
if (engine.isRemoteControlAllowed()) {
    // Remote control operations permitted
}
```

## Implementation Details

### Settings Storage

- **Key**: `allowRemoteControl`
- **Type**: Boolean
- **Default**: `false`
- **Persistence**: User-level settings file (`.settings`)
- **Location**: Platform-specific application data directory

### API Methods

#### Settings Class
```cpp
bool getAllowRemoteControl() const;
void setAllowRemoteControl(bool enabled);
```

#### Engine Class
```cpp
bool isRemoteControlAllowed() const;
```

### Thread Safety

All remote control settings access is thread-safe:
- Settings are managed by the singleton `Settings` class
- Changes trigger automatic persistence and change notifications
- Engine access is read-only and lock-free

## Security Considerations

### What Remote Control Does NOT Allow

- **File System Access**: Remote agents cannot read/write files
- **System Commands**: Remote agents cannot execute arbitrary code
- **Plugin Control**: Remote agents cannot load/unload plugins
- **Audio Routing Changes**: Remote agents cannot modify mixer/routing
- **Settings Modification**: Remote agents cannot change user preferences

### What Remote Control DOES Allow

- **Transport Control**: Play, stop, pause, seek operations
- **Playhead Position**: Read and write playhead position
- **Loop Region**: Set loop start/end points (if explicitly permitted)

### Best Practices

1. **Only enable when needed**: Disable remote control when not actively using AI or collaboration features
2. **Trust verification**: Only join collaboration sessions with trusted partners
3. **Monitor activity**: Watch for unexpected transport changes during remote sessions
4. **Revoke access**: Disable immediately if suspicious activity is detected

## Future Enhancements

Potential future improvements to the remote control system:

- Per-session authorization (allow/deny for each connection)
- Activity logging for auditing
- Fine-grained permissions (separate play/stop/seek controls)
- Temporary time-limited access grants
- Multi-user permission management for collaboration

## Related Documentation

- [Collaboration Features](./COLLABORATION.md) - Multi-user session management
- [Settings Architecture](./ARCHITECTURE.md) - Settings system design
- [Security Model](./SECURITY.md) - Overall security architecture

## Questions or Issues?

For questions about remote control or to report security concerns, please:

1. Check the [Known Issues](./KNOWN_ISSUES.md) document
2. Search existing GitHub issues
3. Open a new issue with the `security` or `collaboration` label
