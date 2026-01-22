"""
Documentation Agent for validating and monitoring project documentation.

This module provides automated checks for documentation presence, freshness,
and synchronization with the codebase.
"""

from typing import List, Optional
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
import json
import sys
from datetime import datetime, timedelta
import re


class DocStatus(Enum):
    """Documentation status levels."""
    PRESENT = "present"
    MISSING = "missing"
    OUTDATED = "outdated"
    NEEDS_UPDATE = "needs_update"


class Severity(Enum):
    """Issue severity levels."""
    CRITICAL = "critical"
    HIGH = "high"
    MEDIUM = "medium"
    LOW = "low"
    INFO = "info"


@dataclass
class DocIssue:
    """Documentation issue finding."""
    severity: Severity
    status: DocStatus
    file_path: Optional[Path] = None
    description: str = ""
    recommendation: str = ""


@dataclass
class DocReport:
    """Comprehensive documentation validation report."""
    issues: List[DocIssue] = field(default_factory=list)
    checked_files: int = 0
    scan_timestamp: str = ""
    
    @property
    def critical_count(self) -> int:
        return sum(1 for i in self.issues if i.severity == Severity.CRITICAL)
    
    @property
    def high_count(self) -> int:
        return sum(1 for i in self.issues if i.severity == Severity.HIGH)
    
    @property
    def missing_count(self) -> int:
        return sum(1 for i in self.issues if i.status == DocStatus.MISSING)


