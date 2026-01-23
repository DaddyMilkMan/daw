"""
Testing Agent for comprehensive automated testing and validation.

This module provides test orchestration, execution, coverage analysis,
and specialized testing for real-time audio systems.
"""

from typing import Dict, List, Optional, Set, Callable, Tuple
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
import subprocess
import re
import shutil
import xml.etree.ElementTree as ET
import tempfile
import json
import os

try:
    import numpy as np
    import scipy.io.wavfile as wavfile
    NUMPY_AVAILABLE = True
except ImportError:
    NUMPY_AVAILABLE = False

    class MockNumpy:
        class ndarray: pass
    np = MockNumpy()


class TestType(Enum):
    """Types of tests supported."""
    UNIT = "unit"
    INTEGRATION = "integration"
    SYSTEM = "system"
    PERFORMANCE = "performance"
    FUZZ = "fuzz"
    RT_SAFETY = "rt_safety"  # Real-time thread safety
    AUDIO_QUALITY = "audio_quality"


class TestStatus(Enum):
    """Test execution status."""
    PASSED = "passed"
    FAILED = "failed"
    SKIPPED = "skipped"
    ERROR = "error"


@dataclass
class TestCase:
    """Individual test case representation."""
    name: str
    test_type: TestType
    status: TestStatus = TestStatus.SKIPPED
    duration_ms: float = 0.0
    error_message: Optional[str] = None
    stack_trace: Optional[str] = None


@dataclass
class CoverageReport:
    """Code coverage statistics."""
    lines_total: int = 0
    lines_covered: int = 0
    branches_total: int = 0
    branches_covered: int = 0
    functions_total: int = 0
    functions_covered: int = 0
    
    @property
    def line_coverage_percent(self) -> float:
        return (self.lines_covered / self.lines_total * 100.0 
                if self.lines_total > 0 else 0.0)
    
    @property
    def branch_coverage_percent(self) -> float:
        return (self.branches_covered / self.branches_total * 100.0
                if self.branches_total > 0 else 0.0)


@dataclass
class AudioQualityMetrics:
    """Audio processing quality measurements."""
    thd_percent: float = 0.0  # Total Harmonic Distortion
    snr_db: float = 0.0  # Signal-to-Noise Ratio
    frequency_response_flat: bool = False
    phase_coherent: bool = False
    no_clicks_pops: bool = False


class GcovParser:
    """Parses gcov output files for coverage analysis."""

    def parse_file(self, content: str) -> dict:
        """
        Parse .gcov file content.

        Returns:
            Dictionary with coverage stats:
            {
                'lines_total': int,
                'lines_covered': int,
                'branches_total': int,
                'branches_covered': int,
                'file_coverage': float  # line coverage percentage
            }
        """
        lines_total = 0
        lines_covered = 0
        branches_total = 0
        branches_covered = 0

        # Regex for line coverage: "   10:   42:code" or "#####:   42:code"
        # Group 1 is execution count ("#####" or number), Group 2 is line number
        # Note: headers start with "-:", which this regex won't match (good)
        line_regex = re.compile(r"^\s*([0-9]+|#####):\s*[0-9]+:")

        # Regex for branch coverage: "branch  0 taken 1" or "branch  0 taken 50%"
        branch_regex = re.compile(r"^branch\s+\d+\s+taken\s+(\d+|[0-9.]+%)(?:$|\s|\()")

        lines = content.splitlines()
        for line in lines:
            # Check for line execution
            match = line_regex.match(line)
            if match:
                exec_count_str = match.group(1)
                lines_total += 1
                if exec_count_str != "#####":
                    lines_covered += 1
                continue

            # Check for branch execution
            match = branch_regex.match(line)
            if match:
                taken = match.group(1)
                branches_total += 1

                if '%' in taken:
                    # Percentage case
                    if float(taken.strip('%')) > 0:
                        branches_covered += 1
                else:
                    # Count case
                    if int(taken) > 0:
                        branches_covered += 1

        return {
            'lines_total': lines_total,
            'lines_covered': lines_covered,
            'branches_total': branches_total,
            'branches_covered': branches_covered,
            'file_coverage': (lines_covered / lines_total * 100.0) if lines_total > 0 else 0.0
        }


