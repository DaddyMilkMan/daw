#!/usr/bin/env python3
"""
SAFE TEST MIGRATOR
==================

Migrates tests to modules/[module]/tests/ structure WITHOUT:
- Copying files (uses git mv)
- Leaving duplicates
- Breaking CMake
- Losing history

Usage:
    python3 scripts/migrate-tests-safe.py --analyze     # See what would migrate
    python3 scripts/migrate-tests-safe.py --execute     # Execute migration
    python3 scripts/migrate-tests-safe.py --verify      # Verify structure
"""

import argparse
import json
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple


@dataclass
class TestMapping:
    """Maps a test file to its new location."""
    source: Path
    target: Path
    module: str
    reason: str


class TestMigrator:
    """Safely migrates tests to mirrored structure."""
    
    # Explicit mappings for known tests
    # Format: (test_filename, target_module, target_subpath, reason)
    KNOWN_MAPPINGS: List[Tuple[str, str, str, str]] = [
        # Engine tests
        ("AudioEngineTests.cpp", "zenith_core", "engine", "Engine tests"),
        ("EngineCoreTests.cpp", "zenith_core", "engine", "Engine core tests"),
        ("EngineTests.cpp", "zenith_core", "engine", "Engine tests"),
        
        # Transport tests
        ("TransportProtocolAgentTest.cpp", "zenith_core", "engine", "Transport protocol tests"),
        ("TrackManagersTest.cpp", "zenith_core", "engine", "Track manager tests"),
        ("ProjectStateTests.cpp", "zenith_core", "engine", "Project state tests"),
        ("ProjectFileIOTests.cpp", "zenith_core", "engine", "Project file I/O tests"),
        
        # Recording tests
        ("RecordingTests.cpp", "zenith_core", "engine", "Recording tests"),
        ("RecordingTempoTest.cpp", "zenith_core", "engine", "Recording tempo tests"),
        
        # DSP tests
        ("AdvancedFiltersTest.cpp", "zenith_dsp", "dsp", "Filter tests"),
        ("AdvancedOscillatorsTest.cpp", "zenith_dsp", "dsp", "Oscillator tests"),
        ("SIMDHelpersTest.cpp", "zenith_dsp", "utils", "SIMD helper tests"),
        
        # Instrument tests
        ("ZenithPolySynthMPETest.cpp", "zenith_core", "instruments", "PolySynth MPE tests"),
        ("ZenithPolySynthMPEIntegrationTest.cpp", "zenith_core", "instruments", "PolySynth MPE integration"),
        
        # UI tests
        ("UITests.cpp", "zenith_ui", "components", "UI component tests"),
        ("ZenithButtonTest.cpp", "zenith_ui", "components", "Button tests"),
        ("ThemeTests.cpp", "zenith_ui", "theme", "Theme tests"),
        
        # Collaboration tests
        ("CollaborationSecurityTest.cpp", "zenith_network", "collaboration", "Collaboration security"),
        ("CRDTSyncTests.cpp", "zenith_network", "collaboration", "CRDT sync tests"),
    ]
    
    def __init__(self, dry_run: bool = True):
        self.dry_run = dry_run
        self.mappings: List[TestMapping] = []
        self.source_dir = Path("apps/desktop/Source/tests")
        self.analysis_results: Dict[str, List[str]] = {
            "will_migrate": [],
            "already_migrated": [],
            "no_mapping": [],
            "conflicts": []
        }
    
    def check_git_state(self) -> bool:
        """Verify git is clean."""
        result = subprocess.run(
            ["git", "status", "--porcelain"],
            capture_output=True,
            text=True
        )
        if result.stdout.strip():
            print("❌ Uncommitted changes detected. Commit or stash first.")
            return False
        return True
    
    def analyze(self) -> bool:
        """Analyze which tests can be migrated."""
        print("\n🔍 Analyzing test migration...")
        print("=" * 60)
        
        if not self.source_dir.exists():
            print(f"❌ Source directory not found: {self.source_dir}")
            return False
        
        # Build mapping lookup
        mapping_dict: Dict[str, Tuple[str, str, str]] = {
            filename: (module, subpath, reason)
            for filename, module, subpath, reason in self.KNOWN_MAPPINGS
        }
        
        # Analyze each test file
        for test_file in sorted(self.source_dir.glob("*.cpp")):
            filename = test_file.name
            
            if filename in mapping_dict:
                module, subpath, reason = mapping_dict[filename]
                target_dir = Path(f"modules/{module}/tests/{subpath}")
                target_file = target_dir / filename
                
                # Check if already migrated
                if target_file.exists():
                    self.analysis_results["already_migrated"].append(filename)
                    continue
                
                # Check for conflicts
                if self._has_conflict(filename, module):
                    self.analysis_results["conflicts"].append(filename)
                    continue
                
                self.mappings.append(TestMapping(
                    source=test_file,
                    target=target_file,
                    module=module,
                    reason=reason
                ))
                self.analysis_results["will_migrate"].append(filename)
            else:
                self.analysis_results["no_mapping"].append(filename)
        
        # Print results
        print(f"\n✅ Will migrate: {len(self.analysis_results['will_migrate'])}")
        for f in self.analysis_results['will_migrate'][:5]:
            print(f"   - {f}")
        if len(self.analysis_results['will_migrate']) > 5:
            print(f"   ... and {len(self.analysis_results['will_migrate']) - 5} more")
        
        if self.analysis_results['already_migrated']:
            print(f"\nℹ️  Already migrated: {len(self.analysis_results['already_migrated'])}")
        
        if self.analysis_results['no_mapping']:
            print(f"\n⚠️  No mapping defined: {len(self.analysis_results['no_mapping'])}")
            for f in self.analysis_results['no_mapping'][:5]:
                print(f"   - {f}")
        
        if self.analysis_results['conflicts']:
            print(f"\n❌ Conflicts: {len(self.analysis_results['conflicts'])}")
            for f in self.analysis_results['conflicts']:
                print(f"   - {f}")
        
        return len(self.mappings) > 0
    
    def _has_conflict(self, filename: str, module: str) -> bool:
        """Check if migrating this file would cause conflicts."""
        # Check if a file with this name exists in multiple modules
        for other_module in ["zenith_core", "zenith_ui", "zenith_dsp", "zenith_network"]:
            if other_module == module:
                continue
            existing = Path(f"modules/{other_module}/tests") / filename
            if existing.exists():
                return True
        return False
    
    def verify_structure(self) -> bool:
        """Verify the migrated test structure is correct."""
        print("\n🔍 Verifying test structure...")
        print("=" * 60)
        
        issues = []
        
        # Check each module
        for module in ["zenith_core", "zenith_ui", "zenith_dsp", "zenith_network", "zenith_commands"]:
            test_dir = Path(f"modules/{module}/tests")
            
            if not test_dir.exists():
                print(f"⚠️  {module}/tests/ does not exist")
                continue
            
            test_files = list(test_dir.rglob("*.cpp"))
            print(f"✅ {module}: {len(test_files)} test files")
            
            # Check for test main
            test_mains = list(test_dir.rglob("TestMain.cpp"))
            if not test_mains:
                issues.append(f"{module}/tests/ missing TestMain.cpp")
        
        # Check source directory
        if self.source_dir.exists():
            remaining = list(self.source_dir.glob("*.cpp"))
            if remaining:
                print(f"\n⚠️  {len(remaining)} tests still in {self.source_dir}")
                print("    Run with --execute to complete migration")
        
        if issues:
            print(f"\n❌ Issues found:")
            for issue in issues:
                print(f"   - {issue}")
            return False
        
        print("\n✅ Test structure looks good!")
        return True
    
    def execute_migration(self) -> bool:
        """Execute the migration."""
        if not self.mappings:
            print("No mappings to execute.")
            return True
        
        print("\n🚀 Executing test migration...")
        print("=" * 60)
        
        success_count = 0
        
        for mapping in self.mappings:
            try:
                # Create target directory
                if not self.dry_run:
                    mapping.target.parent.mkdir(parents=True, exist_ok=True)
                
                # Use git mv
                if self.dry_run:
                    print(f"\nWould migrate:")
                    print(f"  From: {mapping.source}")
                    print(f"  To:   {mapping.target}")
                    print(f"  Reason: {mapping.reason}")
                    success_count += 1
                else:
                    subprocess.run(
                        ["git", "mv", str(mapping.source), str(mapping.target)],
                        check=True,
                        capture_output=True
                    )
                    print(f"✅ Migrated: {mapping.source.name}")
                    success_count += 1
                    
            except subprocess.CalledProcessError as e:
                print(f"❌ Failed: {mapping.source.name}")
                print(f"   Error: {e.stderr.decode() if e.stderr else 'Unknown'}")
        
        print(f"\n{'='*60}")
        print(f"Migrated: {success_count}/{len(self.mappings)}")
        
        if self.dry_run:
            print("\nThis was a dry run. Use --execute to actually migrate.")
        else:
            print("\nNext steps:")
            print("  1. Update CMakeLists.txt to use new test paths")
            print("  2. Build and verify: cmake --build build")
            print("  3. Commit changes")
        
        return success_count == len(self.mappings)
    
    def generate_cmake_update(self):
        """Generate CMakeLists.txt updates needed."""
        print("\n📝 CMake Updates Needed:")
        print("=" * 60)
        
        # Group by module
        by_module: Dict[str, List[TestMapping]] = {}
        for mapping in self.mappings:
            by_module.setdefault(mapping.module, []).append(mapping)
        
        for module, mappings in by_module.items():
            print(f"\n# In cmake/{module.replace('zenith_', '').title()}.cmake or modules/{module}/CMakeLists.txt")
            print(f"set({module.upper()}_TEST_SOURCES")
            for m in mappings:
                rel_path = str(m.target).replace("modules/", "").replace("modules\\", "")
                print(f"    {rel_path}")
            print(")")


