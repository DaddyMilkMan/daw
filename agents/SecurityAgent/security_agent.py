"""
security_agent.py

Agent for automated security analysis and vulnerability detection.

This module provides security scanning, vulnerability detection,
and security best practice enforcement.
"""

import asyncio
import logging
import re
from typing import Dict, List, Optional, Any, Set
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path


class VulnerabilitySeverity(Enum):
    """Vulnerability severity levels"""
    CRITICAL = "critical"
    HIGH = "high"
    MEDIUM = "medium"
    LOW = "low"
    INFO = "info"


class VulnerabilityType(Enum):
    """Types of security vulnerabilities"""
    SQL_INJECTION = "sql_injection"
    XSS = "cross_site_scripting"
    BUFFER_OVERFLOW = "buffer_overflow"
    PATH_TRAVERSAL = "path_traversal"
    HARDCODED_SECRET = "hardcoded_secret"
    WEAK_CRYPTO = "weak_cryptography"
    INSECURE_DESERIALIZE = "insecure_deserialization"
    COMMAND_INJECTION = "command_injection"
    RACE_CONDITION = "race_condition"
    DEPENDENCY_VULN = "dependency_vulnerability"


@dataclass
class SecurityVulnerability:
    """Represents a detected security vulnerability"""
    vuln_type: VulnerabilityType
    severity: VulnerabilitySeverity
    file_path: str
    line_number: int
    description: str
    recommendation: str
    cwe_id: Optional[str] = None
    code_snippet: Optional[str] = None


@dataclass
class DependencyVulnerability:
    """Represents a vulnerability in a dependency"""
    package_name: str
    version: str
    cve_id: str
    severity: VulnerabilitySeverity
    description: str
    fixed_version: Optional[str] = None


