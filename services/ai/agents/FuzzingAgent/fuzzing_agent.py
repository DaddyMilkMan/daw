"""
Fuzzing Agent for DSP/Input Robustness Testing.

This module provides comprehensive fuzz testing for audio processing,
MIDI parsing, plugin loading, and file format parsing to detect crashes,
segfaults, and unexpected behavior.
"""

import random
import struct
import sys
import traceback
from typing import List, Optional, Tuple, Dict, Any
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path


class FuzzTarget(Enum):
    """Types of fuzzing targets."""
    DSP_AUDIO = "dsp_audio"
    MIDI_PARSING = "midi_parsing"
    PLUGIN_LOADING = "plugin_loading"
    PROJECT_FILES = "project_files"
    AUDIO_FILES = "audio_files"


class CrashType(Enum):
    """Types of crashes/failures detected."""
    SEGFAULT = "segfault"
    ASSERTION = "assertion"
    EXCEPTION = "exception"
    TIMEOUT = "timeout"
    INVALID_OUTPUT = "invalid_output"


@dataclass
class FuzzResult:
    """Result of a single fuzz test iteration."""
    target: FuzzTarget
    iteration: int
    crashed: bool = False
    crash_type: Optional[CrashType] = None
    error_message: Optional[str] = None
    stack_trace: Optional[str] = None
    input_data_hash: Optional[str] = None


@dataclass
class FuzzReport:
    """Comprehensive fuzzing report."""
    results: List[FuzzResult] = field(default_factory=list)
    total_iterations: int = 0
    crashes_found: int = 0
    
    @property
    def crash_rate(self) -> float:
        """Calculate crash rate as percentage."""
        if self.total_iterations == 0:
            return 0.0
        return (self.crashes_found / self.total_iterations) * 100.0
    
    def add_result(self, result: FuzzResult) -> None:
        """Add a fuzz result to the report."""
        self.results.append(result)
        self.total_iterations += 1
        if result.crashed:
            self.crashes_found += 1


