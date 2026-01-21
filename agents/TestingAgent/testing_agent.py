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


class RTSafetyValidator:
    """Static analysis validator for Real-Time safety constraints."""

    def __init__(self):
        # Forbidden operations in RT context
        self.forbidden_patterns = [
            (re.compile(r"\b(new|delete|malloc|free)\b"), "Memory allocation/deallocation"),
            (re.compile(r"\.push_back\s*\("), "std::vector::push_back (potential reallocation)"),
            (re.compile(r"\bstd::string\b"), "std::string usage (allocation)"),
            (re.compile(r"\bstd::(mutex|lock_guard|unique_lock)\b"), "Blocking synchronization (mutex/lock)"),
            (re.compile(r"\bjuce::CriticalSection\b"), "Blocking synchronization (CriticalSection)"),
            (re.compile(r"\b(std::cout|printf|sleep|usleep|sleep_for)\b"), "Blocking I/O or sleep"),
            (re.compile(r"\b(throw|try|catch)\b"), "Exception handling"),
            (re.compile(r"\bdynamic_cast\b"), "Runtime Type Information (dynamic_cast)")
        ]

        # Regex to match function headers
        self.func_regex = re.compile(
            r'\b(void|int|float|double|auto)\s+(?:\w+::)?(processBlock|getNextAudioBlock)\s*\('
        )
        # Regex to match RT-SAFE annotation
        # Ensure it doesn't match RT-SAFE-IGNORE by using negative lookahead for dash/word chars
        self.annotation_regex = re.compile(r'//\s*RT-SAFE(?![-\w])')
        self.next_func_regex = re.compile(r'\b(void|int|float|double|auto)\s+(?:\w+::)?(\w+)\s*\(')

    def strip_comments_and_strings(self, code: str) -> str:
        """
        Replace comments and string literals with whitespace, preserving line count and character indices.
        """
        result = []
        i = 0
        n = len(code)

        while i < n:
            # Check for string literals first
            if code[i] == '"':
                result.append(' ') # Replace opening quote
                i += 1
                while i < n:
                    if code[i] == '\\':
                        result.append(' ')
                        i += 1
                        if i < n:
                            result.append(' ')
                            i += 1
                    elif code[i] == '"':
                        result.append(' ') # Replace closing quote
                        i += 1
                        break
                    else:
                        if code[i] == '\n': result.append('\n')
                        else: result.append(' ')
                        i += 1
            elif code[i] == "'":
                result.append(' ')
                i += 1
                while i < n:
                    if code[i] == '\\':
                        result.append(' ')
                        i += 1
                        if i < n:
                            result.append(' ')
                            i += 1
                    elif code[i] == "'":
                        result.append(' ')
                        i += 1
                        break
                    else:
                        if code[i] == '\n': result.append('\n')
                        else: result.append(' ')
                        i += 1
            # Check for line comments
            elif code[i:i+2] == '//':
                result.append(' ')
                result.append(' ')
                i += 2
                while i < n and code[i] != '\n':
                    result.append(' ')
                    i += 1
            # Check for block comments
            elif code[i:i+2] == '/*':
                result.append(' ')
                result.append(' ')
                i += 2
                while i < n:
                    if code[i:i+2] == '*/':
                        result.append(' ')
                        result.append(' ')
                        i += 2
                        break
                    if code[i] == '\n': result.append('\n')
                    else: result.append(' ')
                    i += 1
            else:
                result.append(code[i])
                i += 1

        return "".join(result)

    def extract_functions(self, code: str) -> List[Tuple[str, int, List[str], List[str]]]:
        """
        Extract target function bodies.
        Returns list of (function_name, start_line, original_lines, clean_lines).
        """
        clean_code = self.strip_comments_and_strings(code)
        lines = code.splitlines()
        clean_lines = clean_code.splitlines()
        functions = []
        seen = set() # (name, start_line)

        # Find explicitly named functions
        # Search in clean_code to avoid matching commented-out functions
        for match in self.func_regex.finditer(clean_code):
            func_name = match.group(2)
            start_index = match.end()
            body_info = self._extract_body(clean_code, start_index, lines, clean_lines)
            if body_info:
                key = (func_name, body_info[0])
                if key not in seen:
                    functions.append((func_name, *body_info))
                    seen.add(key)

        # Find annotated functions
        # Search in original code for comments (annotations)
        for match in self.annotation_regex.finditer(code):
            # Start searching for function definition after the annotation
            search_start = match.end()
            # Search in clean_code to avoid matching commented-out functions
            func_match = self.next_func_regex.search(clean_code, search_start)
            if func_match:
                # Check intervening text in clean_code (should be empty/whitespace)
                # clean_code already has comments stripped
                segment_clean = clean_code[search_start:func_match.start()]

                if not segment_clean.strip():
                    func_name = func_match.group(2)
                    start_index = func_match.end()
                    body_info = self._extract_body(clean_code, start_index, lines, clean_lines)
                    if body_info:
                        key = (func_name, body_info[0])
                        if key not in seen:
                            functions.append((func_name, *body_info))
                            seen.add(key)

        return functions

    def _extract_body(self, clean_code: str, start_index: int,
                     lines: List[str], clean_lines: List[str]) -> Optional[Tuple[int, List[str], List[str]]]:
        """Helper to extract body using brace counting on clean code."""
        # Find opening brace
        open_brace = clean_code.find('{', start_index)
        if open_brace == -1:
            return None

        balance = 1
        i = open_brace + 1
        n = len(clean_code)

        while i < n and balance > 0:
            char = clean_code[i]
            if char == '{':
                balance += 1
            elif char == '}':
                balance -= 1
            i += 1

        if balance == 0:
            # Determine line numbers
            # Count newlines up to open_brace
            start_line_idx = clean_code[:open_brace].count('\n')
            end_line_idx = clean_code[:i].count('\n')

            # Extract lines (inclusive of start/end braces)
            # Adjust indices to be safe
            return (
                start_line_idx + 1,
                lines[start_line_idx : end_line_idx + 1],
                clean_lines[start_line_idx : end_line_idx + 1]
            )
        return None

    def validate(self, file_path: Path) -> List[TestCase]:
        """Run validation on a single file."""
        results = []
        try:
            code = file_path.read_text(encoding='utf-8', errors='ignore')
        except Exception as e:
            print(f"Error reading {file_path}: {e}")
            return []

        functions = self.extract_functions(code)

        for func_name, start_line, original_lines, clean_lines in functions:
            for i, (orig_line, clean_line) in enumerate(zip(original_lines, clean_lines)):
                current_line_num = start_line + i

                # Check suppression
                if "// NOLINT" in orig_line or "// RT-SAFE-IGNORE" in orig_line:
                    continue

                for pattern, failure_msg in self.forbidden_patterns:
                    if pattern.search(clean_line):
                        results.append(TestCase(
                            name=f"{file_path.name}::{func_name}::Line{current_line_num}",
                            test_type=TestType.RT_SAFETY,
                            status=TestStatus.FAILED,
                            duration_ms=0,
                            error_message=f"{failure_msg} detected: '{orig_line.strip()}'"
                        ))

        return results


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
        self.rt_validator = RTSafetyValidator()

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
        
        results = []
        
        # If source files not provided, scan all C++ files
        if not source_files:
            source_files = []
            extensions = ['.cpp', '.h', '.hpp', '.mm']
            for path in self.project_root.rglob("*"):
                if path.is_file() and path.suffix in extensions:
                    # Skip build/external directories to avoid noise
                    if "build" in path.parts or "external" in path.parts or "JuceLibraryCode" in path.parts:
                        continue
                    source_files.append(path)

        print(f"Scanning {len(source_files)} files for RT safety...")
        
        for file_path in source_files:
            file_results = self.rt_validator.validate(file_path)
            results.extend(file_results)

        self.test_results.extend(results)
        
        # Summarize findings
        failure_count = sum(1 for r in results if r.status == TestStatus.FAILED)
        if failure_count > 0:
            print(f"Found {failure_count} RT safety violations.")
        else:
            print("No RT safety violations found.")

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