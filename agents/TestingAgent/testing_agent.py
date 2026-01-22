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
import json

try:
    import numpy as np
    import scipy.io.wavfile as wavfile
    NUMPY_AVAILABLE = True
except ImportError:
    NUMPY_AVAILABLE = False


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

    def test_audio_quality(self, audio_processor: Callable,
                          test_signals: Optional[List[str]] = None) -> AudioQualityMetrics:
        """
        Test audio processing quality metrics.
        
        Args:
            audio_processor: Function that processes audio buffers
            test_signals: List of test signals
            
        Returns:
            Audio quality metrics
        """
        print("Testing audio quality...")
        
        test_signals = test_signals or ["sine_1khz", "impulse", "white_noise"]
        
        # TODO: Generate test signals
        # TODO: Process through audio pipeline
        # TODO: Measure THD, SNR, frequency response
        # TODO: Detect clicks, pops, DC offset
        # TODO: Verify phase coherence
        
        metrics = AudioQualityMetrics()
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
