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

    def _strip_comments_and_strings(self, text: str) -> str:
        """
        Remove C++ comments and string literals to prevent false positives.
        Replaces them with spaces/newlines to maintain character positions relative to lines roughly.
        """
        # Pattern to match strings (including escaped quotes), block comments, and line comments
        # " (:? \\. | [^"\\] )* " matches "..." with escaped chars
        # /\* [\s\S]*? \*/ matches /* ... */
        # // .* matches // ...
        pattern = r'("(:?\\.|[^"\\])*"|/\*[\s\S]*?\*/|//.*)'

        def replacer(match):
            s = match.group(0)
            if s.startswith('/'):
                # It's a comment
                # Keep newlines if any, replace rest with space
                return " " * (len(s) - s.count('\n')) + "\n" * s.count("\n")
            else:
                # It's a string literal, replace with empty string literal
                return '""'

        return re.sub(pattern, replacer, text)

    def _extract_rt_function_bodies(self, text: str) -> List[Dict]:
        """
        Parse C++ content to find bodies of RT-critical functions.
        Targets: processBlock, getNextAudioBlock, and functions annotated with // RT-SAFE
        """
        results = []

        # 1. Standard RT functions (processBlock, getNextAudioBlock)
        # Matches: (optional ret type) (optional Class::)Name (args) (modifiers) {
        # Using [^{]* for modifiers to handle const, noexcept, override, final, etc.
        start_pattern = re.compile(
            r'(?P<ret>[\w:<>\*&]+\s+)?(?:[\w:<>\*&]+::)?(?P<name>processBlock|getNextAudioBlock)\s*\((?P<args>[^)]*)\)\s*(?P<modifiers>[^{};]*)\{'
        )

        # Helper to extract body block
        def extract_body(start_idx):
            brace_count = 1
            idx = start_idx + 1
            length = len(text)
            while idx < length and brace_count > 0:
                if text[idx] == '{':
                    brace_count += 1
                elif text[idx] == '}':
                    brace_count -= 1
                idx += 1
            return text[start_idx:idx]

        for match in start_pattern.finditer(text):
            # Verify the match ended at '{'
            start_brace_idx = match.end() - 1

            body = extract_body(start_brace_idx)

            modifiers = match.group('modifiers')
            is_noexcept = 'noexcept' in modifiers

            results.append({
                'name': match.group('name'),
                'body': body,
                'noexcept': is_noexcept,
                'start_idx': match.start()
            })

        # 2. Annotated functions // RT-SAFE
        annot_iter = re.finditer(r'//\s*RT-SAFE', text)
        for match in annot_iter:
            search_start = match.end()
            window = text[search_start:search_start+500]

            # Generic function definition regex
            # Exclude control keywords to avoid matching loops/ifs as functions
            func_pattern = re.compile(
                r'(?:[\w:<>\*&]+\s+)(?:[\w:<>\*&]+::)?(?P<name>(?!if\b|for\b|while\b|switch\b|catch\b|return\b|auto\b)\w+)\s*\((?P<args>[^)]*)\)\s*(?P<modifiers>[^{};]*)\{'
            )

            func_match = func_pattern.search(window)
            if func_match:
                name = func_match.group('name')

                # Deduplicate: if we already found this function (e.g. processBlock tagged with RT-SAFE)
                current_func_start = search_start + func_match.start()
                if any(r['name'] == name and abs(r['start_idx'] - current_func_start) < 200 for r in results):
                    continue

                abs_start_brace = search_start + func_match.end() - 1
                body = extract_body(abs_start_brace)

                modifiers = func_match.group('modifiers')
                is_noexcept = 'noexcept' in modifiers

                results.append({
                    'name': name,
                    'body': body,
                    'noexcept': is_noexcept,
                    'start_idx': current_func_start
                })

        return results

    def validate_rt_safety(self, source_files: Optional[List[Path]] = None) -> List[TestCase]:
        """
        Validate real-time thread safety constraints.
        
        Args:
            source_files: Specific files to validate. If None, scans project.
            
        Returns:
            List of validation results
        """
        print("Validating real-time thread safety...")
        
        if source_files is None:
            source_files = []
            # Scan common source directories
            dirs_to_scan = [
                self.project_root / "Source",
                self.project_root / "apps",
                self.project_root / "agents",
                self.project_root / "modules"
            ]

            for d in dirs_to_scan:
                if d.exists():
                    for ext in ['*.cpp', '*.h', '*.hpp', '*.mm']:
                        source_files.extend(list(d.rglob(ext)))

        results = []
        
        # Forbidden patterns with descriptions
        forbidden_ops = [
            (r'\bnew\b', "Heap allocation (new)"),
            (r'\bdelete\b', "Heap deallocation (delete)"),
            (r'\bmalloc\s*\(', "C-style allocation (malloc)"),
            (r'\bfree\s*\(', "C-style deallocation (free)"),
            (r'\bcalloc\s*\(', "C-style allocation (calloc)"),
            (r'\brealloc\s*\(', "C-style allocation (realloc)"),

            # Vector / Container allocations
            (r'\.push_back\s*\(', "Vector push_back (potential allocation)"),
            (r'\.resize\s*\(', "Vector resize (allocation)"),
            (r'\.reserve\s*\(', "Vector reserve (allocation)"),

            # Locking
            (r'\bstd::mutex\b', "std::mutex usage"),
            (r'\bstd::lock_guard\b', "std::lock_guard usage"),
            (r'\bstd::unique_lock\b', "std::unique_lock usage"),
            (r'\bjuce::CriticalSection\b', "juce::CriticalSection usage"),
            (r'\bjuce::ScopedLock\b', "juce::ScopedLock usage"),

            # Syscalls / Logging
            (r'\bstd::cout\b', "Console output (std::cout)"),
            (r'\bprintf\s*\(', "Console output (printf)"),
            (r'\bjuce::Logger\b', "Logging (juce::Logger)"),
            (r'\bDBG\s*\(', "Debug logging (DBG)"),
            (r'\bjassert\s*\(', "Assertion (jassert)"),

            # Smart pointers creation/reset
            (r'\bstd::make_shared\b', "Shared pointer creation"),
            (r'\bstd::make_unique\b', "Unique pointer creation"),
            (r'\.reset\s*\(', "Smart pointer reset"),

            # Async
            (r'\bstd::async\b', "std::async usage"),
            (r'\bstd::future\b', "std::future usage"),
            (r'\bstd::promise\b', "std::promise usage"),

            # RTTI/Exceptions
            (r'\bdynamic_cast\b', "dynamic_cast usage (RTTI)"),
            (r'\bthrow\b', "Exception throwing"),
            (r'\bcatch\b', "Exception catching"),
        ]

        for file_path in source_files:
            try:
                try:
                    content = file_path.read_text(encoding='utf-8', errors='ignore')
                except Exception:
                    continue

                # Extract functions
                functions = self._extract_rt_function_bodies(content)

                for func in functions:
                    test_name = f"{file_path.name}::{func['name']}"

                    # 1. Check noexcept
                    if not func['noexcept']:
                        results.append(TestCase(
                            name=f"{test_name} [noexcept]",
                            test_type=TestType.RT_SAFETY,
                            status=TestStatus.FAILED,
                            error_message=f"Function {func['name']} in {file_path.name} is missing 'noexcept' specifier."
                        ))

                    # 2. Check forbidden ops in stripped body
                    clean_body = self._strip_comments_and_strings(func['body'])

                    found_errors = []
                    for pattern, desc in forbidden_ops:
                        if re.search(pattern, clean_body):
                            found_errors.append(desc)

                    # Additional checks
                    if re.search(r'\bstd::shared_ptr\b', clean_body):
                         found_errors.append("std::shared_ptr usage")
                    if re.search(r'\bstd::function\b', clean_body):
                         found_errors.append("std::function usage")

                    if found_errors:
                        results.append(TestCase(
                            name=f"{test_name} [rt-violations]",
                            test_type=TestType.RT_SAFETY,
                            status=TestStatus.FAILED,
                            error_message=f"Real-time safety violations: {', '.join(found_errors)}"
                        ))
                    elif func['noexcept']:
                        # Only report pass if noexcept AND no violations
                        results.append(TestCase(
                            name=test_name,
                            test_type=TestType.RT_SAFETY,
                            status=TestStatus.PASSED
                        ))

            except Exception as e:
                print(f"Error validating {file_path}: {e}")

        self.test_results.extend(results)
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
