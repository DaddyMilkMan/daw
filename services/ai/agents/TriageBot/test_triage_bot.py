import pytest
from datetime import datetime, timedelta

from triage_bot import TriageBot, Issue, IssueType, Priority


def make_issue(number=1, title="Test", body="Body", author="user"):
    return Issue(number=number, title=title, body=body, author=author)


def test_none_body_handling():
    bot = TriageBot("test", "repo")
    issue = make_issue(title="Test", body=None)
    result = bot.triage_issue(issue)
    assert result.issue_type in (IssueType.ENHANCEMENT, IssueType.DOCUMENTATION, IssueType.QUESTION, IssueType.BUG, IssueType.SECURITY, IssueType.FEATURE_REQUEST, IssueType.PERFORMANCE)


def test_security_priority_escalation():
    bot = TriageBot("test", "repo")

    doc_issue = make_issue(title="Update security documentation", body="Please update docs.")
    doc_result = bot.triage_issue(doc_issue)
    assert doc_result.priority == Priority.HIGH

    vuln_issue = make_issue(title="Found RCE vulnerability", body="Remote code execution exploit")
    vuln_result = bot.triage_issue(vuln_issue)
    assert vuln_result.priority == Priority.CRITICAL

    cve_issue = make_issue(title="CVE-2024-1234 exploit", body="Critical security issue")
    cve_result = bot.triage_issue(cve_issue)
    assert cve_result.priority == Priority.CRITICAL


def test_inflected_forms_match_bug():
    bot = TriageBot("test", "repo")
    texts = [
        "Application crashed on startup",
        "It keeps crashing when I load audio",
        "The crash happens after export",
    ]
    for i, text in enumerate(texts, 1):
        issue = make_issue(number=i, title=text, body="details")
        result = bot.triage_issue(issue)
        assert result.issue_type == IssueType.BUG


def test_similarity_audio_sound():
    bot = TriageBot("test", "repo")
    score = bot._calculate_similarity("audio crash", "sound crashes")
    assert score > 0.5


def test_duplicate_detection_with_similarity():
    bot = TriageBot("test", "repo")
    known = make_issue(number=10, title="Audio crash on startup", body="x")
    bot.add_known_issue(known)
    issue = make_issue(number=11, title="Sound crashes on startup", body="y")
    is_dup, dup_of = bot._detect_duplicate(issue)
    assert is_dup is True
    assert dup_of == 10


def test_false_positive_insecurity_not_security():
    bot = TriageBot("test", "repo")
    issue = make_issue(title="Insecurity in documentation wording", body="typo")
    assert bot._classify_issue_type(issue) != IssueType.SECURITY


def test_labels_and_routing_and_response():
    bot = TriageBot("test", "repo")
    issue = make_issue(
        title="Audio crash on Windows",
        body="The audio engine crashes and becomes unresponsive on Windows.",
    )
    result = bot.triage_issue(issue)
    assert "component:audio" in result.suggested_labels
    assert "platform:windows" in result.suggested_labels
    assert isinstance(result.suggested_assignees, list)

    question = make_issue(title="How do I export?", body="question about export")
    q_result = bot.triage_issue(question)
    assert q_result.automated_response is not None

    bot.add_known_issue(issue)
    dup = make_issue(title=issue.title, body=issue.body, number=99)
    dup_result = bot.triage_issue(dup)
    assert dup_result.is_duplicate is True
    assert dup_result.automated_response is not None


def test_check_stale_issues():
    bot = TriageBot("test", "repo")
    old_issue = make_issue(number=1, title="Old", body="x")
    old_issue.updated_at = datetime.now() - timedelta(days=60)
    new_issue = make_issue(number=2, title="New", body="y")
    new_issue.updated_at = datetime.now() - timedelta(days=1)
    bot.add_known_issue(old_issue)
    bot.add_known_issue(new_issue)
    stale = bot.check_stale_issues(days=30)
    assert old_issue in stale
    assert new_issue not in stale
