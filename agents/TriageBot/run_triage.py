#!/usr/bin/env python3
"""
GitHub Actions runner for TriageBot.

This script is called by the GitHub Actions workflow to process
issue and PR events. It reads the event data from GitHub Actions
context and uses the TriageBot to analyze and triage the issue/PR.
"""

import os
import sys
import json
from pathlib import Path
from typing import Dict, Any, Optional, List
import urllib.request
import urllib.parse
from urllib.error import HTTPError

# Add the agent directory to the path
sys.path.insert(0, str(Path(__file__).parent))

from triage_bot import TriageBot, Issue, TriageResult


class GitHubAPIClient:
    """Simple GitHub API client using urllib (no external dependencies)."""
    
    def __init__(self, token: str, repo_owner: str, repo_name: str):
        """
        Initialize GitHub API client.
        
        Args:
            token: GitHub API token
            repo_owner: Repository owner
            repo_name: Repository name
        """
        self.token = token
        self.repo_owner = repo_owner
        self.repo_name = repo_name
        self.api_base = "https://api.github.com"
    
    def _make_request(self, method: str, endpoint: str, 
                     data: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        """
        Make an API request to GitHub.
        
        Args:
            method: HTTP method (GET, POST, PATCH, etc.)
            endpoint: API endpoint (e.g., '/repos/owner/repo/issues/1')
            data: Request body data
            
        Returns:
            Response JSON
        """
        url = f"{self.api_base}{endpoint}"
        headers = {
            "Authorization": f"Bearer {self.token}",
            "Accept": "application/vnd.github+json",
            "X-GitHub-Api-Version": "2022-11-28",
            "User-Agent": "TriageBot/1.0"
        }
        
        req_data = json.dumps(data).encode('utf-8') if data else None
        
        request = urllib.request.Request(
            url,
            data=req_data,
            headers=headers,
            method=method
        )
        
        try:
            with urllib.request.urlopen(request) as response:
                return json.loads(response.read().decode('utf-8'))
        except HTTPError as e:
            error_body = e.read().decode('utf-8')
            # Sanitize error output - don't print full response body which may contain sensitive info
            print(f"GitHub API error: {e.code} {e.reason}")
            print(f"URL: {url}")
            raise
    
    def add_labels(self, issue_number: int, labels: List[str]) -> Dict[str, Any]:
        """
        Add labels to an issue or PR.
        
        Args:
            issue_number: Issue/PR number
            labels: List of label names to add
            
        Returns:
            Updated issue data
        """
        endpoint = f"/repos/{self.repo_owner}/{self.repo_name}/issues/{issue_number}/labels"
        return self._make_request("POST", endpoint, {"labels": labels})
    
    def create_comment(self, issue_number: int, body: str) -> Dict[str, Any]:
        """
        Create a comment on an issue or PR.
        
        Args:
            issue_number: Issue/PR number
            body: Comment text
            
        Returns:
            Created comment data
        """
        endpoint = f"/repos/{self.repo_owner}/{self.repo_name}/issues/{issue_number}/comments"
        return self._make_request("POST", endpoint, {"body": body})
    
    def list_repository_labels(self) -> List[Dict[str, Any]]:
        """
        List all labels in the repository.
        
        Returns:
            List of label objects
        """
        endpoint = f"/repos/{self.repo_owner}/{self.repo_name}/labels"
        return self._make_request("GET", endpoint)
    
    def create_label(self, name: str, color: str, description: str = "") -> Dict[str, Any]:
        """
        Create a new label in the repository.
        
        Args:
            name: Label name
            color: Hex color code (without #)
            description: Label description
            
        Returns:
            Created label data
        """
        endpoint = f"/repos/{self.repo_owner}/{self.repo_name}/labels"
        return self._make_request("POST", endpoint, {
            "name": name,
            "color": color,
            "description": description
        })


def ensure_labels_exist(client: GitHubAPIClient) -> None:
    """
    Ensure required labels exist in the repository.
    
    Args:
        client: GitHub API client
    """
    # Define required labels
    required_labels = {
        # Issue types
        "bug": ("d73a4a", "Something isn't working"),
        "enhancement": ("a2eeef", "New feature or request"),
        "question": ("d876e3", "Further information is requested"),
        "documentation": ("0075ca", "Improvements or additions to documentation"),
        "performance": ("fbca04", "Performance-related issue"),
        "security": ("ee0701", "Security vulnerability or concern"),
        
        # Priorities
        "P0": ("b60205", "Critical priority - crashes, data loss, security"),
        "P1": ("d93f0b", "High priority - major functionality broken"),
        "P2": ("fbca04", "Medium priority - moderate impact"),
        "P3": ("0e8a16", "Low priority - minor issues"),
        
        # Component labels
        "component:audio": ("1d76db", "Audio engine and playback"),
        "component:midi": ("5319e7", "MIDI functionality"),
        "component:ui": ("d4c5f9", "User interface"),
        "component:plugin": ("c5def5", "Plugin hosting"),
        "component:ai": ("f9d0c4", "AI features"),
        "component:collaboration": ("fef2c0", "Collaboration features"),
        "component:build": ("bfd4f2", "Build system"),
        
        # Platform labels
        "platform:linux": ("ededed", "Linux-specific issue"),
        "platform:macos": ("ededed", "macOS-specific issue"),
        "platform:windows": ("ededed", "Windows-specific issue"),
    }
    
    # Get existing labels
    try:
        existing_labels = client.list_repository_labels()
        existing_label_names = {label["name"] for label in existing_labels}
    except Exception as e:
        print(f"Warning: Could not list repository labels: {e}")
        print("Attempting to create labels anyway...")
        existing_label_names = set()
    
    # Create missing labels
    for name, (color, description) in required_labels.items():
        if name not in existing_label_names:
            try:
                client.create_label(name, color, description)
                print(f"✓ Created label: {name}")
            except HTTPError as e:
                if e.code == 422:  # Label already exists
                    print(f"✓ Label already exists: {name}")
                else:
                    print(f"✗ Failed to create label {name}: {e}")
            except Exception as e:
                print(f"✗ Failed to create label {name}: {e}")


def parse_event_data(event_path: str) -> Dict[str, Any]:
    """
    Parse GitHub event data from the event file.
    
    Args:
        event_path: Path to the GitHub event JSON file
        
    Returns:
        Event data
        
    Raises:
        ValueError: If the event file is malformed or cannot be read
    """
    try:
        with open(event_path, 'r', encoding='utf-8') as f:
            return json.load(f)
    except (IOError, OSError) as e:
        raise ValueError(f"Failed to read event file: {e}") from e
    except json.JSONDecodeError as e:
        raise ValueError(f"Failed to parse event JSON: {e}") from e


def extract_issue_from_event(event_data: Dict[str, Any]) -> Optional[Issue]:
    """
    Extract issue information from event data.
    
    Args:
        event_data: GitHub event data
        
    Returns:
        Issue object or None if not an issue/PR event
    """
    # Check if it's an issue or PR event
    issue_data = event_data.get("issue") or event_data.get("pull_request")
    
    if not issue_data:
        return None
    
    return Issue(
        number=issue_data["number"],
        title=issue_data["title"],
        body=issue_data.get("body") or "",
        author=issue_data["user"]["login"],
        labels={label["name"] for label in issue_data.get("labels", [])},
        assignees=[assignee["login"] for assignee in issue_data.get("assignees", [])]
    )


def format_triage_comment(result: TriageResult) -> str:
    """
    Format a comment explaining the triage result.
    
    Args:
        result: Triage result
        
    Returns:
        Formatted comment text
    """
    lines = [
        "## 🤖 Automated Triage",
        "",
        f"This issue has been automatically classified:",
        f"- **Type:** {result.issue_type.value}",
        f"- **Priority:** {result.priority.value}",
    ]
    
    if result.suggested_labels:
        # Only show non-type/priority labels
        other_labels = {
            label for label in result.suggested_labels 
            if label not in {result.issue_type.value, result.priority.value}
        }
        if other_labels:
            lines.append(f"- **Labels:** {', '.join(sorted(other_labels))}")
    
    if result.is_duplicate and result.duplicate_of:
        lines.extend([
            "",
            f"⚠️ This appears to be a duplicate of #{result.duplicate_of}.",
            "Please check that issue for existing discussion and updates."
        ])
    
    lines.extend([
        "",
        "_This triage was performed automatically. If you believe this classification is incorrect, please comment and a maintainer will review._"
    ])
    
    return "\n".join(lines)


def main() -> int:
    """
    Main entry point for the TriageBot runner.
    
    Returns:
        Exit code (0 for success, non-zero for failure)
    """
    # Get GitHub context from environment
    github_token = os.environ.get("GITHUB_TOKEN")
    event_path = os.environ.get("GITHUB_EVENT_PATH")
    repository = os.environ.get("GITHUB_REPOSITORY")
    
    if not github_token:
        print("Error: GITHUB_TOKEN environment variable not set")
        return 1
    
    if not event_path:
        print("Error: GITHUB_EVENT_PATH environment variable not set")
        return 1
    
    if not repository:
        print("Error: GITHUB_REPOSITORY environment variable not set")
        return 1
    
    # Parse repository owner and name
    try:
        parts = repository.split("/")
        if len(parts) != 2:
            raise ValueError(f"Expected format 'owner/repo', got: {repository}")
        repo_owner, repo_name = parts
    except (ValueError, AttributeError) as e:
        print(f"Error: Invalid GITHUB_REPOSITORY format: {repository}")
        return 1
    
    print(f"Running TriageBot for {repository}")
    
    # Parse event data
    try:
        event_data = parse_event_data(event_path)
    except Exception as e:
        print(f"Error parsing event data: {e}")
        return 1
    
    # Extract issue from event
    issue = extract_issue_from_event(event_data)
    
    if not issue:
        print("Event does not contain issue or PR data - nothing to triage")
        return 0
    
    print(f"Triaging issue #{issue.number}: {issue.title}")
    
    # Initialize GitHub API client
    client = GitHubAPIClient(github_token, repo_owner, repo_name)
    
    # Ensure required labels exist
    print("Ensuring required labels exist...")
    try:
        ensure_labels_exist(client)
    except Exception as e:
        print(f"Warning: Failed to ensure labels exist: {e}")
        print("Continuing with triage...")
    
    # Initialize TriageBot
    bot = TriageBot(repo_owner, repo_name)
    
    # Perform triage
    result = bot.triage_issue(issue)
    
    print(f"Triage result:")
    print(f"  Type: {result.issue_type.value}")
    print(f"  Priority: {result.priority.value}")
    print(f"  Labels: {result.suggested_labels}")
    print(f"  Duplicate: {result.is_duplicate}")
    
    # Apply labels
    try:
        labels_to_add = [label for label in result.suggested_labels if label not in issue.labels]
        if labels_to_add:
            print(f"Adding labels: {labels_to_add}")
            client.add_labels(issue.number, labels_to_add)
            print("✓ Labels added successfully")
        else:
            print("No new labels to add")
    except Exception as e:
        print(f"Error adding labels: {e}")
        # Continue even if label addition fails
    
    # Add triage comment
    try:
        comment_text = format_triage_comment(result)
        
        # Add automated response if available
        if result.automated_response:
            comment_text += f"\n\n---\n\n{result.automated_response}"
        
        print("Adding triage comment...")
        client.create_comment(issue.number, comment_text)
        print("✓ Comment added successfully")
    except Exception as e:
        print(f"Error adding comment: {e}")
        # Continue even if comment fails
    
    print("✓ Triage completed successfully")
    return 0


if __name__ == "__main__":
    sys.exit(main())
