# MPE (MIDI Polyphonic Expression) User Guide

## Overview

MPE (MIDI Polyphonic Expression) is a standard for expressive MIDI control that allows per-note expression parameters. Unlike traditional MIDI where pitchbend, pressure, and timbre affect the entire channel, MPE allows each note to have independent expression control.

## What is MPE?

MPE extends MIDI by assigning multiple channels to a single instrument, where each note gets its own channel for independent expression control:

- **PitchBend**: Per-note pitch bend (typically ±48 semitones)
- **Pressure**: Per-note aftertouch/pressure (0-127)
- **Timbre**: Per-note timbre control via CC74 (0-127)
- **Expression**: Expression pedal via CC11 (0-127)

## MPE Zones

MPE uses zones to organize channels:

### Lower Zone
- Uses channels starting from channel 1
- Channel 1: Master channel (global settings)
- Channels 2-N: Member channels (individual notes)

### Upper Zone
- Uses channels ending at channel 16
- Channel 16: Master channel (global settings)
- Channels (16-N) to 15: Member channels (individual notes)

## Expression Lanes in Zenith DAW

### Viewing Expression Lanes

1. Open the piano roll editor
2. Click the lane visibility button (⚙) in the toolbar
3. Select which expression lanes to display:
   - **PITCH**: Note pitchbend
   - **PRESSURE**: Note aftertouch
   - **SLIDE**: Note timbre (CC74)
   - **EXPRESSION**: Expression pedal (CC11)

### Editing Expression Automation

#### Adding Points
- **Click** in an expression lane to add a new automation point
- Points are added at the clicked position
- Value is determined by vertical position (0.0 at bottom, 1.0 at top)

#### Editing Points
- **Drag** points to change their time and value
- **Shift+Click** to multi-select points
- **Backspace/Delete** to delete selected points

#### Adjusting Curve Tension
- **Alt+Drag** vertically on a point to adjust curve tension
- Tension affects the curve shape between points:
  - **0.0**: Linear (straight line)
  - **+1.0**: Curve upward (convex)
  - **-1.0**: Curve downward (concave)

### Real-Time Visual Feedback

During MPE performance, notes display real-time feedback:

- **Glowing edges**: Pressure intensity
- **Circle indicators**: Pressure amount
- **Color shifts**: Timbre changes
- **Position shifts**: Pitchbend amount

## Recording MPE

### Enabling MPE Recording

1. Open the piano roll
2. Click the **Record Arm** button on expression lanes
3. Enable recording for desired expression types
4. Press record in the transport

### Recording Process

1. Arm expression lanes for recording
2. Start transport recording
3. Perform on your MPE controller
4. Stop recording
5. Automation data is automatically quantized to grid

### Recording Options

- **Quantize**: Snap recorded points to grid
- **Strength**: How tightly to quantize (0.0-1.0)
- **Filter**: Minimum change threshold to record

## MPE Controllers

### Supported Controllers

Zenith DAW includes presets for:

- **ROLI Seaboard Block**: 15 member channels, lower zone
- **ROLI Seaboard Rise**: 15 member channels, lower zone
- **LinnStrument**: 15 member channels, lower zone
- **K-Board**: 7 member channels per zone, both zones
- **Ableton Push 2**: Custom configuration

### Setting Up MPE Controllers

1. Connect your MPE controller via USB/MIDI
2. Open **Settings → MIDI → MPE Configuration**
3. Select your controller from the preset dropdown
4. Adjust zone settings if needed
5. Click **Apply**

See the [Controller Setup Guide](CONTROLLER_SETUP.md) for detailed instructions.

## Best Practices

### Performance Tips

- Use pressure for dynamic control (volume, brightness)
- Use slide for filter cutoff or timbre changes
- Use pitchbend sparingly for pitch effects
- Combine expressions for organic, evolving sounds

### Automation Tips

- Start with fewer points, add more as needed
- Use tension to smooth between points
- Quantize after recording for cleaner automation
- Draw automation for repetitive patterns

### Sound Design Tips

- Assign pressure to filter cutoff
- Assign slide to oscillator detune or wavetable position
- Assign pitchbend to vibrato LFO amount
- Use expression pedal for global parameters

## Troubleshooting

### MPE Not Working

1. Check MIDI device is connected
2. Verify MPE is enabled in device settings
3. Check zone configuration matches your device
4. Ensure expression lanes are visible

### No Automation Recorded

1. Verify record arm is enabled on lanes
2. Check transport is recording
3. Confirm MIDI messages are being received
4. Check channel assignment matches zone config

### Expression Not Affecting Sound

1. Verify instrument supports MPE
2. Check expression mapping in instrument
3. Confirm expression lanes have automation data
4. Check automation playback is enabled

## Keyboard Shortcuts

- **Alt+Click**: Add tension edit mode
- **Shift+Click**: Multi-select points
- **Backspace/Delete**: Delete selected points
- **Ctrl+Z**: Undo
- **Ctrl+Y**: Redo
- **Ctrl+A**: Select all points in lane

## Additional Resources

- [MPE Specification](https://www.midi.org/midi-1/midi-polyphonic-expression-mpe)
- [Controller Setup Guide](CONTROLLER_SETUP.md)
- [Expression Lane Editing](#editing-expression-automation)
- [Real-Time Feedback](#real-time-visual-feedback)
