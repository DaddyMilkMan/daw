"""
triage_bot.py

Agent for automated issue and pull request triage.

This module provides intelligent triage capabilities including
classification, labeling, priority assignment, and routing.
"""

import asyncio
import logging
import re
from typing import Dict, List, Optional, Any, Set
from dataclasses import dataclass, field
from enum import Enum
from datetime import datetime, timedelta


class IssueType(Enum):
    """Types of issues"""
    BUG = "bug"
    ENHANCEMENT = "enhancement"
    FEATURE = "feature"
    DOCUMENTATION = "documentation"
    QUESTION = "question"
    PERFORMANCE = "performance"
    SECURITY = "security"


class Priority(Enum):
    """Issue priority levels"""
    CRITICAL = "critical"
    HIGH = "high"
    MEDIUM = "medium"
    LOW = "low"


class Component(Enum):
    """DAW components"""
    AUDIO_ENGINE = "audio-engine"
    MIDI = "midi"
    UI = "ui"
    PLUGINS = "plugins"
    NETWORKING = "networking"
    BUILD_SYSTEM = "build-system"
    DOCUMENTATION = "docs"
    TESTING = "testing"


@dataclass
class TriageDecision:
    """Represents a triage decision"""
    issue_number: int
    issue_type: IssueType
    priority: Priority
    components: List[Component] = field(default_factory=list)
    labels: List[str] = field(default_factory=list)
    assignees: List[str] = field(default_factory=list)
    reason: str = ""


@dataclass
class Issue:
    """Represents a GitHub issue or PR"""
    number: int
    title: str
    body: str
    author: str
    created_at: datetime
    updated_at: datetime
    labels: List[str] = field(default_factory=list)
    is_pull_request: bool = False
    comments_count: int = 0


