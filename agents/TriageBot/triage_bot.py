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

        # Compile regex patterns
        self._compile_patterns()

    def _compile_patterns(self):
        """Pre-compile regex patterns for performance."""
        # Issue Type Patterns
        issue_type_keywords = {
            "BUG": ["crash", "crashes", "crashed", "crashing", "segfault", "error", "broken", "fails", "failed", "failure"],
            "SECURITY": ["security", "vulnerability", "cve"],
            "PERFORMANCE": ["slow", "performance", "lag", "latency"],
            "FEATURE_REQUEST": ["feature request", "would be nice", "add support"],
            "QUESTION": ["how to", "how do i", "question", "help"],
            "DOCUMENTATION": ["documentation", "docs", "readme"]
        }
        self.issue_type_regex = self._build_compiled_regex(issue_type_keywords)

        # Priority Patterns
        priority_keywords = {
            "CRITICAL": ["crash", "crashes", "crashed", "crashing", "segfault", "data loss", "security", "vulnerability", "cannot use"],
            "HIGH": ["broken", "not working", "unusable", "blocking"]
        }
        self.priority_regex = self._build_compiled_regex(priority_keywords)

        # Component Labels Patterns
        component_keywords = {
            "audio": ["audio", "sound", "playback", "recording"],
            "midi": ["midi", "controller", "notes"],
            "ui": ["ui", "interface", "rendering", "display"],
            "plugin": ["plugin", "vst", "effect", "instrument"],
            "ai": ["ai", "grok", "assistant"],
            "collaboration": ["collaboration", "network", "webrtc"],
            "build": ["build", "compile", "cmake"],
        }
        self.component_regex = self._build_compiled_regex(component_keywords)

        # Platform Patterns
        platform_keywords = {
            "linux": ["linux"],
            "macos": ["macos"],
            "windows": ["windows"]
        }
        self.platform_regex = self._build_compiled_regex(platform_keywords)

        # Maintainer Patterns
        self.maintainer_regex = self._build_compiled_regex(self.maintainers)

    def _build_compiled_regex(self, keyword_dict: Dict[str, List[str]]) -> re.Pattern:
        """
        Build a single compiled regex from a dictionary of keywords.
        Uses named groups and word boundaries.
        """
        patterns = []
        for key, keywords in keyword_dict.items():
            # Validate key is a valid regex group name
            if not re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*$', key):
                raise ValueError(f"Invalid group name '{key}'. Regex group names must be alphanumeric.")

            # Escape keywords to handle special characters safely
            escaped_keywords = [re.escape(k) for k in keywords]
            # Join keywords with OR
            pattern_group = "|".join(escaped_keywords)
            # Create named group with word boundaries
            # \b(?:...) \b ensures whole word matching
            patterns.append(f"(?P<{key}>\\b(?:{pattern_group})\\b)")

        full_pattern = "|".join(patterns)
        return re.compile(full_pattern, re.IGNORECASE)

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
        response = self._generate_response(issue, issue_type, is_duplicate, duplicate_of)
        
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
        text = issue.title + " " + issue.body

        matches = {m.lastgroup for m in self.issue_type_regex.finditer(text)}
        
        # Check in priority order
        if "BUG" in matches:
            return IssueType.BUG
        
        if "SECURITY" in matches:
            return IssueType.SECURITY
        
        if "PERFORMANCE" in matches:
            return IssueType.PERFORMANCE

        if "FEATURE_REQUEST" in matches:
            return IssueType.FEATURE_REQUEST

        if "QUESTION" in matches:
            return IssueType.QUESTION

        if "DOCUMENTATION" in matches:
            return IssueType.DOCUMENTATION
        
        return IssueType.ENHANCEMENT

    def _determine_priority(self, issue: Issue, issue_type: IssueType) -> Priority:
        """Determine issue priority."""
        text = issue.title + " " + issue.body
        
        matches = {m.lastgroup for m in self.priority_regex.finditer(text)}
        
        if "CRITICAL" in matches:
            return Priority.CRITICAL

        if "HIGH" in matches:
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
        text = issue.title + " " + issue.body
        
        component_matches = {m.lastgroup for m in self.component_regex.finditer(text)}
        for component in component_matches:
            labels.add(f"component:{component}")
        
        # Add platform labels
        platform_matches = {m.lastgroup for m in self.platform_regex.finditer(text)}
        for platform in platform_matches:
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
        text = issue.title + " " + issue.body
        
        # Find all maintainers with matching expertise
        matches = {m.lastgroup for m in self.maintainer_regex.finditer(text)}
        return list(matches)

    def _generate_response(self, issue: Issue, 
                          issue_type: IssueType,
                          is_duplicate: bool,
                          duplicate_of: Optional[int] = None) -> Optional[str]:
        """Generate automated response if applicable."""
        if is_duplicate and duplicate_of:
            return (f"Thank you for reporting this issue. "
                   f"It appears to be a duplicate of issue #{duplicate_of}. "
                   f"Please check that issue for updates.")
        
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
