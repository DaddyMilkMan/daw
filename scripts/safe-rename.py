#!/usr/bin/env python3
"""
SAFE FILE RENAMER FOR ZENITH DAW
================================

This script safely renames files while:
- Using git mv to preserve history
- Updating CMakeLists.txt automatically
- Providing dry-run mode
- Creating rollback capability
- Detecting and skipping active refactoring targets

Usage:
    python3 scripts/safe-rename.py --dry-run              # Preview changes
    python3 scripts/safe-rename.py --execute              # Execute changes
    python3 scripts/safe-rename.py --rollback <timestamp> # Rollback
"""

import argparse
import json
import os
import re
import subprocess
import sys
from dataclasses import asdict, dataclass
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple


@dataclass
class RenameOperation:
    """Represents a single rename operation."""
    old_path: str
    new_path: str
    reason: str
    cmake_updates: List[str]  # CMake files that need updating


@dataclass
class RollbackState:
    """State for potential rollback."""
    timestamp: str
    operations: List[Dict]
    git_commit: Optional[str]


class SafeRenamer:
    """Handles safe file renaming with git integration."""
    
    # Renames that are ACTUALLY duplicates (not refactoring in progress)
    # Format: (old_path, new_path, reason)
    APPROVED_RENAMES: List[Tuple[str, str, str]] = [
        # SkiaRenderer - genuinely different implementations
        (
            "modules/zenith_core/engine/rendering/SkiaRenderer.h",
            "modules/zenith_core/engine/rendering/EngineSkiaRenderer.h",
            "Engine-specific Skia renderer implementation"
        ),
        (
            "modules/zenith_core/engine/rendering/SkiaRenderer.cpp",
            "modules/zenith_core/engine/rendering/EngineSkiaRenderer.cpp",
            "Engine-specific Skia renderer implementation"
        ),
        (
            "modules/zenith_ui/rendering/SkiaRenderer.h",
            "modules/zenith_ui/rendering/UISkiaRenderer.h",
            "UI-specific Skia renderer implementation"
        ),
        (
            "modules/zenith_ui/rendering/SkiaRenderer.cpp",
            "modules/zenith_ui/rendering/UISkiaRenderer.cpp",
            "UI-specific Skia renderer implementation"
        ),
        
        # Note: TransportController is intentionally NOT here
        # It's an active refactoring - engine/ is legacy, engine/core/ is new
    ]
    
    def __init__(self, dry_run: bool = True):
        self.dry_run = dry_run
        self.operations: List[RenameOperation] = []
        self.rollback_dir = Path(".rename-rollback")
        self.rollback_file: Optional[Path] = None
        
    def check_git_state(self) -> bool:
        """Verify git is clean and we're on a branch."""
        try:
            # Check for uncommitted changes
            result = subprocess.run(
                ["git", "status", "--porcelain"],
                capture_output=True,
                text=True
            )
            if result.stdout.strip():
                print("❌ ERROR: You have uncommitted changes.")
                print("   Commit or stash them before running this script.")
                print("\n   Current status:")
                print(result.stdout[:500])
                return False
            
            # Check we're on a branch
            result = subprocess.run(
                ["git", "branch", "--show-current"],
                capture_output=True,
                text=True
            )
            branch = result.stdout.strip()
            if not branch:
                print("❌ ERROR: Not on a git branch.")
                return False
            
            print(f"✅ Git state clean on branch: {branch}")
            return True
            
        except Exception as e:
            print(f"❌ ERROR checking git state: {e}")
            return False
    
    def detect_active_refactoring(self, filepath: str) -> Optional[str]:
        """
        Detect if a file is part of an active refactoring.
        Returns reason if it should be skipped, None if safe to rename.
        """
        path = Path(filepath)
        basename = path.name
        
        # Check for TransportController pattern (legacy vs new)
        if "TransportController" in basename:
            # Check if both legacy and new exist
            legacy = Path("modules/zenith_core/engine/TransportController.h")
            new_core = Path("modules/zenith_core/engine/core/TransportController.h")
            
            if legacy.exists() and new_core.exists():
                return ("Active refactoring detected: legacy file exists alongside "
                        "new implementation in engine/core/. Manual review required.")
        
        # Check for files with "2" suffix (views2, etc.)
        if "2" in path.parent.name:
            return f"Directory '{path.parent.name}' suggests migration in progress"
        
        return None
    
    def find_cmake_references(self, old_path: str) -> List[str]:
        """Find CMake files that reference the old path."""
        cmake_files = []
        old_basename = Path(old_path).name
        old_name_no_ext = Path(old_path).stem
        
        # Search in cmake/ and module CMakeLists.txt
        for cmake_path in list(Path("cmake").glob("*.cmake")) + \
                          list(Path("modules").rglob("CMakeLists.txt")):
            try:
                content = cmake_path.read_text()
                # Check for various ways the file might be referenced
                if (old_basename in content or 
                    old_name_no_ext in content or
                    old_path.replace("modules/", "") in content):
                    cmake_files.append(str(cmake_path))
            except Exception:
                continue
        
        return cmake_files
    
    def validate_rename(self, old_path: str, new_path: str) -> Tuple[bool, str]:
        """Validate a rename operation."""
        old = Path(old_path)
        new = Path(new_path)
        
        # Check old file exists
        if not old.exists():
            return False, f"Source file does not exist: {old_path}"
        
        # Check new file doesn't exist
        if new.exists():
            return False, f"Destination already exists: {new_path}"
        
        # Check for active refactoring
        refactoring = self.detect_active_refactoring(old_path)
        if refactoring:
            return False, f"Active refactoring detected: {refactoring}"
        
        # Validate new path directory exists or can be created
        if not new.parent.exists():
            return False, f"Parent directory does not exist: {new.parent}"
        
        return True, "OK"
    
    def stage_operation(self, old_path: str, new_path: str, reason: str) -> bool:
        """Stage a rename operation after validation."""
        valid, msg = self.validate_rename(old_path, new_path)
        
        if not valid:
            print(f"⚠️  SKIPPING: {old_path}")
            print(f"   Reason: {msg}")
            return False
        
        cmake_updates = self.find_cmake_references(old_path)
        
        op = RenameOperation(
            old_path=old_path,
            new_path=new_path,
            reason=reason,
            cmake_updates=cmake_updates
        )
        self.operations.append(op)
        return True
    
    def prepare_all_operations(self):
        """Prepare all rename operations from APPROVED_RENAMES."""
        print("\n🔍 Analyzing rename operations...")
        print("=" * 60)
        
        for old_path, new_path, reason in self.APPROVED_RENAMES:
            if self.stage_operation(old_path, new_path, reason):
                print(f"✅ Staged: {old_path}")
                print(f"   → {new_path}")
                if op := [o for o in self.operations if o.old_path == old_path]:
                    if op[0].cmake_updates:
                        print(f"   CMake updates needed: {len(op[0].cmake_updates)} files")
        
        print(f"\n📊 Total operations staged: {len(self.operations)}")
        return len(self.operations) > 0
    
    def save_rollback_state(self):
        """Save current state for potential rollback."""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.rollback_dir.mkdir(exist_ok=True)
        self.rollback_file = self.rollback_dir / f"rollback_{timestamp}.json"
        
        # Get current git commit
        result = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            capture_output=True,
            text=True
        )
        current_commit = result.stdout.strip()
        
        state = RollbackState(
            timestamp=timestamp,
            operations=[asdict(op) for op in self.operations],
            git_commit=current_commit
        )
        
        self.rollback_file.write_text(json.dumps(asdict(state), indent=2))
        print(f"📄 Rollback state saved: {self.rollback_file}")
        return timestamp
    
    def update_cmake_file(self, cmake_path: str, old_path: str, new_path: str):
        """Update a single CMake file."""
        old_basename = Path(old_path).name
        new_basename = Path(new_path).name
        old_rel = old_path.replace("modules/", "")
        new_rel = new_path.replace("modules/", "")
        
        try:
            content = Path(cmake_path).read_text()
            original = content
            
            # Replace various forms of the path
            replacements = [
                (old_path, new_path),
                (old_rel, new_rel),
                (old_basename, new_basename),
            ]
            
            for old, new in replacements:
                content = content.replace(old, new)
            
            if content != original:
                if not self.dry_run:
                    Path(cmake_path).write_text(content)
                    subprocess.run(["git", "add", cmake_path], check=True)
                print(f"   📝 Updated: {cmake_path}")
                return True
            
        except Exception as e:
            print(f"   ⚠️  Error updating {cmake_path}: {e}")
        
        return False
    
    def execute_rename(self, op: RenameOperation) -> bool:
        """Execute a single rename operation."""
        try:
            if not self.dry_run:
                # Use git mv
                subprocess.run(
                    ["git", "mv", op.old_path, op.new_path],
                    check=True,
                    capture_output=True
                )
            
            print(f"✅ Renamed: {op.old_path}")
            print(f"   → {op.new_path}")
            
            # Update CMake files
            for cmake_file in op.cmake_updates:
                self.update_cmake_file(cmake_file, op.old_path, op.new_path)
            
            return True
            
        except subprocess.CalledProcessError as e:
            print(f"❌ FAILED: {op.old_path}")
            print(f"   Error: {e.stderr.decode() if e.stderr else 'Unknown'}")
            return False
    
    def execute_all(self) -> bool:
        """Execute all staged operations."""
        if not self.operations:
            print("No operations to execute.")
            return True
        
        print("\n" + "=" * 60)
        if self.dry_run:
            print("🔍 DRY RUN - No changes will be made")
            print("=" * 60)
            for op in self.operations:
                print(f"\nWould rename:")
                print(f"  From: {op.old_path}")
                print(f"  To:   {op.new_path}")
                print(f"  Reason: {op.reason}")
                if op.cmake_updates:
                    print(f"  CMake files to update: {', '.join(op.cmake_updates)}")
            print("\n✅ Dry run complete. Use --execute to apply changes.")
            return True
        
        print("🚀 EXECUTING RENAMES")
        print("=" * 60)
        
        # Save rollback state
        timestamp = self.save_rollback_state()
        
        # Execute renames
        success_count = 0
        for op in self.operations:
            if self.execute_rename(op):
                success_count += 1
        
        print(f"\n📊 Results: {success_count}/{len(self.operations)} operations successful")
        
        if success_count == len(self.operations):
            print(f"\n✅ All operations successful!")
            print(f"📄 Rollback info saved to: {self.rollback_file}")
            print(f"\nNext steps:")
            print(f"  1. Review changes: git status")
            print(f"  2. Build to verify: cmake --build build")
            print(f"  3. Commit: git commit -m 'refactor: standardize filenames'")
            print(f"\nTo rollback if needed:")
            print(f"  python3 scripts/safe-rename.py --rollback {timestamp}")
            return True
        else:
            print(f"\n⚠️  Some operations failed!")
            print(f"   Check git status and consider rollback")
            return False
    
    def rollback(self, timestamp: str):
        """Rollback to previous state."""
        rollback_file = self.rollback_dir / f"rollback_{timestamp}.json"
        
        if not rollback_file.exists():
            print(f"❌ Rollback file not found: {rollback_file}")
            print(f"   Available rollbacks:")
            for f in sorted(self.rollback_dir.glob("rollback_*.json")):
                print(f"     - {f.stem.replace('rollback_', '')}")
            return False
        
        state = json.loads(rollback_file.read_text())
        
        print(f"\n🔄 ROLLBACK to {timestamp}")
        print("=" * 60)
        print(f"This will restore git to commit: {state['git_commit'][:8]}")
        
        # Check current state
        result = subprocess.run(
            ["git", "status", "--porcelain"],
            capture_output=True,
            text=True
        )
        
        if result.stdout.strip():
            print("\n⚠️  WARNING: You have uncommitted changes!")
            print("   Rollback may fail or lose work.")
            response = input("   Continue anyway? (yes/no): ")
            if response.lower() != "yes":
                print("Rollback cancelled.")
                return False
        
        # Perform rollback
        try:
            # Reset to the original commit
            subprocess.run(
                ["git", "reset", "--hard", state['git_commit']],
                check=True
            )
            print(f"✅ Rolled back to commit {state['git_commit'][:8]}")
            return True
            
        except subprocess.CalledProcessError as e:
            print(f"❌ Rollback failed: {e}")
            return False


