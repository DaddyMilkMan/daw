# Branches to Delete

## Successfully Merged PR Branches (Safe to Delete)
These have been merged into master and can be safely deleted:

### Local Branches:
```bash
git branch -d pr-42 pr-43 pr-45 pr-48 pr-51 pr-52 pr-53 pr-54 pr-55 pr-56 pr-65 pr-69 pr-70 pr-74 pr-76 pr-77 pr-79 pr-80 pr-82 pr-84 pr-85 pr-88
```

### Remote Branches (on GitHub):
Close these PRs on GitHub as merged, which will delete the remote branches:
- PR #42, #43, #45, #48, #51, #52, #53, #54, #55, #56, #65, #69, #70, #74, #76, #77, #79, #80, #82, #84, #85, #88

## Legacy/Web App Branches to Delete (Skipped - Not Merged)
These contain legacy TypeScript/Electron code and should be deleted:

### Local Branches:
```bash
git branch -D pr-47 pr-50 pr-63 pr-75 pr-78 pr-81
```

### Remote Branches:
Close these PRs on GitHub as "won't merge" or "obsolete":
- PR #47 (Build artifacts cleanup - contains web files)
- PR #50 (Legacy web app code)
- PR #63 (Legacy web app code)
- PR #75 (Legacy web app code)
- PR #78 (Legacy web app code)
- PR #81 (Legacy web app code)

## Documentation-Only Branch to Delete

### Local Branch:
```bash
git branch -D pr-73
```

### Remote Branch:
Close PR #73 on GitHub as documentation is already in master

## All Claude Feature Branches
You may also want to delete old claude/* branches that have been merged. List them with:
```bash
git branch -a | grep claude/
```

Then delete locally with:
```bash
git branch -D claude/branch-name
```

And remotely with:
```bash
git push origin --delete claude/branch-name
```

## Quick Delete All Commands

### Delete all merged PR branches locally:
```bash
git branch -d pr-42 pr-43 pr-45 pr-48 pr-51 pr-52 pr-53 pr-54 pr-55 pr-56 pr-65 pr-69 pr-70 pr-74 pr-76 pr-77 pr-79 pr-80 pr-82 pr-84 pr-85 pr-88
```

### Force delete all skipped PR branches locally:
```bash
git branch -D pr-47 pr-50 pr-63 pr-73 pr-75 pr-78 pr-81
```

### Delete all PR branches remotely (after closing PRs on GitHub):
```bash
git push origin --delete pr-42 pr-43 pr-45 pr-47 pr-48 pr-50 pr-51 pr-52 pr-53 pr-54 pr-55 pr-56 pr-63 pr-65 pr-69 pr-70 pr-73 pr-74 pr-75 pr-76 pr-77 pr-78 pr-79 pr-80 pr-81 pr-82 pr-84 pr-85 pr-88
```
