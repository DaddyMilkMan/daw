#!/usr/bin/env python3
"""
Integration test for TriageBot workflow.

This test simulates the GitHub Actions workflow execution
to verify the bot works end-to-end.
"""

import json
import os
import sys
import tempfile
from pathlib import Path

# Add the agent directory to the path
sys.path.insert(0, str(Path(__file__).parent))

from run_triage import main, parse_event_data, extract_issue_from_event
from triage_bot import TriageBot


def create_test_event(issue_data: dict) -> str:
    """Create a test event file."""
    event = {
        "action": "opened",
        "issue": {
            "number": issue_data.get("number", 1),
            "title": issue_data.get("title", "Test Issue"),
            "body": issue_data.get("body", "Test body"),
            "user": {"login": "test-user"},
            "labels": [],
            "assignees": []
        },
        "repository": {
            "name": "daw",
            "owner": {"login": "micahcooley"}
        }
    }
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
        json.dump(event, f)
        return f.name


def test_classification():
    """Test issue classification."""
    print("Testing TriageBot classification...")
    
    bot = TriageBot("micahcooley", "daw")
    
    test_cases = [
        {
            "title": "Application crashes on startup",
            "body": "The app crashes when I start it on Linux.",
            "expected_type": "bug",
            "expected_priority": "P0",
            "expected_labels": {"bug", "P0", "platform:linux"}
        },
        {
            "title": "Add dark mode support",
            "body": "Would be nice to have a dark theme.",
            "expected_type": "feature",
            "expected_priority": "P3",
            "expected_labels": {"feature", "P3"}
        },
        {
            "title": "How do I export audio?",
            "body": "I can't find the export button.",
            "expected_type": "question",
            "expected_priority": "P3",
            "expected_labels": {"question", "P3"}
        },
    ]
    
    passed = 0
    failed = 0
    
    for i, test in enumerate(test_cases, 1):
        from triage_bot import Issue
        issue = Issue(
            number=i,
            title=test["title"],
            body=test["body"],
            author="test-user"
        )
        
        result = bot.triage_issue(issue)
        
        # Check type
        if result.issue_type.value != test["expected_type"]:
            print(f"✗ Test {i} FAILED: Expected type {test['expected_type']}, got {result.issue_type.value}")
            failed += 1
            continue
        
        # Check priority
        if result.priority.value != test["expected_priority"]:
            print(f"✗ Test {i} FAILED: Expected priority {test['expected_priority']}, got {result.priority.value}")
            failed += 1
            continue
        
        # Check labels (subset check)
        if not test["expected_labels"].issubset(result.suggested_labels):
            print(f"✗ Test {i} FAILED: Expected labels {test['expected_labels']}, got {result.suggested_labels}")
            failed += 1
            continue
        
        print(f"✓ Test {i} PASSED: {test['title'][:50]}")
        passed += 1
    
    print(f"\nResults: {passed} passed, {failed} failed")
    return failed == 0


def test_event_parsing():
    """Test event file parsing."""
    print("\nTesting event parsing...")
    
    event_file = create_test_event({
        "number": 42,
        "title": "Test parsing",
        "body": "Test body"
    })
    
    try:
        event_data = parse_event_data(event_file)
        issue = extract_issue_from_event(event_data)
        
        assert issue.number == 42
        assert issue.title == "Test parsing"
        assert issue.body == "Test body"
        
        print("✓ Event parsing test PASSED")
        return True
    except Exception as e:
        print(f"✗ Event parsing test FAILED: {e}")
        return False
    finally:
        if os.path.exists(event_file):
            os.unlink(event_file)


def main_test():
    """Run all tests."""
    print("=" * 60)
    print("TriageBot Integration Tests")
    print("=" * 60)
    
    results = []
    
    results.append(("Classification", test_classification()))
    results.append(("Event Parsing", test_event_parsing()))
    
    print("\n" + "=" * 60)
    print("Test Summary")
    print("=" * 60)
    
    for name, passed in results:
        status = "✓ PASS" if passed else "✗ FAIL"
        print(f"{status}: {name}")
    
    all_passed = all(passed for _, passed in results)
    
    if all_passed:
        print("\n✓ All tests passed!")
        return 0
    else:
        print("\n✗ Some tests failed")
        return 1


if __name__ == "__main__":
    sys.exit(main_test())