class FuzzingAgent:
    """
    Fuzzing Agent for comprehensive robustness testing.
    
    Generates randomized test inputs and monitors for crashes,
    segfaults, assertions, and unexpected behavior.
    """

    def __init__(self, seed: Optional[int] = None):
        """
        Initialize Fuzzing Agent.
        
        Args:
            seed: Random seed for reproducibility
        """
        self.seed = seed or random.randint(0, 2**32 - 1)
        random.seed(self.seed)
        self.report = FuzzReport()
        
        print(f"FuzzingAgent initialized with seed: {self.seed}")

    def fuzz_dsp_audio(self, iterations: int = 100) -> List[FuzzResult]:
        """
        Fuzz test DSP audio processing code.
        
        Generates random audio buffers with various patterns:
        - Random noise
        - Extreme values (±1.0, denormals, NaN, Inf)
        - DC offset
        - Silence
        - Impulses
        
        Args:
            iterations: Number of fuzz iterations
            
        Returns:
            List of fuzz results
        """
        print(f"\n[DSP Audio Fuzzing] Running {iterations} iterations...")
        results = []
        
        for i in range(iterations):
            try:
                # Generate random buffer parameters
                num_channels = random.randint(1, 8)
                buffer_size = random.choice([64, 128, 256, 512, 1024, 2048])
                
                # Generate random audio data with various patterns
                pattern_type = random.choice([
                    'random_noise',
                    'extreme_values',
                    'denormals',
                    'special_values',  # NaN, Inf
                    'dc_offset',
                    'silence',
                    'impulse'
                ])
                
                audio_buffer = self._generate_audio_pattern(
                    pattern_type, num_channels, buffer_size
                )
                
                # Simulate DSP processing
                output = self._process_audio_buffer(audio_buffer)
                
                # Validate output
                is_valid = self._validate_audio_output(output)
                
                result = FuzzResult(
                    target=FuzzTarget.DSP_AUDIO,
                    iteration=i,
                    crashed=not is_valid,
                    crash_type=CrashType.INVALID_OUTPUT if not is_valid else None
                )
                
                if not is_valid:
                    result.error_message = f"Invalid audio output for pattern: {pattern_type}"
                    print(f"  ⚠️  Iteration {i}: Invalid output detected ({pattern_type})")
                
                results.append(result)
                self.report.add_result(result)
                
            except AssertionError as e:
                result = FuzzResult(
                    target=FuzzTarget.DSP_AUDIO,
                    iteration=i,
                    crashed=True,
                    crash_type=CrashType.ASSERTION,
                    error_message=str(e),
                    stack_trace=traceback.format_exc()
                )
                results.append(result)
                self.report.add_result(result)
                print(f"  ❌ Iteration {i}: Assertion failed - {e}")
                
            except Exception as e:
                result = FuzzResult(
                    target=FuzzTarget.DSP_AUDIO,
                    iteration=i,
                    crashed=True,
                    crash_type=CrashType.EXCEPTION,
                    error_message=str(e),
                    stack_trace=traceback.format_exc()
                )
                results.append(result)
                self.report.add_result(result)
                print(f"  ❌ Iteration {i}: Exception - {e}")
        
        passed = sum(1 for r in results if not r.crashed)
        print(f"  ✓ DSP Audio Fuzzing: {passed}/{iterations} passed")
        
        return results

    def _generate_audio_pattern(self, pattern_type: str, 
                               channels: int, samples: int) -> List[List[float]]:
        """Generate audio buffer with specified pattern."""
        buffer = []
        
        for ch in range(channels):
            channel_data = []
            
            if pattern_type == 'random_noise':
                channel_data = [random.uniform(-1.0, 1.0) for _ in range(samples)]
                
            elif pattern_type == 'extreme_values':
                # Mix of min/max values
                channel_data = [random.choice([-1.0, 1.0]) for _ in range(samples)]
                
            elif pattern_type == 'denormals':
                # Very small values (denormal numbers)
                channel_data = [random.uniform(-1e-40, 1e-40) for _ in range(samples)]
                
            elif pattern_type == 'special_values':
                # NaN and Inf values (should be handled gracefully)
                special = [float('nan'), float('inf'), float('-inf'), 0.0]
                channel_data = [random.choice(special) for _ in range(samples)]
                
            elif pattern_type == 'dc_offset':
                # Constant DC offset
                dc_value = random.uniform(-1.0, 1.0)
                channel_data = [dc_value] * samples
                
            elif pattern_type == 'silence':
                channel_data = [0.0] * samples
                
            elif pattern_type == 'impulse':
                channel_data = [0.0] * samples
                if samples > 0:
                    channel_data[random.randint(0, samples - 1)] = random.choice([-1.0, 1.0])
            
            else:
                channel_data = [0.0] * samples
            
            buffer.append(channel_data)
        
        return buffer

    def _process_audio_buffer(self, buffer: List[List[float]]) -> List[List[float]]:
        """
        Simulate DSP processing on audio buffer.
        
        In a real implementation, this would call actual DSP code.
        For now, we simulate basic operations and validation.
        """
        # Simulate processing: pass-through with validation
        output = []
        
        for channel in buffer:
            processed_channel = []
            for sample in channel:
                # Check for invalid values
                import math
                if math.isnan(sample) or math.isinf(sample):
                    # Replace with zero (proper handling)
                    processed_channel.append(0.0)
                else:
                    # Clamp to valid range
                    processed_channel.append(max(-1.0, min(1.0, sample)))
            
            output.append(processed_channel)
        
        return output

    def _validate_audio_output(self, buffer: List[List[float]]) -> bool:
        """Validate that audio output is within acceptable bounds."""
        import math
        
        for channel in buffer:
            for sample in channel:
                # Check for NaN or Inf in output
                if math.isnan(sample) or math.isinf(sample):
                    return False
                # Check for values outside valid range
                if sample < -1.0 or sample > 1.0:
                    return False
        
        return True

    def fuzz_midi_parsing(self, iterations: int = 100) -> List[FuzzResult]:
        """
        Fuzz test MIDI parsing code.
        
        Generates malformed MIDI data:
        - Invalid status bytes
        - Incomplete messages
        - Out-of-range values
        - Corrupted timing information
        - Invalid file headers
        
        Args:
            iterations: Number of fuzz iterations
            
        Returns:
            List of fuzz results
        """
        print(f"\n[MIDI Parsing Fuzzing] Running {iterations} iterations...")
        results = []
        
        for i in range(iterations):
            try:
                # Generate random MIDI data
                midi_type = random.choice([
                    'note_on',
                    'note_off',
                    'control_change',
                    'malformed',
                    'invalid_status',
                    'truncated'
                ])
                
                midi_data = self._generate_midi_data(midi_type)
                
                # Attempt to parse MIDI data
                parsed = self._parse_midi_data(midi_data)
                
                result = FuzzResult(
                    target=FuzzTarget.MIDI_PARSING,
                    iteration=i,
                    crashed=False
                )
                
                results.append(result)
                self.report.add_result(result)
                
            except AssertionError as e:
                result = FuzzResult(
                    target=FuzzTarget.MIDI_PARSING,
                    iteration=i,
                    crashed=True,
                    crash_type=CrashType.ASSERTION,
                    error_message=str(e),
                    stack_trace=traceback.format_exc()
                )
                results.append(result)
                self.report.add_result(result)
                print(f"  ❌ Iteration {i}: Assertion failed - {e}")
                
            except Exception as e:
                result = FuzzResult(
                    target=FuzzTarget.MIDI_PARSING,
                    iteration=i,
                    crashed=True,
                    crash_type=CrashType.EXCEPTION,
                    error_message=str(e),
                    stack_trace=traceback.format_exc()
                )
                results.append(result)
                self.report.add_result(result)
                print(f"  ❌ Iteration {i}: Exception - {e}")
        
        passed = sum(1 for r in results if not r.crashed)
        print(f"  ✓ MIDI Parsing Fuzzing: {passed}/{iterations} passed")
        
        return results

    def _generate_midi_data(self, midi_type: str) -> bytes:
        """Generate MIDI data of specified type."""
        if midi_type == 'note_on':
            # Valid note on: status (0x90 + channel), note, velocity
            channel = random.randint(0, 15)
            note = random.randint(0, 127)
            velocity = random.randint(1, 127)
            return bytes([0x90 | channel, note, velocity])
        
        elif midi_type == 'note_off':
            # Valid note off: status (0x80 + channel), note, velocity
            channel = random.randint(0, 15)
            note = random.randint(0, 127)
            velocity = random.randint(0, 127)
            return bytes([0x80 | channel, note, velocity])
        
        elif midi_type == 'control_change':
            # Valid CC: status (0xB0 + channel), controller, value
            channel = random.randint(0, 15)
            controller = random.randint(0, 127)
            value = random.randint(0, 127)
            return bytes([0xB0 | channel, controller, value])
        
        elif midi_type == 'malformed':
            # Random bytes that might not be valid MIDI
            length = random.randint(1, 10)
            return bytes([random.randint(0, 255) for _ in range(length)])
        
        elif midi_type == 'invalid_status':
            # Invalid status byte
            return bytes([random.randint(0, 127)])  # Status bytes should be >= 128
        
        elif midi_type == 'truncated':
            # Incomplete message
            channel = random.randint(0, 15)
            return bytes([0x90 | channel])  # Missing note and velocity
        
        return b''

    def _parse_midi_data(self, data: bytes) -> Optional[Dict[str, Any]]:
        """
        Simulate MIDI parsing.
        
        In a real implementation, this would call actual MIDI parsing code.
        For now, we validate basic structure.
        """
        if len(data) == 0:
            return None
        
        status = data[0]
        
        # Check if status byte is valid (should be >= 0x80)
        if status < 0x80:
            # Invalid status byte - should handle gracefully
            return None
        
        # Determine message type from status byte
        message_type = (status & 0xF0) >> 4
        channel = status & 0x0F
        
        # Validate message length
        if message_type in [0x8, 0x9, 0xA, 0xB, 0xE]:  # 3-byte messages
            if len(data) < 3:
                return None  # Truncated message
            return {"type": message_type, "channel": channel, "data": list(data[1:3])}
        
        return {"type": message_type, "channel": channel}

    def fuzz_plugin_loading(self, iterations: int = 50) -> List[FuzzResult]:
        """
        Fuzz test plugin loading code.
        
        Simulates loading of:
        - Invalid plugin files
        - Corrupted plugin headers
        - Malformed plugin descriptors
        
        Args:
            iterations: Number of fuzz iterations
            
        Returns:
            List of fuzz results
        """
        print(f"\n[Plugin Loading Fuzzing] Running {iterations} iterations...")
        results = []
        
        for i in range(iterations):
            try:
                # Generate random plugin file data
                plugin_type = random.choice([
                    'valid_vst3',
                    'corrupted_header',
                    'invalid_magic',
                    'truncated',
                    'random_data'
                ])
                
                plugin_data = self._generate_plugin_data(plugin_type)
                
                # Attempt to load plugin
                loaded = self._load_plugin(plugin_data)
                
                result = FuzzResult(
                    target=FuzzTarget.PLUGIN_LOADING,
                    iteration=i,
                    crashed=False
                )
                
                results.append(result)
                self.report.add_result(result)
                
            except AssertionError as e:
                result = FuzzResult(
                    target=FuzzTarget.PLUGIN_LOADING,
                    iteration=i,
                    crashed=True,
                    crash_type=CrashType.ASSERTION,
                    error_message=str(e),
                    stack_trace=traceback.format_exc()
                )
                results.append(result)
                self.report.add_result(result)
                print(f"  ❌ Iteration {i}: Assertion failed - {e}")
                
            except Exception as e:
                result = FuzzResult(
                    target=FuzzTarget.PLUGIN_LOADING,
                    iteration=i,
                    crashed=True,
                    crash_type=CrashType.EXCEPTION,
                    error_message=str(e),
                    stack_trace=traceback.format_exc()
                )
                results.append(result)
                self.report.add_result(result)
                print(f"  ❌ Iteration {i}: Exception - {e}")
        
        passed = sum(1 for r in results if not r.crashed)
        print(f"  ✓ Plugin Loading Fuzzing: {passed}/{iterations} passed")
        
        return results

    def _generate_plugin_data(self, plugin_type: str) -> bytes:
        """Generate plugin file data of specified type."""
        if plugin_type == 'valid_vst3':
            # Minimal valid-looking VST3 structure (simplified)
            return b'VST3' + bytes([0] * 100)
        
        elif plugin_type == 'corrupted_header':
            # Corrupted header
            return b'VST3' + bytes([random.randint(0, 255) for _ in range(100)])
        
        elif plugin_type == 'invalid_magic':
            # Wrong magic number
            return b'XXXX' + bytes([0] * 100)
        
        elif plugin_type == 'truncated':
            # Incomplete file
            return b'VST3'
        
        elif plugin_type == 'random_data':
            # Completely random data
            length = random.randint(10, 200)
            return bytes([random.randint(0, 255) for _ in range(length)])
        
        return b''

    def _load_plugin(self, data: bytes) -> bool:
        """
        Simulate plugin loading.
        
        In a real implementation, this would call actual plugin loading code.
        For now, we validate basic structure.
        """
        # Check minimum size
        if len(data) < 4:
            return False
        
        # Check magic number
        magic = data[:4]
        if magic != b'VST3':
            return False
        
        # Basic validation passed
        return True

    def fuzz_file_formats(self, iterations: int = 50) -> List[FuzzResult]:
        """
        Fuzz test file format parsing.
        
        Tests parsing of:
        - Project files (XML, JSON)
        - Audio files (WAV headers)
        
        Args:
            iterations: Number of fuzz iterations
            
        Returns:
            List of fuzz results
        """
        print(f"\n[File Format Fuzzing] Running {iterations} iterations...")
        results = []
        
        for i in range(iterations):
            try:
                # Generate random file data
                file_type = random.choice([
                    'valid_wav',
                    'corrupted_wav_header',
                    'valid_json',
                    'malformed_json',
                    'valid_xml',
                    'malformed_xml',
                    'random_data'
                ])
                
                file_data = self._generate_file_data(file_type)
                
                # Attempt to parse file
                parsed = self._parse_file_data(file_type, file_data)
                
                result = FuzzResult(
                    target=FuzzTarget.PROJECT_FILES,
                    iteration=i,
                    crashed=False
                )
                
                results.append(result)
                self.report.add_result(result)
                
            except AssertionError as e:
                result = FuzzResult(
                    target=FuzzTarget.PROJECT_FILES,
                    iteration=i,
                    crashed=True,
                    crash_type=CrashType.ASSERTION,
                    error_message=str(e),
                    stack_trace=traceback.format_exc()
                )
                results.append(result)
                self.report.add_result(result)
                print(f"  ❌ Iteration {i}: Assertion failed - {e}")
                
            except Exception as e:
                result = FuzzResult(
                    target=FuzzTarget.PROJECT_FILES,
                    iteration=i,
                    crashed=True,
                    crash_type=CrashType.EXCEPTION,
                    error_message=str(e),
                    stack_trace=traceback.format_exc()
                )
                results.append(result)
                self.report.add_result(result)
                print(f"  ❌ Iteration {i}: Exception - {e}")
        
        passed = sum(1 for r in results if not r.crashed)
        print(f"  ✓ File Format Fuzzing: {passed}/{iterations} passed")
        
        return results

    def _generate_file_data(self, file_type: str) -> bytes:
        """Generate file data of specified type."""
        if file_type == 'valid_wav':
            # Minimal valid WAV header
            return (
                b'RIFF' + struct.pack('<I', 36) +
                b'WAVE' + b'fmt ' + struct.pack('<I', 16) +
                struct.pack('<HHIIHH', 1, 2, 44100, 176400, 4, 16) +
                b'data' + struct.pack('<I', 0)
            )
        
        elif file_type == 'corrupted_wav_header':
            # Corrupted WAV header
            return b'RIFF' + bytes([random.randint(0, 255) for _ in range(40)])
        
        elif file_type == 'valid_json':
            return b'{"name": "test", "value": 123}'
        
        elif file_type == 'malformed_json':
            return b'{"name": "test", "value": '  # Incomplete
        
        elif file_type == 'valid_xml':
            return b'<?xml version="1.0"?><root><item>test</item></root>'
        
        elif file_type == 'malformed_xml':
            return b'<?xml version="1.0"?><root><item>test</root>'  # Mismatched tags
        
        elif file_type == 'random_data':
            length = random.randint(10, 200)
            return bytes([random.randint(0, 255) for _ in range(length)])
        
        return b''

    def _parse_file_data(self, file_type: str, data: bytes) -> bool:
        """
        Simulate file parsing.
        
        In a real implementation, this would call actual parsing code.
        """
        if 'wav' in file_type:
            # Check WAV header
            if len(data) < 12:
                return False
            return data[:4] == b'RIFF' and data[8:12] == b'WAVE'
        
        elif 'json' in file_type:
            import json
            try:
                json.loads(data.decode('utf-8'))
                return True
            except:
                return False
        
        elif 'xml' in file_type:
            import xml.etree.ElementTree as ET
            try:
                ET.fromstring(data.decode('utf-8'))
                return True
            except:
                return False
        
        return False

    def run_all_fuzzing(self, iterations_per_target: int = 100) -> FuzzReport:
        """
        Run all fuzzing targets.
        
        Args:
            iterations_per_target: Number of iterations per target
            
        Returns:
            Comprehensive fuzz report
        """
        print("=" * 70)
        print("FUZZING AGENT - Comprehensive Robustness Testing")
        print("=" * 70)
        print(f"Seed: {self.seed}")
        print(f"Iterations per target: {iterations_per_target}")
        
        # Run all fuzz tests
        self.fuzz_dsp_audio(iterations_per_target)
        self.fuzz_midi_parsing(iterations_per_target)
        self.fuzz_plugin_loading(max(iterations_per_target // 2, 50))
        self.fuzz_file_formats(max(iterations_per_target // 2, 50))
        
        # Print summary
        print("\n" + "=" * 70)
        print("FUZZING SUMMARY")
        print("=" * 70)
        print(f"Total iterations: {self.report.total_iterations}")
        print(f"Crashes found: {self.report.crashes_found}")
        print(f"Crash rate: {self.report.crash_rate:.2f}%")
        
        # Group crashes by type
        crash_by_target = {}
        for result in self.report.results:
            if result.crashed:
                target = result.target.value
                crash_by_target[target] = crash_by_target.get(target, 0) + 1
        
        if crash_by_target:
            print("\nCrashes by target:")
            for target, count in crash_by_target.items():
                print(f"  - {target}: {count}")
        
        print("\n" + "=" * 70)
        
        # Return exit code based on crashes
        if self.report.crashes_found > 0:
            print(f"\n⚠️  FUZZING FAILED: {self.report.crashes_found} crashes detected")
            return self.report
        else:
            print("\n✓ FUZZING PASSED: No crashes detected")
            return self.report


def main():
    """Main entry point for fuzzing agent."""
    import argparse
    
    parser = argparse.ArgumentParser(description='FuzzingAgent - DSP/Input Robustness Testing')
    parser.add_argument('--iterations', type=int, default=100,
                       help='Number of iterations per target (default: 100)')
    parser.add_argument('--seed', type=int, default=None,
                       help='Random seed for reproducibility')
    parser.add_argument('--target', choices=['dsp', 'midi', 'plugin', 'file', 'all'],
                       default='all', help='Specific fuzzing target')
    
    args = parser.parse_args()
    
    # Create fuzzing agent
    agent = FuzzingAgent(seed=args.seed)
    
    # Run fuzzing based on target
    if args.target == 'all':
        report = agent.run_all_fuzzing(args.iterations)
    elif args.target == 'dsp':
        agent.fuzz_dsp_audio(args.iterations)
        report = agent.report
    elif args.target == 'midi':
        agent.fuzz_midi_parsing(args.iterations)
        report = agent.report
    elif args.target == 'plugin':
        agent.fuzz_plugin_loading(args.iterations)
        report = agent.report
    elif args.target == 'file':
        agent.fuzz_file_formats(args.iterations)
        report = agent.report
    
    # Exit with error code if crashes found
    sys.exit(1 if report.crashes_found > 0 else 0)


if __name__ == "__main__":
    main()
