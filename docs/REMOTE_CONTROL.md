# Remote Control of Playback

## Overview

Zenith DAW includes an opt-in security feature that controls whether external agents, collaboration sessions, or remote systems may control transport operations (play, stop, seek) for your project.

## Default Behavior

**By default, remote control is DISABLED.** This ensures that no external system can control your DAW's playback without your explicit permission.

## Security Model

The `allowRemoteControl` preference provides a security boundary between:
- Local user actions (always allowed)
- Remote/external control requests (require opt-in)

When remote control is disabled:
- Only direct user interaction with the UI or local keyboard shortcuts can control transport
- External agents, collaboration clients, or automation scripts cannot start/stop playback
- This prevents unwanted interruptions during critical sessions

When remote control is enabled:
- Trusted remote agents (e.g., AI assistants, collaboration partners) may control transport
- External systems can call play/stop/seek operations
- Use this mode only when working with trusted systems

## How to Enable/Disable

### Via Preferences UI

1. Open **Preferences** (Ctrl+, or Cmd+, on macOS)
2. Navigate to the **Collaboration** tab
3. Toggle **"Allow remote control of playback"**
4. When enabling, a confirmation dialog will appear:
   - Click **Confirm** to allow remote control
   - Click **Cancel** to keep remote control disabled
5. Changes are saved immediately and persist across sessions

### Programmatically

```cpp
// Check if remote control is allowed
if (engine.isRemoteControlAllowed()) {
    // Safe to call transport operations from remote context
    engine.play();
}

// Enable/disable via Settings
zenith::Settings::getInstance().setAllowRemoteControl(true);
bool allowed = zenith::Settings::getInstance().getAllowRemoteControl();
```

## Technical Details

### Storage

The setting is persisted in the application settings file:
- **Key**: `playback.allowRemoteControl`
- **Type**: Boolean
- **Default**: `false`
- **Location**: Platform-specific settings directory (e.g., `~/Library/Application Support/ZenithAudio/ZenithDAW.settings` on macOS)

### API Reference

#### Settings API
```cpp
// Get current state
bool zenith::Settings::getInstance().getAllowRemoteControl() const;

// Set new state (persists immediately)
void zenith::Settings::getInstance().setAllowRemoteControl(bool enabled);
```

#### Engine API
```cpp
// Check if remote control is currently allowed
bool zenith::Engine::isRemoteControlAllowed() const;
```

### Thread Safety

- `Settings::getAllowRemoteControl()` is thread-safe (reads from singleton)
- `Settings::setAllowRemoteControl()` should be called from the message thread
- `Engine::isRemoteControlAllowed()` is thread-safe

## Use Cases

### Enable Remote Control When:
- Collaborating with trusted partners in real-time
- Using AI-assisted mixing/mastering agents
- Integrating with external automation systems
- Running scripted workflows that require transport control

### Disable Remote Control When:
- Recording critical performances
- Working on confidential projects
- Unsure about external system trustworthiness
- Operating in untrusted network environments

## Best Practices

1. **Keep disabled by default** unless actively using remote features
2. **Only enable for trusted sessions** - verify the identity of remote systems
3. **Disable after collaboration** to prevent accidental remote control
4. **Audit settings regularly** if multiple users access the system

## Related Documentation

- [Collaboration Features](COLLABORATION.md)
- [Security Model](SECURITY.md)
- [Settings System](SETTINGS.md)

## Changelog

- **2026-01-11**: Initial implementation of remote control preference
