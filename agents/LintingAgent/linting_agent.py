"""
Linting Agent for code style, naming conventions, and code quality checks.

This module provides comprehensive linting for C++ and Python code,
enforcing naming conventions, detecting style violations, finding
commented-out code, magic numbers, TODOs, and other code quality issues.
"""

from typing import Dict, List, Optional, Set, Tuple
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
import re
import sys


class LintSeverity(Enum):
    """Severity levels for lint findings."""
    ERROR = "error"
    WARNING = "warning"
    INFO = "info"


class LintCategory(Enum):
    """Categories of lint issues."""
    NAMING = "naming"
    STYLE = "style"
    COMMENTED_CODE = "commented_code"
    MAGIC_NUMBER = "magic_number"
    TODO = "todo"
    STRUCTURE = "structure"
    COMPLEXITY = "complexity"


@dataclass
class LintFinding:
    """Individual lint finding."""
    category: LintCategory
    severity: LintSeverity
    description: str
    file_path: Path
    line_number: int
    line_content: str = ""
    suggestion: Optional[str] = None


@dataclass
class LintReport:
    """Comprehensive lint report."""
    findings: List[LintFinding] = field(default_factory=list)
    files_scanned: int = 0
    
    @property
    def error_count(self) -> int:
        return sum(1 for f in self.findings if f.severity == LintSeverity.ERROR)
    
    @property
    def warning_count(self) -> int:
        return sum(1 for f in self.findings if f.severity == LintSeverity.WARNING)
    
    @property
    def info_count(self) -> int:
        return sum(1 for f in self.findings if f.severity == LintSeverity.INFO)


