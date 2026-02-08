"""
Security Agent for vulnerability scanning and threat detection.

This module provides security analysis, input validation,
and protection against common vulnerabilities.
"""

from typing import Dict, List, Optional, Set
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
import hashlib
import re
import subprocess
import platform


class VulnerabilityType(Enum):
    """Types of security vulnerabilities."""
    BUFFER_OVERFLOW = "buffer_overflow"
    INJECTION = "injection"
    PATH_TRAVERSAL = "path_traversal"
    INSECURE_DESERIALIZATION = "insecure_deserialization"
    MEMORY_LEAK = "memory_leak"
    RACE_CONDITION = "race_condition"
    WEAK_CRYPTO = "weak_crypto"
    HARDCODED_SECRETS = "hardcoded_secrets"


class Severity(Enum):
    """Vulnerability severity levels."""
    CRITICAL = "critical"
    HIGH = "high"
    MEDIUM = "medium"
    LOW = "low"
    INFO = "info"


@dataclass
class Vulnerability:
    """Security vulnerability finding."""
    vuln_type: VulnerabilityType
    severity: Severity
    description: str
    file_path: Optional[Path] = None
    line_number: Optional[int] = None
    recommendation: str = ""
    cwe_id: Optional[str] = None  # Common Weakness Enumeration


@dataclass
class SecurityReport:
    """Comprehensive security analysis report."""
    vulnerabilities: List[Vulnerability] = field(default_factory=list)
    scanned_files: int = 0
    scan_duration_seconds: float = 0.0
    
    @property
    def critical_count(self) -> int:
        return sum(1 for v in self.vulnerabilities if v.severity == Severity.CRITICAL)
    
    @property
    def high_count(self) -> int:
        return sum(1 for v in self.vulnerabilities if v.severity == Severity.HIGH)


