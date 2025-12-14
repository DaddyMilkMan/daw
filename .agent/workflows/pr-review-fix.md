---
description: Fix Gemini Code Assist PR review issues, resolve conflicts, commit, and sync with origin
---

# PR Review Fix Workflow

This workflow is triggered when the user pastes a Gemini Code Assist PR review or mentions fixing a PR.

## Prerequisites
- You have cloned the repository and have access to git.
- The user has provided the branch name or it can be inferred from context.
- You have the Gemini Code Assist feedback (pasted by the user).

## Steps

### 1. Checkout the Feature Branch
// turbo
```bash
git fetch origin
git checkout <branch-name>
git pull origin <branch-name>
```
Ensure you are on the correct feature branch. If there are local changes, stash them first.

### 2. Analyze Gemini Code Assist Feedback
Read the pasted review carefully. Categorize issues into:
- **Critical**: Build-breaking issues (e.g., misplaced CMakeLists.txt, deleted essential files).
- **Medium**: Code quality issues (e.g., magic numbers, missing constants).
- **Low**: Style suggestions.

**Prioritize Critical issues first.**

### 3. Fix Critical Issues
For each critical issue:
1. Understand the exact problem (e.g., "CMakeLists.txt was moved to logs/ directory").
2. Revert the problematic change using `git checkout origin/master -- <file>` or manual editing.
3. Verify the fix does not break other parts of the code.

Example for a misplaced CMakeLists.txt:
```bash
git checkout origin/master -- CMakeLists.txt
git rm logs/CMakeLists.txt  # If it was wrongly moved
```

### 4. Fix Medium Issues (Magic Numbers, etc.)
For code quality issues like magic numbers:
1. Identify the file and line numbers from the review.
2. Extract hardcoded values into named constants at the top of the function or in a shared header.
3. Example:
   ```cpp
   // Before
   gridPaint.setColor(SkColorSetARGB(100, 255, 255, 255));
   
   // After
   constexpr SkAlpha kBarLineAlpha = 100;
   gridPaint.setColor(SkColorSetARGB(kBarLineAlpha, 255, 255, 255));
   ```

### 5. Resolve Merge Conflicts
If there are merge conflicts (indicated by `CONFLICT` messages):
1. Open the conflicted file(s).
2. Look for `<<<<<<<`, `=======`, `>>>>>>>` markers.
3. Decide which version to keep (or merge both).
4. Remove the conflict markers.
5. Stage the resolved file: `git add <file>`.

For binary or log file conflicts (e.g., build logs):
- If the file should not be tracked, delete it and add to `.gitignore`.
- Use `git rm --cached <file>` to untrack.

### 6. Verify Build
// turbo
```bash
cmake --build build --config Debug 2>&1 | Select-Object -First 50
```
Ensure the project compiles successfully before committing.

### 7. Commit Fixes
// turbo
```bash
git add .
git commit -m "fix(review): address Gemini Code Assist feedback"
```
Use a conventional commit message format. Include a summary of critical and medium fixes in the commit body if needed.

### 8. Push to Origin
// turbo
```bash
git push origin <branch-name>
```
This syncs the fixes with the remote repository.

### 9. Notify User
Inform the user that the PR is ready for re-review and merging:
- "The PR branch `<branch-name>` has been updated with fixes for Gemini Code Assist feedback."
- "Merge conflicts have been resolved."
- "The branch is now synced with origin. You can merge it on GitHub."

## Notes
- If another agent is handling a specific file (as noted by user), skip that file.
- Always run the build after fixes to catch regressions.
- Never commit build artifacts or log files to git.
