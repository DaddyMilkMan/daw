#!/bin/bash

# Check each remote branch to see if it's been merged into HEAD
git branch -r | grep -v HEAD | sed 's|origin/||' | while read branch; do
    if git merge-base --is-ancestor origin/$branch HEAD 2>/dev/null; then
        echo "$branch: MERGED"
    else
        echo "$branch: NOT_MERGED"
    fi
done
