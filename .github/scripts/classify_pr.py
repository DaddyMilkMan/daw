#!/usr/bin/env python3
"""
PR Classification Script
Automatically classifies PRs based on title and content
"""

import sys
import os
import json


def classify_pr(repo, pr_number, title, body):
    """Classify PR based on title and content"""

    classification = {"labels": [], "priority": "medium", "area": "general"}

    # Convert to lowercase for case-insensitive matching
    title_lower = title.lower() if title else ""
    body_lower = body.lower() if body else ""

    # Bug fixes
    if any(
        keyword in title_lower
        for keyword in ["bug", "fix", "issue", "error", "problem"]
    ):
        classification["labels"].append("bug")
        classification["priority"] = "high"

    # Features
    if any(
        keyword in title_lower
        for keyword in ["feature", "add", "new", "implement", "create"]
    ):
        classification["labels"].append("feature")
        classification["priority"] = "medium"

    # Documentation
    if any(
        keyword in title_lower
        for keyword in ["doc", "documentation", "readme", "comment"]
    ):
        classification["labels"].append("documentation")
        classification["priority"] = "low"

    # Performance
    if any(
        keyword in title_lower
        for keyword in ["performance", "optimization", "speed", "memory", "cpu"]
    ):
        classification["labels"].append("performance")
        classification["priority"] = "medium"

    # Security
    if any(
        keyword in title_lower
        for keyword in ["security", "vulnerability", "safe", "safe", "auth"]
    ):
        classification["labels"].append("security")
        classification["priority"] = "critical"

    # Testing
    if any(
        keyword in title_lower
        for keyword in ["test", "testing", "unit", "integration", "coverage"]
    ):
        classification["labels"].append("testing")
        classification["priority"] = "medium"

    # Refactoring
    if any(
        keyword in title_lower
        for keyword in ["refactor", "refactoring", "clean", "style", "format"]
    ):
        classification["labels"].append("refactoring")
        classification["priority"] = "low"

    # UI/UX
    if any(
        keyword in title_lower
        for keyword in ["ui", "ux", "interface", "design", "layout", "theme"]
    ):
        classification["labels"].append("ui/ux")
        classification["priority"] = "medium"

    # Audio specific
    if any(
        keyword in title_lower
        for keyword in ["audio", "sound", "mix", "track", "midi", "plugin"]
    ):
        classification["labels"].append("audio")
        classification["priority"] = "medium"

    # Engine specific
    if any(
        keyword in title_lower
        for keyword in ["engine", "core", "render", "playback", "record"]
    ):
        classification["labels"].append("engine")
        classification["priority"] = "high"

    # Platform specific
    if "linux" in title_lower:
        classification["labels"].append("platform-linux")
    if "mac" in title_lower or "osx" in title_lower:
        classification["labels"].append("platform-mac")
    if "windows" in title_lower or "win" in title_lower:
        classification["labels"].append("platform-windows")

    # Size classification
    # Count lines of changes in the PR (would need API call)
    # For now, based on title length
    if title:
        if len(title.split()) < 10:
            classification["labels"].append("size-xs")
        elif len(title.split()) < 30:
            classification["labels"].append("size-s")
        elif len(title.split()) < 100:
            classification["labels"].append("size-m")
        else:
            classification["labels"].append("size-l")

    # Always add needs-review label
    classification["labels"].append("needs-review")

    # Write classification to file
    with open("classification.txt", "w") as f:
        f.write("\n".join(classification["labels"]))

    return classification


if __name__ == "__main__":
    # Read from environment variables to avoid shell injection
    repo = os.environ.get("PR_REPO")
    pr_number = os.environ.get("PR_NUMBER")
    title = os.environ.get("PR_TITLE", "")
    body = os.environ.get("PR_BODY", "")

    if not repo or not pr_number:
        print("Error: PR_REPO and PR_NUMBER environment variables are required.")
        sys.exit(1)

    classify_pr(repo, pr_number, title, body)
