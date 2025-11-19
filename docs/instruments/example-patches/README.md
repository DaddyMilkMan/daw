# Example Patches for Zenith Sampler

This directory contains example patch templates for the Zenith Sampler.

## Quick Start

1. **Copy the example patch to your content directory:**

   ```bash
   # macOS/Linux
   cp -r SimplePatch ~/Music/Zenith/Instruments/ZenithSampler/

   # Windows (PowerShell)
   Copy-Item -Path SimplePatch -Destination "$env:USERPROFILE\Music\Zenith\Instruments\ZenithSampler\" -Recurse
   ```

2. **Add your own WAV samples:**

   Place your WAV files in the `Samples/` subdirectory:

   ```bash
   cp /path/to/your/sample.wav ~/Music/Zenith/Instruments/ZenithSampler/SimplePatch/Samples/example_c3.wav
   ```

3. **Edit the .zpatch file** to match your samples:

   - Update the `file` field to match your WAV filename
   - Set the `rootNote` to the MIDI note number of the original pitch
   - Optionally set key ranges with `lowNote` and `highNote`

4. **Load in Zenith DAW:**

   - Launch Zenith DAW
   - Create an Instrument track
   - Load ZenithSampler
   - Select "SimplePatch" from the preset dropdown

## Creating Patches from Scratch

See the main documentation: [ZENITH_SAMPLER.md](../ZENITH_SAMPLER.md)

## Included Examples

### SimplePatch

A minimal single-sample patch. Good starting point for:
- Testing the sampler
- Learning the .zpatch format
- Creating your first custom instrument

**Structure:**
```
SimplePatch/
  SimplePatch.zpatch     ← Patch definition (JSON)
  Samples/
    example_c3.wav       ← Place your WAV file here (you must provide this)
```

### Creating Test Samples

If you don't have samples, you can generate a simple test tone:

**Using Sox (macOS/Linux):**
```bash
# Install sox first: brew install sox (macOS) or apt-get install sox (Linux)
sox -n -r 44100 -c 1 example_c3.wav synth 1 sine 261.63
```

**Using Python + numpy:**
```python
import numpy as np
import scipy.io.wavfile as wav

# Generate 1 second of 261.63 Hz (Middle C)
sample_rate = 44100
duration = 1.0
frequency = 261.63

t = np.linspace(0, duration, int(sample_rate * duration))
audio = np.sin(2 * np.pi * frequency * t) * 0.5
audio = (audio * 32767).astype(np.int16)

wav.write('example_c3.wav', sample_rate, audio)
```

**Using Audacity:**
1. Generate > Tone
2. Waveform: Sine
3. Frequency: 261.63 Hz
4. Duration: 1.0 seconds
5. File > Export > Export as WAV

## Advanced Examples

### Multi-Sample Patch

```json
{
  "name": "MultiSample",
  "samples": [
    {
      "file": "c2.wav",
      "rootNote": 36,
      "lowNote": 0,
      "highNote": 41
    },
    {
      "file": "c3.wav",
      "rootNote": 48,
      "lowNote": 42,
      "highNote": 53
    },
    {
      "file": "c4.wav",
      "rootNote": 60,
      "lowNote": 54,
      "highNote": 65
    },
    {
      "file": "c5.wav",
      "rootNote": 72,
      "lowNote": 66,
      "highNote": 77
    },
    {
      "file": "c6.wav",
      "rootNote": 84,
      "lowNote": 78,
      "highNote": 127
    }
  ]
}
```

### Velocity-Layered Patch

```json
{
  "name": "VelocityLayers",
  "samples": [
    {
      "file": "piano_soft.wav",
      "rootNote": 60,
      "lowNote": 0,
      "highNote": 127,
      "lowVelocity": 0,
      "highVelocity": 63
    },
    {
      "file": "piano_loud.wav",
      "rootNote": 60,
      "lowNote": 0,
      "highNote": 127,
      "lowVelocity": 64,
      "highVelocity": 127
    }
  ]
}
```

## Tips

1. **Sample Quality**: Use 16-bit or 24-bit WAV files at 44.1kHz or 48kHz
2. **File Size**: Keep individual samples under 10 seconds for best performance
3. **Naming**: Use descriptive filenames (e.g., `piano_c4_soft.wav`)
4. **Key Ranges**: Overlap key ranges slightly to avoid gaps
5. **Root Note**: Set accurately to ensure correct pitch across the keyboard

## Troubleshooting

**"No patches found"**
- Verify directory structure matches the expected format
- Ensure .zpatch filename matches directory name

**"Patch won't load"**
- Check .zpatch JSON syntax (use jsonlint.com)
- Verify all referenced WAV files exist in Samples/

**"No sound"**
- Check that WAV files contain actual audio (not silence)
- Verify sample gain is not 0.0
- Test with a generated sine wave first

## Resources

- [Main Sampler Documentation](../ZENITH_SAMPLER.md)
- [.zpatch Format Specification](../ZENITH_SAMPLER.md#zpatch-format)
- [MIDI Note Number Reference](../ZENITH_SAMPLER.md#midi-note-numbers)
