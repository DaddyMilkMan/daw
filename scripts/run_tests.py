#!/usr/bin/env python3
"""
Automated Test Runner for Zenith DAW
Comprehensive testing automation with performance profiling and regression detection.

Usage:
    python3 run_tests.py --all                    # Run all tests
    python3 run_tests.py --quick                 # Quick smoke tests
    python3 run_tests.py --performance           # Performance benchmarks only
    python3 run_tests.py --stress                # Stress tests only
    python3 run_tests.py --coverage              # Generate coverage report
"""

import argparse
import subprocess
import sys
import json
import os
from pathlib import Path
from datetime import datetime
import time


class Colors:
    """Terminal color codes."""

    HEADER = "\033[95m"
    OKBLUE = "\033[94m"
    OKCYAN = "\033[96m"
    OKGREEN = "\033[92m"
    WARNING = "\033[93m"
    FAIL = "\033[91m"
    ENDC = "\033[0m"
    BOLD = "\033[1m"


class TestRunner:
    """Orchestrates running different test suites."""

    def __init__(self, build_dir: str = "build"):
        self.build_dir = Path(build_dir)
        self.results = {
            "start_time": datetime.now().isoformat(),
            "tests": {},
            "errors": [],
            "warnings": [],
        }
        self.failed = False

    def run_all_tests(self):
        """Run complete test suite."""
        print(f"{Colors.BOLD}{Colors.HEADER}")
        print("=" * 70)
        print("        ZENITH DAW - COMPREHENSIVE TEST SUITE")
        print("=" * 70)
        print(f"{Colors.ENDC}")

        # Check build exists
        if not self.build_dir.exists():
            print(
                f"{Colors.FAIL}❌ Build directory not found: {self.build_dir}{Colors.ENDC}"
            )
            print("Run: cmake -B build && cmake --build build")
            return False

        tests = [
            ("Unit Tests", self.run_unit_tests),
            ("Performance Benchmarks", self.run_performance_tests),
            ("Stress Tests", self.run_stress_tests),
            ("Integration Tests", self.run_integration_tests),
        ]

        for test_name, test_func in tests:
            print(f"\n{Colors.BOLD}[{test_name}]{Colors.ENDC}")
            print("-" * 70)
            try:
                test_func()
            except Exception as e:
                self.results["errors"].append(f"{test_name}: {str(e)}")
                self.failed = True
                print(f"{Colors.FAIL}❌ {test_name} failed: {e}{Colors.ENDC}")

        # Generate report
        self.generate_report()

        return not self.failed

    def run_quick_tests(self):
        """Run quick smoke tests."""
        print(f"{Colors.BOLD}{Colors.HEADER}")
        print("=" * 70)
        print("        ZENITH DAW - QUICK SMOKE TESTS")
        print("=" * 70)
        print(f"{Colors.ENDC}")

        print("\n[1/3] Running Basic Audio Engine Tests...")
        self._run_test_executable("ZenithDAWTests", "AudioEngine")

        print("\n[2/3] Running Basic UI Tests...")
        self._run_test_executable("ZenithDAWTests", "UI")

        print("\n[3/3] Running Stability Tests...")
        self._run_test_executable("ZenithDAWTests", "Stability")

        return not self.failed

    def run_unit_tests(self):
        """Run all unit tests."""
        print("Running unit tests...")

        categories = [
            "AudioEngine",
            "UI",
            "Project",
            "DSP",
            "MIDI",
            "AI",
            "Collaboration",
        ]

        for category in categories:
            print(f"\n  Testing category: {category}")
            result = self._run_test_executable("ZenithDAWTests", category)
            self.results["tests"][f"unit_{category}"] = result

    def run_performance_tests(self):
        """Run performance benchmarks."""
        print("Running performance benchmarks...")

        benchmarks = [
            ("MixerChannelBenchmark", "Mixer Channel"),
            ("WavetableBenchmark", "Wavetable Loader"),
            ("SampleGeneratorBenchmarkApp", "Sample Generator"),
        ]

        for exe, name in benchmarks:
            print(f"\n  Benchmarking: {name}")
            result = self._run_benchmark(exe)
            self.results["tests"][f"perf_{name.lower().replace(' ', '_')}"] = result

    def run_stress_tests(self):
        """Run stress tests (WCET)."""
        print("Running stress tests...")

        exe = self.build_dir / "WCETStressTests"
        if not exe.exists():
            print(
                f"  {Colors.WARNING}⚠️  WCETStressTests not found, skipping{Colors.ENDC}"
            )
            self.results["warnings"].append("WCET stress tests not built")
            return

        print(f"\n  Running WCET stress test (30 seconds)...")
        result = self._run_command(
            [str(exe), "--duration", "30", "--json-output", "wcet_results.json"],
            timeout=60,
        )

        if result["success"]:
            # Parse results
            try:
                with open(self.build_dir / "wcet_results.json", "r") as f:
                    wcet_data = json.load(f)

                missed = wcet_data.get("missedDeadlines", 0)
                max_exec = wcet_data.get("maxExecutionMs", 0)

                if missed > 0:
                    print(
                        f"  {Colors.WARNING}⚠️  {missed} missed deadlines detected{Colors.ENDC}"
                    )
                    self.results["warnings"].append(f"WCET: {missed} missed deadlines")

                if max_exec > 10:  # 10ms threshold
                    print(
                        f"  {Colors.WARNING}⚠️  High max execution time: {max_exec:.2f}ms{Colors.ENDC}"
                    )
                    self.results["warnings"].append(
                        f"WCET: high max execution time ({max_exec:.2f}ms)"
                    )

                print(f"  {Colors.OKGREEN}✓ WCET test completed{Colors.ENDC}")
                print(f"     Avg: {wcet_data.get('avgExecutionMs', 0):.3f}ms")
                print(f"     Max: {max_exec:.3f}ms")
                print(f"     P99: {wcet_data.get('p99ExecutionMs', 0):.3f}ms")

                self.results["tests"]["wcet_stress"] = {
                    "success": True,
                    "data": wcet_data,
                }

            except Exception as e:
                print(
                    f"  {Colors.WARNING}⚠️  Could not parse WCET results: {e}{Colors.ENDC}"
                )
                self.results["tests"]["wcet_stress"] = {
                    "success": False,
                    "error": str(e),
                }
        else:
            print(f"  {Colors.FAIL}❌ WCET test failed{Colors.ENDC}")
            self.failed = True
            self.results["tests"]["wcet_stress"] = {"success": False}

    def run_integration_tests(self):
        """Run integration tests."""
        print("Running integration tests...")

        # Collaboration integration test
        print("\n  Testing collaboration features...")
        result = self._run_test_executable("ZenithDAWTests", "Collaboration")
        self.results["tests"]["integration_collaboration"] = result

        # Plugin integration test
        print("\n  Testing plugin hosting...")
        result = self._run_test_executable("ZenithDAWTests", "Plugin")
        self.results["tests"]["integration_plugin"] = result

    def run_coverage(self):
        """Generate coverage report."""
        print(f"{Colors.BOLD}{Colors.HEADER}")
        print("=" * 70)
        print("        GENERATING CODE COVERAGE REPORT")
        print("=" * 70)
        print(f"{Colors.ENDC}")

        # Check if coverage was enabled in build
        if not (self.build_dir / "coverage.info").exists():
            print(f"{Colors.WARNING}⚠️  Coverage data not found.{Colors.ENDC}")
            print("Rebuild with: cmake -B build -DZENITH_ENABLE_COVERAGE=ON")
            return False

        print("\nGenerating HTML coverage report...")
        result = self._run_command(
            [
                "genhtml",
                str(self.build_dir / "coverage.info"),
                "--output-directory",
                str(self.build_dir / "coverage_report"),
            ]
        )

        if result["success"]:
            print(f"{Colors.OKGREEN}✓ Coverage report generated{Colors.ENDC}")
            print(f"  Location: {self.build_dir / 'coverage_report'}")

            # Parse coverage summary
            try:
                with open(self.build_dir / "coverage.info", "r") as f:
                    lines = f.readlines()
                    lines_found = 0
                    lines_hit = 0
                    for line in lines:
                        if line.startswith("LF:"):
                            lines_found = int(line.split(":")[1])
                        elif line.startswith("LH:"):
                            lines_hit = int(line.split(":")[1])

                    coverage = (lines_hit / lines_found * 100) if lines_found > 0 else 0
                    print(
                        f"\n  Line Coverage: {coverage:.1f}% ({lines_hit}/{lines_found})"
                    )
            except Exception as e:
                print(f"  Could not parse coverage: {e}")

            return True
        else:
            print(f"{Colors.FAIL}❌ Failed to generate coverage report{Colors.ENDC}")
            return False

    def _run_test_executable(self, exe_name: str, category: str = None) -> dict:
        """Run a test executable with optional category filter."""
        exe = self.build_dir / exe_name
        if not exe.exists():
            print(f"  {Colors.WARNING}⚠️  {exe_name} not found{Colors.ENDC}")
            return {"success": False, "skipped": True}

        cmd = [str(exe)]
        if category:
            cmd.append(category)

        result = self._run_command(cmd, timeout=300)

        if result["success"]:
            print(f"  {Colors.OKGREEN}✓ {exe_name} passed{Colors.ENDC}")
            return {"success": True}
        else:
            print(f"  {Colors.FAIL}❌ {exe_name} failed{Colors.ENDC}")
            self.failed = True
            return {"success": False, "error": result.get("stderr", "")}

    def _run_benchmark(self, exe_name: str) -> dict:
        """Run a benchmark executable."""
        exe = self.build_dir / exe_name
        if not exe.exists():
            print(f"  {Colors.WARNING}⚠️  {exe_name} not found{Colors.ENDC}")
            return {"success": False, "skipped": True}

        # Run benchmark with JSON output
        json_file = f"{exe_name}_results.json"
        result = self._run_command([str(exe), "--json-output", json_file], timeout=300)

        if result["success"]:
            # Parse benchmark results
            try:
                with open(self.build_dir / json_file, "r") as f:
                    bench_data = json.load(f)

                avg_ms = bench_data.get("avg_ms", 0)
                p99_ms = bench_data.get("p99_ms", 0)

                print(f"  {Colors.OKGREEN}✓ Benchmark completed{Colors.ENDC}")
                print(f"     Avg: {avg_ms:.3f}ms, P99: {p99_ms:.3f}ms")

                return {"success": True, "data": bench_data}

            except Exception as e:
                print(
                    f"  {Colors.WARNING}⚠️  Could not parse benchmark: {e}{Colors.ENDC}"
                )
                return {"success": True, "error": str(e)}
        else:
            print(f"  {Colors.FAIL}❌ Benchmark failed{Colors.ENDC}")
            self.failed = True
            return {"success": False}

    def _run_command(self, cmd: list, timeout: int = 300) -> dict:
        """Run a command and return results."""
        try:
            result = subprocess.run(
                cmd, cwd=self.build_dir, capture_output=True, text=True, timeout=timeout
            )

            return {
                "success": result.returncode == 0,
                "returncode": result.returncode,
                "stdout": result.stdout,
                "stderr": result.stderr,
            }
        except subprocess.TimeoutExpired:
            return {"success": False, "error": f"Command timed out after {timeout}s"}
        except Exception as e:
            return {"success": False, "error": str(e)}

    def generate_report(self):
        """Generate comprehensive test report."""
        self.results["end_time"] = datetime.now().isoformat()
        self.results["passed"] = not self.failed

        # Save JSON report
        report_file = self.build_dir / "test_results_full.json"
        with open(report_file, "w") as f:
            json.dump(self.results, f, indent=2)

        print(f"\n{Colors.BOLD}Report saved to: {report_file}{Colors.ENDC}")

    def print_summary(self):
        """Print test summary."""
        print(f"\n{Colors.BOLD}{Colors.HEADER}")
        print("=" * 70)
        print("                      TEST SUMMARY")
        print("=" * 70)
        print(f"{Colors.ENDC}")

        passed = sum(1 for r in self.results["tests"].values() if r.get("success"))
        total = len(self.results["tests"])
        failed = total - passed

        print(f"\nTotal Tests: {total}")
        print(f"{Colors.OKGREEN}Passed: {passed}{Colors.ENDC}")
        if failed > 0:
            print(f"{Colors.FAIL}Failed: {failed}{Colors.ENDC}")

        if self.results["errors"]:
            print(f"\n{Colors.FAIL}Errors:{Colors.ENDC}")
            for error in self.results["errors"]:
                print(f"  - {error}")

        if self.results["warnings"]:
            print(f"\n{Colors.WARNING}Warnings:{Colors.ENDC}")
            for warning in self.results["warnings"]:
                print(f"  - {warning}")

        print(f"\n{Colors.BOLD}{'=' * 70}{Colors.ENDC}")

        if self.failed:
            print(f"{Colors.FAIL}❌ TEST SUITE FAILED{Colors.ENDC}")
            return False
        else:
            print(f"{Colors.OKGREEN}✅ ALL TESTS PASSED{Colors.ENDC}")
            return True


def main():
    parser = argparse.ArgumentParser(description="Automated Test Runner for Zenith DAW")
    parser.add_argument("--all", action="store_true", help="Run complete test suite")
    parser.add_argument(
        "--quick", action="store_true", help="Run quick smoke tests only"
    )
    parser.add_argument(
        "--performance", action="store_true", help="Run performance benchmarks only"
    )
    parser.add_argument("--stress", action="store_true", help="Run stress tests only")
    parser.add_argument(
        "--coverage", action="store_true", help="Generate coverage report"
    )
    parser.add_argument(
        "--build-dir", default="build", help="Build directory (default: build)"
    )

    args = parser.parse_args()

    runner = TestRunner(args.build_dir)
    success = False

    if args.all or not any([args.quick, args.performance, args.stress, args.coverage]):
        success = runner.run_all_tests()
        runner.print_summary()
    elif args.quick:
        success = runner.run_quick_tests()
        runner.print_summary()
    elif args.performance:
        runner.run_performance_tests()
    elif args.stress:
        runner.run_stress_tests()
    elif args.coverage:
        success = runner.run_coverage()

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
