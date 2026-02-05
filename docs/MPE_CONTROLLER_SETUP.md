# MPE Controller Setup Guide

## ROLI Seaboard Setup

### Seaboard Block

1. **Connect via USB**
   - Use included USB cable
   - Wait for driver installation (automatic on most systems)

2. **Configure in Zenith DAW**
   - Open Settings → MIDI → MPE Configuration
   - Select "ROLI Seaboard Block" preset
   - Settings:
     - Lower Zone: Enabled
     - Master Channel: 1
     - Member Channels: 15

3. **Verify Connection**
   - Play notes on the Seaboard
   - Check for MIDI input indicator in DAW
   - Test pressure, slide, and pitchbend

4. **Calibration** (Optional)
   - Use ROLI Dashboard for firmware updates
   - Adjust sensitivity curves
   - Customize touch response

### Seaboard Rise 25/49

1. **Connection**
   - USB connection recommended (lowest latency)
   - Bluetooth available (may have latency)

2. **Configuration**
   - Select "ROLI Seaboard Rise" preset
   - Same zone settings as Block

3. **Advanced Features**
   - Configure in ROLI Dashboard:
     - Strike sensitivity
     - Lift threshold
     - Glide sensitivity
     - Press depth

## LinnStrument Setup

1. **Connection**
   - USB cable to computer
   - Power adapter (optional, bus-powered via USB)

2. **Configure LinnStrument**
   - Press **Global Settings** button
   - Use **Split** button to set to single zone (lower)
   - Set **Channel Per Note** setting
   - Save settings

3. **Configure in Zenith DAW**
   - Select "LinnStrument" preset
   - Lower Zone: Enabled
   - Master Channel: 1
   - Member Channels: 15

4. **Firmware Updates**
   - Download from LinnStrument website
   - Follow update instructions carefully
   - Restore settings after update

5. **Custom Settings**
   - Adjust touch sensitivity
   - Set pressure curve
   - Configure X/Y/Z axis behavior

## K-Board Setup

1. **Connection**
   - USB connection
   - Compatible with USB-C adapters

2. **Configuration**
   - Select "K-Board" preset
   - Uses both lower and upper zones:
     - Lower: Channels 1-8
     - Upper: Channels 9-16

3. **Unique Features**
   - Continuous controller surface
   - No moving parts
   - Velocity-sensitive
   - Aftertouch-enabled

## Ableton Push 2 Setup

1. **Connection**
   - USB connection
   - Requires InControl mode

2. **Enable InControl**
   - Press User button + Delete (factory reset)
   - Wait for "InControl" message

3. **Configuration**
   - Select "Ableton Push 2" preset
   - Custom channel mapping required
   - May need to adjust zones based on use case

4. **Limitations**
   - Not a native MPE controller
   - Limited per-note expression
   - Best for step sequencing, not expressive performance

## General MIDI Controllers

### Checking MPE Support

1. **Check Documentation**
   - Look for "MPE" or "MPE-compatible"
   - Check manufacturer website
   - Verify MPE implementation

2. **Test MPE**
   - Try MPE preset in Zenith DAW
   - Test each expression type
   - Verify per-note control

3. **Manual Configuration**
   - If no preset exists:
     - Identify zone structure
     - Set master channel
     - Set member channel count
     - Test with actual performance

### Non-MPE Controllers

Even without MPE support, you can still use:

1. **Global MIDI CC**
   - Use standard MIDI learn
   - Map CC to parameters
   - Single channel operation

2. **Channel Aftertouch**
   - Some keyboards support channel aftertouch
   - Maps to pressure for all notes
   - Not per-note, but still expressive

3. **Pitchbend Wheel**
   - Traditional pitchbend
   - Affects all notes on channel
   - Can be recorded to automation

## Troubleshooting Setup Issues

### Device Not Recognized

1. **Check Connections**
   - Verify USB cable is securely connected
   - Try different USB port
   - Test with powered hub if needed

2. **Driver Installation**
   - Install manufacturer drivers
   - Check Device Manager (Windows)
   - Check Audio/MIDI Setup (Mac)

3. **DAW Settings**
   - Enable MIDI input in DAW preferences
   - Check device is not filtered
   - Verify device is not in use by another app

### MPE Not Working

1. **Verify MPE Mode**
   - Check device is in MPE mode
   - Some devices require enabling MPE
   - Reset to factory defaults if needed

2. **Zone Configuration**
   - Verify zone settings match device
   - Check master channel assignment
   - Confirm member channel count

3. **Firmware Updates**
   - Update device firmware
   - Update DAW to latest version
   - Restart both after update

### Latency Issues

1. **Reduce Buffer Size**
   - Lower audio buffer in DAW
   - Use USB 2.0 or higher
   - Disable other USB devices

2. **Optimization**
   - Close other applications
   - Disable Wi-Fi/Bluetooth
   - Use high-performance power mode

3. **Direct Connection**
   - Avoid USB hubs if possible
   - Use dedicated USB controller
   - Disconnect unnecessary peripherals

## Advanced Configuration

### Custom Zone Setup

For unique controller configurations:

1. **Identify Channel Layout**
   - Determine which channels device uses
   - Note overlap restrictions
   - Check for fixed master channels

2. **Configure Manually**
   - Set lower/upper zone channels
   - Adjust member channel counts
   - Test with actual performance

3. **Save as Preset**
   - Name your configuration
   - Document for future use
   - Share with community if useful

### Multiple Controllers

Using multiple MPE controllers:

1. **Separate Zones**
   - Assign each controller to different zone
   - Avoid channel conflicts
   - Test simultaneous use

2. **Layering**
   - Same MIDI channel for layered sounds
   - Different zones for independent control
   - Configure MIDI routing as needed

3. **Performance Tips**
   - Label controllers clearly
   - Practice switching between them
   - Use consistent configurations

## Resources

### Manufacturer Resources

- **ROLI**: [roli.com/support](https://roli.com/support)
- **LinnStrument**: [linnstrument.com/support](https://www.linnstrument.com/)
- **K-Board**: [keithmcmillen.com/support](https://www.keithmcmillen.com/)

### Community Resources

- **MPE Forum**: [midi.org/forum](https://www.midi.org/forum)
- **Zenith DAW Community**: [github.com/zenith-daw](https://github.com/zenith-daw)
- **Reddit**: r/midiproduction, r/synthrecipes

### Tutorials

- **ROLI Learning**: [roli.com/learning](https://roli.com/learning)
- **LinnStrument Videos**: [YouTube channel](https://www.youtube.com/user/rogerlinndesign)
- **MPE Performance**: Search YouTube for "MPE performance"

## Getting Help

If you encounter issues:

1. **Check Documentation**
   - Controller manual
   - Zenith DAW manual
   - This setup guide

2. **Community Forums**
   - Post details of your setup
   - Include controller model
   - Describe symptoms clearly

3. **Support**
   - Contact Zenith DAW support
   - Include system information
   - Provide diagnostic logs if requested
