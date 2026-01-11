# Remote Control of Playback

## Overview

Zenith DAW includes an opt-in setting to allow external agents or remote collaboration sessions to control playback (play/stop/seek operations). This feature is **disabled by default** for security and privacy reasons.

## Default Behavior

- **Default state:** Remote control is **disabled**
- **Access control:** Remote commands to control playback are denied unless this setting is explicitly enabled
- **Persistence:** The setting is stored in the application configuration and persists across sessions

## Enabling Remote Control

### Via Preferences UI

1. Open **Preferences** (Ctrl+, or via menu)
2. Navigate to the **Collaboration** tab
3. Toggle **"Allow remote control of playback"**
4. Confirm the security warning dialog

### Confirmation Dialog

When enabling remote control, you will see a confirmation dialog:

> **Enable Remote Control**
> 
> Enabling remote control allows connected agents to control playback. Only enable for trusted sessions.
> 
> Do you want to continue?

This ensures users are aware of the implications before enabling the feature.

## Security Considerations

- **Trust:** Only enable remote control when working with trusted agents or collaboration partners
- **Session Control:** Remote control applies to the current project/session
- **Disable When Done:** Consider disabling remote control after your collaboration session ends
- **No Automatic Elevation:** Remote control permissions never escalate automatically

## Technical Implementation

### Configuration Storage

The setting is stored as:
- **Key:** `playback.allowRemoteControl`
- **Type:** Boolean
- **Default:** `false`
- **Location:** Application settings file (managed by `Settings` class)

### Runtime Access

Other systems can check the current permission state at runtime:

```cpp
// Check if remote control is allowed
bool allowed = engine.isRemoteControlAllowed();

if (allowed) {
    // Process remote playback command
} else {
    // Deny remote playback command
}
```

### Settings API

```cpp
// Get current setting
bool allowed = Settings::getInstance().getAllowRemoteControl();

// Set setting (also persists automatically)
Settings::getInstance().setAllowRemoteControl(true);
```

## Use Cases

Remote control is useful for:

1. **AI Assistants:** Allowing AI agents to control playback during automated workflows
2. **Collaboration:** Enabling remote collaborators to control the transport
3. **External Controllers:** Integrating with external hardware or software controllers
4. **Automated Testing:** Allowing test harnesses to control playback programmatically

## Disabling Remote Control

To disable remote control:

1. Open **Preferences → Collaboration**
2. Toggle off **"Allow remote control of playback"**

No confirmation is required when disabling the feature.

## Best Practices

- ✅ Enable only when actively collaborating or using AI assistants
- ✅ Disable when working solo or with sensitive projects
- ✅ Review who has access to your collaboration session before enabling
- ❌ Don't leave remote control enabled permanently unless necessary
- ❌ Don't enable for untrusted or unknown agents

## Related Documentation

- [Collaboration Features](./COLLABORATION.md) (if available)
- [AI Integration Guide](./ai/README_GROK_INTEGRATION.md)
- [Security Guidelines](./SECURITY.md) (if available)

---

**Version:** 1.0  
**Last Updated:** 2026-01-11
