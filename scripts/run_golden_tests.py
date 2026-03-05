#!/usr/bin/env python3
"""
run_golden_tests.py
Phase 3 – A+ Audio Production Quality

Helper script for managing and verifying the golden render master file.

Usage
-----
# Verify existing golden file against current engine output:
    python3 scripts/run_golden_tests.py --verify --build-dir build

# Regenerate the golden file after an approved engine change:
    python3 scripts/run_golden_tests.py --regenerate --build-dir build

# Run all Phase 3 tests (golden + stress + soak) and report results:
    python3 scripts/run_golden_tests.py --all --build-dir build

The script delegates the actual rendering to the ZenithDAWTests binary;
it reads/writes tests/golden/canonical_session_golden.json for the stored
reference hash.
"""

import argparse
import json
import os
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path


# ─────────────────────────────────────────────────────────────────────────────
# Colour helpers
# ─────────────────────────────────────────────────────────────────────────────

class C:
    RED    = "\033[91m"
    GREEN  = "\033[92m"
    YELLOW = "\033[93m"
    BOLD   = "\033[1m"
    RESET  = "\033[0m"


def ok(msg: str) -> None:
    print(f"{C.GREEN}✅  {msg}{C.RESET}")


def warn(msg: str) -> None:
    print(f"{C.YELLOW}⚠️   {msg}{C.RESET}")


def fail(msg: str) -> None:
    print(f"{C.RED}❌  {msg}{C.RESET}")


def info(msg: str) -> None:
    print(f"    {msg}")


# ─────────────────────────────────────────────────────────────────────────────
# Paths
# ─────────────────────────────────────────────────────────────────────────────

REPO_ROOT   = Path(__file__).resolve().parent.parent
GOLDEN_FILE = REPO_ROOT / "tests" / "golden" / "canonical_session_golden.json"


# ─────────────────────────────────────────────────────────────────────────────
# Test binary helpers
# ─────────────────────────────────────────────────────────────────────────────

def find_test_binary(build_dir: Path) -> Path | None:
    """Locate the ZenithDAWTests binary under build_dir."""
    candidates = [
        build_dir / "ZenithDAWTests",
        build_dir / "ZenithDAWTests_artefacts" / "Release" / "ZenithDAWTests",
        build_dir / "ZenithDAWTests_artefacts" / "Debug"   / "ZenithDAWTests",
        build_dir / "ZenithDAWTests.exe",
        build_dir / "ZenithDAWTests_artefacts" / "Release" / "ZenithDAWTests.exe",
    ]
    for p in candidates:
        if p.exists():
            return p
    return None


def run_test_category(binary: Path, category: str, timeout: int = 600) -> bool:
    """Run a subset of ZenithDAWTests matching *category*. Returns True on pass."""
    cmd = [str(binary), category]
    info(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=False, timeout=timeout)
    return result.returncode == 0


# ─────────────────────────────────────────────────────────────────────────────
# Golden file operations
# ─────────────────────────────────────────────────────────────────────────────

def load_golden_file() -> dict | None:
    if not GOLDEN_FILE.exists():
        return None
    try:
        with open(GOLDEN_FILE) as f:
            return json.load(f)
    except json.JSONDecodeError as e:
        fail(f"Could not parse golden file: {e}")
        return None


def save_golden_file(data: dict) -> None:
    GOLDEN_FILE.parent.mkdir(parents=True, exist_ok=True)
    with open(GOLDEN_FILE, "w") as f:
        json.dump(data, f, indent=2)
        f.write("\n")


# ─────────────────────────────────────────────────────────────────────────────
# Commands
# ─────────────────────────────────────────────────────────────────────────────

def cmd_verify(build_dir: Path) -> bool:
    """Verify that the engine output matches the stored golden hash."""
    print(f"\n{C.BOLD}=== Golden Render Verification ==={C.RESET}")

    binary = find_test_binary(build_dir)
    if binary is None:
        fail(f"Test binary not found in {build_dir}. Run cmake --build first.")
        return False

    golden = load_golden_file()
    if golden is None:
        warn("Golden file not found – running once to bootstrap it.")
        # GoldenRenderTest creates the file on first run; pass the test.
        success = run_test_category(binary, "AudioEngine")
        if success:
            ok("Golden file bootstrapped.  Commit tests/golden/canonical_session_golden.json.")
        else:
            fail("Bootstrap run failed.")
        return success

    stored_hash = golden.get("canonical_hash", "PLACEHOLDER")
    if stored_hash == "PLACEHOLDER":
        warn("Golden file has placeholder hash – running once to populate it.")
        success = run_test_category(binary, "AudioEngine")
        if success:
            ok("Placeholder replaced.  Commit tests/golden/canonical_session_golden.json.")
        return success

    info(f"Stored hash : {stored_hash}")
    success = run_test_category(binary, "AudioEngine")
    if success:
        ok("Golden render verification passed.")
    else:
        fail("Golden render verification FAILED.  "
             "If this is an intentional engine change, run --regenerate.")
    return success


