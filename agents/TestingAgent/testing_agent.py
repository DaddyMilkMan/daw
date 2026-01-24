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
    import scipy.signal as signal
    import scipy.fft as fft
    NUMPY_AVAILABLE = True
except ImportError:
    NUMPY_AVAILABLE = False
    from typing import Any
    class MockNumpy:
        ndarray = Any
        def __getattr__(self, _): return None
    np = MockNumpy()
    # Mock scipy submodules if needed, or rely on NUMPY_AVAILABLE checks
    signal = MockNumpy()
    fft = MockNumpy()


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
        line_regex = re.compile(r"^\s*([0-9]+|#####):\s*([0-9]+):")

        # Regex for branch coverage: "branch  0 taken 1" or "branch  0 taken 50%"
        branch_regex = re.compile(r"^branch\s+\d+\s+taken\s+(\d+|[0-9.]+%)(?:$|\s|\()")

        # Regex for source file path in header: "-:    0:Source:/path/to/file.cpp"
        source_regex = re.compile(r"^\s*-:\s*0:Source:(.*)$")

        lines_data = {} # line_num -> count
        source_path = None

        lines = content.splitlines()
        for line in lines:
            # Check for source path (usually at top)
            if not source_path:
                src_match = source_regex.match(line)
                if src_match:
                    source_path = src_match.group(1).strip()

            # Check for line execution
            match = line_regex.match(line)
            if match:
                exec_count_str = match.group(1)
                line_num = int(match.group(2))

                count = 0
                if exec_count_str != "#####":
                    count = int(exec_count_str)
                    lines_covered += 1

                lines_data[line_num] = count
                lines_total += 1
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
            'source_path': source_path,
            'lines_total': lines_total,
            'lines_covered': lines_covered,
            'branches_total': branches_total,
            'branches_covered': branches_covered,
            'file_coverage': (lines_covered / lines_total * 100.0) if lines_total > 0 else 0.0,
            'lines_data': lines_data
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
        "Smart Pointer": re.compile(r"\bstd::(make_unique|make_shared|allocate_shared)\b"),
        "Heavy Type": re.compile(r"\bstd::(function|any|variant)\b"),
        "Container Mutation": re.compile(r"\.(push_back|emplace_back|resize|reserve|insert)\s*\("),
        "String Usage": re.compile(r"\b(std::string|juce::String)\b"),
        "JUCE Object": re.compile(r"\bjuce::(Array|OwnedArray|HashMap|ReferenceCountedObjectPtr)\b"),
        "Lock": re.compile(r"\bstd::(mutex|lock_guard|unique_lock|condition_variable)\b|\bjuce::(CriticalSection|ScopedLock|MessageManagerLock)\b"),
        "I/O": re.compile(r"\b(std::cout|std::cerr|printf|fprintf|std::fstream|fopen|fdopen)\b|\bjuce::(Logger|File)\b|\bDBG\b"),
        "System Call": re.compile(r"\b(open|read|write|socket|recv|send)\s*\("),
        "Formatting": re.compile(r"\b(std::format|fmt::format)\b|\.formatted\s*\(|\.toStdString\s*\("),
        "Flow Control": re.compile(r"\b(throw|try|catch|dynamic_cast)\b"),
        "Waiting": re.compile(r"\b(sleep|std::this_thread::sleep_for|std::atomic_wait)\b|\bwait\s*\(")
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

    def _extract_rt_function_bodies(self, source: str) -> List[Tuple[str, str, int, str]]:
        """
        Extract bodies of functions that must be RT-safe.
        Returns list of (function_name, body_text, start_line_number, signature_suffix).
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
                    # Extract signature (text between trigger and body start)
                    signature = source[i:brace_idx]

                    # Calculate line number
                    start_line = source.count('\n', 0, brace_idx) + 1
                    extracted.append((found_trigger, body, start_line, signature))

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

                for func_name, body, start_line, signature in functions:
                    # Check for noexcept in signature
                    # We mask comments/strings in signature to avoid false positives (e.g. noexcept in comment)
                    masked_signature = self._mask_comments_and_strings(signature, preserve_rt_safe=False)

                    # Only check the last part of the signature after any potential intervening declarations
                    relevant_signature = masked_signature.rpartition(';')[2]

                    if "noexcept" not in relevant_signature:
                        results.append(TestCase(
                            name=f"{file_path.name}::{func_name}::Signature",
                            test_type=TestType.RT_SAFETY,
                            status=TestStatus.FAILED,
                            error_message=f"RT-Safety Violation: Missing 'noexcept' specifier in {func_name}"
                        ))

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
                            match = pattern.search(masked_line)
                            if match:
                                matched_text = match.group(0).strip()
                                error_msg = f"RT-Safety Violation: {violation_type} detected ('{matched_text}') in {func_name} at line {current_line_num}"

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

    def _align_signals(self, ref: np.ndarray, deg: np.ndarray) -> np.ndarray:
        """
        Align degraded signal (deg) to reference signal (ref) using cross-correlation.
        Returns aligned version of deg, trimmed/padded to match ref length.
        """
        if len(ref) == 0 or len(deg) == 0:
            return deg

        # Use FFT-based correlation for speed
        correlation = signal.correlate(deg, ref, mode='full', method='fft')
        lags = signal.correlation_lags(len(deg), len(ref), mode='full')
        lag = lags[np.argmax(correlation)]

        if lag > 0:
            # deg is delayed relative to ref (common case)
            # e.g., lag=100 means deg[100] matches ref[0]
            aligned = deg[lag:]
        elif lag < 0:
            # deg is advanced relative to ref
            # e.g., lag=-100 means deg[0] matches ref[100]
            aligned = np.pad(deg, (-lag, 0))
        else:
            aligned = deg

        # Ensure length matches ref
        if len(aligned) > len(ref):
            aligned = aligned[:len(ref)]
        elif len(aligned) < len(ref):
            aligned = np.pad(aligned, (0, len(ref) - len(aligned)))

        return aligned

    def _generate_sine_wave(self, freq_hz: float, sample_rate: int, duration_sec: float) -> np.ndarray:
        """Generate a sine wave signal."""
        t = np.linspace(0, duration_sec, int(sample_rate * duration_sec), endpoint=False)
        return np.sin(2 * np.pi * freq_hz * t).astype(np.float32)

    def _generate_silence(self, sample_rate: int, duration_sec: float) -> np.ndarray:
        """Generate a silence signal."""
        return np.zeros(int(sample_rate * duration_sec), dtype=np.float32)

    def _measure_spectral_metrics(self, signal_in: np.ndarray, sample_rate: int, fundamental_freq: float) -> Tuple[float, float]:
        """
        Measure THD (%) and SNR (dB) from a single tone signal using robust spectral analysis.

        Returns:
            (thd_percent, snr_db)
        """
        if len(signal_in) == 0:
            return 0.0, 0.0

        # Use Blackman-Harris window for superior sidelobe rejection (needed for high SNR/low THD measurement)
        window = signal.windows.blackmanharris(len(signal_in))
        # Compensate for coherent gain of the window?
        # For ratio metrics (THD, SNR), gain cancels out, so raw windowing is fine.
        windowed = signal_in * window

        # Compute FFT
        X = fft.rfft(windowed)
        mag = np.abs(X)
        freqs = fft.rfftfreq(len(signal_in), 1/sample_rate)

        bin_width = freqs[1] - freqs[0]
        if bin_width <= 0:
            return 0.0, 0.0

        # Helper to sum power in a band
        def get_band_power(center_idx, radius):
            s = max(0, center_idx - radius)
            e = min(len(mag), center_idx + radius + 1)
            return np.sum(mag[s:e]**2)

        # 1. Find Fundamental
        target_idx = int(round(fundamental_freq / bin_width))
        search_radius = max(2, int(50 / bin_width)) # +/- 50Hz search

        start = max(0, target_idx - search_radius)
        end = min(len(mag), target_idx + search_radius)

        if start >= end:
            return 0.0, 0.0

        peak_idx = start + np.argmax(mag[start:end])

        # Main lobe radius: Blackman-Harris main lobe is wider (~4 bins).
        # Use 5 bins radius to capture most energy.
        lobe_radius = 5

        fund_power = get_band_power(peak_idx, lobe_radius)
        if fund_power < 1e-15:
            return 0.0, 0.0

        # 2. Identify Harmonics
        harm_power = 0.0
        harmonic_order = 2

        # Keep track of bins used by signal (fundamental + harmonics) to exclude from noise
        used_indices = set(range(max(0, peak_idx - lobe_radius), min(len(mag), peak_idx + lobe_radius + 1)))

        while True:
            h_freq = fundamental_freq * harmonic_order
            if h_freq >= sample_rate / 2:
                break

            h_idx = int(round(h_freq / bin_width))

            # Refine peak position
            s = max(0, h_idx - search_radius)
            e = min(len(mag), h_idx + search_radius)

            if s < e:
                local_peak = s + np.argmax(mag[s:e])
                h_p = get_band_power(local_peak, lobe_radius)
                harm_power += h_p

                # Mark used bins
                s_u = max(0, local_peak - lobe_radius)
                e_u = min(len(mag), local_peak + lobe_radius + 1)
                for i in range(s_u, e_u):
                    used_indices.add(i)

            harmonic_order += 1

        # 3. Calculate THD
        thd = 100.0 * np.sqrt(harm_power) / np.sqrt(fund_power)

        # 4. Calculate Noise (SNR)
        # Sum all energy NOT in fundamental or harmonics (and ignore DC)
        noise_power = 0.0

        # Exclude DC (0 and maybe 1)
        used_indices.add(0)

        # We can optimize this loop
        all_indices = np.arange(len(mag))
        # Create mask
        noise_mask = np.ones(len(mag), dtype=bool)
        noise_mask[list(used_indices)] = False

        noise_power = np.sum(mag[noise_mask]**2)

        if noise_power < 1e-15:
            snr = 100.0 # Cap SNR at 100dB to avoid Inf
        else:
            snr = 10 * np.log10(fund_power / noise_power)

        return float(thd), float(snr)

    def _rms_amplitude_db(self, signal: np.ndarray) -> float:
        """Calculate RMS amplitude in dB."""
        rms = np.sqrt(np.mean(signal**2))
        if rms < 1e-10:
            return -100.0
        return 20 * np.log10(rms)

    def _measure_frequency_response(self, audio_processor: Callable[[np.ndarray], np.ndarray],
                                  sample_rate: int) -> bool:
        """
        Measure frequency response flatness using Log Sine Sweep.
        Returns True if flat within +/- 3dB from 20Hz to 20kHz.
        """
        duration = 0.5
        # Log sweep 20Hz to 20kHz
        t = np.linspace(0, duration, int(sample_rate * duration), endpoint=False)

        # Logarithmic chirp
        # Note: input_sweep magnitude is 1.0
        input_sweep = signal.chirp(t, f0=20, f1=20000, t1=duration, method='logarithmic')

        # Apply gentle fade in/out to avoid FFT edge artifacts
        window = signal.windows.tukey(len(input_sweep), alpha=0.1)
        input_sweep *= window

        # Add silence padding for latency/decay
        padding_samples = int(sample_rate * 0.2)
        input_sweep = np.pad(input_sweep, (0, padding_samples))

        try:
            output_sweep = audio_processor(input_sweep.astype(np.float32))
        except Exception as e:
            print(f"Error processing sweep: {e}")
            return False

        # Align output to input
        output_aligned = self._align_signals(input_sweep, output_sweep)

        # Compute Transfer Function H(f) = Y(f) / X(f)
        # Use a large FFT size covering the whole sweep
        n_fft = 1 << (len(input_sweep) - 1).bit_length() # Next power of 2

        # Pad to n_fft
        input_padded = np.pad(input_sweep, (0, n_fft - len(input_sweep)))
        # Pad output similarly (it might be shorter after alignment/trimming)
        if len(output_aligned) < n_fft:
            output_padded = np.pad(output_aligned, (0, n_fft - len(output_aligned)))
        else:
            output_padded = output_aligned[:n_fft]

        X = fft.rfft(input_padded)
        Y = fft.rfft(output_padded)

        freqs = fft.rfftfreq(n_fft, 1/sample_rate)

        # Analyze only 20Hz - 20kHz
        mask = (freqs >= 20) & (freqs <= 20000)

        if not np.any(mask):
            print("Error: No frequencies in range 20-20000Hz")
            return False

        # Avoid division by zero
        denom = np.abs(X[mask])
        denom[denom < 1e-12] = 1e-12

        H_mag = np.abs(Y[mask]) / denom

        # Convert to dB
        # Add small epsilon to avoid log(0)
        mag_db = 20 * np.log10(H_mag + 1e-12)

        # Normalize to mean (center around 0dB deviation)
        mean_db = np.mean(mag_db)
        mag_db_norm = mag_db - mean_db

        # Check flatness (tolerance +/- 3dB)
        min_dev = np.min(mag_db_norm)
        max_dev = np.max(mag_db_norm)

        is_flat = (min_dev > -3.0) and (max_dev < 3.0)

        if not is_flat:
             print(f"Freq Response Deviation: {min_dev:.2f}dB to {max_dev:.2f}dB")

        return is_flat

    def _detect_artifacts(self, signal_in: np.ndarray) -> tuple[bool, bool]:
        """
        Detect clicks/pops and DC offset.
        Assumes input is a test signal (Sine/Silence) and NOT high-frequency full-scale noise.

        Returns: (no_clicks_pops, has_low_dc_offset)
        """
        if len(signal_in) == 0:
            return True, True

        # 1. DC Offset check
        # Use simple mean. For integer number of cycles, mean is 0.
        # For large N, mean approximates DC.
        dc_offset = np.abs(np.mean(signal_in))
        # Threshold: 0.01 (-40dB)
        has_low_dc_offset = dc_offset < 0.01

        # 2. Clicks/Pops (Discontinuities)
        # Use first difference (derivative)
        diff = np.diff(signal_in)
        max_jump = np.max(np.abs(diff))

        # Threshold: 0.5 (half of full scale range per sample)
        # A 1kHz sine at 0dBFS has max slope ~0.14/sample at 44.1kHz.
        # A 0.5 jump implies a very high frequency component or discontinuity.
        no_clicks_pops = max_jump < 0.5

        if not no_clicks_pops:
             print(f"Artifact detail: Max sample-to-sample jump {max_jump:.4f}")

        if not has_low_dc_offset:
             print(f"Artifact detail: DC Offset {dc_offset:.4f}")

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

        # 1. THD & SNR Measurement (using 1kHz Sine)
        input_sine = self._generate_sine_wave(freq_1khz, sample_rate, duration)
        try:
            output_sine = audio_processor(input_sine)

            # Align output to compensate for latency before analysis
            # This ensures windowing covers the signal properly
            output_sine_aligned = self._align_signals(input_sine, output_sine)

            thd, snr = self._measure_spectral_metrics(output_sine_aligned, sample_rate, freq_1khz)
        except Exception as e:
            print(f"Error processing sine wave: {e}")
            output_sine_aligned = np.zeros_like(input_sine)
            thd = 0.0
            snr = 0.0

        # 2. Artifact Detection (on Sine output)
        no_clicks, valid_dc = self._detect_artifacts(output_sine_aligned)
        if not no_clicks:
            print("Artifact detected: Clicks/Pops found in output.")
        if not valid_dc:
            print("Artifact detected: High DC Offset found in output.")
        
        # 3. Frequency Response (Log Sine Sweep)
        frequency_response_flat = self._measure_frequency_response(audio_processor, sample_rate)

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

    def _is_coverage_enabled(self, build_dir: Path) -> bool:
        """Check if coverage is enabled in the build directory."""
        cache_file = build_dir / "CMakeCache.txt"
        if not cache_file.exists():
            return False

        try:
            content = cache_file.read_text(encoding='utf-8', errors='ignore')
            # Look for ZENITH_ENABLE_COVERAGE:BOOL=ON
            if re.search(r"ZENITH_ENABLE_COVERAGE:BOOL=ON", content):
                return True
        except Exception:
            pass
        return False

    def _rebuild_project(self, build_dir: Path):
        """Reconfigure and rebuild the project with coverage enabled."""
        print("Rebuilding project with coverage enabled...")

        # Configure
        cmd_config = ["cmake", "-B", str(build_dir), "-S", str(self.project_root), "-DZENITH_ENABLE_COVERAGE=ON"]
        if shutil.which("ninja"):
            cmd_config.extend(["-G", "Ninja"])

        print(f"Running: {' '.join(cmd_config)}")
        subprocess.run(cmd_config, check=True)

        # Build
        cmd_build = ["cmake", "--build", str(build_dir)]
        print(f"Running: {' '.join(cmd_build)}")
        subprocess.run(cmd_build, check=True)

    def _open_file(self, path: str, mode: str):
        """Wrapper for open() to facilitate mocking."""
        return open(path, mode)

    def _write_manual_lcov(self, file_stats_with_lines: List[Dict], output_file: Path):
        """Generate LCOV info file manually from parsed Gcov data."""
        with self._open_file(str(output_file), 'w') as f:
            f.write("TN:\n")
            for entry in file_stats_with_lines:
                # Use absolute path if available, else relative
                f.write(f"SF:{entry['file_path']}\n")

                # Lines
                for line_num, count in entry['lines'].items():
                    f.write(f"DA:{line_num},{count}\n")

                # Summary
                f.write(f"LF:{entry['lines_total']}\n")
                f.write(f"LH:{entry['lines_covered']}\n")
                f.write("end_of_record\n")
        print(f"LCOV report saved to {output_file} (manual generation)")

    def generate_coverage_report(self, 
                                 build_dir: Optional[Path] = None,
                                 rebuild_if_needed: bool = True,
                                 run_tests: bool = True,
                                 formats: List[str] = ["json", "lcov"]) -> CoverageReport:
        """
        Generate code coverage report.
        
        Args:
            build_dir: Build directory with coverage data
            rebuild_if_needed: Trigger rebuild if coverage not enabled
            run_tests: Run tests to generate fresh data
            formats: List of output formats ("json", "lcov", "html")
            
        Returns:
            Coverage statistics
        """
        print("Generating coverage report...")
        
        build_path = build_dir or self.project_root / "build"

        # 1. Check/Enable Coverage
        if not build_path.exists():
            build_path.mkdir(parents=True, exist_ok=True)

        if rebuild_if_needed:
            if not self._is_coverage_enabled(build_path):
                print("Coverage not enabled in build. Triggering rebuild...")
                try:
                    self._rebuild_project(build_path)
                except subprocess.CalledProcessError as e:
                    print(f"Error rebuilding project: {e}")
                    return CoverageReport()

        # 2. Run Tests
        if run_tests:
            print("Running tests to generate coverage data...")
            results = self.run_unit_tests()
            failed = sum(1 for t in results if t.status == TestStatus.FAILED)
            if failed > 0:
                print(f"Warning: {failed} tests failed. Coverage data might be incomplete.")

        # 3. Detect Coverage Type (LLVM vs GCC)
        # Check for Clang/LLVM .profraw
        profraw_files = list(build_path.rglob("*.profraw"))
        if profraw_files:
            return self._process_llvm_coverage(build_path, formats)

        # Check for GCC .gcno/.gcda
        gcno_files = list(build_path.rglob("*.gcno"))
        if gcno_files:
            return self._process_gcov_coverage(build_path, gcno_files, formats)

        print("Error: No coverage artifacts (.gcno or .profraw) found.")
        print("Please ensure the build was successful and ZENITH_ENABLE_COVERAGE=ON.")
        return CoverageReport()

    def _process_llvm_coverage(self, build_path: Path, formats: List[str]) -> CoverageReport:
        """Process LLVM/Clang coverage data."""
        print("Detected LLVM/Clang coverage artifacts.")

        llvm_profdata = shutil.which("llvm-profdata")
        llvm_cov = shutil.which("llvm-cov")

        if not llvm_profdata or not llvm_cov:
            print("Error: llvm-profdata or llvm-cov not found.")
            return CoverageReport()

        # 1. Merge profiles
        profdata_path = build_path / "coverage.profdata"
        # Find all profraw files
        cmd_merge = [llvm_profdata, "merge", "-sparse"]
        cmd_merge.extend([str(p) for p in build_path.rglob("*.profraw")])
        cmd_merge.extend(["-o", str(profdata_path)])

        subprocess.run(cmd_merge, check=False)

        if not profdata_path.exists():
            print("Error: Failed to merge profile data.")
            return CoverageReport()

        # 2. Locate binary (needed for llvm-cov)
        test_binary = self._find_test_binary("ZenithDAWTests")
        if not test_binary:
            print("Error: Test binary not found.")
            return CoverageReport()

        # 3. Generate Reports
        report_stats = CoverageReport()

        # Generate JSON for parsing
        try:
            cmd_export = [
                llvm_cov, "export",
                "-format=text",
                str(test_binary),
                f"-instr-profile={profdata_path}",
                "-ignore-filename-regex=.*(test|Test|gtest|catch|JuceLibraryCode|external).*"
            ]

            result = subprocess.run(cmd_export, capture_output=True, text=True, check=False)
            if result.returncode == 0:
                data = json.loads(result.stdout)
                # Parse summary from LLVM JSON
                # Structure: data['data'][0]['totals']['lines']['count'] ...
                if data.get('data'):
                    totals = data['data'][0].get('totals', {})
                    lines = totals.get('lines', {})
                    branches = totals.get('branches', {})
                    functions = totals.get('functions', {})

                    report_stats = CoverageReport(
                        lines_total=lines.get('count', 0),
                        lines_covered=lines.get('covered', 0),
                        branches_total=branches.get('count', 0),
                        branches_covered=branches.get('covered', 0),
                        functions_total=functions.get('count', 0),
                        functions_covered=functions.get('covered', 0)
                    )

                    if "json" in formats:
                        json_path = build_path / "coverage_report.json"
                        with self._open_file(str(json_path), 'w') as f:
                            json.dump(data, f, indent=2)
                        print(f"JSON report saved to {json_path}")

        except Exception as e:
            print(f"Error processing LLVM JSON: {e}")

        # Generate LCOV if requested
        if "lcov" in formats:
            lcov_path = build_path / "coverage.info"
            cmd_lcov = [
                llvm_cov, "export",
                "-format=lcov",
                str(test_binary),
                f"-instr-profile={profdata_path}",
                "-ignore-filename-regex=.*(test|Test|gtest|catch|JuceLibraryCode|external).*"
            ]
            with self._open_file(str(lcov_path), 'w') as f:
                subprocess.run(cmd_lcov, stdout=f, check=False)
            print(f"LCOV report saved to {lcov_path}")

        # Generate HTML if requested (using llvm-cov show)
        if "html" in formats:
             html_dir = build_path / "coverage_html"
             cmd_html = [
                llvm_cov, "show",
                "-format=html",
                str(test_binary),
                f"-instr-profile={profdata_path}",
                "-ignore-filename-regex=.*(test|Test|gtest|catch|JuceLibraryCode|external).*",
                f"-output-dir={html_dir}"
             ]
             subprocess.run(cmd_html, check=False)
             print(f"HTML report saved to {html_dir}")

        self.coverage = report_stats
        return report_stats

    def _process_gcov_coverage(self, build_path: Path, gcno_files: List[Path], formats: List[str]) -> CoverageReport:
        """Process GCC/Gcov coverage data."""
        print(f"Processing {len(gcno_files)} Gcov files...")

        # Check for .gcda files
        gcda_files = list(build_path.rglob("*.gcda"))
        if not gcda_files:
            print("Warning: No execution data (.gcda) found.")
            return CoverageReport()

        # 1. Internal Parsing (for JSON report and stats)
        # Note: We reuse the existing logic but refactored slightly
        parser = GcovParser()
        total_lines = 0
        covered_lines = 0
        total_branches = 0
        covered_branches = 0
        file_stats = []

        if shutil.which("gcov"):
             for gcno in gcno_files:
                try:
                    cwd = gcno.parent
                    cmd = ["gcov", "-b", "-c", gcno.name]
                    result = subprocess.run(cmd, cwd=str(cwd), capture_output=True, text=True, check=False)

                    if result.returncode != 0: continue

                    # Find generated .gcov files
                    generated_files = []
                    for line in result.stdout.splitlines():
                        if "Creating '" in line:
                            try:
                                fname = line.split("'")[1]
                                generated_files.append(cwd / fname)
                            except IndexError: pass

                    for gcov_file in generated_files:
                        if not gcov_file.exists(): continue

                        # Parse
                        content = gcov_file.read_text(encoding='utf-8', errors='ignore')
                        stats = parser.parse_file(content)

                        file_name = gcov_file.name.replace('.gcov', '')

                        # Filter out system headers and tests if possible
                        if any(x in file_name for x in ["test", "Test", "gtest", "catch", "JuceLibraryCode", "external"]):
                             gcov_file.unlink()
                             continue

                        total_lines += stats['lines_total']
                        covered_lines += stats['lines_covered']
                        total_branches += stats['branches_total']
                        covered_branches += stats['branches_covered']

                        # Determine source path
                        src_path = stats.get('source_path')
                        if src_path:
                            # If path is relative, try to resolve it relative to gcno directory (compilation dir)
                            p = Path(src_path)
                            if not p.is_absolute():
                                p = (cwd / p).resolve()
                            src_path = str(p)
                        else:
                            # Fallback: strip .gcov extension
                            # e.g. foo.cpp.gcov -> foo.cpp
                            src_path = file_name

                        file_stats.append({
                            'file': file_name,
                            'file_path': src_path,
                            'coverage_percent': stats['file_coverage'],
                            'lines_total': stats['lines_total'],
                            'lines_covered': stats['lines_covered'],
                            'lines': stats['lines_data']
                        })

                        # Cleanup .gcov file to save space?
                        # Or keep them if we want to parse line-by-line later?
                        # For now, unlink as in original code
                        gcov_file.unlink()

                except Exception as e:
                    print(f"Error processing {gcno}: {e}")
        else:
            print("Warning: 'gcov' tool not found. Skipping internal stats parsing.")

        # Save JSON
        if "json" in formats:
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
                "files": [{k:v for k,v in f.items() if k != 'lines'} for f in file_stats] # Exclude heavy line data from JSON summary
            }
            json_path = build_path / "coverage_report.json"
            try:
                with self._open_file(str(json_path), 'w') as f:
                    json.dump(report_data, f, indent=2)
                print(f"JSON report saved to {json_path}")
            except Exception as e:
                print(f"Failed to save JSON report: {e}")

        # 2. LCOV Generation
        if "lcov" in formats:
            lcov_path = build_path / "coverage.info"

            if shutil.which("lcov"):
                # capture
                cmd_lcov = [
                    "lcov", "--capture",
                    "--directory", str(build_path),
                    "--output-file", str(lcov_path),
                    "--ignore-errors", "gcov"
                ]
                subprocess.run(cmd_lcov, check=False)

                # filter
                cmd_remove = [
                    "lcov", "--remove", str(lcov_path),
                    "*test*", "*Test*", "*gtest*", "*catch*", "*JuceLibraryCode*", "*external*", "/usr/*",
                    "--output-file", str(lcov_path)
                ]
                subprocess.run(cmd_remove, check=False)

                print(f"LCOV report saved to {lcov_path}")

                # HTML from LCOV
                if "html" in formats and shutil.which("genhtml"):
                    html_dir = build_path / "coverage_html"
                    cmd_genhtml = ["genhtml", str(lcov_path), "--output-directory", str(html_dir)]
                    subprocess.run(cmd_genhtml, check=False)
                    print(f"HTML report saved to {html_dir}")

            elif file_stats:
                # Fallback to manual generation
                print("Generating LCOV report manually (lcov tool not found)...")
                self._write_manual_lcov(file_stats, lcov_path)
            else:
                 print("Warning: No coverage data found to generate LCOV.")

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