class SecurityAgent:
    """
    Security Agent for comprehensive vulnerability scanning and protection.
    
    Performs static analysis, input validation, and threat detection
    to ensure secure operation of the DAW.
    """

    def __init__(self):
        """Initialize Security Agent."""
        self.known_safe_plugins: Set[str] = set()  # SHA256 hashes
        self.blocked_patterns: List[re.Pattern] = []
        self._initialize_patterns()

    def _initialize_patterns(self) -> None:
        """Initialize security scanning patterns."""
        # TODO: Load comprehensive vulnerability patterns
        self.blocked_patterns = [
            re.compile(r"\bstrcpy\s*\("),  # Unsafe string copy
            re.compile(r"\bsprintf\s*\("),  # Unsafe string formatting
            re.compile(r"\bgets\s*\("),  # Unsafe input reading (not method names)
            re.compile(r"\bsystem\s*\("),  # Command injection risk
        ]

    def scan_source_code(self, source_files: List[Path]) -> SecurityReport:
        """
        Perform static analysis on source code for vulnerabilities.
        
        Args:
            source_files: List of source files to scan
            
        Returns:
            Security report with findings
        """
        print(f"Scanning {len(source_files)} source files...")
        
        report = SecurityReport()
        report.scanned_files = len(source_files)
        
        for file_path in source_files:
            vulnerabilities = self._scan_file(file_path)
            report.vulnerabilities.extend(vulnerabilities)
        
        return report

    def _scan_file(self, file_path: Path) -> List[Vulnerability]:
        """Scan a single file for vulnerabilities."""
        vulnerabilities = []
        
        if not file_path.exists():
            return vulnerabilities
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
            
            for line_num, line in enumerate(lines, start=1):
                # Check for unsafe functions
                for pattern in self.blocked_patterns:
                    if pattern.search(line):
                        vulnerabilities.append(Vulnerability(
                            vuln_type=VulnerabilityType.BUFFER_OVERFLOW,
                            severity=Severity.HIGH,
                            description=f"Unsafe function usage: {pattern.pattern}",
                            file_path=file_path,
                            line_number=line_num,
                            recommendation="Use safe alternatives (strncpy, snprintf)"
                        ))
                
                # Check for hardcoded secrets
                if self._check_hardcoded_secrets(line):
                    vulnerabilities.append(Vulnerability(
                        vuln_type=VulnerabilityType.HARDCODED_SECRETS,
                        severity=Severity.CRITICAL,
                        description="Potential hardcoded secret detected",
                        file_path=file_path,
                        line_number=line_num,
                        recommendation="Use environment variables or secure vaults"
                    ))
        
        except Exception as e:
            print(f"Error scanning {file_path}: {e}")
        
        return vulnerabilities

    def _check_hardcoded_secrets(self, line: str) -> bool:
        """Check for hardcoded secrets in code."""
        # TODO: Implement entropy-based detection
        # TODO: Check for API key patterns
        
        secret_patterns = [
            r"password\s*=\s*['\"][^'\"]+['\"]",
            r"api_key\s*=\s*['\"][^'\"]+['\"]",
            r"secret\s*=\s*['\"][^'\"]+['\"]",
        ]
        
        for pattern in secret_patterns:
            if re.search(pattern, line, re.IGNORECASE):
                return True
        
        return False

    def validate_input(self, data: bytes, expected_format: str) -> bool:
        """
        Validate and sanitize external input data.
        
        Args:
            data: Input data to validate
            expected_format: Expected format (midi, wav, xml, json)
            
        Returns:
            True if input is valid and safe
        """
        # TODO: Implement format-specific validation
        # TODO: Check for malformed data
        # TODO: Validate size limits
        # TODO: Scan for malicious patterns
        
        if len(data) == 0:
            return False
        
        # Check size limits
        max_sizes = {
            "midi": 10 * 1024 * 1024,  # 10MB
            "wav": 500 * 1024 * 1024,  # 500MB
            "xml": 10 * 1024 * 1024,   # 10MB
            "json": 10 * 1024 * 1024,  # 10MB
        }
        
        max_size = max_sizes.get(expected_format, 10 * 1024 * 1024)
        if len(data) > max_size:
            print(f"Input exceeds size limit for {expected_format}")
            return False
        
        return True

    def verify_plugin_signature(self, plugin_path: Path) -> bool:
        """
        Verify plugin code signature and integrity.
        
        Args:
            plugin_path: Path to plugin binary
            
        Returns:
            True if plugin is trusted
        """
        if not plugin_path.exists():
            return False
        
        try:
            # Calculate SHA256 hash
            sha256_hash = hashlib.sha256()
            with open(plugin_path, "rb") as f:
                for chunk in iter(lambda: f.read(4096), b""):
                    sha256_hash.update(chunk)

            plugin_hash = sha256_hash.hexdigest()

            # Fast path: Check against trusted plugin database
            if plugin_hash in self.known_safe_plugins:
                return True

            # If not in allowlist, verify digital signature
            is_signed = self._verify_signature(plugin_path)

            if not is_signed:
                # Avoid spamming logs on unsupported platforms
                if platform.system() in ["Darwin", "Windows"]:
                    print(f"Warning: Plugin verification failed for {plugin_path.name}")
                return False

            return True

        except Exception as e:
            print(f"Error verifying plugin {plugin_path}: {e}")
            return False

    def _verify_signature(self, plugin_path: Path) -> bool:
        """
        Verify digital signature based on OS.
        """
        system = platform.system()
        
        if system == "Darwin":
            return self._verify_macos_signature(plugin_path)
        elif system == "Windows":
            return self._verify_windows_signature(plugin_path)
        else:
            # On Linux/Other, we fail closed if not in allowlist
            # We don't spam warnings here as verify_plugin_signature handles the warning
            return False

    def _verify_macos_signature(self, plugin_path: Path) -> bool:
        """Verify macOS codesign signature."""
        try:
            result = subprocess.run(
                ["codesign", "-v", "--strict", str(plugin_path)],
                capture_output=True,
                text=True,
                timeout=10,
                check=False
            )
            return result.returncode == 0
        except Exception:
            return False

    def _verify_windows_signature(self, plugin_path: Path) -> bool:
        """Verify Windows Authenticode signature."""
        try:
            # Escape single quotes for PowerShell (which uses double single-quotes)
            safe_path = str(plugin_path).replace("'", "''")

            # Use PowerShell to get deterministic status string
            cmd = [
                "powershell",
                "-NoProfile",
                "-NonInteractive",
                "-Command",
                f"(Get-AuthenticodeSignature -FilePath '{safe_path}').Status.ToString()"
            ]

            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=10,
                check=False
            )

            if result.returncode != 0:
                return False

            # Strict checking: must be exactly "Valid"
            return result.stdout.strip() == "Valid"
        except Exception:
            return False

    def add_trusted_plugin(self, plugin_path: Path) -> None:
        """
        Add a plugin to the trusted list.
        
        Args:
            plugin_path: Path to trusted plugin
        """
        if not plugin_path.exists():
            return
        
        sha256_hash = hashlib.sha256()
        with open(plugin_path, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                sha256_hash.update(chunk)
        
        self.known_safe_plugins.add(sha256_hash.hexdigest())
        print(f"Added trusted plugin: {plugin_path.name}")

    def scan_dependencies(self, dependency_file: Path) -> List[Vulnerability]:
        """
        Scan project dependencies for known vulnerabilities.
        
        Args:
            dependency_file: Package manifest (requirements.txt, package.json)
            
        Returns:
            List of vulnerabilities in dependencies
        """
        # TODO: Parse dependency file
        # TODO: Query vulnerability databases (CVE, NVD)
        # TODO: Check for outdated packages
        # TODO: Recommend updates
        
        vulnerabilities = []
        return vulnerabilities


def print_report(report: SecurityReport, output_format: str = "standard") -> None:
    """Print security report in specified format."""
    if output_format == "ci":
        # GitHub Actions friendly output
        print("\n" + "="*60)
        print("🔒 SECURITY SCAN RESULTS")
        print("="*60)
        print(f"Files scanned: {report.scanned_files}")
        print(f"Vulnerabilities found: {len(report.vulnerabilities)}")
        print(f"  - Critical: {report.critical_count}")
        print(f"  - High: {report.high_count}")
        print("="*60)
        
        if report.vulnerabilities:
            print("\n📋 DETAILED FINDINGS:\n")
            for vuln in report.vulnerabilities:
                severity_emoji = {
                    Severity.CRITICAL: "🚨",
                    Severity.HIGH: "⚠️",
                    Severity.MEDIUM: "⚡",
                    Severity.LOW: "ℹ️",
                    Severity.INFO: "💡"
                }
                print(f"{severity_emoji.get(vuln.severity, '•')} [{vuln.severity.value.upper()}] {vuln.description}")
                if vuln.file_path:
                    print(f"   File: {vuln.file_path}:{vuln.line_number or 0}")
                if vuln.recommendation:
                    print(f"   Recommendation: {vuln.recommendation}")
                if vuln.cwe_id:
                    print(f"   CWE: {vuln.cwe_id}")
                print()
        else:
            print("\n✅ No vulnerabilities detected!\n")
    else:
        # Standard output
        print(f"Scanned {report.scanned_files} files")
        print(f"Found {len(report.vulnerabilities)} vulnerabilities")
        for vuln in report.vulnerabilities:
            print(f"  {vuln.severity.value}: {vuln.description} at {vuln.file_path}:{vuln.line_number}")


# CLI usage
if __name__ == "__main__":
    import argparse
    import sys
    import time
    
    parser = argparse.ArgumentParser(description="Security Agent - Vulnerability Scanner")
    parser.add_argument("--scan-dir", default=".", help="Directory to scan")
    parser.add_argument("--output", choices=["standard", "ci"], default="standard", 
                       help="Output format (standard or ci)")
    parser.add_argument("--extensions", nargs="+", 
                       default=[".cpp", ".h", ".hpp", ".cc", ".c", ".py", ".js", ".ts"],
                       help="File extensions to scan")
    parser.add_argument("--max-files", type=int, default=None, 
                       help="Maximum number of files to scan (for testing)")
    
    args = parser.parse_args()
    
    agent = SecurityAgent()
    scan_dir = Path(args.scan_dir)
    
    if not scan_dir.exists():
        print(f"Error: Directory not found: {scan_dir}")
        sys.exit(1)
    
    # Collect source files
    source_files = []
    for ext in args.extensions:
        source_files.extend(scan_dir.rglob(f"*{ext}"))
    
    # Filter out generated/external files
    filtered_files = [
        f for f in source_files 
        if not any(part in f.parts for part in 
                  ["build", "external", "_deps", "node_modules", ".git", "venv"])
    ]
    
    if args.max_files:
        filtered_files = filtered_files[:args.max_files]
    
    # Perform scan
    start_time = time.time()
    report = agent.scan_source_code(filtered_files)
    report.scan_duration_seconds = time.time() - start_time
    
    # Print results
    print_report(report, args.output)
    
    # Exit with error code if critical or high vulnerabilities found
    if report.critical_count > 0 or report.high_count > 0:
        sys.exit(1)
    else:
        sys.exit(0)