class SecurityAgent:
    """
    Performs automated security analysis and vulnerability detection.
    
    Scans code for security vulnerabilities, checks dependencies,
    and enforces security best practices.
    """
    
    # Patterns for detecting potential vulnerabilities
    VULNERABILITY_PATTERNS = {
        VulnerabilityType.HARDCODED_SECRET: [
            r'(?i)(password|passwd|pwd)\s*=\s*["\'][^"\']+["\']',
            r'(?i)(api[_-]?key|apikey)\s*=\s*["\'][^"\']+["\']',
            r'(?i)(secret|token)\s*=\s*["\'][^"\']{20,}["\']',
        ],
        VulnerabilityType.SQL_INJECTION: [
            r'execute\s*\(\s*["\'].*\+.*["\']',
            r'query\s*\(\s*[f"\'].*{.*}',
        ],
        VulnerabilityType.COMMAND_INJECTION: [
            r'system\s*\(\s*.*\+',
            r'exec\s*\(\s*.*\+',
            r'shell\s*=\s*True',
        ],
        VulnerabilityType.PATH_TRAVERSAL: [
            r'open\s*\(\s*.*\+.*["\']/',
            r'Path\s*\(\s*.*\+',
        ],
    }
    
    def __init__(self, config: Optional[Dict[str, Any]] = None):
        """
        Initialize security agent.
        
        Args:
            config: Optional configuration dictionary
        """
        self.config = config or {}
        self.logger = logging.getLogger(__name__)
        self.vulnerabilities: List[SecurityVulnerability] = []
        self.dependency_vulns: List[DependencyVulnerability] = []
        self.scanned_files: Set[str] = set()
        
    async def initialize(self) -> None:
        """Initialize the agent"""
        self.logger.info("Initializing Security Agent")
        # TODO: Initialize vulnerability databases
        # TODO: Load security policy configurations
        
    async def scan_file(self, file_path: str) -> List[SecurityVulnerability]:
        """
        Scan a file for security vulnerabilities.
        
        Args:
            file_path: Path to file to scan
            
        Returns:
            List of detected vulnerabilities
        """
        self.logger.info(f"Scanning file: {file_path}")
        
        vulnerabilities = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
                
            for line_num, line in enumerate(lines, start=1):
                # Check for vulnerability patterns
                for vuln_type, patterns in self.VULNERABILITY_PATTERNS.items():
                    for pattern in patterns:
                        if re.search(pattern, line):
                            vuln = self._create_vulnerability(
                                vuln_type, file_path, line_num, line.strip()
                            )
                            vulnerabilities.append(vuln)
            
            self.scanned_files.add(file_path)
            self.vulnerabilities.extend(vulnerabilities)
            
        except Exception as e:
            self.logger.error(f"Error scanning {file_path}: {e}")
        
        return vulnerabilities
    
    def _create_vulnerability(self, vuln_type: VulnerabilityType, 
                             file_path: str, line_number: int,
                             code_snippet: str) -> SecurityVulnerability:
        """Create a vulnerability object with appropriate metadata"""
        
        descriptions = {
            VulnerabilityType.HARDCODED_SECRET: 
                "Hardcoded credential or secret detected",
            VulnerabilityType.SQL_INJECTION: 
                "Potential SQL injection vulnerability",
            VulnerabilityType.COMMAND_INJECTION: 
                "Potential command injection vulnerability",
            VulnerabilityType.PATH_TRAVERSAL: 
                "Potential path traversal vulnerability",
        }
        
        recommendations = {
            VulnerabilityType.HARDCODED_SECRET:
                "Use environment variables or secure key management service",
            VulnerabilityType.SQL_INJECTION:
                "Use parameterized queries or prepared statements",
            VulnerabilityType.COMMAND_INJECTION:
                "Avoid using shell=True and sanitize all user input",
            VulnerabilityType.PATH_TRAVERSAL:
                "Validate and sanitize file paths, use safe path operations",
        }
        
        severities = {
            VulnerabilityType.HARDCODED_SECRET: VulnerabilitySeverity.CRITICAL,
            VulnerabilityType.SQL_INJECTION: VulnerabilitySeverity.HIGH,
            VulnerabilityType.COMMAND_INJECTION: VulnerabilitySeverity.HIGH,
            VulnerabilityType.PATH_TRAVERSAL: VulnerabilitySeverity.MEDIUM,
        }
        
        return SecurityVulnerability(
            vuln_type=vuln_type,
            severity=severities.get(vuln_type, VulnerabilitySeverity.MEDIUM),
            file_path=file_path,
            line_number=line_number,
            description=descriptions.get(vuln_type, "Security issue detected"),
            recommendation=recommendations.get(vuln_type, "Review and fix"),
            code_snippet=code_snippet
        )
    
    async def scan_dependencies(self, requirements_file: str) -> List[DependencyVulnerability]:
        """
        Scan dependencies for known vulnerabilities.
        
        Args:
            requirements_file: Path to requirements.txt or similar
            
        Returns:
            List of dependency vulnerabilities
        """
        self.logger.info(f"Scanning dependencies: {requirements_file}")
        
        # TODO: Integrate with vulnerability database (e.g., OSV, NVD)
        # TODO: Parse dependency file
        # TODO: Check each dependency against CVE database
        
        vulnerabilities = []
        self.dependency_vulns.extend(vulnerabilities)
        return vulnerabilities
    
    async def check_authentication_code(self, file_path: str) -> List[SecurityVulnerability]:
        """
        Check authentication/authorization code for security issues.
        
        Args:
            file_path: Path to file containing auth code
            
        Returns:
            List of detected vulnerabilities
        """
        self.logger.info(f"Checking authentication code: {file_path}")
        
        vulnerabilities = []
        
        # TODO: Check for weak password policies
        # TODO: Verify proper session management
        # TODO: Check for authorization bypasses
        # TODO: Verify secure token handling
        
        return vulnerabilities
    
    def get_critical_vulnerabilities(self) -> List[SecurityVulnerability]:
        """
        Get all critical and high severity vulnerabilities.
        
        Returns:
            List of critical/high vulnerabilities
        """
        return [
            vuln for vuln in self.vulnerabilities
            if vuln.severity in (VulnerabilitySeverity.CRITICAL, VulnerabilitySeverity.HIGH)
        ]
    
    def generate_report(self) -> str:
        """
        Generate security analysis report.
        
        Returns:
            Formatted report string
        """
        report_lines = ["=== Security Agent Report ===\n"]
        
        # Summary
        total_vulns = len(self.vulnerabilities)
        critical = sum(1 for v in self.vulnerabilities if v.severity == VulnerabilitySeverity.CRITICAL)
        high = sum(1 for v in self.vulnerabilities if v.severity == VulnerabilitySeverity.HIGH)
        medium = sum(1 for v in self.vulnerabilities if v.severity == VulnerabilitySeverity.MEDIUM)
        low = sum(1 for v in self.vulnerabilities if v.severity == VulnerabilitySeverity.LOW)
        
        report_lines.append(f"Total Vulnerabilities: {total_vulns}")
        report_lines.append(f"  🔴 Critical: {critical}")
        report_lines.append(f"  🟠 High: {high}")
        report_lines.append(f"  🟡 Medium: {medium}")
        report_lines.append(f"  🟢 Low: {low}\n")
        
        # Critical vulnerabilities detail
        if critical > 0 or high > 0:
            report_lines.append("Critical/High Severity Issues:")
            for vuln in self.get_critical_vulnerabilities()[:10]:  # Show top 10
                severity_emoji = {
                    VulnerabilitySeverity.CRITICAL: "🔴",
                    VulnerabilitySeverity.HIGH: "🟠"
                }.get(vuln.severity, "⚪")
                
                report_lines.append(f"\n  {severity_emoji} {vuln.description}")
                report_lines.append(f"     File: {vuln.file_path}:{vuln.line_number}")
                report_lines.append(f"     Type: {vuln.vuln_type.value}")
                report_lines.append(f"     Fix: {vuln.recommendation}")
                if vuln.code_snippet:
                    report_lines.append(f"     Code: {vuln.code_snippet[:80]}...")
        
        # Dependency vulnerabilities
        if self.dependency_vulns:
            report_lines.append(f"\n\nDependency Vulnerabilities ({len(self.dependency_vulns)}):")
            for dep_vuln in self.dependency_vulns[:5]:  # Show top 5
                report_lines.append(f"  - {dep_vuln.package_name} {dep_vuln.version}")
                report_lines.append(f"    CVE: {dep_vuln.cve_id}")
                report_lines.append(f"    Severity: {dep_vuln.severity.value}")
                if dep_vuln.fixed_version:
                    report_lines.append(f"    Fix: Upgrade to {dep_vuln.fixed_version}")
        
        # Recommendations
        if total_vulns > 0:
            report_lines.append("\n\nRecommendations:")
            report_lines.append("  1. Address all critical and high severity issues immediately")
            report_lines.append("  2. Review and fix medium severity issues")
            report_lines.append("  3. Update dependencies with known vulnerabilities")
            report_lines.append("  4. Implement automated security scanning in CI/CD")
        else:
            report_lines.append("\n✅ No vulnerabilities detected!")
        
        return "\n".join(report_lines)


# Example usage
if __name__ == "__main__":
    async def main():
        agent = SecurityAgent()
        await agent.initialize()
        
        # Scan a file
        # vulns = await agent.scan_file("example.py")
        # print(f"Found {len(vulns)} vulnerabilities")
        
        # Generate report
        report = agent.generate_report()
        print(report)
    
    asyncio.run(main())
