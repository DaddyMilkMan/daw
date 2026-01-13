"""
Triage Bot for automated issue and PR management.

This module provides intelligent issue triage, labeling, routing,
and automated responses for GitHub repository management.
"""

from typing import Dict, List, Optional, Set, Tuple
from dataclasses import dataclass, field
from enum import Enum
from datetime import datetime, timedelta
import re


class IssueType(Enum):
    """Types of GitHub issues."""
    BUG = "bug"
    FEATURE_REQUEST = "feature"
    ENHANCEMENT = "enhancement"
    QUESTION = "question"
    DOCUMENTATION = "documentation"
    PERFORMANCE = "performance"
    SECURITY = "security"


class Priority(Enum):
    """Issue priority levels."""
    CRITICAL = "P0"  # Crashes, data loss, security
    HIGH = "P1"      # Major functionality broken
    MEDIUM = "P2"    # Moderate impact
    LOW = "P3"       # Minor issues, nice-to-haves


@dataclass
class Issue:
    """GitHub issue representation."""
    number: int
    title: str
    body: str
    author: str
    labels: Set[str] = field(default_factory=set)
    assignees: List[str] = field(default_factory=list)
    created_at: datetime = field(default_factory=datetime.now)
    updated_at: datetime = field(default_factory=datetime.now)


@dataclass
class TriageResult:
    """Result of issue triage."""
    issue_type: IssueType
    priority: Priority
    suggested_labels: Set[str]
    suggested_assignees: List[str]
    is_duplicate: bool
    duplicate_of: Optional[int] = None
    automated_response: Optional[str] = None


