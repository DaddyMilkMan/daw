#!/usr/bin/env python3
"""
PR Classification Script
Automatically classifies PRs based on title and content
"""

import sys
import os
import re
import json
from pathlib import Path

def classify_pr(repo, pr_number, title, body):
    """Classify PR based on title and content"""

    classification = {
        'labels': [],
        'priority': 'medium',
        'area': 'general'
    }

    # Convert to lowercase for case-insensitive matching
    title_lower = title.lower()
    body_lower = body.lower()

    # Bug fixes
    if any(keyword in title_lower for keyword in ['bug', 'fix', 'issue', 'error', 'problem']):
        classification['labels'].append('bug')
        classification['priority'] = 'high'

    # Features
    if any(keyword in title_lower for keyword in ['feature', 'add', 'new', 'implement', 'create']):
        classification['labels'].append('feature')
        classification['priority'] = 'medium'

    # Documentation
    if any(keyword in title_lower for keyword in ['doc', 'documentation', 'readme', 'comment']):
        classification['labels'].append('documentation')
        classification['priority'] = 'low'

    # Performance
    if any(keyword in title_lower for keyword in ['performance', 'optimization', 'speed', 'memory', 'cpu']):
        classification['labels'].append('performance')
        classification['priority'] = 'medium'

    # Security
    if any(keyword in title_lower for keyword in ['security', 'vulnerability', 'safe', 'safe', 'auth']):
        classification['labels'].append('security')
        classification['priority'] = 'critical'

    # Testing
    if any(keyword in title_lower for keyword in ['test', 'testing', 'unit', 'integration', 'coverage']):
        classification['labels'].append('testing')
        classification['priority'] = 'medium'

    # Refactoring
    if any(keyword in title_lower for keyword in ['refactor', 'refactoring', 'clean', 'style', 'format']):
        classification['labels'].append('refactoring')
        classification['priority'] = 'low'

    # UI/UX
    if any(keyword in title_lower for keyword in ['ui', 'ux', 'interface', 'design', 'layout', 'theme']):
        classification['labels'].append('ui/ux')
        classification['priority'] = 'medium'

    # Audio specific
    if any(keyword in title_lower for keyword in ['audio', 'sound', 'mix', 'track', 'midi', 'plugin']):
        classification['labels'].append('audio')
        classification['priority'] = 'medium')

    # Engine specific
    if any(keyword in title_lower for keyword in ['engine', 'core', 'render', 'playback', 'record']):
        classification['labels'].append('engine')
        classification['priority'] = 'high')

    # Platform specific
    if 'linux' in title_lower:
        classification['labels'].append('platform-linux')
    if 'mac' in title_lower or 'osx' in title_lower:
        classification['labels'].append('platform-mac')
    if 'windows' in title_lower or 'win' in title_lower:
        classification['labels'].append('platform-windows')

    # Size classification
    # Count lines of changes in the PR (would need API call)
    # For now, based on title length
    if len(title.split()) < 10:
        classification['labels'].append('size-xs')
    elif len(title.split()) < 30:
        classification['labels'].append('size-s')
    elif len(title.split()) < 100:
        classification['labels'].append('size-m')
    else:
        classification['labels'].append('size-l')

    # Always add needs-review label
    classification['labels'].append('needs-review')

    # Write classification to file
    with open('classification.txt', 'w') as f:
        f.write('\n'.join(classification['labels']))

    return classification

if __name__ == '__main__':
    if len(sys.argv) != 5:
        print("Usage: python classify_pr.py --repo <repo> --pr <number> --title <title> --body <body>")
        sys.exit(1)

    repo = sys.argv[2]
    pr_number = sys.argv[4]
    title = sys.argv[6]
    body = sys.argv[8]

    classify_pr(repo, pr_number, title, body)