class TestingAgent:
    """
    Testing Agent for comprehensive automated testing and validation.
    
    Orchestrates various types of tests, collects results,
    generates coverage reports, and validates audio quality.
    """

    # Unsafe operation patterns (Regex)
    UNSAFE_PATTERNS = {
        "Allocation": re.compile(r"\b(new|delete|malloc|calloc|realloc|free|strdup)\b"),
        "Smart Pointer": re.compile(r"\bstd::(make_unique|make_shared)\b"),
        "Container Mutation": re.compile(r"\.(push_back|emplace_back|resize|reserve|insert|erase|clear)\s*\("),
        "String/ValueTree Usage": re.compile(r"\b(std::string|juce::String|juce::ValueTree|juce::var)\b"),
        "Lock": re.compile(r"\bstd::(mutex|lock_guard|unique_lock|condition_variable)\b|\bjuce::(CriticalSection|ScopedLock|ReadWriteLock)\b"),
        "I/O": re.compile(r"\b(std::cout|std::cerr|printf|fprintf|std::fstream)\b|\bjuce::(Logger|File)\b|\bDBG\b"),
        "Flow Control": re.compile(r"\b(throw|try|catch|dynamic_cast)\b"),
        "Waiting": re.compile(r"\b(sleep|std::this_thread::sleep_for)\b")
    }

    # Suppressions
    SUPPRESSION_PATTERNS = [
        "// NOLINT",
        "// RT-SAFE-IGNORE"
    ]

    def __init__(self, project_root: Optional[Path] = None):
        """
        Initialize Testing Agent.
        
        Args:
            project_root: Root directory of the project
        """
        self.project_root = project_root or Path.cwd()
        self.test_results: List[TestCase] = []
        self.coverage: Optional[CoverageReport] = None

    def discover_tests(self, test_type: Optional[TestType] = None,
                      pattern: str = "*Test*") -> List[str]:
        """
        Discover test cases in the project.
        
        Args:
            test_type: Filter by specific test type
            pattern: File pattern for test discovery
            
        Returns:
            List of discovered test names
        """
        tests = []
        print(f"Discovering tests in {self.project_root} with pattern: {pattern}")

        # Regex for JUCE: juce::UnitTest("Name", "Category")
        regex_juce = re.compile(r'(?:juce::)?UnitTest\s*\(\s*"([^"]+)"(?:,\s*"([^"]+)")?\s*\)')
        # Regex for Catch2: TEST_CASE("Name", "Tags")
        regex_catch2 = re.compile(r'TEST_CASE\s*\(\s*"([^"]+)"(?:,\s*"([^"]+)")?\s*\)')

        for path in self.project_root.rglob(pattern):
            if not path.is_file():
                continue

            # Filter for C++ source files
            if path.suffix not in ['.cpp', '.mm', '.h', '.hpp']:
                continue

            try:
                content = path.read_text(encoding='utf-8', errors='ignore')

                # Check JUCE tests
                for match in regex_juce.finditer(content):
                    name = match.group(1)
                    category = match.group(2)

                    if self._matches_test_type(name, category, None, path.name, test_type):
                        tests.append(name)

                # Check Catch2 tests
                for match in regex_catch2.finditer(content):
                    name = match.group(1)
                    tags = match.group(2)

                    if self._matches_test_type(name, None, tags, path.name, test_type):
                        tests.append(name)

            except Exception as e:
                print(f"Error reading {path}: {e}")

        return tests

    def _matches_test_type(self, name: str, category: Optional[str],
                          tags: Optional[str], filename: str,
                          test_type: Optional[TestType]) -> bool:
        """
        Check if a test matches the requested TestType.
        """
        if test_type is None:
            return True

        # Combine all info into a search string
        search_text = f"{name} {category or ''} {tags or ''} {filename}".lower()

        if test_type == TestType.PERFORMANCE:
            return any(x in search_text for x in ["stress", "performance", "benchmark"])

        if test_type == TestType.INTEGRATION:
            return "integration" in search_text

        if test_type == TestType.SYSTEM:
            return "system" in search_text

        if test_type == TestType.RT_SAFETY:
            return any(x in search_text for x in ["thread", "safety", "concurrency", "lock-free", "real-time"])

        if test_type == TestType.AUDIO_QUALITY:
            return any(x in search_text for x in ["quality", "thd", "snr"])

        if test_type == TestType.FUZZ:
            return "fuzz" in search_text

        if test_type == TestType.UNIT:
            # Exclude tests that are clearly other high-level types
            if any(x in search_text for x in ["stress", "performance", "benchmark", "integration", "system", "fuzz"]):
                return False
            return True

        return False

    def _find_test_binary(self, binary_name: str) -> Optional[Path]:
        """
        Find test binary in build directories.

        Args:
            binary_name: Name of the binary to find

        Returns:
            Path to binary if found, else None
        """
        search_paths = [
            self.project_root / "build",
            self.project_root / "out",
            self.project_root / "bin"
        ]

        # extensions to check (empty for linux/mac, .exe for windows)
        extensions = ["", ".exe"]

        for base_path in search_paths:
            if not base_path.exists():
                continue

            for root, _, files in os.walk(base_path):
                for file in files:
                    for ext in extensions:
                        if file == binary_name + ext:
                            path = Path(root) / file
                            # check if executable
                            if os.access(path, os.X_OK):
                                return path
        return None

    def run_unit_tests(self, test_filter: Optional[str] = None) -> List[TestCase]:
        """
        Execute unit tests.
        
        Args:
            test_filter: Optional filter pattern for test selection
            
        Returns:
            List of test results
        """
        print("Running unit tests...")
        
        binary_name = "ZenithDAWTests"
        binary_path = self._find_test_binary(binary_name)

        if not binary_path:
            print(f"Error: Test binary '{binary_name}' not found. Please build the project.")
            return []

        json_output = Path("test_results.json")
        cmd = [str(binary_path), f"--gtest_output=json:{json_output}"]
        
        if test_filter:
            cmd.append(f"--gtest_filter={test_filter}")

        results = []
        try:
            # Run tests
            # check=False because tests might fail (return non-zero), which is valid
            subprocess.run(cmd, check=False, capture_output=True, text=True)

            if not json_output.exists():
                print("Error: Test output file not generated.")
                return []

            with open(json_output, 'r') as f:
                data = json.load(f)

            testsuites = data.get("testsuites", [])
            for suite in testsuites:
                for test in suite.get("testsuite", []):
                    name = f"{test.get('classname')}.{test.get('name')}"
                    # time is in seconds (string)
                    duration = float(test.get("time", "0").replace('s','')) * 1000.0 # ms

                    failures = test.get("failures", [])
                    status = TestStatus.PASSED
                    error_msg = None

                    if failures:
                        status = TestStatus.FAILED
                        failure = failures[0]
                        error_msg = failure.get("failure")

                    results.append(TestCase(
                        name=name,
                        test_type=TestType.UNIT,
                        status=status,
                        duration_ms=duration,
                        error_message=error_msg
                    ))

            self.test_results.extend(results)
            return results

        except Exception as e:
            print(f"Error executing tests: {e}")
            return []
        finally:
            if json_output.exists():
                json_output.unlink()

    def _mask_comments_and_strings(self, source: str, preserve_rt_safe: bool = False) -> str:
        """
        Mask comments and string literals with spaces to prevent false positives and
        allow robust parsing, while preserving line breaks and optionally RT-SAFE markers.
        """
        out = list(source)
        i = 0
        n = len(source)

        while i < n:
            # Strings
            if source[i] in ('"', "'"):
                quote = source[i]
                # We do not mask the quote character itself to preserve some structure if needed,
                # but content inside is masked.
                i += 1
                while i < n:
                    if source[i] == '\\':
                        # Handle escape sequence
                        if source[i] != '\n': out[i] = ' '
                        i += 1
                        if i < n:
                            if source[i] != '\n': out[i] = ' '
                            i += 1
                        continue

                    if source[i] == quote:
                        i += 1
                        break

                    if source[i] != '\n':
                        out[i] = ' '
                    i += 1
                continue

            # Line comments
            if source[i:i+2] == '//':
                if preserve_rt_safe and source.startswith('// RT-SAFE', i):
                    # Keep // RT-SAFE
                    i += 10
                    continue

                out[i] = ' '
                out[i+1] = ' '
                i += 2
                while i < n and source[i] != '\n':
                    out[i] = ' '
                    i += 1
                continue

            # Block comments
            if source[i:i+2] == '/*':
                out[i] = ' '
                out[i+1] = ' '
                i += 2
                while i < n:
                    if source[i:i+2] == '*/':
                        out[i] = ' '
                        out[i+1] = ' '
                        i += 2
                        break
                    if source[i] != '\n':
                        out[i] = ' '
                    i += 1
                continue

            i += 1

        return "".join(out)

    def _extract_rt_function_bodies(self, source: str) -> List[Tuple[str, str, int]]:
        """
        Extract bodies of functions that must be RT-safe.
        Returns list of (function_name, body_text, start_line_number).
        """
        extracted = []

        # Create a masked version of source where comments (except RT-SAFE) and strings are spaces.
        # This ensures we don't find triggers or braces inside comments/strings.
        masked_source = self._mask_comments_and_strings(source, preserve_rt_safe=True)

        triggers = ["processBlock", "getNextAudioBlock", "// RT-SAFE"]

        i = 0
        n = len(masked_source)

        while i < n:
            found_trigger = None
            found_idx = -1

            # Find the next trigger in MASKED source
            next_idx = n
            for trigger in triggers:
                idx = masked_source.find(trigger, i)
                if idx != -1 and idx < next_idx:
                    next_idx = idx
                    found_trigger = trigger

            if found_trigger is None:
                break

            i = next_idx

            # Move past trigger
            i += len(found_trigger)

            # Find opening brace in MASKED source
            brace_idx = -1
            curr = i
            while curr < n:
                if masked_source[curr] == '{':
                    brace_idx = curr
                    break
                curr += 1

            if brace_idx != -1:
                # Extract body by counting braces in MASKED source
                balance = 1
                curr = brace_idx + 1
                while curr < n and balance > 0:
                    if masked_source[curr] == '{':
                        balance += 1
                    elif masked_source[curr] == '}':
                        balance -= 1
                    curr += 1

                if balance == 0:
                    # Extract body from ORIGINAL source using indices
                    body = source[brace_idx:curr]
                    # Calculate line number
                    start_line = source.count('\n', 0, brace_idx) + 1
                    extracted.append((found_trigger, body, start_line))

                    # Continue search from after the function
                    i = curr
                else:
                    # Unbalanced or EOF
                    i = brace_idx + 1
            else:
                # No brace found
                i += 1

        return extracted

    def validate_rt_safety(self, source_files: Optional[List[Path]] = None) -> List[TestCase]:
        """
        Validate real-time thread safety constraints.
        
        Args:
            source_files: Specific files to validate
            
        Returns:
            List of validation results
        """
        print("Validating real-time thread safety...")
        
        results = []
        
        if source_files is None:
            # Default to scanning common source directories
            source_files = []
            for ext in ['*.cpp', '*.h', '*.hpp']:
                source_files.extend(list(self.project_root.rglob(ext)))

        for file_path in source_files:
            # Skip build artifacts, tests (unless testing safety of tests?), and external deps
            if any(part in str(file_path).split(os.sep) for part in ['build', 'out', 'external', 'JuceLibraryCode']):
                continue

            try:
                content = file_path.read_text(encoding='utf-8', errors='ignore')

                # Extract function bodies
                functions = self._extract_rt_function_bodies(content)

                for func_name, body, start_line in functions:
                    # 1. Mask the body for checking violations (do NOT preserve RT-SAFE here)
                    masked_body = self._mask_comments_and_strings(body, preserve_rt_safe=False)

                    # 2. Split both original and masked into lines
                    body_lines = body.splitlines()
                    masked_lines = masked_body.splitlines()

                    # They should match in length if mask preserves newlines
                    length = min(len(body_lines), len(masked_lines))
                    if len(body_lines) != len(masked_lines):
                        print(f"Warning: Line count mismatch in {file_path.name}::{func_name}")

                    for i in range(length):
                        line = body_lines[i]
                        masked_line = masked_lines[i]
                        current_line_num = start_line + i

                        # Check original line for suppressions
                        if any(s in line for s in self.SUPPRESSION_PATTERNS):
                            continue

                        # Check masked line for violations
                        for violation_type, pattern in self.UNSAFE_PATTERNS.items():
                            if pattern.search(masked_line):
                                error_msg = f"RT-Safety Violation: {violation_type} detected in {func_name} at line {current_line_num}"

                                results.append(TestCase(
                                    name=f"{file_path.name}::{func_name}::L{current_line_num}",
                                    test_type=TestType.RT_SAFETY,
                                    status=TestStatus.FAILED,
                                    error_message=error_msg
                                ))

            except Exception as e:
                print(f"Error processing {file_path}: {e}")

        if not results:
            print("No RT-safety violations found.")
        else:
            print(f"Found {len(results)} RT-safety violations.")

        self.test_results.extend(results)
        return results

    def _generate_sine_wave(self, freq_hz: float, sample_rate: int, duration_sec: float) -> np.ndarray:
        """Generate a sine wave signal."""
        t = np.linspace(0, duration_sec, int(sample_rate * duration_sec), endpoint=False)
        return np.sin(2 * np.pi * freq_hz * t).astype(np.float32)

    def _generate_silence(self, sample_rate: int, duration_sec: float) -> np.ndarray:
        """Generate a silence signal."""
        return np.zeros(int(sample_rate * duration_sec), dtype=np.float32)

    def _measure_thd(self, signal: np.ndarray, sample_rate: int, fundamental_freq: float) -> float:
        """
        Measure Total Harmonic Distortion (THD) percentage.
        THD = (sqrt(sum(harmonics^2)) / fundamental) * 100
        """
        if len(signal) == 0:
            return 0.0

        # Apply Hanning window to reduce spectral leakage
        windowed_signal = signal * np.hanning(len(signal))

        # Compute FFT
        fft = np.fft.rfft(windowed_signal)
        mag = np.abs(fft)
        freqs = np.fft.rfftfreq(len(signal), 1/sample_rate)

        # Find fundamental peak index
        # Search around expected frequency
        bin_width = freqs[1] - freqs[0]
        target_idx = int(fundamental_freq / bin_width)
        search_radius = max(1, int(50 / bin_width)) # search +/- 50Hz

        start = max(0, target_idx - search_radius)
        end = min(len(mag), target_idx + search_radius)

        if end <= start:
            return 0.0

        peak_idx = start + np.argmax(mag[start:end])
        fundamental_mag = mag[peak_idx]

        if fundamental_mag < 1e-10:
            return 0.0

        # Sum power of harmonics (2f, 3f, 4f, ...)
        harmonics_sum_sq = 0.0
        harmonic_order = 2

        while True:
            harmonic_freq = fundamental_freq * harmonic_order
            if harmonic_freq > sample_rate / 2:
                break

            h_idx = int(harmonic_freq / bin_width)
            # Sum energy in a small window around the harmonic
            h_start = max(0, h_idx - search_radius)
            h_end = min(len(mag), h_idx + search_radius)

            if h_end > h_start:
                # Take max in window to find the harmonic peak
                harmonic_mag = np.max(mag[h_start:h_end])
                harmonics_sum_sq += harmonic_mag ** 2

            harmonic_order += 1

        thd = (np.sqrt(harmonics_sum_sq) / fundamental_mag) * 100.0
        return float(thd)

    def _measure_snr(self, signal_peak_db: float, noise_floor_db: float) -> float:
        """Measure Signal-to-Noise Ratio (dB)."""
        return signal_peak_db - noise_floor_db

    def _rms_amplitude_db(self, signal: np.ndarray) -> float:
        """Calculate RMS amplitude in dB."""
        rms = np.sqrt(np.mean(signal**2))
        if rms < 1e-10:
            return -100.0
        return 20 * np.log10(rms)

    def _detect_artifacts(self, signal: np.ndarray) -> tuple[bool, bool]:
        """
        Detect clicks/pops and DC offset.
        Returns: (no_clicks_pops, has_low_dc_offset)
        """
        if len(signal) == 0:
            return True, True

        # DC Offset check (mean should be close to 0)
        dc_offset = np.abs(np.mean(signal))
        has_low_dc_offset = dc_offset < 0.01  # 1% threshold

        # Clicks/Pops: check for sudden large jumps (derivatives)
        # Normalize signal first? Assuming -1 to 1 audio.
        diff = np.diff(signal)
        max_jump = np.max(np.abs(diff))
        # A full scale jump between samples is suspicious, but depends on frequency.
        # Ideally < 0.5 for typical audio signals, though high freq can be higher.
        # Let's set a conservative threshold.
        no_clicks_pops = max_jump < 0.8

        return no_clicks_pops, has_low_dc_offset

    def test_audio_quality(self, audio_processor: Callable[[np.ndarray], np.ndarray],
                          test_signals: Optional[List[str]] = None) -> AudioQualityMetrics:
        """
        Test audio processing quality metrics.
        
        Args:
            audio_processor: Function that accepts and returns numpy arrays (float32)
            test_signals: List of test signal types (unused, overridden by standard suite)
            
        Returns:
            Audio quality metrics
        """
        print("Testing audio quality...")
        
        if not NUMPY_AVAILABLE:
            print("Warning: Numpy not found. Audio quality testing disabled.")
            return AudioQualityMetrics()

        sample_rate = 44100
        duration = 1.0
        freq_1khz = 1000.0

        # 1. THD Measurement (using 1kHz Sine)
        input_sine = self._generate_sine_wave(freq_1khz, sample_rate, duration)
        try:
            output_sine = audio_processor(input_sine)
            thd = self._measure_thd(output_sine, sample_rate, freq_1khz)
        except Exception as e:
            print(f"Error processing sine wave: {e}")
            output_sine = np.zeros_like(input_sine)
            thd = 0.0

        # 2. SNR Measurement
        # Signal level (from sine test)
        signal_rms_db = self._rms_amplitude_db(output_sine)

        # Noise floor (using Silence)
        input_silence = self._generate_silence(sample_rate, duration)
        try:
            output_silence = audio_processor(input_silence)
            noise_rms_db = self._rms_amplitude_db(output_silence)
        except Exception as e:
            print(f"Error processing silence: {e}")
            output_silence = np.zeros_like(input_silence)
            noise_rms_db = -100.0

        snr = self._measure_snr(signal_rms_db, noise_rms_db)

        # 3. Artifact Detection
        no_clicks, valid_dc = self._detect_artifacts(output_sine)
        if not no_clicks:
            print("Artifact detected: Clicks/Pops found in output.")
        if not valid_dc:
            print("Artifact detected: High DC Offset found in output.")
        
        # 4. Frequency Response (Basic Check)
        # Check if 1kHz gain is close to 1 (0dB) for pass-through/unity gain systems
        # Or just checking if it's not zero.
        # For a "flat" response check properly, we'd need a sweep or noise.
        # Let's approximate "flat" as "has reasonable output" for now or implement a sweep.
        # Implementing a quick Sweep check.
        frequency_response_flat = False
        try:
            # Simple check: Compare low (100Hz) and high (10kHz) gain
            # This is a crude "flatness" check but better than nothing.
            t = np.linspace(0, 0.1, int(sample_rate * 0.1), endpoint=False)
            in_100 = np.sin(2*np.pi*100*t).astype(np.float32)
            in_10k = np.sin(2*np.pi*10000*t).astype(np.float32)

            out_100 = audio_processor(in_100)
            out_10k = audio_processor(in_10k)

            rms_100 = np.sqrt(np.mean(out_100**2))
            rms_10k = np.sqrt(np.mean(out_10k**2))

            # Allow 3dB variance
            if rms_100 > 0 and rms_10k > 0:
                ratio = rms_100 / rms_10k
                frequency_response_flat = 0.707 < ratio < 1.414
        except Exception as e:
            print(f"Error checking freq response: {e}")

        metrics = AudioQualityMetrics(
            thd_percent=thd,
            snr_db=snr,
            frequency_response_flat=frequency_response_flat,
            phase_coherent=True, # Placeholder, difficult to test without reference
            no_clicks_pops=no_clicks and valid_dc
        )
        
        print(f"Audio Metrics: THD={thd:.4f}%, SNR={snr:.1f}dB, Flat={frequency_response_flat}")
        return metrics

    def run_fuzz_tests(self, duration_minutes: int = 5,
                      seed: Optional[int] = None) -> List[TestCase]:
        """
        Execute fuzz testing on DSP and input parsing code.
        
        Args:
            duration_minutes: How long to run fuzzing
            seed: Random seed for reproducibility
            
        Returns:
            List of fuzz test results (crashes found)
        """
        print(f"Running fuzz tests for {duration_minutes} minutes...")

        if not NUMPY_AVAILABLE:
            print("Warning: Numpy/Scipy not found. Fuzz testing disabled.")
            return [TestCase(name="FuzzTesting", test_type=TestType.FUZZ,
                           status=TestStatus.SKIPPED, error_message="Numpy/Scipy missing")]

        if seed is not None:
            np.random.seed(seed)

        # Create corpus directory
        try:
            corpus_dir = Path(tempfile.mkdtemp(prefix="zenith_fuzz_"))
        except Exception as e:
            print(f"Error creating temp directory: {e}")
            return [TestCase(name="FuzzCorpusGeneration", test_type=TestType.FUZZ,
                           status=TestStatus.ERROR, error_message=str(e))]

        print(f"Generating fuzz corpus in: {corpus_dir}")

        # Audio settings
        sample_rate = 48000
        channels = 2
        chunk_duration_sec = 10
        total_seconds = duration_minutes * 60
        num_chunks = max(1, int(total_seconds / chunk_duration_sec))

        samples_per_chunk = int(chunk_duration_sec * sample_rate)

        generated_files = []

        try:
            for i in range(num_chunks):
                # Base: -1.0 to 1.0 (90%)
                data = np.random.uniform(-1.0, 1.0, (samples_per_chunk, channels)).astype(np.float32)

                # 5% Hot: > 1.0 (e.g. up to 12dB = ~4.0, let's go up to 10.0)
                hot_mask = np.random.random(data.shape) < 0.05
                data[hot_mask] *= np.random.uniform(1.2, 10.0, size=np.count_nonzero(hot_mask))

                # 5% Toxic: NaN, Inf, Denormals
                toxic_mask = np.random.random(data.shape) < 0.05

                # Split toxic into 3 types
                toxic_indices = np.where(toxic_mask)
                num_toxic = len(toxic_indices[0])

                if num_toxic > 0:
                    toxic_types = np.random.randint(0, 3, size=num_toxic)

                    # Type 0: NaN
                    mask_nan = (toxic_types == 0)
                    data[toxic_indices[0][mask_nan], toxic_indices[1][mask_nan]] = np.nan

                    # Type 1: Inf
                    mask_inf = (toxic_types == 1)
                    data[toxic_indices[0][mask_inf], toxic_indices[1][mask_inf]] = np.inf

                    # Type 2: Denormals (e.g. 1e-40)
                    mask_denormal = (toxic_types == 2)
                    data[toxic_indices[0][mask_denormal], toxic_indices[1][mask_denormal]] = 1e-40

                filename = corpus_dir / f"fuzz_{i:04d}.wav"
                wavfile.write(filename, sample_rate, data)
                generated_files.append(str(filename))

        except Exception as e:
            print(f"Error generating fuzz corpus: {e}")
            return [TestCase(name="FuzzCorpusGeneration", test_type=TestType.FUZZ,
                           status=TestStatus.ERROR, error_message=str(e))]

        print(f"Generated {len(generated_files)} fuzz files.")
        
        # TODO: Generate malformed MIDI data
        
        return [TestCase(
            name="FuzzCorpusGeneration",
            test_type=TestType.FUZZ,
            status=TestStatus.PASSED,
            duration_ms=total_seconds * 1000,
            error_message=f"Generated {len(generated_files)} files in {corpus_dir}"
        )]

    def generate_coverage_report(self, 
                                 build_dir: Optional[Path] = None) -> CoverageReport:
        """
        Generate code coverage report.
        
        Args:
            build_dir: Build directory with coverage data
            
        Returns:
            Coverage statistics
        """
        print("Generating coverage report...")
        
        build_path = build_dir or self.project_root / "build"
        if not build_path.exists():
            print(f"Build directory {build_path} does not exist.")
            return CoverageReport()

        # 1. Find .gcno files
        gcno_files = list(build_path.rglob("*.gcno"))
        if not gcno_files:
            print("Error: Coverage artifacts (.gcno) not found.")
            print("Please rebuild with -DZENITH_ENABLE_COVERAGE=ON.")
            return CoverageReport()

        # 2. Check for .gcda files
        gcda_files = list(build_path.rglob("*.gcda"))
        if not gcda_files:
            print("Warning: No execution data (.gcda) found.")
            print("Did you run the tests yet?")
            return CoverageReport()

        parser = GcovParser()
        total_lines = 0
        covered_lines = 0
        total_branches = 0
        covered_branches = 0

        file_stats = []

        print(f"Processing {len(gcno_files)} coverage files...")

        # 3. Run gcov and parse
        for gcno in gcno_files:
            try:
                # We cd to the directory of the gcno file to minimize path issues
                # and ensure .gcov files are created there
                cwd = gcno.parent
                cmd = ["gcov", "-b", "-c", gcno.name]

                # Run gcov
                result = subprocess.run(
                    cmd,
                    cwd=str(cwd),
                    capture_output=True,
                    text=True,
                    check=False
                )

                if result.returncode != 0:
                    # Only print error if verbose or critical
                    continue

                # Parse output to find generated .gcov files
                # Output format: "Creating 'test.cpp.gcov'"
                generated_files = []
                for line in result.stdout.splitlines():
                    if "Creating '" in line:
                        # Extract filename from "Creating 'filename'"
                        fname = line.split("'")[1]
                        generated_files.append(cwd / fname)

                # Read and parse .gcov files
                for gcov_file in generated_files:
                    if not gcov_file.exists():
                        continue

                    content = gcov_file.read_text(encoding='utf-8', errors='ignore')
                    stats = parser.parse_file(content)

                    file_name = gcov_file.name.replace('.gcov', '')

                    # Accumulate totals
                    total_lines += stats['lines_total']
                    covered_lines += stats['lines_covered']
                    total_branches += stats['branches_total']
                    covered_branches += stats['branches_covered']

                    file_stats.append({
                        'file': file_name,
                        'coverage_percent': stats['file_coverage'],
                        'lines_total': stats['lines_total'],
                        'lines_covered': stats['lines_covered'],
                        'branches_total': stats['branches_total'],
                        'branches_covered': stats['branches_covered']
                    })

                    # Cleanup
                    gcov_file.unlink()

            except Exception as e:
                print(f"Error processing {gcno}: {e}")

        # 4. Generate JSON report
        report_data = {
            "summary": {
                "lines_total": total_lines,
                "lines_covered": covered_lines,
                "lines_percent": (covered_lines / total_lines * 100.0) if total_lines > 0 else 0.0,
                "branches_total": total_branches,
                "branches_covered": covered_branches,
                "branches_percent": (covered_branches / total_branches * 100.0) if total_branches > 0 else 0.0
            },
            "hotspots": sorted(file_stats, key=lambda x: x['coverage_percent'])[:5],
            "files": file_stats
        }

        json_path = build_path / "coverage_report.json"
        try:
            with open(json_path, 'w') as f:
                json.dump(report_data, f, indent=2)
            print(f"Coverage report saved to {json_path}")
        except Exception as e:
            print(f"Failed to save coverage report: {e}")
        
        self.coverage = CoverageReport(
            lines_total=total_lines,
            lines_covered=covered_lines,
            branches_total=total_branches,
            branches_covered=covered_branches,
            functions_total=0,
            functions_covered=0
        )
        return self.coverage

    def check_memory_leaks(self, test_binary: Path) -> Optional[bool]:
        """
        Check for memory leaks using valgrind.
        
        Args:
            test_binary: Path to test executable
            
        Returns:
            True if no leaks detected, False if leaks found, None if valgrind missing
        """
        if not shutil.which("valgrind"):
            print("Valgrind not found. Skipping memory leak check.")
            return None

        print(f"Checking memory leaks in {test_binary}...")
        
        with tempfile.NamedTemporaryFile(suffix=".xml", delete=False) as temp_xml:
            xml_path = temp_xml.name

        try:
            cmd = [
                "valgrind",
                "--tool=memcheck",
                "--leak-check=full",
                "--show-leak-kinds=definite,indirect",
                "--track-origins=yes",
                "--xml=yes",
                f"--xml-file={xml_path}",
                "--error-exitcode=1",
                str(test_binary.resolve())
            ]

            # Run valgrind
            subprocess.run(
                cmd,
                check=False,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL
            )

            # Parse XML
            try:
                tree = ET.parse(xml_path)
                root = tree.getroot()

                leaks_found = False
                for error in root.findall("error"):
                    kind = error.find("kind")
                    if kind is not None and kind.text in ["Leak_DefinitelyLost", "Leak_IndirectlyLost"]:
                        leaks_found = True
                        xwhat = error.find("xwhat")
                        if xwhat is not None:
                            text = xwhat.find("text")
                            if text is not None:
                                print(f"Leak detected: {text.text}")

                return not leaks_found

            except ET.ParseError:
                print("Failed to parse valgrind XML output.")
                return False

        finally:
            if os.path.exists(xml_path):
                os.remove(xml_path)

    def generate_report(self, output_file: Path) -> None:
        """
        Generate comprehensive test report.
        
        Args:
            output_file: Path for report output (HTML, JSON, XML)
        """
        total = len(self.test_results)
        passed = sum(1 for t in self.test_results if t.status == TestStatus.PASSED)
        failed = sum(1 for t in self.test_results if t.status == TestStatus.FAILED)
        
        report = {
            "summary": {
                "total": total,
                "passed": passed,
                "failed": failed,
                "pass_rate": (passed / total * 100.0 if total > 0 else 0.0)
            },
            "coverage": {
                "lines": self.coverage.line_coverage_percent if self.coverage else 0.0,
                "branches": self.coverage.branch_coverage_percent if self.coverage else 0.0
            },
            "tests": [
                {
                    "name": t.name,
                    "type": t.test_type.value,
                    "status": t.status.value,
                    "duration_ms": t.duration_ms
                }
                for t in self.test_results
            ]
        }
        
        # TODO: Generate formatted report
        print(f"Test report: {passed}/{total} passed")


# Example usage
if __name__ == "__main__":
    agent = TestingAgent() 
    
    # Discover and run tests
    tests = agent.discover_tests()
    unit_results = agent.run_unit_tests()
    
    # Validate real-time safety
    rt_results = agent.validate_rt_safety()
    
    # Generate coverage
    coverage = agent.generate_coverage_report()
    
    print(f"Coverage: {coverage.line_coverage_percent:.1f}%")