def cmd_regenerate(build_dir: Path) -> bool:
    """Re-run the engine, capture the new hash, update the golden file."""
    print(f"\n{C.BOLD}=== Regenerating Golden Master ==={C.RESET}")

    binary = find_test_binary(build_dir)
    if binary is None:
        fail(f"Test binary not found in {build_dir}.")
        return False

    warn("This will overwrite the stored golden hash.  Commit the result only "
         "after reviewing the audio change is intentional.")

    # Delete existing canonical_hash so GoldenRenderTest auto-writes a new one
    golden = load_golden_file() or {}
    golden["canonical_hash"] = "PLACEHOLDER"
    golden["generated_at"]   = "PLACEHOLDER"
    save_golden_file(golden)

    success = run_test_category(binary, "AudioEngine")

    # Reload to show the newly written hash
    updated = load_golden_file()
    if updated:
        new_hash = updated.get("canonical_hash", "unknown")
        generated = updated.get("generated_at", "unknown")
        if success:
            ok(f"New golden hash : {new_hash}")
            info(f"Generated at    : {generated}")
            info("Commit tests/golden/canonical_session_golden.json to lock the reference.")
        else:
            fail("Regeneration run failed.")
    return success


def cmd_stress(build_dir: Path) -> bool:
    """Run the engine stress tests."""
    print(f"\n{C.BOLD}=== Engine Stress Tests ==={C.RESET}")
    binary = find_test_binary(build_dir)
    if binary is None:
        fail(f"Test binary not found in {build_dir}.")
        return False
    success = run_test_category(binary, "Performance", timeout=300)
    if success:
        ok("Stress tests passed.")
    else:
        fail("Stress tests FAILED.")
    return success


def cmd_soak(build_dir: Path, soak_seconds: int | None = None) -> bool:
    """Run the soak tests."""
    print(f"\n{C.BOLD}=== Soak Tests ==={C.RESET}")
    binary = find_test_binary(build_dir)
    if binary is None:
        fail(f"Test binary not found in {build_dir}.")
        return False

    env = os.environ.copy()
    if soak_seconds is not None:
        env["ZENITH_SOAK_SECONDS"] = str(soak_seconds)
        info(f"ZENITH_SOAK_SECONDS={soak_seconds}")

    soak_sec = soak_seconds or int(env.get("ZENITH_SOAK_SECONDS", "60"))
    timeout  = soak_sec + 120  # Allow 2 min buffer above soak duration

    cmd = [str(binary), "Stability"]
    info(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=False, timeout=timeout, env=env)
    success = result.returncode == 0
    if success:
        ok("Soak tests passed.")
    else:
        fail("Soak tests FAILED.")
    return success


def cmd_all(build_dir: Path, soak_seconds: int | None = None) -> bool:
    """Run all Phase 3 tests: golden + stress + soak."""
    print(f"\n{C.BOLD}{'=' * 60}{C.RESET}")
    print(f"{C.BOLD}  Zenith DAW – Phase 3 Audio Quality Test Suite{C.RESET}")
    print(f"{C.BOLD}{'=' * 60}{C.RESET}")

    results: dict[str, bool] = {}

    results["golden"] = cmd_verify(build_dir)
    results["stress"] = cmd_stress(build_dir)
    results["soak"]   = cmd_soak(build_dir, soak_seconds)

    print(f"\n{C.BOLD}=== Phase 3 Summary ==={C.RESET}")
    all_passed = True
    for name, passed in results.items():
        if passed:
            ok(f"{name:<10} PASSED")
        else:
            fail(f"{name:<10} FAILED")
            all_passed = False

    if all_passed:
        print(f"\n{C.BOLD}{C.GREEN}✅  All Phase 3 tests passed!{C.RESET}")
    else:
        print(f"\n{C.BOLD}{C.RED}❌  One or more Phase 3 tests failed.{C.RESET}")

    return all_passed


# ─────────────────────────────────────────────────────────────────────────────
# Entry point
# ─────────────────────────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Zenith DAW Phase 3 golden test helper",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--build-dir", default="build",
                        help="CMake build directory (default: build)")
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument("--verify",     action="store_true",
                       help="Verify engine output against stored golden hash")
    group.add_argument("--regenerate", action="store_true",
                       help="Regenerate the golden file from current engine output")
    group.add_argument("--stress",     action="store_true",
                       help="Run engine stress tests only")
    group.add_argument("--soak",       action="store_true",
                       help="Run soak tests only")
    group.add_argument("--all",        action="store_true",
                       help="Run all Phase 3 tests (golden + stress + soak)")
    parser.add_argument("--soak-seconds", type=int, default=None,
                        help="Override ZENITH_SOAK_SECONDS for soak / all runs")

    args  = parser.parse_args()
    build = Path(args.build_dir).resolve()

    if args.regenerate:
        success = cmd_regenerate(build)
    elif args.stress:
        success = cmd_stress(build)
    elif args.soak:
        success = cmd_soak(build, args.soak_seconds)
    elif args.all:
        success = cmd_all(build, args.soak_seconds)
    else:
        # Default: verify
        success = cmd_verify(build)

    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
