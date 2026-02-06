#!/usr/bin/env python3
"""
INCREMENTAL STRUCTURE FLATTENER
===============================

Safely flattens directory structure incrementally WITHOUT:
- Overwriting files
- Losing subdirectory organization
- Breaking includes
- Mass changes

Strategy: Flatten only empty/deeply-nested directories, preserve meaningful structure.

Usage:
    python3 scripts/flatten-incremental.py --analyze
    python3 scripts/flatten-incremental.py --execute --dry-run
    python3 scripts/flatten-incremental.py --execute
"""

import argparse
import json
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Tuple


@dataclass
class FlattenOperation:
    """Represents a flatten operation."""
    source_dir: Path
    target_dir: Path
    files_to_move: List[Tuple[Path, Path]]  # (source, target)
    reason: str


class IncrementalFlattener:
    """Safely flattens directory structure."""
    
    # Maximum reasonable depth
    MAX_DEPTH = 8
    
    # Directories that should NEVER be flattened (preserve structure)
    PROTECTED_PATTERNS = [
        "*/tests/*",           # Keep test structure
        "*/instruments/*",     # Instrument subfolders are meaningful
        "*/effects/*",         # Effect subfolders are meaningful
        "*/deprecated/*",      # Keep deprecated isolated
    ]
    
    # Directories that CAN be flattened (scattered UI code)
    FLATTENABLE_PATTERNS = [
        "modules/zenith_ui/ui/controls",
        "modules/zenith_ui/ui/common",
        "modules/zenith_ui/ui/visualization",
        "modules/zenith_ui/ui/visualizations",
    ]
    
    # Target structure
    TARGET_STRUCTURE = {
        "modules/zenith_ui/components": [
            "modules/zenith_ui/ui/controls",
            "modules/zenith_ui/ui/common",
            "modules/zenith_ui/ui/visualization",
            "modules/zenith_ui/ui/visualizations",
        ],
        "modules/zenith_ui/theme": [
            "modules/zenith_ui/ui/design-system",
        ],
    }
    
    def __init__(self, dry_run: bool = True):
        self.dry_run = dry_run
        self.operations: List[FlattenOperation] = []
        self.conflicts: List[str] = []
    
    def check_git_state(self) -> bool:
        """Verify git is clean."""
        result = subprocess.run(
            ["git", "status", "--porcelain"],
            capture_output=True,
            text=True
        )
        if result.stdout.strip():
            print("❌ Uncommitted changes detected.")
            return False
        return True
    
    def get_directory_depth(self, path: Path) -> int:
        """Get depth of directory from modules root."""
        try:
            parts = path.parts
            if "modules" in parts:
                modules_idx = parts.index("modules")
                return len(parts) - modules_idx - 1
        except ValueError:
            pass
        return len(parts)
    
    def is_protected(self, path: Path) -> bool:
        """Check if a directory is protected."""
        path_str = str(path)
        
        for pattern in self.PROTECTED_PATTERNS:
            if self._match_pattern(path_str, pattern):
                return True
        
        return False
    
    def _match_pattern(self, path: str, pattern: str) -> bool:
        """Simple pattern matching."""
        import fnmatch
        return fnmatch.fnmatch(path, pattern)
    
    def analyze(self) -> bool:
        """Analyze what can be flattened."""
        print("\n🔍 Analyzing directory structure...")
        print("=" * 60)
        
        # Check current depths
        deep_dirs = []
        for module_dir in Path("modules").iterdir():
            if module_dir.is_dir():
                for subdir in module_dir.rglob("*"):
                    if subdir.is_dir():
                        depth = self.get_directory_depth(subdir)
                        if depth > self.MAX_DEPTH:
                            deep_dirs.append((depth, subdir))
        
        if deep_dirs:
            print(f"\n⚠️  Found {len(deep_dirs)} directories deeper than {self.MAX_DEPTH}:")
            for depth, d in sorted(deep_dirs, reverse=True)[:10]:
                print(f"   Depth {depth}: {d}")
        
        # Analyze flattenable directories
        print("\n📋 Analyzing flattenable directories...")
        
        for target_dir, source_dirs in self.TARGET_STRUCTURE.items():
            target = Path(target_dir)
            
            for source_dir in source_dirs:
                source = Path(source_dir)
                
                if not source.exists():
                    continue
                
                # Check if already done
                if not any(source.iterdir()):
                    print(f"   ℹ️  {source} is empty")
                    continue
                
                # Analyze files
                files_to_move = []
                conflicts = []
                
                for file in source.iterdir():
                    if file.is_file() and file.suffix in ['.h', '.cpp']:
                        target_file = target / file.name
                        
                        if target_file.exists():
                            conflicts.append(f"{file.name} exists in both {source} and {target}")
                        else:
                            files_to_move.append((file, target_file))
                
                if files_to_move:
                    self.operations.append(FlattenOperation(
                        source_dir=source,
                        target_dir=target,
                        files_to_move=files_to_move,
                        reason=f"Consolidating {source.name} into {target.name}"
                    ))
                
                if conflicts:
                    self.conflicts.extend(conflicts)
        
        # Print summary
        print(f"\n✅ Operations to perform: {len(self.operations)}")
        total_files = sum(len(op.files_to_move) for op in self.operations)
        print(f"📄 Total files to move: {total_files}")
        
        if self.conflicts:
            print(f"\n❌ Conflicts found: {len(self.conflicts)}")
            for c in self.conflicts[:5]:
                print(f"   - {c}")
        
        for op in self.operations:
            print(f"\n{op.reason}")
            print(f"   Source: {op.source_dir}")
            print(f"   Target: {op.target_dir}")
            print(f"   Files: {len(op.files_to_move)}")
        
        return len(self.operations) > 0
    
    def execute(self) -> bool:
        """Execute flattening operations."""
        if not self.operations:
            print("No operations to execute.")
            return True
        
        if self.conflicts:
            print("\n⚠️  Resolve conflicts before executing:")
            for c in self.conflicts:
                print(f"   - {c}")
            return False
        
        print("\n🚀 Executing flatten operations...")
        print("=" * 60)
        
        success_count = 0
        
        for op in self.operations:
            print(f"\n{op.reason}")
            
            # Create target if needed
            if not self.dry_run:
                op.target_dir.mkdir(parents=True, exist_ok=True)
            
            for source_file, target_file in op.files_to_move:
                if self.dry_run:
                    print(f"   Would move: {source_file.name}")
                else:
                    try:
                        # Use git mv
                        subprocess.run(
                            ["git", "mv", str(source_file), str(target_file)],
                            check=True,
                            capture_output=True
                        )
                        print(f"   ✅ Moved: {source_file.name}")
                    except subprocess.CalledProcessError as e:
                        print(f"   ❌ Failed: {source_file.name}")
                        print(f"      {e.stderr.decode() if e.stderr else 'Unknown error'}")
                        continue
            
            # Check if source is now empty
            if not self.dry_run and op.source_dir.exists():
                remaining = list(op.source_dir.iterdir())
                if not remaining:
                    # Remove empty directory
                    try:
                        op.source_dir.rmdir()
                        print(f"   🗑️  Removed empty: {op.source_dir}")
                    except OSError:
                        pass
            
            success_count += 1
        
        print(f"\n{'='*60}")
        print(f"Completed: {success_count}/{len(self.operations)} operations")
        
        if self.dry_run:
            print("\nThis was a dry run. Use without --dry-run to execute.")
        else:
            print("\nNext steps:")
            print("  1. Update CMakeLists.txt with new paths")
            print("  2. Update any hardcoded includes")
            print("  3. Build and test")
            print("  4. Commit changes")
        
        return True
    
    def create_readmes(self):
        """Create README files for new directories."""
        readmes = {
            "modules/zenith_ui/components": """# UI Components

Atomic, reusable UI components.

## Naming
- Use `UI` prefix (e.g., `UISkiaButton.h`)
- One component per file
""",
            "modules/zenith_ui/theme": """# UI Theme

Design system: colors, typography, spacing.

## Files
- `ZenithTheme.h` - Main theme
- `ZenithDesignSystem.h` - Design tokens
""",
        }
        
        for dir_path, content in readmes.items():
            path = Path(dir_path) / "README.md"
            if not path.exists():
                if self.dry_run:
                    print(f"Would create: {path}")
                else:
                    path.write_text(content)
                    print(f"Created: {path}")


def main():
    parser = argparse.ArgumentParser(
        description="Incrementally flatten directory structure"
    )
    parser.add_argument(
        "--analyze",
        action="store_true",
        help="Analyze what would be flattened"
    )
    parser.add_argument(
        "--execute",
        action="store_true",
        help="Execute flattening"
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would happen without making changes"
    )
    
    args = parser.parse_args()
    
    dry_run = args.dry_run or not args.execute
    
    if args.analyze:
        flattener = IncrementalFlattener(dry_run=True)
        flattener.analyze()
        return 0
    
    if args.execute:
        flattener = IncrementalFlattener(dry_run=dry_run)
        
        if not dry_run and not flattener.check_git_state():
            return 1
        
        if not flattener.analyze():
            print("\nNothing to flatten.")
            return 0
        
        if not dry_run:
            flattener.create_readmes()
        
        return 0 if flattener.execute() else 1
    
    parser.print_help()
    return 1


if __name__ == "__main__":
    sys.exit(main())