def main():
    parser = argparse.ArgumentParser(
        description="Safely rename files in Zenith DAW"
    )
    parser.add_argument(
        "--execute",
        action="store_true",
        help="Actually execute the renames (default is dry-run)"
    )
    parser.add_argument(
        "--rollback",
        metavar="TIMESTAMP",
        help="Rollback to previous state (use timestamp from rollback file)"
    )
    parser.add_argument(
        "--list-approved",
        action="store_true",
        help="List all approved renames without executing"
    )
    
    args = parser.parse_args()
    
    renamer = SafeRenamer(dry_run=not args.execute)
    
    if args.list_approved:
        print("\n📋 APPROVED RENAMES:")
        print("=" * 60)
        for old, new, reason in renamer.APPROVED_RENAMES:
            print(f"\n{old}")
            print(f"  → {new}")
            print(f"  Reason: {reason}")
        return 0
    
    if args.rollback:
        return 0 if renamer.rollback(args.rollback) else 1
    
    # Check git state
    if not renamer.check_git_state():
        return 1
    
    # Prepare operations
    if not renamer.prepare_all_operations():
        print("\n⚠️  No operations to perform.")
        print("   This could mean:")
        print("   - All files already renamed")
        print("   - Files are part of active refactoring (skipped)")
        print("   - Files don't exist")
        return 0
    
    # Execute
    return 0 if renamer.execute_all() else 1


if __name__ == "__main__":
    sys.exit(main())
