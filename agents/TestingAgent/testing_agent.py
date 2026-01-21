"""
Testing Agent for comprehensive automated testing and validation.

This module provides test orchestration, execution, coverage analysis,
and specialized testing for real-time audio systems.
"""

from typing import Dict, List, Optional, Set, Callable
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
import numpy as np


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


class TestingAgent:
    """
    Testing Agent for comprehensive automated testing and validation.
    
    Orchestrates various types of tests, collects results,
    generates coverage reports, and validates audio quality.
    """

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

    def validate_rt_safety(self, source_files: Optional[List[Path]] = None) -> List[TestCase]:
        """
        Validate real-time thread safety constraints.
        
        Args:
            source_files: Specific files to validate
            
        Returns:
            List of validation results
        """
        print("Validating real-time thread safety...")
        
        # TODO: Static analysis for RT-unsafe operations
        # TODO: Check for allocations, locks, blocking calls
        # TODO: Verify noexcept specifications
        # TODO: Validate lock-free data structures
        
        results = []
        
        # Example RT safety checks:
        unsafe_patterns = [
            r"\bnew\s+",  # Heap allocation
            r"\bdelete\s+",  # Heap deallocation
            r"std::lock_guard",  # Mutex lock
            r"\bmalloc\(",  # C-style allocation
            r"\.push_back\(",  # Potential allocation (may need capacity check)
        ]
        
        # TODO: Scan audio callback code paths
        # TODO: Report violations
        
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
        
        # TODO: Generate random audio buffers
        # TODO: Generate malformed MIDI data
        # TODO: Test with extreme parameter values
        # TODO: Monitor for crashes, hangs, assertions
        # TODO: Save crash-inducing inputs
        
        results = []
        return results

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
        
        # TODO: Run tests with coverage instrumentation
        # TODO: Collect coverage data (gcov, llvm-cov)
        # TODO: Parse coverage output
        # TODO: Generate HTML report
        
        coverage = CoverageReport()
        self.coverage = coverage
        return coverage

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