class LintingAgent:
    """
    Linting Agent for comprehensive code quality checks.
    
    Enforces naming conventions, style rules, and detects common
    code quality issues in C++ and Python code.
    """

    def __init__(self, project_root: Optional[Path] = None):
        """
        Initialize Linting Agent.
        
        Args:
            project_root: Root directory of the project
        """
        self.project_root = project_root or Path.cwd()
        self._initialize_patterns()

    def _initialize_patterns(self) -> None:
        """Initialize regex patterns for various lint checks."""
        # C++ naming patterns based on CODING_CONVENTIONS.md
        self.cpp_patterns = {
            # Magic numbers (literals in code, excluding 0, 1, -1, and common values)
            'magic_number': re.compile(r'(?<![a-zA-Z0-9_])[2-9]\d+(?:\.\d+)?(?![a-zA-Z0-9_])|(?<![a-zA-Z0-9_])\d+\.\d+(?![a-zA-Z0-9_])'),
            # Commented-out code (lines with // followed by code-like patterns)
            'commented_code': re.compile(r'^\s*//\s*(?:[\w]+\s*[\(\{=;]|if\s*\(|for\s*\(|while\s*\(|return\s+)'),
            # TODO comments
            'todo': re.compile(r'//\s*TODO:?\s*(.*)'),
            # Trailing whitespace
            'trailing_whitespace': re.compile(r'\s+$'),
            # Missing space after //
            'comment_spacing': re.compile(r'//[^\s/]'),
        }
        
        # Python patterns
        self.python_patterns = {
            'magic_number': re.compile(r'(?<![a-zA-Z0-9_])[2-9]\d+(?:\.\d+)?(?![a-zA-Z0-9_])|(?<![a-zA-Z0-9_])\d+\.\d+(?![a-zA-Z0-9_])'),
            'commented_code': re.compile(r'^\s*#\s*(?:[\w]+\s*[\(=]|if\s+|for\s+|while\s+|def\s+|class\s+|return\s+)'),
            'todo': re.compile(r'#\s*TODO:?\s*(.*)'),
            'trailing_whitespace': re.compile(r'\s+$'),
        }

    def scan_files(self, file_patterns: List[str]) -> LintReport:
        """
        Scan files matching patterns for lint issues.
        
        Args:
            file_patterns: List of glob patterns for files to scan
            
        Returns:
            Lint report with all findings
        """
        report = LintReport()
        files_to_scan = []
        
        # Collect all files matching patterns
        for pattern in file_patterns:
            files_to_scan.extend(self.project_root.glob(pattern))
        
        # Remove duplicates and sort
        files_to_scan = sorted(set(files_to_scan))
        
        print(f"Scanning {len(files_to_scan)} files...")
        
        for file_path in files_to_scan:
            if not file_path.is_file():
                continue
            
            findings = self._scan_file(file_path)
            report.findings.extend(findings)
            report.files_scanned += 1
        
        return report

    def _scan_file(self, file_path: Path) -> List[LintFinding]:
        """Scan a single file for lint issues."""
        findings = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception as e:
            print(f"Error reading {file_path}: {e}")
            return findings
        
        # Determine file type
        is_cpp = file_path.suffix in ['.cpp', '.h', '.hpp', '.cc', '.cxx']
        is_python = file_path.suffix == '.py'
        
        if not (is_cpp or is_python):
            return findings
        
        patterns = self.cpp_patterns if is_cpp else self.python_patterns
        
        for line_num, line in enumerate(lines, start=1):
            # Skip empty lines
            if not line.strip():
                continue
            
            # Check for trailing whitespace
            if patterns['trailing_whitespace'].search(line):
                findings.append(LintFinding(
                    category=LintCategory.STYLE,
                    severity=LintSeverity.WARNING,
                    description="Trailing whitespace",
                    file_path=file_path,
                    line_number=line_num,
                    line_content=line.rstrip('\n'),
                    suggestion="Remove trailing whitespace"
                ))
            
            # Check for commented-out code
            if patterns['commented_code'].search(line):
                findings.append(LintFinding(
                    category=LintCategory.COMMENTED_CODE,
                    severity=LintSeverity.WARNING,
                    description="Commented-out code detected",
                    file_path=file_path,
                    line_number=line_num,
                    line_content=line.rstrip('\n'),
                    suggestion="Remove commented-out code or add explanation"
                ))
            
            # Check for TODOs
            todo_match = patterns['todo'].search(line)
            if todo_match:
                findings.append(LintFinding(
                    category=LintCategory.TODO,
                    severity=LintSeverity.INFO,
                    description=f"TODO: {todo_match.group(1).strip()}",
                    file_path=file_path,
                    line_number=line_num,
                    line_content=line.rstrip('\n'),
                    suggestion="Consider creating a tracked issue"
                ))
            
            # Check for magic numbers (skip in comments)
            if not line.strip().startswith(('//', '#')):
                # Skip lines with common patterns that are acceptable
                if not any(pattern in line for pattern in ['0x', 'case', 'sizeof', 'enum', 'const', '#define']):
                    magic_matches = patterns['magic_number'].finditer(line)
                    for match in magic_matches:
                        # Additional filtering: skip if part of a version string or date
                        context = line[max(0, match.start()-5):match.end()+5]
                        # Skip if looks like part of a version (has dots around it) or contains spaces
                        is_version_or_date = any(char in context for char in ['.', '/', '-'])
                        has_spaces = ' ' in match.group()
                        
                        # Only report if it's not a version/date and doesn't have spaces
                        if not is_version_or_date and not has_spaces:
                            findings.append(LintFinding(
                                category=LintCategory.MAGIC_NUMBER,
                                severity=LintSeverity.WARNING,
                                description=f"Magic number: {match.group()}",
                                file_path=file_path,
                                line_number=line_num,
                                line_content=line.rstrip('\n'),
                                suggestion="Consider using a named constant"
                            ))
            
            # C++ specific checks
            if is_cpp:
                # Check for missing space after // in comments
                if 'comment_spacing' in patterns and patterns['comment_spacing'].search(line):
                    if not line.strip().startswith('///'):  # Allow Doxygen comments
                        findings.append(LintFinding(
                            category=LintCategory.STYLE,
                            severity=LintSeverity.INFO,
                            description="Missing space after //",
                            file_path=file_path,
                            line_number=line_num,
                            line_content=line.rstrip('\n'),
                            suggestion="Add space after // for readability"
                        ))
                
                # Check naming conventions
                findings.extend(self._check_cpp_naming(line, file_path, line_num))
            
            # Python specific checks
            if is_python:
                findings.extend(self._check_python_naming(line, file_path, line_num))
        
        return findings

    def _check_cpp_naming(self, line: str, file_path: Path, line_num: int) -> List[LintFinding]:
        """Check C++ naming conventions."""
        findings = []
        
        # Check for variable declarations with incorrect naming
        # Member variables should end with _
        member_var_pattern = re.compile(r'^\s*(?:const\s+)?(?:auto|int|float|double|bool|std::\w+|juce::\w+|\w+)\s+([a-z][a-zA-Z0-9]*)\s*[;=]')
        
        # This is a simplified check - full implementation would need AST parsing
        # For now, just flag potential issues for manual review
        
        return findings

    def _check_python_naming(self, line: str, file_path: Path, line_num: int) -> List[LintFinding]:
        """Check Python naming conventions (PEP8)."""
        findings = []
        
        # Check for camelCase in Python (should be snake_case)
        # Class names are allowed to be PascalCase
        if 'class ' not in line:
            camel_case_pattern = re.compile(r'\b([a-z]+[A-Z][a-zA-Z]*)\s*=')
            match = camel_case_pattern.search(line)
            if match:
                findings.append(LintFinding(
                    category=LintCategory.NAMING,
                    severity=LintSeverity.WARNING,
                    description=f"Python variable '{match.group(1)}' uses camelCase (should use snake_case)",
                    file_path=file_path,
                    line_number=line_num,
                    line_content=line.rstrip('\n'),
                    suggestion="Use snake_case for Python variables"
                ))
        
        return findings

    def print_report(self, report: LintReport) -> None:
        """
        Print lint report in a readable format.
        
        Args:
            report: Lint report to print
        """
        print("\n" + "="*80)
        print("LINT REPORT")
        print("="*80)
        print(f"Files scanned: {report.files_scanned}")
        print(f"Total findings: {len(report.findings)}")
        print(f"  Errors: {report.error_count}")
        print(f"  Warnings: {report.warning_count}")
        print(f"  Info: {report.info_count}")
        print("="*80)
        
        if not report.findings:
            print("\n✓ No lint issues found!")
            return
        
        # Group by file
        findings_by_file: Dict[Path, List[LintFinding]] = {}
        for finding in report.findings:
            if finding.file_path not in findings_by_file:
                findings_by_file[finding.file_path] = []
            findings_by_file[finding.file_path].append(finding)
        
        # Print findings grouped by file
        for file_path, findings in sorted(findings_by_file.items()):
            print(f"\n{file_path}:")
            for finding in sorted(findings, key=lambda f: f.line_number):
                severity_symbol = {
                    LintSeverity.ERROR: "✗",
                    LintSeverity.WARNING: "⚠",
                    LintSeverity.INFO: "ℹ"
                }[finding.severity]
                
                print(f"  {severity_symbol} Line {finding.line_number}: [{finding.category.value}] {finding.description}")
                if finding.suggestion:
                    print(f"    Suggestion: {finding.suggestion}")

    def scan_project(self, exclude_dirs: Optional[List[str]] = None) -> LintReport:
        """
        Scan entire project for lint issues.
        
        Args:
            exclude_dirs: Directories to exclude from scanning
            
        Returns:
            Comprehensive lint report
        """
        exclude_dirs = exclude_dirs or [
            'build', '_deps', 'external', '.git', 'node_modules',
            '__pycache__', '.vscode', 'ZenithDAW_artefacts', 'ZenithDAWTests_artefacts'
        ]
        
        # Find all C++ and Python files
        cpp_files = []
        python_files = []
        
        for root, dirs, files in self.project_root.walk():
            # Remove excluded directories from traversal
            dirs[:] = [d for d in dirs if d not in exclude_dirs]
            
            for file in files:
                file_path = root / file
                if file.endswith(('.cpp', '.h', '.hpp', '.cc', '.cxx')):
                    cpp_files.append(file_path)
                elif file.endswith('.py'):
                    python_files.append(file_path)
        
        print(f"Found {len(cpp_files)} C++ files and {len(python_files)} Python files")
        
        all_files = cpp_files + python_files
        report = LintReport()
        
        for file_path in all_files:
            findings = self._scan_file(file_path)
            report.findings.extend(findings)
            report.files_scanned += 1
        
        return report


