#!/bin/bash

# Analyze each NOT_MERGED branch
echo "# Branch Analysis Report"
echo ""

for branch in $(git branch -r | grep -v HEAD | grep -v "zenith-pr-audit" | grep -v "daw-features-comparison" | grep -v "full-implementation-011CUwyfsaHote8BFZdzD92s" | grep -v "phase-13-track-automation-mvp" | sed 's|origin/||' | sort); do
    if ! git merge-base --is-ancestor origin/$branch HEAD 2>/dev/null; then
        echo "## Branch: $branch"

        # Get commit count
        commit_count=$(git log HEAD..origin/$branch --oneline 2>/dev/null | wc -l)
        echo "Commits ahead of HEAD: $commit_count"

        # Get latest commit message
        latest_commit=$(git log origin/$branch --oneline -1 2>/dev/null)
        echo "Latest commit: $latest_commit"

        # Get first commit message on this branch
        first_commit=$(git log origin/$branch --oneline --reverse | head -1 2>/dev/null)
        echo "First commit: $first_commit"

        # Get file changes summary
        files_changed=$(git diff HEAD...origin/$branch --stat 2>/dev/null | tail -1)
        echo "Changes: $files_changed"

        echo ""
    fi
done