class TriageBot:
    """
    Automates issue and pull request triage.
    
    Classifies issues, assigns labels and priorities, routes to
    appropriate team members, and manages stale issues.
    """
    
    # Keywords for classification
    BUG_KEYWORDS = [
        "bug", "crash", "error", "broken", "fail", "issue", "problem",
        "not working", "doesn't work", "regression"
    ]
    
    ENHANCEMENT_KEYWORDS = [
        "enhance", "improve", "optimization", "optimize", "better", "refactor"
    ]
    
    FEATURE_KEYWORDS = [
        "feature", "add", "new", "implement", "support", "allow"
    ]
    
    SECURITY_KEYWORDS = [
        "security", "vulnerability", "cve", "exploit", "xss", "injection",
        "authentication", "authorization", "crypto"
    ]
    
    PERFORMANCE_KEYWORDS = [
        "performance", "slow", "fast", "speed", "latency", "optimize",
        "memory", "cpu", "lag"
    ]
    
    # Component detection patterns
    COMPONENT_PATTERNS = {
        Component.AUDIO_ENGINE: [
            r"audio\s*(engine|render|process)",
            r"realtime|rt[\s-]safe",
            r"buffer|sample\s*rate"
        ],
        Component.MIDI: [
            r"midi",
            r"note\s*(on|off)",
            r"cc|control\s*change"
        ],
        Component.UI: [
            r"ui|user\s*interface",
            r"gui|window|display",
            r"button|component"
        ],
        Component.PLUGINS: [
            r"plugin|vst",
            r"effect|instrument"
        ],
        Component.NETWORKING: [
            r"network|socket",
            r"collaboration|sync",
            r"webrtc"
        ],
    }
    
    def __init__(self, config: Optional[Dict[str, Any]] = None):
        """
        Initialize triage bot.
        
        Args:
            config: Optional configuration dictionary
        """
        self.config = config or {}
        self.logger = logging.getLogger(__name__)
        self.triage_decisions: Dict[int, TriageDecision] = {}
        self.stale_threshold_days = self.config.get("stale_threshold_days", 60)
        
    async def initialize(self) -> None:
        """Initialize the bot"""
        self.logger.info("Initializing Triage Bot")
        # TODO: Initialize GitHub API client
        # TODO: Load triage history
        
    async def triage_issue(self, issue: Issue) -> TriageDecision:
        """
        Triage an issue or pull request.
        
        Args:
            issue: Issue to triage
            
        Returns:
            Triage decision
        """
        self.logger.info(f"Triaging issue #{issue.number}: {issue.title}")
        
        # Classify issue type
        issue_type = self._classify_issue_type(issue)
        
        # Determine priority
        priority = self._determine_priority(issue, issue_type)
        
        # Identify affected components
        components = self._identify_components(issue)
        
        # Generate labels
        labels = self._generate_labels(issue_type, priority, components)
        
        # Suggest assignees
        assignees = self._suggest_assignees(components)
        
        decision = TriageDecision(
            issue_number=issue.number,
            issue_type=issue_type,
            priority=priority,
            components=components,
            labels=labels,
            assignees=assignees,
            reason=f"Classified as {issue_type.value} with {priority.value} priority"
        )
        
        self.triage_decisions[issue.number] = decision
        return decision
    
    def _classify_issue_type(self, issue: Issue) -> IssueType:
        """Classify issue type based on content"""
        text = f"{issue.title} {issue.body}".lower()
        
        # Check for security issues first (highest priority)
        if any(kw in text for kw in self.SECURITY_KEYWORDS):
            return IssueType.SECURITY
        
        # Check for bugs
        if any(kw in text for kw in self.BUG_KEYWORDS):
            return IssueType.BUG
        
        # Check for performance issues
        if any(kw in text for kw in self.PERFORMANCE_KEYWORDS):
            return IssueType.PERFORMANCE
        
        # Check for features
        if any(kw in text for kw in self.FEATURE_KEYWORDS):
            return IssueType.FEATURE
        
        # Check for enhancements
        if any(kw in text for kw in self.ENHANCEMENT_KEYWORDS):
            return IssueType.ENHANCEMENT
        
        # Check for documentation
        if re.search(r'\bdoc(umentation)?\b', text) or re.search(r'\breadme\b', text):
            return IssueType.DOCUMENTATION
        
        # Default to question
        return IssueType.QUESTION
    
    def _determine_priority(self, issue: Issue, issue_type: IssueType) -> Priority:
        """Determine issue priority"""
        text = f"{issue.title} {issue.body}".lower()
        
        # Security issues are always critical
        if issue_type == IssueType.SECURITY:
            return Priority.CRITICAL
        
        # Check for critical keywords
        if any(kw in text for kw in ["crash", "data loss", "critical", "urgent"]):
            return Priority.CRITICAL
        
        # Bugs are typically high priority
        if issue_type == IssueType.BUG:
            return Priority.HIGH
        
        # Performance issues are medium-high
        if issue_type == IssueType.PERFORMANCE:
            return Priority.MEDIUM
        
        # Features and enhancements are medium
        if issue_type in (IssueType.FEATURE, IssueType.ENHANCEMENT):
            return Priority.MEDIUM
        
        # Everything else is low
        return Priority.LOW
    
    def _identify_components(self, issue: Issue) -> List[Component]:
        """Identify affected components"""
        text = f"{issue.title} {issue.body}".lower()
        components = []
        
        for component, patterns in self.COMPONENT_PATTERNS.items():
            for pattern in patterns:
                if re.search(pattern, text, re.IGNORECASE):
                    components.append(component)
                    break
        
        return components
    
    def _generate_labels(self, issue_type: IssueType, 
                        priority: Priority,
                        components: List[Component]) -> List[str]:
        """Generate labels for the issue"""
        labels = [issue_type.value, f"priority-{priority.value}"]
        labels.extend([f"component-{comp.value}" for comp in components])
        return labels
    
    def _suggest_assignees(self, components: List[Component]) -> List[str]:
        """Suggest assignees based on components"""
        # TODO: Implement actual team member assignment logic
        # This is a placeholder
        assignee_map = {
            Component.AUDIO_ENGINE: ["audio-team"],
            Component.MIDI: ["audio-team"],
            Component.UI: ["ui-team"],
            Component.PLUGINS: ["audio-team"],
            Component.NETWORKING: ["backend-team"],
        }
        
        assignees = []
        for component in components:
            if component in assignee_map:
                assignees.extend(assignee_map[component])
        
        return list(set(assignees))  # Remove duplicates
    
    async def find_stale_issues(self, issues: List[Issue]) -> List[Issue]:
        """
        Find stale issues that haven't been updated recently.
        
        Args:
            issues: List of issues to check
            
        Returns:
            List of stale issues
        """
        stale_cutoff = datetime.now() - timedelta(days=self.stale_threshold_days)
        
        stale = [
            issue for issue in issues
            if issue.updated_at < stale_cutoff and not issue.is_pull_request
        ]
        
        self.logger.info(f"Found {len(stale)} stale issues")
        return stale
    
    async def apply_triage_decision(self, issue_number: int) -> bool:
        """
        Apply triage decision to GitHub issue.
        
        Args:
            issue_number: Issue number
            
        Returns:
            True if successful
        """
        if issue_number not in self.triage_decisions:
            self.logger.error(f"No triage decision for issue #{issue_number}")
            return False
        
        decision = self.triage_decisions[issue_number]
        
        # TODO: Use GitHub API to apply labels
        # TODO: Add comment explaining triage decision
        # TODO: Assign to team members
        
        self.logger.info(
            f"Applied triage to #{issue_number}: "
            f"{decision.issue_type.value}, {decision.priority.value}"
        )
        return True
    
    def generate_report(self) -> str:
        """
        Generate triage report.
        
        Returns:
            Formatted report string
        """
        report_lines = ["=== Triage Bot Report ===\n"]
        
        if not self.triage_decisions:
            report_lines.append("No issues triaged yet.")
            return "\n".join(report_lines)
        
        # Count by type
        type_counts = {}
        priority_counts = {}
        
        for decision in self.triage_decisions.values():
            issue_type = decision.issue_type.value
            priority = decision.priority.value
            
            type_counts[issue_type] = type_counts.get(issue_type, 0) + 1
            priority_counts[priority] = priority_counts.get(priority, 0) + 1
        
        report_lines.append(f"Total Issues Triaged: {len(self.triage_decisions)}\n")
        
        report_lines.append("By Type:")
        for issue_type, count in sorted(type_counts.items()):
            report_lines.append(f"  - {issue_type}: {count}")
        
        report_lines.append("\nBy Priority:")
        priority_emoji = {
            "critical": "🔴",
            "high": "🟠",
            "medium": "🟡",
            "low": "🟢"
        }
        for priority, count in sorted(priority_counts.items()):
            emoji = priority_emoji.get(priority, "⚪")
            report_lines.append(f"  {emoji} {priority}: {count}")
        
        # Recent decisions
        report_lines.append("\n\nRecent Triage Decisions:")
        for issue_num, decision in list(self.triage_decisions.items())[-5:]:
            report_lines.append(
                f"  #{issue_num}: {decision.issue_type.value} "
                f"({decision.priority.value})"
            )
            if decision.components:
                comps = ", ".join([c.value for c in decision.components])
                report_lines.append(f"    Components: {comps}")
        
        return "\n".join(report_lines)


# Example usage
if __name__ == "__main__":
    async def main():
        bot = TriageBot()
        await bot.initialize()
        
        # Create test issue
        issue = Issue(
            number=123,
            title="Audio engine crashes when loading VST plugin",
            body="The audio engine crashes immediately when I try to load a VST3 plugin.",
            author="testuser",
            created_at=datetime.now(),
            updated_at=datetime.now()
        )
        
        # Triage the issue
        decision = await bot.triage_issue(issue)
        print(f"Triage decision: {decision}")
        
        # Generate report
        report = bot.generate_report()
        print("\n" + report)
    
    asyncio.run(main())
