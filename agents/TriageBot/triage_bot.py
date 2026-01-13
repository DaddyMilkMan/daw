"""
Triage Bot for automated issue and PR management.

This agent automates issue triage, labeling, and routing for the DAW project,
helping maintainers efficiently process incoming issues and pull requests.
"""

import logging
import re
from dataclasses import dataclass
from datetime import datetime
from typing import Dict, List, Optional, Set
from enum import Enum

logger = logging.getLogger(__name__)


class IssueType(Enum):
    """Types of issues."""
    BUG = "bug"
    FEATURE = "feature"
    ENHANCEMENT = "enhancement"
    DOCUMENTATION = "documentation"
    QUESTION = "question"
    SECURITY = "security"


class Priority(Enum):
    """Issue priority levels."""
    CRITICAL = "critical"
    HIGH = "high"
    MEDIUM = "medium"
    LOW = "low"


class Component(Enum):
    """DAW components/subsystems."""
    AUDIO_ENGINE = "audio-engine"
    TRANSPORT = "transport"
    UI = "ui"
    NETWORKING = "networking"
    PLUGINS = "plugins"
    DSP = "dsp"
    BUILD = "build"
    TESTING = "testing"
    DOCUMENTATION = "documentation"


@dataclass
class TriageResult:
    """Results of issue triage."""
    issue_type: IssueType
    priority: Priority
    components: List[Component]
    labels: List[str]
    assignees: List[str]
    response: Optional[str]
    timestamp: datetime
    
    def __init__(self):
        self.issue_type = IssueType.BUG
        self.priority = Priority.MEDIUM
        self.components = []
        self.labels = []
        self.assignees = []
        self.response = None
        self.timestamp = datetime.now()