class DocumentationAgent:
    """
    Documentation Agent for automated validation and monitoring.
    
    Validates presence and freshness of key documentation files,
    checks for code/doc synchronization, and provides actionable feedback.
    """

    # Required documentation files (relative to project root)
    REQUIRED_DOCS = [
        "README.md",
        "CONTRIBUTING.md",
        "docs/RENDERING_ARCHITECTURE.md",
        "docs/ARCHITECTURE.md",
        "docs/BUILD.md",
        "docs/DEVELOPER.md",
        "docs/RT_SAFETY.md",
        "docs/CODING_CONVENTIONS.md",
        "docs/THREADING_MODEL.md",
    ]
    
    # Documentation directories to check for presence
    REQUIRED_DOC_DIRS = [
        "docs/",
        "docs/tech-briefs/",
    ]
    
    # Minimum number of tech briefs expected
    MIN_TECH_BRIEFS = 3

    def __init__(self, project_root: Optional[Path] = None, max_age_days: int = 180):
        """
        Initialize Documentation Agent.
        
        Args:
            project_root: Root directory of the project
            max_age_days: Maximum age in days before flagging as outdated
        """
        self.project_root = project_root or Path.cwd()
        self.max_age_days = max_age_days
        self.report = DocReport()
        self.report.scan_timestamp = datetime.now().isoformat()

    def validate_required_docs(self) -> None:
        """Check for presence of required documentation files."""
        print("Checking required documentation files...")
        
        for doc_path in self.REQUIRED_DOCS:
            full_path = self.project_root / doc_path
            
            if not full_path.exists():
                self.report.issues.append(DocIssue(
                    severity=Severity.CRITICAL,
                    status=DocStatus.MISSING,
                    file_path=Path(doc_path),
                    description=f"Required documentation file is missing: {doc_path}",
                    recommendation=f"Create {doc_path} with relevant project information"
                ))
            else:
                self.report.checked_files += 1

    def check_doc_directories(self) -> None:
        """Verify required documentation directories exist."""
        print("Checking required documentation directories...")
        
        for doc_dir in self.REQUIRED_DOC_DIRS:
            full_path = self.project_root / doc_dir
            
            if not full_path.exists():
                self.report.issues.append(DocIssue(
                    severity=Severity.HIGH,
                    status=DocStatus.MISSING,
                    file_path=Path(doc_dir),
                    description=f"Required documentation directory is missing: {doc_dir}",
                    recommendation=f"Create {doc_dir} directory for project documentation"
                ))

    def check_tech_briefs(self) -> None:
        """Check for ADRs/tech-briefs."""
        print("Checking for Architecture Decision Records and tech briefs...")
        
        tech_briefs_dir = self.project_root / "docs/tech-briefs"
        
        if not tech_briefs_dir.exists():
            self.report.issues.append(DocIssue(
                severity=Severity.MEDIUM,
                status=DocStatus.MISSING,
                file_path=Path("docs/tech-briefs"),
                description="Tech briefs directory not found",
                recommendation="Create docs/tech-briefs/ for architecture decision records"
            ))
            return
        
        # Count tech briefs
        tech_brief_files = list(tech_briefs_dir.glob("*.md"))
        self.report.checked_files += len(tech_brief_files)
        
        if len(tech_brief_files) < self.MIN_TECH_BRIEFS:
            self.report.issues.append(DocIssue(
                severity=Severity.LOW,
                status=DocStatus.NEEDS_UPDATE,
                file_path=Path("docs/tech-briefs"),
                description=f"Only {len(tech_brief_files)} tech briefs found - consider documenting key architectural decisions",
                recommendation="Add architecture decision records for major technical choices"
            ))

    def check_doc_freshness(self) -> None:
        """
        Check if documentation files have been updated recently.
        
        Uses the max_age_days configured during initialization.
        """
        print(f"Checking documentation freshness (max age: {self.max_age_days} days)...")
        
        cutoff_date = datetime.now() - timedelta(days=self.max_age_days)
        
        for doc_path in self.REQUIRED_DOCS:
            full_path = self.project_root / doc_path
            
            if not full_path.exists():
                continue
            
            try:
                # Get last modification time
                mtime = datetime.fromtimestamp(full_path.stat().st_mtime)
                
                if mtime < cutoff_date:
                    self.report.issues.append(DocIssue(
                        severity=Severity.LOW,
                        status=DocStatus.OUTDATED,
                        file_path=Path(doc_path),
                        description=f"Documentation may be outdated (last modified: {mtime.strftime('%Y-%m-%d')})",
                        recommendation=f"Review and update {doc_path} to ensure accuracy"
                    ))
            except Exception as e:
                print(f"Warning: Could not check freshness of {doc_path}: {e}")

    def check_agent_documentation(self) -> None:
        """Check that each agent has a README."""
        print("Checking agent documentation...")
        
        agents_dir = self.project_root / "agents"
        if not agents_dir.exists():
            return
        
        for agent_dir in agents_dir.iterdir():
            if not agent_dir.is_dir() or agent_dir.name.startswith('.'):
                continue
            
            readme_path = agent_dir / "README.md"
            if not readme_path.exists():
                self.report.issues.append(DocIssue(
                    severity=Severity.MEDIUM,
                    status=DocStatus.MISSING,
                    file_path=readme_path.relative_to(self.project_root),
                    description=f"Agent {agent_dir.name} is missing README.md",
                    recommendation=f"Create README.md documenting {agent_dir.name}'s purpose and usage"
                ))
            else:
                self.report.checked_files += 1

    def check_public_api_documentation(self) -> None:
        """Check for documentation of public APIs and classes."""
        print("Checking public API documentation...")
        
        # Look for public header files
        include_dir = self.project_root / "include"
        if not include_dir.exists():
            return
        
        header_files = list(include_dir.rglob("*.h")) + list(include_dir.rglob("*.hpp"))
        
        for header in header_files:
            try:
                with open(header, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                
                # Look for class declarations
                classes = re.findall(r'class\s+(\w+)', content)
                
                # Check for Doxygen-style comments (/** */ or ///)
                has_docs = bool(re.search(r'/\*\*|///', content))
                
                if classes and not has_docs:
                    self.report.issues.append(DocIssue(
                        severity=Severity.LOW,
                        status=DocStatus.NEEDS_UPDATE,
                        file_path=header.relative_to(self.project_root),
                        description=f"Public header {header.name} has {len(classes)} classes but no Doxygen comments",
                        recommendation="Add Doxygen-style documentation comments for public APIs"
                    ))
            except Exception as e:
                print(f"Warning: Could not check {header}: {e}")

    def run_all_checks(self) -> DocReport:
        """
        Run all documentation validation checks.
        
        Returns:
            Comprehensive documentation report
        """
        print("=" * 60)
        print("Documentation Agent - Running Validation Checks")
        print("=" * 60)
        
        self.validate_required_docs()
        self.check_doc_directories()
        self.check_tech_briefs()
        self.check_doc_freshness()
        self.check_agent_documentation()
        self.check_public_api_documentation()
        
        return self.report

    def print_report(self) -> None:
        """Print formatted report to console."""
        print("\n" + "=" * 60)
        print("Documentation Validation Report")
        print("=" * 60)
        print(f"Timestamp: {self.report.scan_timestamp}")
        print(f"Files checked: {self.report.checked_files}")
        print(f"Issues found: {len(self.report.issues)}")
        print(f"  Critical: {self.report.critical_count}")
        print(f"  High: {self.report.high_count}")
        print(f"  Missing docs: {self.report.missing_count}")
        
        if self.report.issues:
            print("\n" + "-" * 60)
            print("Issues:")
            print("-" * 60)
            
            # Sort by severity
            severity_order = [Severity.CRITICAL, Severity.HIGH, Severity.MEDIUM, Severity.LOW, Severity.INFO]
            sorted_issues = sorted(self.report.issues, key=lambda i: severity_order.index(i.severity))
            
            for issue in sorted_issues:
                print(f"\n[{issue.severity.value.upper()}] {issue.status.value.upper()}")
                if issue.file_path:
                    print(f"  File: {issue.file_path}")
                print(f"  {issue.description}")
                if issue.recommendation:
                    print(f"  → {issue.recommendation}")
        
        print("\n" + "=" * 60)
        
        if self.report.critical_count > 0 or self.report.high_count > 0:
            print("⚠️  ATTENTION: Critical or high-severity documentation issues found!")
            return
        
        print("✅ Documentation validation complete")

    def save_report_json(self, output_path: Optional[Path] = None) -> Path:
        """
        Save report to JSON file.
        
        Args:
            output_path: Path to save JSON report
            
        Returns:
            Path to saved report
        """
        if output_path is None:
            output_path = self.project_root / "documentation_report.json"
        
        data = {
            "timestamp": self.report.scan_timestamp,
            "checked_files": self.report.checked_files,
            "total_issues": len(self.report.issues),
            "critical_issues": self.report.critical_count,
            "high_issues": self.report.high_count,
            "missing_docs": self.report.missing_count,
            "issues": [
                {
                    "severity": issue.severity.value,
                    "status": issue.status.value,
                    "file": str(issue.file_path) if issue.file_path else None,
                    "description": issue.description,
                    "recommendation": issue.recommendation
                }
                for issue in self.report.issues
            ]
        }
        
        with open(output_path, 'w') as f:
            json.dump(data, f, indent=2)
        
        print(f"Report saved to: {output_path}")
        return output_path


def main():
    """Main entry point for documentation agent."""
    import argparse
    
    parser = argparse.ArgumentParser(description="Documentation Agent - Validate project documentation")
    parser.add_argument("--project-root", type=Path, help="Project root directory")
    parser.add_argument("--output-json", type=Path, help="Save report as JSON")
    parser.add_argument("--max-age-days", type=int, default=180, help="Max age in days before flagging as outdated")
    parser.add_argument("--fail-on-critical", action="store_true", help="Exit with error code if critical issues found")
    
    args = parser.parse_args()
    
    agent = DocumentationAgent(project_root=args.project_root, max_age_days=args.max_age_days)
    report = agent.run_all_checks()
    agent.print_report()
    
    if args.output_json:
        agent.save_report_json(args.output_json)
    
    # Exit with error if critical issues and flag is set
    if args.fail_on_critical and report.critical_count > 0:
        print("\n❌ Exiting with error due to critical documentation issues")
        sys.exit(1)
    
    sys.exit(0)


if __name__ == "__main__":
    main()
