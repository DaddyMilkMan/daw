#!/usr/bin/env python3
"""
TriageBot - Coordinator agent for automated issue and PR triage

Automatically categorizes, prioritizes, and routes issues and pull requests
for the DAW project.
"""

import logging
import re
from typing import Dict, List, Optional, Set
from dataclasses import dataclass, field
from enum import Enum
from datetime import datetime, timedelta


class Priority(Enum):
    """Issue priority levels"""
    CRITICAL = "critical"
    HIGH = "high"
    MEDIUM = "medium"
    LOW = "low"


class IssueType(Enum):
    """Types of issues"""
    BUG = "bug"
    FEATURE = "feature"
    ENHANCEMENT = "enhancement"
    DOCUMENTATION = "documentation"
    PERFORMANCE = "performance"
    SECURITY = "security"
    QUESTION = "question"


@dataclass
class Issue:
    """Represents a GitHub issue or PR"""
    number: int
    title: str
    body: str
    author: str
    labels: Set[str] = field(default_factory=set)
    created_at: datetime = field(default_factory=datetime.now)
    updated_at: datetime = field(default_factory=datetime.now)
    is_pull_request: bool = False


@dataclass
class TriageResult:
    """Result of issue triage"""
    issue_number: int
    suggested_type: IssueType
    suggested_priority: Priority
    suggested_labels: Set[str]
    suggested_assignees: List[str]
    duplicate_of: Optional[int] = None
    related_issues: List[int] = field(default_factory=list)
    reasoning: str = ""


@dataclass
class TriageReport:
    """Triage analysis report"""
    total_issues: int
    triaged_issues: int
    duplicates_found: int
    stale_issues: List[int] = field(default_factory=list)
    high_priority_issues: List[int] = field(default_factory=list)
    recommendations: List[str] = field(default_factory=list)


class TriageBot:
    """
    Coordinator agent for automated issue triage.
    
    Analyzes, categorizes, and routes issues and pull requests.
    """

    def __init__(self, repository: str = "micahcooley/daw"):
        self.logger = logging.getLogger(__name__)
        self.repository = repository
        self.triage_history: List[TriageResult] = []
        self.category_keywords = self._load_category_keywords()

    def _load_category_keywords(self) -> Dict[IssueType, List[str]]:
        """Load keywords for issue categorization"""
        return {
            IssueType.BUG: ["crash", "error", "broken", "fails", "doesn't work", "bug"],
            IssueType.FEATURE: ["feature", "add", "new", "implement"],
            IssueType.ENHANCEMENT: ["improve", "enhance", "optimize", "better"],
            IssueType.DOCUMENTATION: ["docs", "documentation", "readme", "guide"],
            IssueType.PERFORMANCE: ["slow", "performance", "lag", "optimization", "latency"],
            IssueType.SECURITY: ["security", "vulnerability", "cve", "exploit"],
            IssueType.QUESTION: ["question", "how to", "help", "?"],
        }

    def triage_issue(self, issue: Issue) -> TriageResult:
        """
        Triage a single issue
        
        Args:
            issue: Issue to triage
            
        Returns:
            Triage result with recommendations
        """
        self.logger.info(f"Triaging issue #{issue.number}: {issue.title}")
        
        # Categorize
        issue_type = self._categorize_issue(issue)
        
        # Prioritize
        priority = self._determine_priority(issue, issue_type)
        
        # Suggest labels
        labels = self._suggest_labels(issue, issue_type, priority)
        
        # Suggest assignees
        assignees = self._suggest_assignees(issue, issue_type)
        
        result = TriageResult(
            issue_number=issue.number,
            suggested_type=issue_type,
            suggested_priority=priority,
            suggested_labels=labels,
            suggested_assignees=assignees,
            reasoning=f"Categorized as {issue_type.value} with {priority.value} priority"
        )
        
        self.triage_history.append(result)
        return result

    def _categorize_issue(self, issue: Issue) -> IssueType:
        """Categorize issue based on content"""
        text = (issue.title + " " + issue.body).lower()
        
        # Score each category
        scores = {}
        for issue_type, keywords in self.category_keywords.items():
            score = sum(1 for keyword in keywords if keyword in text)
            scores[issue_type] = score
        
        # Return highest scoring category or default to BUG
        if scores and max(scores.values()) > 0:
            return max(scores, key=scores.get)
        return IssueType.BUG

    def _determine_priority(self, issue: Issue, issue_type: IssueType) -> Priority:
        """Determine issue priority"""
        text = (issue.title + " " + issue.body).lower()
        
        # Critical keywords
        if any(word in text for word in ["crash", "data loss", "security", "critical"]):
            return Priority.CRITICAL
        
        # High priority for bugs and performance issues
        if issue_type in [IssueType.BUG, IssueType.SECURITY, IssueType.PERFORMANCE]:
            return Priority.HIGH
        
        # Medium for features and enhancements
        if issue_type in [IssueType.FEATURE, IssueType.ENHANCEMENT]:
            return Priority.MEDIUM
        
        return Priority.LOW

    def _suggest_labels(self, issue: Issue, issue_type: IssueType, priority: Priority) -> Set[str]:
        """Suggest appropriate labels"""
        labels = {issue_type.value, priority.value}
        
        # Add component labels based on keywords
        text = (issue.title + " " + issue.body).lower()
        if any(word in text for word in ["audio", "dsp", "realtime"]):
            labels.add("audio-engine")
        if any(word in text for word in ["ui", "interface", "visual"]):
            labels.add("ui")
        if any(word in text for word in ["plugin", "vst"]):
            labels.add("plugins")
        
        return labels

    def _suggest_assignees(self, issue: Issue, issue_type: IssueType) -> List[str]:
        """Suggest maintainers to assign"""
        # TODO: Implement assignee routing logic
        return []

    def find_duplicates(self, issue: Issue, all_issues: List[Issue]) -> List[int]:
        """
        Find potential duplicate issues
        
        Args:
            issue: Issue to check
            all_issues: All existing issues
            
        Returns:
            List of potential duplicate issue numbers
        """
        duplicates = []
        # TODO: Implement similarity detection
        return duplicates

    def find_stale_issues(self, issues: List[Issue], days: int = 90) -> List[int]:
        """
        Find stale issues that haven't been updated
        
        Args:
            issues: List of issues to check
            days: Number of days without update to consider stale
            
        Returns:
            List of stale issue numbers
        """
        threshold = datetime.now() - timedelta(days=days)
        stale = [issue.number for issue in issues if issue.updated_at < threshold]
        self.logger.info(f"Found {len(stale)} stale issues (>{days} days)")
        return stale

    def generate_triage_report(self, issues: List[Issue]) -> TriageReport:
        """Generate comprehensive triage report"""
        report = TriageReport(
            total_issues=len(issues),
            triaged_issues=len(self.triage_history),
            duplicates_found=0
        )
        
        # TODO: Generate detailed report
        return report


if __name__ == "__main__":
    # Basic test
    logging.basicConfig(level=logging.INFO)
    bot = TriageBot()
    print("Triage Bot initialized")
    
    # Test with a sample issue
    test_issue = Issue(
        number=123,
        title="Audio crashes on startup",
        body="The application crashes immediately when trying to initialize audio",
        author="testuser"
    )
    
    result = bot.triage_issue(test_issue)
    print(f"Triage result: {result.suggested_type.value}, {result.suggested_priority.value}")
    print(f"Suggested labels: {', '.join(result.suggested_labels)}")
