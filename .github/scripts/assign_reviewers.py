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
        'bug': ['developer1', 'developer2'],
        'feature': ['developer1', 'developer3', 'developer4'],
        'documentation': ['developer5'],
        'performance': ['developer6', 'developer7'],
        'security': ['developer8'],
        'testing': ['developer9'],
        'ui/ux': ['developer10', 'developer11'],
        'audio': ['developer12', 'developer13'],
        'engine': ['developer14', 'developer15'],
        'platform-linux': ['developer16'],
        'platform-mac': ['developer17'],
        'platform-windows': ['developer18']
    }

    # Convert to lowercase for case-insensitive matching
    title_lower = title.lower()
    body_lower = body.lower()

    # Determine areas based on title and body
    areas = []
    for area, keywords in {
        'bug': ['bug', 'fix', 'issue', 'error', 'problem'],
        'feature': ['feature', 'add', 'new', 'implement', 'create'],
        'documentation': ['doc', 'documentation', 'readme', 'comment'],
        'performance': ['performance', 'optimization', 'speed', 'memory', 'cpu'],
        'security': ['security', 'vulnerability', 'safe', 'auth'],
        'testing': ['test', 'testing', 'unit', 'integration', 'coverage'],
        'ui/ux': ['ui', 'ux', 'interface', 'design', 'layout', 'theme'],
        'audio': ['audio', 'sound', 'mix', 'track', 'midi', 'plugin'],
        'engine': ['engine', 'core', 'render', 'playback', 'record'],
        'platform-linux': ['linux'],
        'platform-mac': ['mac', 'osx'],
        'platform-windows': ['windows', 'win']
    }.items():
        if any(keyword in title_lower for keyword in keywords) or any(keyword in body_lower for keyword in keywords):
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
        selected_reviewers = random.sample(
            list(reviewers_by_area.values())[0],
            min(2, len(reviewers_by_area.values()[0]))
        )

    # Remove duplicate reviewers and limit to max 3
    final_reviewers = list(selected_reviewers)[:3]

    # Write reviewers to file
    with open('reviewers.txt', 'w') as f:
        f.write(','.join(final_reviewers))

    return final_reviewers

if __name__ == '__main__':
    if len(sys.argv) != 5:
        print("Usage: python assign_reviewers.py --repo <repo> --pr <number> --title <title> --body <body>")
        sys.exit(1)

    repo = sys.argv[2]
    pr_number = sys.argv[4]
    title = sys.argv[6]
    body = sys.argv[8]

    get_reviewers(repo, pr_number, title, body)