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
            test_signals: List of test signal types
            
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