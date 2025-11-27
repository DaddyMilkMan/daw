import os
import struct

def create_wav(filename):
    # Minimal WAV header (44 bytes)
    # RIFF chunk
    header = b'RIFF'
    header += struct.pack('<I', 36 + 1000) # Chunk size (36 + data size)
    header += b'WAVE'
    
    # fmt chunk
    header += b'fmt '
    header += struct.pack('<I', 16) # Subchunk1Size (16 for PCM)
    header += struct.pack('<H', 1)  # AudioFormat (1 for PCM)
    header += struct.pack('<H', 1)  # NumChannels (1 for Mono)
    header += struct.pack('<I', 44100) # SampleRate
    header += struct.pack('<I', 44100 * 2) # ByteRate (SampleRate * NumChannels * BitsPerSample/8)
    header += struct.pack('<H', 2) # BlockAlign (NumChannels * BitsPerSample/8)
    header += struct.pack('<H', 16) # BitsPerSample
    
    # data chunk
    header += b'data'
    header += struct.pack('<I', 1000) # Subchunk2Size (data size)
    
    # Dummy data (silence or noise)
    # Dummy data (noise)
    import random
    data = bytearray(1000)
    for i in range(1000):
        data[i] = random.randint(0, 255)
    data = bytes(data)
    
    with open(filename, 'wb') as f:
        f.write(header)
        f.write(data)

base_dir = r'C:\zenith\daw\zenith-core\Content\Instruments\ZenithSampler\Samples'
if not os.path.exists(base_dir):
    os.makedirs(base_dir)

files = [
    '808-kick.wav', '808-snare.wav', '808-hihat-closed.wav', '808-hihat-open.wav', '808-bass.wav',
    'lofi-piano-C3.wav', 'lofi-piano-C4.wav', 'lofi-piano-C5.wav',
    'lofi-piano-C3-hard.wav', 'lofi-piano-C4-hard.wav', 'lofi-piano-C5-hard.wav',
    'fx-riser.wav', 'fx-impact.wav', 'fx-reverse.wav', 'fx-whoosh.wav'
]

for f in files:
    create_wav(os.path.join(base_dir, f))
    print(f"Created {f}")
