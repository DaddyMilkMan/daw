import json
import time
import sys
import os
import random
import dataclasses
from enum import Enum
from typing import List, Dict

# Ensure we can import triage_bot
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from triage_bot import TriageBot, Issue, IssueType, Priority

def generate_test_issues(count: int = 100) -> List[Issue]:
    # Set seed for reproducibility
    random.seed(42)
    issues = []

    # Keyword pools based on triage_bot.py
    bug_keywords = ["crash", "segfault", "error", "broken", "fails"]
    security_keywords = ["security", "vulnerability", "cve"]
    perf_keywords = ["slow", "performance", "lag", "latency"]
    feature_keywords = ["feature request", "would be nice", "add support"]
    question_keywords = ["how to", "how do i", "question", "help"]
    docs_keywords = ["documentation", "docs", "readme"]

    components = ["audio", "sound", "midi", "ui", "interface", "plugin", "vst", "ai", "grok", "collaboration", "build", "cmake"]
    platforms = ["linux", "macos", "windows"]

    maintainer_areas = ["audio", "dsp", "ui", "rendering", "ai", "ml", "collaboration", "webrtc", "build", "cmake"]

    # Filler text
    lorem = "lorem ipsum dolor sit amet consectetur adipiscing elit sed do eiusmod tempor incididunt ut labore et dolore magna aliqua " * 10

    for i in range(count):
        # Determine likely type for this issue
        issue_type_idx = i % 7
        title = f"Issue {i}: "
        body = ""

        if issue_type_idx == 0: # Bug
            title += f"Application {random.choice(bug_keywords)}"
            body += f"It {random.choice(bug_keywords)} when I do X. "
        elif issue_type_idx == 1: # Security
            title += f"Found a {random.choice(security_keywords)}"
            body += f"There is a severe {random.choice(security_keywords)}. "
        elif issue_type_idx == 2: # Performance
            title += f"Very {random.choice(perf_keywords)}"
            body += f"The app is {random.choice(perf_keywords)}. "
        elif issue_type_idx == 3: # Feature
            title += f"{random.choice(feature_keywords)}"
            body += "I want this feature. "
        elif issue_type_idx == 4: # Question
            title += f"{random.choice(question_keywords)} use this?"
            body += "I need help. "
        elif issue_type_idx == 5: # Docs
            title += f"Missing {random.choice(docs_keywords)}"
            body += "Read the docs. "
        else: # Enhancement (default)
            title += "Make it better"
            body += "General improvement. "

        # Add random components and platforms
        if random.random() > 0.5:
            body += f" related to {random.choice(components)} "
        if random.random() > 0.5:
            body += f" on {random.choice(platforms)}. "

        # Add maintainer keywords
        if random.random() > 0.7:
            body += f" expertise in {random.choice(maintainer_areas)}. "

        # Add potential false positives (substrings)
        # "crash" -> "crashed", "crashing"
        # "security" -> "insecurity"
        # "slow" -> "slowly"
        body += " The system crashed and is behaving insecurely while slowly loading. "

        # Add bulk text
        body += lorem

        issues.append(Issue(
            number=i,
            title=title,
            body=body,
            author=f"user{i}"
        ))

    return issues

class EnhancedJSONEncoder(json.JSONEncoder):
    def default(self, o):
        if dataclasses.is_dataclass(o):
            return dataclasses.asdict(o)
        if isinstance(o, Enum):
            return o.value
        if isinstance(o, set):
            return list(sorted(o))
        if isinstance(o, (datetime.datetime, datetime.date)):
            return o.isoformat()
        return super().default(o)

import datetime

def run_benchmark():
    bot = TriageBot(repo_owner="test", repo_name="repo")
    issues = generate_test_issues(500) # 500 issues

    print(f"Generated {len(issues)} issues.")

    # Warmup
    for issue in issues[:10]:
        bot.triage_issue(issue)

    start_time = time.time()
    results = []

    for issue in issues:
        result = bot.triage_issue(issue)
        # Store simplified result for verification
        results.append({
            "issue_number": issue.number,
            "type": result.issue_type.value,
            "priority": result.priority.value,
            "labels": sorted(list(result.suggested_labels)),
            "assignees": sorted(result.suggested_assignees)
        })

    end_time = time.time()
    duration = end_time - start_time

    print(f"Time taken: {duration:.4f} seconds")
    print(f"Average time per issue: {duration/len(issues):.6f} seconds")

    with open("golden_master.json", "w") as f:
        json.dump(results, f, indent=2, cls=EnhancedJSONEncoder)

    # Also save the execution time to compare later
    with open("baseline_time.txt", "w") as f:
        f.write(str(duration))

if __name__ == "__main__":
    run_benchmark()
