#!/usr/bin/env python3
"""
Scan the repository for JUCE rendering APIs and other risky rendering/back-end patterns.
Exits with non-zero code if any occurrences are found. Designed to be run in CI.
"""
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
PATTERNS = [
    r"\bjuce::Graphics\b",
    r"\bjuce::Component\b",
    r"\bComponent::paint\b",
    r"\bvoid\s+paint\s*\(",
    r"\brepaint\s*\(",
    r"\bJUCE_DECLARE",
    r"\bjuce::Image\b",
    r"\bjuce::TextEditor\b",
]

exclude_dirs = {'.git', 'build', 'out', '.vs', '.idea', '__pycache__', 'external', 'third_party'}

matches = []

for dirpath, dirnames, filenames in os.walk(ROOT):
    # prune excludes
    dirnames[:] = [d for d in dirnames if d not in exclude_dirs]
    for fname in filenames:
        if not fname.endswith(('.cpp', '.cc', '.c', '.h', '.hpp', '.ipp', '.inl')):
            continue
        path = os.path.join(dirpath, fname)
        try:
            with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                text = f.read()
        except Exception:
            continue
        for pat in PATTERNS:
            if re.search(pat, text):
                # record first occurrence line number
                for i, line in enumerate(text.splitlines(), 1):
                    if re.search(pat, line):
                        matches.append((path, i, pat, line.strip()))
                        break

if matches:
    print('Found JUCE rendering-related symbols (this CI job enforces Skia-only rendering):')
    for path, lineno, pat, snippet in matches:
        print(f" - {path}:{lineno}: {pat} -> {snippet}")
    sys.exit(2)
else:
    print('No JUCE rendering APIs found.')
    sys.exit(0)
