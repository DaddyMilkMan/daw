#!/usr/bin/env python3
"""
Reviewer Assignment Script
Automatically assigns reviewers based on PR content
"""

import sys
import os
import json
import random


def get_reviewers(repo, pr_number, title, body):
    """Get appropriate reviewers for PR"""

    # This is a simplified version - in practice, you'd maintain a database
    # of reviewers and their areas of expertise
    reviewers_by_area = {
        "bug": ["developer1", "developer2"],
        "feature": ["developer1", "developer3", "developer4"],
        "documentation": ["developer5"],
        "performance": ["developer6", "developer7"],
        "security": ["developer8"],
        "testing": ["developer9"],
        "ui/ux": ["developer10", "developer11"],
        "audio": ["developer12", "developer13"],
        "engine": ["developer14", "developer15"],
        "platform-linux": ["developer16"],
        "platform-mac": ["developer17"],
        "platform-windows": ["developer18"],
    }

    # Convert to lowercase for case-insensitive matching
    title_lower = title.lower() if title else ""
    body_lower = body.lower() if body else ""

    # Determine areas based on title and body
    areas = []
    for area, keywords in {
        "bug": ["bug", "fix", "issue", "error", "problem"],
        "feature": ["feature", "add", "new", "implement", "create"],
        "documentation": ["doc", "documentation", "readme", "comment"],
        "performance": ["performance", "optimization", "speed", "memory", "cpu"],
        "security": ["security", "vulnerability", "safe", "auth"],
        "testing": ["test", "testing", "unit", "integration", "coverage"],
        "ui/ux": ["ui", "ux", "interface", "design", "layout", "theme"],
        "audio": ["audio", "sound", "mix", "track", "midi", "plugin"],
        "engine": ["engine", "core", "render", "playback", "record"],
        "platform-linux": ["linux"],
        "platform-mac": ["mac", "osx"],
        "platform-windows": ["windows", "win"],
    }.items():
        if any(keyword in title_lower for keyword in keywords) or any(
            keyword in body_lower for keyword in keywords
        ):
            areas.append(area)

    # Get reviewers for each area
    selected_reviewers = set()
    for area in areas:
        if area in reviewers_by_area:
            for reviewer in reviewers_by_area[area]:
                if len(selected_reviewers) < 3:  # Max 3 reviewers
                    selected_reviewers.add(reviewer)

    # If no specific areas found, assign random reviewers
    if not selected_reviewers:
        # Avoid KeyError if dictionary is empty, though hardcoded here
        if reviewers_by_area:
            first_group = list(reviewers_by_area.values())[0]
            selected_reviewers = set(
                random.sample(first_group, min(2, len(first_group)))
            )

    # Remove duplicate reviewers and limit to max 3
    final_reviewers = list(selected_reviewers)[:3]

    # Write reviewers to file
    with open("reviewers.txt", "w") as f:
        f.write(",".join(final_reviewers))

    return final_reviewers


if __name__ == "__main__":
    # Read from environment variables to avoid shell injection
    repo = os.environ.get("PR_REPO")
    pr_number = os.environ.get("PR_NUMBER")
    title = os.environ.get("PR_TITLE", "")
    body = os.environ.get("PR_BODY", "")

    if not repo or not pr_number:
        print("Error: PR_REPO and PR_NUMBER environment variables are required.")
        sys.exit(1)

    get_reviewers(repo, pr_number, title, body)