class TriageBot:
    """
    Automates issue triage and PR management.
    
    This agent analyzes issues and pull requests to automatically categorize,
    label, and route them to appropriate maintainers.
    """
    
    def __init__(self, github_token: Optional[str] = None):
        """
        Initialize the Triage Bot.
        
        Args:
            github_token: GitHub API token for authentication
        """
        self.github_token = github_token
        self.initialized = False
        
        # Keyword patterns for classification
        self.bug_keywords = [
            "crash", "error", "exception", "bug", "broken", "failed",
            "doesn't work", "not working", "issue with"
        ]
        
        self.feature_keywords = [
            "feature request", "enhancement", "improvement", "add support for",
            "would be nice", "please add"
        ]
        
        self.security_keywords = [
            "security", "vulnerability", "exploit", "CVE", "injection",
            "buffer overflow", "authentication bypass"
        ]
        
        # Component detection patterns
        self.component_patterns = {
            Component.AUDIO_ENGINE: [
                "audio engine", "AudioRenderer", "processBlock", "audio callback",
                "buffer", "sample rate"
            ],
            Component.TRANSPORT: [
                "transport", "play", "stop", "record", "playhead", "tempo",
                "TransportController"
            ],
            Component.UI: [
                "UI", "GUI", "user interface", "button", "menu", "window",
                "display", "render", "draw"
            ],
            Component.NETWORKING: [
                "network", "WebRTC", "collaboration", "sync", "connection",
                "signaling"
            ],
            Component.PLUGINS: [
                "VST", "plugin", "AU", "instrument", "effect", "synthesizer"
            ],
            Component.DSP: [
                "DSP", "filter", "oscillator", "envelope", "LFO", "signal processing"
            ],
            Component.BUILD: [
                "build", "CMake", "compilation", "compiler", "linker",
                "dependencies"
            ],
            Component.TESTING: [
                "test", "unit test", "integration test", "CI", "continuous integration"
            ]
        }
        
        logger.info("TriageBot created")
    
    def initialize(self) -> bool:
        """
        Initialize the agent and connect to GitHub API.
        
        Returns:
            True if initialization was successful
        """
        # TODO: Initialize GitHub API connection
        # from github import Github
        # self.github = Github(self.github_token)
        
        self.initialized = True
        logger.info("TriageBot initialized")
        return True
    
    def triage_issue(self, title: str, body: str) -> TriageResult:
        """
        Triage an issue based on its title and body.
        
        Args:
            title: Issue title
            body: Issue body/description
            
        Returns:
            Triage results with classification and labels
        """
        logger.info(f"Triaging issue: {title}")
        
        result = TriageResult()
        content = (title + " " + body).lower()
        
        # Detect issue type
        result.issue_type = self._detect_issue_type(content)
        
        # Detect priority
        result.priority = self._detect_priority(content)
        
        # Detect components
        result.components = self._detect_components(content)
        
        # Generate labels
        result.labels = self._generate_labels(result)
        
        # Generate automated response if applicable
        result.response = self._generate_response(result)
        
        logger.info(f"Triage completed: type={result.issue_type.value}, "
                   f"priority={result.priority.value}, "
                   f"components={[c.value for c in result.components]}")
        
        return result
    
    def _detect_issue_type(self, content: str) -> IssueType:
        """Detect issue type from content."""
        
        # Check for security issues first (highest priority)
        if any(keyword in content for keyword in self.security_keywords):
            return IssueType.SECURITY
        
        # Check for bugs
        if any(keyword in content for keyword in self.bug_keywords):
            return IssueType.BUG
        
        # Check for feature requests
        if any(keyword in content for keyword in self.feature_keywords):
            return IssueType.FEATURE
        
        # Check for documentation
        if "documentation" in content or "docs" in content or "readme" in content:
            return IssueType.DOCUMENTATION
        
        # Check for questions
        if content.startswith("how to") or content.startswith("how do i"):
            return IssueType.QUESTION
        
        # Default to bug
        return IssueType.BUG
    
    def _detect_priority(self, content: str) -> Priority:
        """Detect priority from content."""
        
        # Critical indicators
        critical_keywords = [
            "crash", "data loss", "security", "vulnerability", "critical",
            "production down", "urgent"
        ]
        if any(keyword in content for keyword in critical_keywords):
            return Priority.CRITICAL
        
        # High priority indicators
        high_keywords = [
            "regression", "broken", "not working", "blocking", "important"
        ]
        if any(keyword in content for keyword in high_keywords):
            return Priority.HIGH
        
        # Low priority indicators
        low_keywords = [
            "minor", "nice to have", "enhancement", "cosmetic", "typo"
        ]
        if any(keyword in content for keyword in low_keywords):
            return Priority.LOW
        
        # Default to medium
        return Priority.MEDIUM
    
    def _detect_components(self, content: str) -> List[Component]:
        """Detect affected components from content."""
        
        detected = []
        
        for component, keywords in self.component_patterns.items():
            if any(keyword.lower() in content for keyword in keywords):
                detected.append(component)
        
        return detected
    
    def _generate_labels(self, result: TriageResult) -> List[str]:
        """Generate GitHub labels from triage result."""
        
        labels = []
        
        # Add type label
        labels.append(result.issue_type.value)
        
        # Add priority label
        if result.priority in (Priority.CRITICAL, Priority.HIGH):
            labels.append(result.priority.value)
        
        # Add component labels
        for component in result.components:
            labels.append(component.value)
        
        return labels
    
    def _generate_response(self, result: TriageResult) -> Optional[str]:
        """Generate automated response if applicable."""
        
        # Security issues get special handling
        if result.issue_type == IssueType.SECURITY:
            return (
                "Thank you for reporting a potential security issue. "
                "Please email security@zenith-daw.com with details instead "
                "of posting publicly. We will address this as soon as possible."
            )
        
        # Questions get directed to discussions
        if result.issue_type == IssueType.QUESTION:
            return (
                "Thanks for your question! For support questions, please use "
                "GitHub Discussions instead of issues. Issues are for bug reports "
                "and feature requests."
            )
        
        return None
    
    def triage_pull_request(self, title: str, body: str, files_changed: List[str]) -> TriageResult:
        """
        Triage a pull request.
        
        Args:
            title: PR title
            body: PR description
            files_changed: List of files changed in PR
            
        Returns:
            Triage results
        """
        logger.info(f"Triaging PR: {title}")
        
        # Start with issue triage logic
        result = self.triage_issue(title, body)
        
        # Enhance with file-based component detection
        for file_path in files_changed:
            if "audio" in file_path.lower() or "engine" in file_path.lower():
                if Component.AUDIO_ENGINE not in result.components:
                    result.components.append(Component.AUDIO_ENGINE)
            
            if "transport" in file_path.lower():
                if Component.TRANSPORT not in result.components:
                    result.components.append(Component.TRANSPORT)
            
            if "ui" in file_path.lower() or "component" in file_path.lower():
                if Component.UI not in result.components:
                    result.components.append(Component.UI)
            
            if ".md" in file_path.lower() or "docs/" in file_path.lower():
                if Component.DOCUMENTATION not in result.components:
                    result.components.append(Component.DOCUMENTATION)
        
        # Regenerate labels with updated components
        result.labels = self._generate_labels(result)
        
        logger.info(f"PR triage completed: {result.labels}")
        return result
    
    def apply_triage(self, issue_number: int, result: TriageResult) -> bool:
        """
        Apply triage results to a GitHub issue/PR.
        
        Args:
            issue_number: Issue or PR number
            result: Triage results to apply
            
        Returns:
            True if application was successful
        """
        logger.info(f"Applying triage to issue #{issue_number}")
        
        # TODO: Implement GitHub API calls to:
        # - Add labels
        # - Set priority
        # - Assign to maintainers
        # - Post automated response
        
        logger.info(f"Triage applied to issue #{issue_number}")
        return True


# Example usage
if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    
    bot = TriageBot()
    if bot.initialize():
        # Example issue triage
        result = bot.triage_issue(
            "Audio engine crash on stop",
            "When I press stop during recording, the audio engine crashes."
        )
        print(f"Issue Type: {result.issue_type.value}")
        print(f"Priority: {result.priority.value}")
        print(f"Labels: {result.labels}")
        print(f"Components: {[c.value for c in result.components]}")