class TriageBot:
    """
    Triage Bot for automated GitHub issue and PR management.
    
    Classifies issues, assigns labels, detects duplicates,
    and provides automated responses.
    """

    def __init__(self, repo_owner: str, repo_name: str):
        """
        Initialize Triage Bot.
        
        Args:
            repo_owner: GitHub repository owner
            repo_name: GitHub repository name
        """
        self.repo_owner = repo_owner
        self.repo_name = repo_name
        self.maintainers = self._load_maintainer_expertise()
        self.known_issues: List[Issue] = []

    def _load_maintainer_expertise(self) -> Dict[str, List[str]]:
        """Load maintainer expertise areas."""
        # TODO: Load from CODEOWNERS or configuration
        return {
            "audio_expert": ["audio", "dsp", "realtime", "performance"],
            "ui_expert": ["ui", "rendering", "skia"],
            "ai_expert": ["ai", "ml", "grok"],
            "network_expert": ["collaboration", "webrtc", "networking"],
            "platform_expert": ["build", "cmake", "platform-specific"],
        }

    def triage_issue(self, issue: Issue) -> TriageResult:
        """
        Perform automated triage on an issue.
        
        Args:
            issue: Issue to triage
            
        Returns:
            Triage result with recommendations
        """
        # Classify issue type
        issue_type = self._classify_issue_type(issue)
        
        # Determine priority
        priority = self._determine_priority(issue, issue_type)
        
        # Suggest labels
        labels = self._suggest_labels(issue, issue_type, priority)
        
        # Detect duplicates
        is_duplicate, duplicate_of = self._detect_duplicate(issue)
        
        # Route to appropriate maintainer
        assignees = self._route_to_maintainer(issue, issue_type)
        
        # Generate automated response if applicable
        response = self._generate_response(issue, issue_type, is_duplicate)
        
        return TriageResult(
            issue_type=issue_type,
            priority=priority,
            suggested_labels=labels,
            suggested_assignees=assignees,
            is_duplicate=is_duplicate,
            duplicate_of=duplicate_of,
            automated_response=response
        )

    def _classify_issue_type(self, issue: Issue) -> IssueType:
        """Classify issue type from title and body."""
        text = (issue.title + " " + issue.body).lower()
        
        # Pattern matching for common keywords
        if any(word in text for word in ["crash", "segfault", "error", "broken", "fails"]):
            return IssueType.BUG
        
        if any(word in text for word in ["security", "vulnerability", "cve"]):
            return IssueType.SECURITY
        
        if any(word in text for word in ["slow", "performance", "lag", "latency"]):
            return IssueType.PERFORMANCE
        
        if any(word in text for word in ["feature request", "would be nice", "add support"]):
            return IssueType.FEATURE_REQUEST
        
        if any(word in text for word in ["how to", "how do i", "question", "help"]):
            return IssueType.QUESTION
        
        if any(word in text for word in ["documentation", "docs", "readme"]):
            return IssueType.DOCUMENTATION
        
        return IssueType.ENHANCEMENT

    def _determine_priority(self, issue: Issue, issue_type: IssueType) -> Priority:
        """Determine issue priority."""
        text = (issue.title + " " + issue.body).lower()
        
        # Critical keywords
        critical_keywords = ["crash", "data loss", "security", "vulnerability", "cannot use"]
        if any(keyword in text for keyword in critical_keywords):
            return Priority.CRITICAL
        
        # High priority keywords
        high_keywords = ["broken", "not working", "unusable", "blocking"]
        if any(keyword in text for keyword in high_keywords):
            return Priority.HIGH
        
        # Security issues are always at least high priority
        if issue_type == IssueType.SECURITY:
            return Priority.CRITICAL
        
        # Bug issues default to medium
        if issue_type == IssueType.BUG:
            return Priority.MEDIUM
        
        # Everything else is low
        return Priority.LOW

    def _suggest_labels(self, issue: Issue, 
                       issue_type: IssueType, 
                       priority: Priority) -> Set[str]:
        """Suggest appropriate labels for issue."""
        labels = set()
        
        # Add type label
        labels.add(issue_type.value)
        
        # Add priority label
        labels.add(priority.value)
        
        # Add component labels based on content
        text = (issue.title + " " + issue.body).lower()
        
        component_keywords = {
            "audio": ["audio", "sound", "playback", "recording"],
            "midi": ["midi", "controller", "notes"],
            "ui": ["ui", "interface", "rendering", "display"],
            "plugin": ["plugin", "vst", "effect", "instrument"],
            "ai": ["ai", "grok", "assistant"],
            "collaboration": ["collaboration", "network", "webrtc"],
            "build": ["build", "compile", "cmake"],
        }
        
        for component, keywords in component_keywords.items():
            if any(keyword in text for keyword in keywords):
                labels.add(f"component:{component}")
        
        # Add platform labels
        for platform in ["linux", "macos", "windows"]:
            if platform in text:
                labels.add(f"platform:{platform}")
        
        return labels

    def _detect_duplicate(self, issue: Issue) -> Tuple[bool, Optional[int]]:
        """Detect if issue is a duplicate."""
        # TODO: Implement similarity search using embeddings
        # TODO: Compare against known issues
        # TODO: Use fuzzy matching on titles
        
        # Simple keyword matching as placeholder
        for known_issue in self.known_issues:
            similarity = self._calculate_similarity(issue.title, known_issue.title)
            if similarity > 0.8:  # High similarity threshold
                return True, known_issue.number
        
        return False, None

    def _calculate_similarity(self, text1: str, text2: str) -> float:
        """Calculate text similarity (0-1)."""
        # TODO: Use better similarity metric (embeddings, TF-IDF)
        
        # Simple word overlap
        words1 = set(text1.lower().split())
        words2 = set(text2.lower().split())
        
        if not words1 or not words2:
            return 0.0
        
        overlap = len(words1 & words2)
        total = len(words1 | words2)
        
        return overlap / total if total > 0 else 0.0

    def _route_to_maintainer(self, issue: Issue, 
                            issue_type: IssueType) -> List[str]:
        """Route issue to appropriate maintainer."""
        text = (issue.title + " " + issue.body).lower()
        
        # Find maintainer with matching expertise
        for maintainer, expertise in self.maintainers.items():
            if any(area in text for area in expertise):
                return [maintainer]
        
        # Default to no assignment (will be manually triaged)
        return []

    def _generate_response(self, issue: Issue, 
                          issue_type: IssueType,
                          is_duplicate: bool) -> Optional[str]:
        """Generate automated response if applicable."""
        if is_duplicate:
            return ("Thank you for reporting this issue. "
                   "It appears to be a duplicate of an existing issue. "
                   "Please check the linked issue for updates.")
        
        if issue_type == IssueType.QUESTION:
            return ("Thank you for your question! "
                   "For faster assistance, please check our documentation at docs/ "
                   "or join our community discussions.")
        
        # No automated response for other types
        return None

    def check_stale_issues(self, days: int = 30) -> List[Issue]:
        """
        Find issues that have been inactive for specified days.
        
        Args:
            days: Number of days of inactivity
            
        Returns:
            List of stale issues
        """
        cutoff_date = datetime.now() - timedelta(days=days)
        stale_issues = [
            issue for issue in self.known_issues
            if issue.updated_at < cutoff_date
        ]
        
        return stale_issues

    def add_known_issue(self, issue: Issue) -> None:
        """Add issue to known issues database."""
        self.known_issues.append(issue)


# Example usage
if __name__ == "__main__":
    bot = TriageBot(repo_owner="micahcooley", repo_name="daw")
    
    # Example issue
    issue = Issue(
        number=123,
        title="Audio crashes when loading large project",
        body="When I load a project with more than 50 tracks, the application crashes.",
        author="user123"
    )
    
    result = bot.triage_issue(issue)
    print(f"Type: {result.issue_type.value}")
    print(f"Priority: {result.priority.value}")
    print(f"Labels: {result.suggested_labels}")
    print(f"Assignees: {result.suggested_assignees}")
