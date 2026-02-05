#!/usr/bin/env python3
"""
Comprehensive Test Reporting System for Zenith DAW
Generates detailed reports from test results, performance benchmarks, and coverage data.

Usage:
    python3 test_reporter.py --input-dir ./build --output-dir ./test_reports
"""

import argparse
import json
import sys
import os
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any, Optional
import xml.etree.ElementTree as ET


class TestReporter:
    """Generates comprehensive test reports from various test outputs."""

    def __init__(self, input_dir: str, output_dir: str):
        self.input_dir = Path(input_dir)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.results = {
            "summary": {},
            "unit_tests": {},
            "performance": {},
            "coverage": {},
            "stress_tests": {},
            "security": {},
        }

    def run(self):
        """Execute full report generation."""
        print("=" * 70)
        print("ZENITH DAW - Comprehensive Test Report Generator")
        print("=" * 70)

        # Parse all test results
        self.parse_unit_tests()
        self.parse_performance_tests()
        self.parse_coverage_data()
        self.parse_stress_tests()
        self.parse_security_results()

        # Generate reports
        self.generate_json_report()
        self.generate_html_report()
        self.generate_markdown_summary()
        self.generate_ci_summary()

        # Print summary
        self.print_summary()

    def parse_unit_tests(self):
        """Parse unit test results from JSON files."""
        print("\n[1/5] Parsing Unit Test Results...")

        test_files = list(self.input_dir.glob("**/test_results*.json"))

        if not test_files:
            print("  ⚠️  No test result files found")
            return

        total_tests = 0
        total_passed = 0
        total_failed = 0
        platforms = {}

        for test_file in test_files:
            platform = test_file.stem.replace("test_results_", "").replace("_", " ")
            try:
                with open(test_file, "r") as f:
                    data = json.load(f)

                suite_tests = 0
                suite_passed = 0
                suite_failed = 0

                for suite in data.get("testsuites", []):
                    for test in suite.get("testsuite", []):
                        suite_tests += 1
                        if test.get("failures"):
                            suite_failed += 1
                        else:
                            suite_passed += 1

                total_tests += suite_tests
                total_passed += suite_passed
                total_failed += suite_failed

                platforms[platform] = {
                    "total": suite_tests,
                    "passed": suite_passed,
                    "failed": suite_failed,
                    "success_rate": (suite_passed / suite_tests * 100)
                    if suite_tests > 0
                    else 0,
                }

                print(f"  ✓ {platform}: {suite_passed}/{suite_tests} passed")

            except Exception as e:
                print(f"  ❌ Error parsing {test_file}: {e}")

        self.results["unit_tests"] = {
            "total": total_tests,
            "passed": total_passed,
            "failed": total_failed,
            "success_rate": (total_passed / total_tests * 100)
            if total_tests > 0
            else 0,
            "platforms": platforms,
        }

    def parse_performance_tests(self):
        """Parse performance benchmark results."""
        print("\n[2/5] Parsing Performance Benchmarks...")

        benchmark_files = list(self.input_dir.glob("**/*_benchmark.json"))

        if not benchmark_files:
            print("  ⚠️  No benchmark files found")
            return

        benchmarks = {}

        for bench_file in benchmark_files:
            test_name = (
                bench_file.stem.replace("_benchmark", "").replace("_", " ").title()
            )
            try:
                with open(bench_file, "r") as f:
                    data = json.load(f)

                benchmarks[test_name] = {
                    "avg_ms": data.get("avg_ms", 0),
                    "p99_ms": data.get("p99_ms", 0),
                    "min_ms": data.get("min_ms", 0),
                    "max_ms": data.get("max_ms", 0),
                    "total_samples": data.get("total_samples", 0),
                    "samples_per_second": data.get("samples_per_second", 0),
                }

                print(
                    f"  ✓ {test_name}: avg={data.get('avg_ms', 0):.3f}ms, p99={data.get('p99_ms', 0):.3f}ms"
                )

            except Exception as e:
                print(f"  ❌ Error parsing {bench_file}: {e}")

        self.results["performance"] = benchmarks

    def parse_coverage_data(self):
        """Parse code coverage data from lcov/cobertura."""
        print("\n[3/5] Parsing Code Coverage Data...")

        coverage_files = list(self.input_dir.glob("**/coverage.info"))

        if not coverage_files:
            print("  ⚠️  No coverage data files found")
            return

        # Parse lcov info file
        for cov_file in coverage_files:
            try:
                lines_executed = 0
                lines_total = 0
                functions_executed = 0
                functions_total = 0

                with open(cov_file, "r") as f:
                    for line in f:
                        if line.startswith("LF:"):
                            lines_total = int(line.split(":")[1])
                        elif line.startswith("LH:"):
                            lines_executed = int(line.split(":")[1])
                        elif line.startswith("FNF:"):
                            functions_total = int(line.split(":")[1])
                        elif line.startswith("FNH:"):
                            functions_executed = int(line.split(":")[1])

                line_coverage = (
                    (lines_executed / lines_total * 100) if lines_total > 0 else 0
                )
                function_coverage = (
                    (functions_executed / functions_total * 100)
                    if functions_total > 0
                    else 0
                )

                self.results["coverage"] = {
                    "lines_executed": lines_executed,
                    "lines_total": lines_total,
                    "line_coverage": line_coverage,
                    "functions_executed": functions_executed,
                    "functions_total": functions_total,
                    "function_coverage": function_coverage,
                }

                print(
                    f"  ✓ Line coverage: {line_coverage:.1f}% ({lines_executed}/{lines_total})"
                )
                print(
                    f"  ✓ Function coverage: {function_coverage:.1f}% ({functions_executed}/{functions_total})"
                )

            except Exception as e:
                print(f"  ❌ Error parsing {cov_file}: {e}")

    def parse_stress_tests(self):
        """Parse WCET stress test results."""
        print("\n[4/5] Parsing Stress Test Results...")

        stress_files = list(self.input_dir.glob("**/wcet_results.json"))

        if not stress_files:
            print("  ⚠️  No stress test result files found")
            return

        for stress_file in stress_files:
            try:
                with open(stress_file, "r") as f:
                    data = json.load(f)

                self.results["stress_tests"] = {
                    "duration_seconds": data.get("duration", 0),
                    "total_callbacks": data.get("totalCallbacks", 0),
                    "avg_execution_ms": data.get("avgExecutionMs", 0),
                    "max_execution_ms": data.get("maxExecutionMs", 0),
                    "p99_execution_ms": data.get("p99ExecutionMs", 0),
                    "missed_deadlines": data.get("missedDeadlines", 0),
                    "callback_rate_hz": data.get("callbackRateHz", 0),
                    "buffer_size": data.get("bufferSize", 0),
                }

                print(
                    f"  ✓ WCET: avg={data.get('avgExecutionMs', 0):.3f}ms, max={data.get('maxExecutionMs', 0):.3f}ms"
                )
                print(f"  ✓ Missed deadlines: {data.get('missedDeadlines', 0)}")

            except Exception as e:
                print(f"  ❌ Error parsing {stress_file}: {e}")

    def parse_security_results(self):
        """Parse security scan results."""
        print("\n[5/5] Parsing Security Scan Results...")

        security_files = list(self.input_dir.glob("**/bandit-results.json"))

        if not security_files:
            print("  ⚠️  No security scan files found")
            return

        for sec_file in security_files:
            try:
                with open(sec_file, "r") as f:
                    data = json.load(f)

                issues = data.get("results", [])
                severity_counts = {"HIGH": 0, "MEDIUM": 0, "LOW": 0}

                for issue in issues:
                    severity = issue.get("issue_severity", "LOW")
                    severity_counts[severity] += 1

                self.results["security"] = {
                    "total_issues": len(issues),
                    "high_severity": severity_counts["HIGH"],
                    "medium_severity": severity_counts["MEDIUM"],
                    "low_severity": severity_counts["LOW"],
                }

                print(
                    f"  ✓ Security issues: {len(issues)} total ({severity_counts['HIGH']} high, {severity_counts['MEDIUM']} medium, {severity_counts['LOW']} low)"
                )

            except Exception as e:
                print(f"  ❌ Error parsing {sec_file}: {e}")

    def generate_json_report(self):
        """Generate comprehensive JSON report."""
        report_file = self.output_dir / "test_report.json"

        report = {
            "generated_at": datetime.now().isoformat(),
            "project": "Zenith DAW",
            "version": "0.1.0-alpha",
            "results": self.results,
        }

        with open(report_file, "w") as f:
            json.dump(report, f, indent=2)

        print(f"\n📄 Generated JSON report: {report_file}")

    def generate_html_report(self):
        """Generate beautiful HTML report."""
        html_file = self.output_dir / "test_report.html"

        html_content = f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Zenith DAW Test Report</title>
    <style>
        * {{ margin: 0; padding: 0; box-sizing: border-box; }}
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: #333;
            line-height: 1.6;
            min-height: 100vh;
            padding: 20px;
        }}
        .container {{
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            border-radius: 12px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }}
        .header {{
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 40px;
            text-align: center;
        }}
        .header h1 {{ font-size: 2.5em; margin-bottom: 10px; }}
        .header p {{ opacity: 0.9; font-size: 1.1em; }}
        .content {{ padding: 40px; }}
        .section {{ margin-bottom: 40px; }}
        .section h2 {{
            color: #667eea;
            margin-bottom: 20px;
            padding-bottom: 10px;
            border-bottom: 3px solid #667eea;
        }}
        .metrics-grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }}
        .metric-card {{
            background: #f8f9fa;
            border-radius: 12px;
            padding: 25px;
            text-align: center;
            transition: transform 0.2s, box-shadow 0.2s;
            border-left: 5px solid;
        }}
        .metric-card:hover {{ transform: translateY(-5px); box-shadow: 0 10px 30px rgba(0,0,0,0.1); }}
        .metric-card.success {{ border-left-color: #28a745; }}
        .metric-card.warning {{ border-left-color: #ffc107; }}
        .metric-card.error {{ border-left-color: #dc3545; }}
        .metric-value {{
            font-size: 3em;
            font-weight: bold;
            color: #333;
            margin-bottom: 5px;
        }}
        .metric-label {{ color: #666; font-size: 0.9em; text-transform: uppercase; letter-spacing: 1px; }}
        table {{
            width: 100%;
            border-collapse: collapse;
            margin-top: 20px;
            background: white;
            border-radius: 8px;
            overflow: hidden;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }}
        th {{
            background: #667eea;
            color: white;
            padding: 15px;
            text-align: left;
            font-weight: 600;
        }}
        td {{
            padding: 12px 15px;
            border-bottom: 1px solid #eee;
        }}
        tr:hover {{ background: #f8f9fa; }}
        .status-badge {{
            padding: 5px 12px;
            border-radius: 20px;
            font-size: 0.85em;
            font-weight: bold;
            display: inline-block;
        }}
        .status-pass {{ background: #d4edda; color: #155724; }}
        .status-fail {{ background: #f8d7da; color: #721c24; }}
        .footer {{
            background: #f8f9fa;
            padding: 20px 40px;
            text-align: center;
            color: #666;
            border-top: 1px solid #eee;
        }}
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🎵 Zenith DAW Test Report</h1>
            <p>Generated: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}</p>
        </div>
        
        <div class="content">
            <div class="section">
                <h2>📊 Test Summary</h2>
                <div class="metrics-grid">
                    {self._generate_summary_cards()}
                </div>
            </div>
            
            <div class="section">
                <h2>🧪 Unit Test Results by Platform</h2>
                {self._generate_platform_table()}
            </div>
            
            <div class="section">
                <h2>⚡ Performance Benchmarks</h2>
                {self._generate_performance_table()}
            </div>
            
            <div class="section">
                <h2>📈 Code Coverage</h2>
                {self._generate_coverage_section()}
            </div>
            
            <div class="section">
                <h2>🔥 Stress Test Results (WCET)</h2>
                {self._generate_stress_section()}
            </div>
            
            <div class="section">
                <h2>🔒 Security Scan Results</h2>
                {self._generate_security_section()}
            </div>
        </div>
        
        <div class="footer">
            <p>Zenith DAW v0.1.0-alpha • Professional Digital Audio Workstation</p>
        </div>
    </div>
</body>
</html>
"""

        with open(html_file, "w") as f:
            f.write(html_content)

        print(f"📄 Generated HTML report: {html_file}")

    def _generate_summary_cards(self) -> str:
        """Generate summary metric cards HTML."""
        cards = []

        # Unit tests summary
        unit_tests = self.results.get("unit_tests", {})
        total = unit_tests.get("total", 0)
        passed = unit_tests.get("passed", 0)
        failed = unit_tests.get("failed", 0)
        success_rate = unit_tests.get("success_rate", 0)

        status_class = (
            "success" if failed == 0 else "error" if failed > 5 else "warning"
        )
        cards.append(f"""
            <div class="metric-card {status_class}">
                <div class="metric-value">{passed}/{total}</div>
                <div class="metric-label">Tests Passed</div>
            </div>
        """)

        cards.append(f"""
            <div class="metric-card {status_class}">
                <div class="metric-value">{success_rate:.1f}%</div>
                <div class="metric-label">Success Rate</div>
            </div>
        """)

        # Coverage
        coverage = self.results.get("coverage", {})
        line_cov = coverage.get("line_coverage", 0)
        cov_class = (
            "success" if line_cov >= 80 else "warning" if line_cov >= 60 else "error"
        )
        cards.append(f"""
            <div class="metric-card {cov_class}">
                <div class="metric-value">{line_cov:.1f}%</div>
                <div class="metric-label">Code Coverage</div>
            </div>
        """)

        # Platforms tested
        platforms = unit_tests.get("platforms", {})
        cards.append(f"""
            <div class="metric-card success">
                <div class="metric-value">{len(platforms)}</div>
                <div class="metric-label">Platforms Tested</div>
            </div>
        """)

        return "\n".join(cards)

    def _generate_platform_table(self) -> str:
        """Generate platform test results table."""
        platforms = self.results.get("unit_tests", {}).get("platforms", {})

        if not platforms:
            return "<p>No platform test data available.</p>"

        rows = []
        for platform, data in platforms.items():
            status_class = (
                "status-pass" if data.get("failed", 0) == 0 else "status-fail"
            )
            status_text = "✓ PASS" if data.get("failed", 0) == 0 else "✗ FAIL"
            success_rate = data.get("success_rate", 0)

            rows.append(f"""
                <tr>
                    <td><strong>{platform}</strong></td>
                    <td>{data.get("total", 0)}</td>
                    <td>{data.get("passed", 0)}</td>
                    <td>{data.get("failed", 0)}</td>
                    <td>{success_rate:.1f}%</td>
                    <td><span class="status-badge {status_class}">{status_text}</span></td>
                </tr>
            """)

        return f"""
            <table>
                <thead>
                    <tr>
                        <th>Platform</th>
                        <th>Total Tests</th>
                        <th>Passed</th>
                        <th>Failed</th>
                        <th>Success Rate</th>
                        <th>Status</th>
                    </tr>
                </thead>
                <tbody>
                    {"".join(rows)}
                </tbody>
            </table>
        """

    def _generate_performance_table(self) -> str:
        """Generate performance benchmarks table."""
        benchmarks = self.results.get("performance", {})

        if not benchmarks:
            return "<p>No performance benchmark data available.</p>"

        rows = []
        for name, data in benchmarks.items():
            rows.append(f"""
                <tr>
                    <td><strong>{name}</strong></td>
                    <td>{data.get("avg_ms", 0):.3f} ms</td>
                    <td>{data.get("p99_ms", 0):.3f} ms</td>
                    <td>{data.get("min_ms", 0):.3f} ms</td>
                    <td>{data.get("max_ms", 0):.3f} ms</td>
                    <td>{data.get("samples_per_second", 0):,.0f}</td>
                </tr>
            """)

        return f"""
            <table>
                <thead>
                    <tr>
                        <th>Benchmark</th>
                        <th>Avg (ms)</th>
                        <th>P99 (ms)</th>
                        <th>Min (ms)</th>
                        <th>Max (ms)</th>
                        <th>Samples/sec</th>
                    </tr>
                </thead>
                <tbody>
                    {"".join(rows)}
                </tbody>
            </table>
        """

    def _generate_coverage_section(self) -> str:
        """Generate code coverage section."""
        coverage = self.results.get("coverage", {})

        if not coverage:
            return "<p>No code coverage data available.</p>"

        line_cov = coverage.get("line_coverage", 0)
        func_cov = coverage.get("function_coverage", 0)

        return f"""
            <div class="metrics-grid">
                <div class="metric-card {"success" if line_cov >= 80 else "warning" if line_cov >= 60 else "error"}">
                    <div class="metric-value">{line_cov:.1f}%</div>
                    <div class="metric-label">Line Coverage</div>
                </div>
                <div class="metric-card {"success" if func_cov >= 80 else "warning" if func_cov >= 60 else "error"}">
                    <div class="metric-value">{func_cov:.1f}%</div>
                    <div class="metric-label">Function Coverage</div>
                </div>
            </div>
            <p style="margin-top: 20px;">
                Lines executed: {coverage.get("lines_executed", 0):,} / {coverage.get("lines_total", 0):,}<br>
                Functions executed: {coverage.get("functions_executed", 0):,} / {coverage.get("functions_total", 0):,}
            </p>
        """

    def _generate_stress_section(self) -> str:
        """Generate stress test results section."""
        stress = self.results.get("stress_tests", {})

        if not stress:
            return "<p>No stress test data available.</p>"

        missed = stress.get("missed_deadlines", 0)
        status_class = (
            "success" if missed == 0 else "error" if missed > 10 else "warning"
        )

        return f"""
            <div class="metrics-grid">
                <div class="metric-card {status_class}">
                    <div class="metric-value">{missed}</div>
                    <div class="metric-label">Missed Deadlines</div>
                </div>
                <div class="metric-card">
                    <div class="metric-value">{stress.get("avg_execution_ms", 0):.3f}</div>
                    <div class="metric-label">Avg Execution (ms)</div>
                </div>
                <div class="metric-card">
                    <div class="metric-value">{stress.get("max_execution_ms", 0):.3f}</div>
                    <div class="metric-label">Max Execution (ms)</div>
                </div>
                <div class="metric-card">
                    <div class="metric-value">{stress.get("p99_execution_ms", 0):.3f}</div>
                    <div class="metric-label">P99 Execution (ms)</div>
                </div>
            </div>
            <p style="margin-top: 20px;">
                Test duration: {stress.get("duration_seconds", 0)}s | 
                Callback rate: {stress.get("callback_rate_hz", 0):.1f} Hz | 
                Buffer size: {stress.get("buffer_size", 0)} samples
            </p>
        """

    def _generate_security_section(self) -> str:
        """Generate security scan results section."""
        security = self.results.get("security", {})

        if not security:
            return "<p>No security scan data available.</p>"

        high = security.get("high_severity", 0)
        medium = security.get("medium_severity", 0)
        low = security.get("low_severity", 0)
        total = security.get("total_issues", 0)

        return f"""
            <div class="metrics-grid">
                <div class="metric-card {"error" if high > 0 else "success"}">
                    <div class="metric-value">{high}</div>
                    <div class="metric-label">High Severity</div>
                </div>
                <div class="metric-card {"warning" if medium > 0 else "success"}">
                    <div class="metric-value">{medium}</div>
                    <div class="metric-label">Medium Severity</div>
                </div>
                <div class="metric-card {"success" if low == 0 else "warning"}">
                    <div class="metric-value">{low}</div>
                    <div class="metric-label">Low Severity</div>
                </div>
                <div class="metric-card">
                    <div class="metric-value">{total}</div>
                    <div class="metric-label">Total Issues</div>
                </div>
            </div>
        """

    def generate_markdown_summary(self):
        """Generate markdown summary for PRs."""
        md_file = self.output_dir / "test_summary.md"

        unit_tests = self.results.get("unit_tests", {})
        coverage = self.results.get("coverage", {})
        stress = self.results.get("stress_tests", {})
        security = self.results.get("security", {})

        md_content = f"""# 🧪 Zenith DAW Test Summary

Generated: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}

## 📊 Test Results

| Metric | Value |
|--------|-------|
| Total Tests | {unit_tests.get("total", 0)} |
| ✅ Passed | {unit_tests.get("passed", 0)} |
| ❌ Failed | {unit_tests.get("failed", 0)} |
| Success Rate | {unit_tests.get("success_rate", 0):.1f}% |

### Platform Results

| Platform | Status | Passed | Failed | Rate |
|----------|--------|--------|--------|------|
"""

        for platform, data in unit_tests.get("platforms", {}).items():
            status = "✅" if data.get("failed", 0) == 0 else "❌"
            md_content += f"| {platform} | {status} | {data.get('passed', 0)} | {data.get('failed', 0)} | {data.get('success_rate', 0):.1f}% |\n"

        md_content += f"""
## 📈 Code Coverage

- **Line Coverage:** {coverage.get("line_coverage", 0):.1f}%
- **Function Coverage:** {coverage.get("function_coverage", 0):.1f}%

## ⚡ Performance

"""

        for name, data in self.results.get("performance", {}).items():
            md_content += f"- **{name}:** avg={data.get('avg_ms', 0):.3f}ms, p99={data.get('p99_ms', 0):.3f}ms\n"

        if stress:
            md_content += f"""
## 🔥 Stress Tests (WCET)

- Missed Deadlines: {stress.get("missed_deadlines", 0)}
- Avg Execution: {stress.get("avg_execution_ms", 0):.3f}ms
- Max Execution: {stress.get("max_execution_ms", 0):.3f}ms

"""

        if security:
            md_content += f"""## 🔒 Security

- High Severity: {security.get("high_severity", 0)}
- Medium Severity: {security.get("medium_severity", 0)}
- Low Severity: {security.get("low_severity", 0)}
"""

        with open(md_file, "w") as f:
            f.write(md_content)

        print(f"📄 Generated Markdown summary: {md_file}")

    def generate_ci_summary(self):
        """Generate summary for CI systems."""
        summary_file = self.output_dir / "ci_summary.txt"

        unit_tests = self.results.get("unit_tests", {})
        coverage = self.results.get("coverage", {})

        exit_code = 0 if unit_tests.get("failed", 0) == 0 else 1

        summary = f"""
================================================================================
                        ZENITH DAW TEST SUMMARY
================================================================================

Test Results:
  Total Tests:    {unit_tests.get("total", 0)}
  Passed:         {unit_tests.get("passed", 0)}
  Failed:         {unit_tests.get("failed", 0)}
  Success Rate:   {unit_tests.get("success_rate", 0):.1f}%

Code Coverage:
  Line Coverage:     {coverage.get("line_coverage", 0):.1f}%
  Function Coverage: {coverage.get("function_coverage", 0):.1f}%

Platforms Tested: {len(unit_tests.get("platforms", {}))}

Exit Code: {exit_code}
================================================================================
"""

        with open(summary_file, "w") as f:
            f.write(summary)

        print(f"📄 Generated CI summary: {summary_file}")

    def print_summary(self):
        """Print summary to console."""
        print("\n" + "=" * 70)
        print("                    TEST REPORT GENERATION COMPLETE")
        print("=" * 70)

        unit_tests = self.results.get("unit_tests", {})
        coverage = self.results.get("coverage", {})

        print(f"""
Summary:
  Tests:     {unit_tests.get("passed", 0)}/{unit_tests.get("total", 0)} passed ({unit_tests.get("success_rate", 0):.1f}%)
  Coverage:  {coverage.get("line_coverage", 0):.1f}% lines, {coverage.get("function_coverage", 0):.1f}% functions
  Platforms: {len(unit_tests.get("platforms", {}))} tested

Reports generated in: {self.output_dir}
  - test_report.json
  - test_report.html
  - test_summary.md
  - ci_summary.txt
""")

        # Determine overall status
        if unit_tests.get("failed", 0) > 0:
            print("❌ Some tests FAILED")
            sys.exit(1)
        else:
            print("✅ All tests PASSED")
            sys.exit(0)


def main():
    parser = argparse.ArgumentParser(
        description="Generate comprehensive test reports for Zenith DAW"
    )
    parser.add_argument(
        "--input-dir",
        default="build",
        help="Directory containing test result files (default: build)",
    )
    parser.add_argument(
        "--output-dir",
        default="test_reports",
        help="Directory for generated reports (default: test_reports)",
    )

    args = parser.parse_args()

    reporter = TestReporter(args.input_dir, args.output_dir)
    reporter.run()


if __name__ == "__main__":
    main()