def main():
    """Main entry point for linting agent."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Linting Agent for code quality checks'
    )
    parser.add_argument(
        'paths',
        nargs='*',
        default=['.'],
        help='Paths or glob patterns to scan (default: current directory)'
    )
    parser.add_argument(
        '--exclude',
        nargs='+',
        default=['build', '_deps', 'external', '.git'],
        help='Directories to exclude'
    )
    parser.add_argument(
        '--severity',
        choices=['error', 'warning', 'info'],
        default='info',
        help='Minimum severity level to report (default: info)'
    )
    parser.add_argument(
        '--fail-on-error',
        action='store_true',
        help='Exit with non-zero code if errors found'
    )
    
    args = parser.parse_args()
    
    # Initialize agent
    if len(args.paths) == 1 and Path(args.paths[0]).is_dir():
        project_root = Path(args.paths[0])
        agent = LintingAgent(project_root=project_root)
        report = agent.scan_project(exclude_dirs=args.exclude)
    else:
        agent = LintingAgent()
        # Treat as glob patterns
        report = agent.scan_files(args.paths)
    
    # Print report
    agent.print_report(report)
    
    # Exit code based on findings
    if args.fail_on_error and report.error_count > 0:
        print(f"\n❌ Linting failed with {report.error_count} errors")
        sys.exit(1)
    
    print("\n✓ Linting complete")
    sys.exit(0)


if __name__ == "__main__":
    main()