def main():
    parser = argparse.ArgumentParser(
        description="Safely migrate tests to module structure"
    )
    parser.add_argument(
        "--analyze",
        action="store_true",
        help="Analyze what would be migrated"
    )
    parser.add_argument(
        "--execute",
        action="store_true",
        help="Execute the migration (requires clean git state)"
    )
    parser.add_argument(
        "--verify",
        action="store_true",
        help="Verify current test structure"
    )
    parser.add_argument(
        "--cmake-updates",
        action="store_true",
        help="Show CMake updates needed"
    )
    
    args = parser.parse_args()
    
    if args.verify:
        migrator = TestMigrator(dry_run=True)
        return 0 if migrator.verify_structure() else 1
    
    if args.cmake_updates:
        migrator = TestMigrator(dry_run=True)
        migrator.analyze()
        migrator.generate_cmake_update()
        return 0
    
    if args.analyze or args.execute:
        dry_run = not args.execute
        migrator = TestMigrator(dry_run=dry_run)
        
        if args.execute and not migrator.check_git_state():
            return 1
        
        if not migrator.analyze():
            print("\nNothing to migrate.")
            return 0
        
        if args.execute:
            return 0 if migrator.execute_migration() else 1
        else:
            # Show CMake updates even in analyze mode
            migrator.generate_cmake_update()
            return 0
    
    parser.print_help()
    return 1


if __name__ == "__main__":
    sys.exit(main())
