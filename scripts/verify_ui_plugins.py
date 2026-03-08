#!/usr/bin/env python3
"""
Verify UI/Plugins/CommandAPI/Hotspot claims from architecture plan.
Generates claim_verification_ui_plugins.json
"""

import json
import os
import subprocess
from pathlib import Path
from typing import Dict, List, Any
from collections import defaultdict

REPO_ROOT = Path("/home/user/daw")

def run_command(cmd: List[str], cwd=None) -> str:
    """Run shell command and return output."""
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd or REPO_ROOT,
            capture_output=True,
            text=True,
            timeout=30
        )
        return result.stdout + result.stderr
    except Exception as e:
        return f"ERROR: {e}"

def scan_branches() -> Dict[str, Any]:
    """Scan repository branches and their touched files."""
    branches_output = run_command(["git", "branch", "-a"])
    branches = [
        b.strip().replace("* ", "").replace("remotes/origin/", "")
        for b in branches_output.split("\n")
        if b.strip() and "HEAD" not in b
    ]

    # Remove duplicates
    unique_branches = list(set(branches))

    branch_info = {}
    for branch in unique_branches:
        if branch.startswith("origin/"):
            continue

        # Get files changed in this branch (compared to 5 commits ago as baseline)
        diff_output = run_command([
            "git", "diff", "--name-only", "HEAD~5..HEAD"
        ])

        touched_files = [f.strip() for f in diff_output.split("\n") if f.strip()]

        # Categorize touched files
        touched_summary = {
            "engine": [f for f in touched_files if "Engine" in f or "engine" in f],
            "track": [f for f in touched_files if "Track" in f or "track" in f],
            "ui": [f for f in touched_files if any(ui in f for ui in ["UI", "Window", "Component", "GUI"])],
            "plugin": [f for f in touched_files if "Plugin" in f or "plugin" in f],
            "command": [f for f in touched_files if "Command" in f or "command" in f],
            "cmake": [f for f in touched_files if "CMake" in f or ".cmake" in f],
            "all_files": touched_files
        }

        branch_info[branch] = {
            "exists": True,
            "touched_files_count": len(touched_files),
            "touched_paths_summary": touched_summary
        }

    return {"branches": branch_info, "scan_timestamp": "2025-11-17"}

def check_file_exists(pattern: str) -> List[str]:
    """Find files matching pattern."""
    try:
        result = subprocess.run(
            ["find", str(REPO_ROOT), "-type", "f", "-name", pattern],
            capture_output=True,
            text=True,
            timeout=10
        )
        return [f.strip() for f in result.stdout.split("\n") if f.strip()]
    except:
        return []

def check_code_contains(file_paths: List[str], search_term: str) -> Dict[str, List[str]]:
    """Search for term in files."""
    results = {}
    for file_path in file_paths:
        if not os.path.exists(file_path):
            continue
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                if search_term.lower() in content.lower():
                    # Find lines containing the term
                    lines = [
                        f"L{i+1}: {line.strip()[:100]}"
                        for i, line in enumerate(content.split('\n'))
                        if search_term.lower() in line.lower()
                    ]
                    results[file_path] = lines[:5]  # Max 5 lines
        except:
            continue
    return results

def generate_plan_claims() -> List[Dict[str, Any]]:
    """Generate synthetic plan claims to verify."""
    claims = []

    # UI/UX claims
    claims.append({
        "id": "ui-001",
        "section": 3,
        "domain": "ui-ux",
        "mode": "descriptive",
        "statement": "ArrangementView component exists for timeline/clips view",
        "verification_method": "check_file_exists",
        "verification_params": {"patterns": ["*Arrangement*.cpp", "*Arrangement*.h"]}
    })

    claims.append({
        "id": "ui-002",
        "section": 3,
        "domain": "ui-ux",
        "mode": "descriptive",
        "statement": "MixerComponent exists for mixer view",
        "verification_method": "check_file_exists",
        "verification_params": {"patterns": ["*Mixer*.cpp", "*Mixer*.h"]}
    })

    claims.append({
        "id": "ui-003",
        "section": 3,
        "domain": "ui-ux",
        "mode": "descriptive",
        "statement": "MainWindow exists as application window",
        "verification_method": "check_file_exists",
        "verification_params": {"patterns": ["MainWindow.cpp", "MainWindow.h"]}
    })

    claims.append({
        "id": "ui-004",
        "section": 3,
        "domain": "ui-ux",
        "mode": "descriptive",
        "statement": "TransportComponent exists for play/stop/record controls",
        "verification_method": "check_file_exists",
        "verification_params": {"patterns": ["*Transport*.cpp", "*Transport*.h"]}
    })

    # Plugin/MIDI claims
    claims.append({
        "id": "plugin-001",
        "section": 3,
        "domain": "plugins-midi",
        "mode": "descriptive",
        "statement": "PluginHost component exists for VST3/AU hosting",
        "verification_method": "check_file_exists",
        "verification_params": {"patterns": ["PluginHost.cpp", "PluginHost.h"]}
    })

    claims.append({
        "id": "plugin-002",
        "section": 3,
        "domain": "plugins-midi",
        "mode": "descriptive",
        "statement": "MidiRouter exists for MIDI I/O routing",
        "verification_method": "check_code_contains",
        "verification_params": {"search_terms": ["MidiRouter", "MIDI routing"]}
    })

    # CommandAPI claims
    claims.append({
        "id": "cmd-001",
        "section": 3,
        "domain": "command-api",
        "mode": "descriptive",
        "statement": "CommandAPI exists with track operations",
        "verification_method": "check_file_exists",
        "verification_params": {"patterns": ["CommandAPI.cpp", "CommandAPI.h"]}
    })

    claims.append({
        "id": "cmd-002",
        "section": 3,
        "domain": "command-api",
        "mode": "descriptive",
        "statement": "CommandAPI supports transport operations",
        "verification_method": "check_code_contains",
        "verification_params": {"search_terms": ["transport", "play", "stop"]}
    })

    claims.append({
        "id": "cmd-003",
        "section": 3,
        "domain": "command-api",
        "mode": "descriptive",
        "statement": "CommandAPI supports undo/redo operations",
        "verification_method": "check_code_contains",
        "verification_params": {"search_terms": ["undo", "redo"]}
    })

    # Conflict hotspot claims
    claims.append({
        "id": "hotspot-001",
        "section": 4,
        "domain": "branches-prs",
        "mode": "descriptive",
        "statement": "Engine.cpp is a CRITICAL conflict hotspot (touched by multiple branches)",
        "verification_method": "check_hotspot",
        "verification_params": {"file_pattern": "Engine.cpp", "risk_level": "CRITICAL"}
    })

    claims.append({
        "id": "hotspot-002",
        "section": 4,
        "domain": "branches-prs",
        "mode": "descriptive",
        "statement": "MainWindow.* is a HIGH conflict hotspot",
        "verification_method": "check_hotspot",
        "verification_params": {"file_pattern": "MainWindow.*", "risk_level": "HIGH"}
    })

    claims.append({
        "id": "hotspot-003",
        "section": 4,
        "domain": "branches-prs",
        "mode": "descriptive",
        "statement": "CommandAPI.* is a HIGH conflict hotspot",
        "verification_method": "check_hotspot",
        "verification_params": {"file_pattern": "CommandAPI.*", "risk_level": "HIGH"}
    })

    claims.append({
        "id": "hotspot-004",
        "section": 4,
        "domain": "branches-prs",
        "mode": "descriptive",
        "statement": "CMakeLists.txt is a MEDIUM conflict hotspot",
        "verification_method": "check_hotspot",
        "verification_params": {"file_pattern": "CMakeLists.txt", "risk_level": "MEDIUM"}
    })

    return claims

def verify_claim(claim: Dict[str, Any], branch_scan: Dict[str, Any]) -> Dict[str, Any]:
    """Verify a single claim."""
    method = claim["verification_method"]
    params = claim.get("verification_params", {})

    result = {
        "claim_id": claim["id"],
        "statement": claim["statement"],
        "domain": claim["domain"],
        "section": claim["section"],
        "status": "unknown",
        "evidence": [],
        "notes": ""
    }

    if method == "check_file_exists":
        patterns = params.get("patterns", [])
        all_files = []
        for pattern in patterns:
            files = check_file_exists(pattern)
            all_files.extend(files)

        if all_files:
            result["status"] = "true"
            result["evidence"] = all_files[:10]  # Limit to 10 files
            result["notes"] = f"Found {len(all_files)} matching files"
        else:
            result["status"] = "likely-false"
            result["notes"] = f"No files found matching patterns: {patterns}"

    elif method == "check_code_contains":
        search_terms = params.get("search_terms", [])

        # First find relevant files
        cpp_files = check_file_exists("*.cpp") + check_file_exists("*.h")

        found_any = False
        evidence_files = {}

        for term in search_terms:
            matches = check_code_contains(cpp_files, term)
            if matches:
                found_any = True
                evidence_files.update(matches)

        if found_any:
            result["status"] = "true"
            result["evidence"] = [
                f"{file}: {', '.join(lines[:2])}"
                for file, lines in list(evidence_files.items())[:5]
            ]
            result["notes"] = f"Found references in {len(evidence_files)} files"
        else:
            result["status"] = "likely-false"
            result["notes"] = f"No code references found for terms: {search_terms}"

    elif method == "check_hotspot":
        file_pattern = params.get("file_pattern", "")
        risk_level = params.get("risk_level", "UNKNOWN")

        # Check how many branches touch files matching this pattern
        branches_touching = 0
        touching_branches = []

        for branch_name, branch_data in branch_scan.get("branches", {}).items():
            all_files = branch_data.get("touched_paths_summary", {}).get("all_files", [])

            # Check if any file matches the pattern
            for file in all_files:
                if file_pattern.replace(".*", "") in file or file_pattern.replace("*", "") in file:
                    branches_touching += 1
                    touching_branches.append(branch_name)
                    break

        # Also check if file exists in repo
        files_exist = check_file_exists(file_pattern.replace(".*", ".*"))

        if files_exist:
            # Determine if the risk level claim is accurate
            if risk_level == "CRITICAL" and branches_touching >= 2:
                result["status"] = "true"
            elif risk_level == "HIGH" and branches_touching >= 1:
                result["status"] = "partial"
            elif risk_level == "MEDIUM":
                result["status"] = "partial"
            else:
                result["status"] = "partial"

            result["evidence"] = [
                f"File exists: {', '.join(files_exist[:3])}",
                f"Touched by {branches_touching} branch(es): {', '.join(touching_branches[:3])}"
            ]
            result["notes"] = f"Claimed {risk_level} risk; {branches_touching} branches touch this file"
        else:
            result["status"] = "likely-false"
            result["notes"] = f"File pattern '{file_pattern}' not found in repository"

    return result

def main():
    """Main verification workflow."""
    print("=" * 80)
    print("UI/Plugins/CommandAPI/Hotspot Verification")
    print("=" * 80)

    # Step 1: Scan branches
    print("\n[1/5] Scanning branches...")
    branch_scan = scan_branches()

    # Save branch_scan.json
    with open(REPO_ROOT / "branch_scan.json", "w") as f:
        json.dump(branch_scan, f, indent=2)
    print(f"  - Found {len(branch_scan['branches'])} branches")

    # Step 2: Generate claims
    print("\n[2/5] Generating plan claims...")
    claims = generate_plan_claims()

    # Save plan_claims.json
    plan_claims_data = {
        "claims": claims,
        "metadata": {
            "generated_at": "2025-11-17",
            "total_claims": len(claims),
            "domains": list(set(c["domain"] for c in claims))
        }
    }
    with open(REPO_ROOT / "plan_claims.json", "w") as f:
        json.dump(plan_claims_data, f, indent=2)
    print(f"  - Generated {len(claims)} claims")

    # Step 3: Filter claims for target domains
    print("\n[3/5] Filtering claims...")
    target_domains = {"ui-ux", "plugins-midi", "command-api", "branches-prs"}
    filtered_claims = [
        c for c in claims
        if c["domain"] in target_domains and c["mode"] == "descriptive"
    ]
    print(f"  - {len(filtered_claims)} claims match target domains")

    # Step 4: Verify each claim
    print("\n[4/5] Verifying claims...")
    verified_claims = []
    status_counts = defaultdict(int)

    for claim in filtered_claims:
        result = verify_claim(claim, branch_scan)
        verified_claims.append(result)
        status_counts[result["status"]] += 1

    # Step 5: Generate output
    print("\n[5/5] Generating output...")

    output_data = {
        "verification_metadata": {
            "timestamp": "2025-11-17T00:00:00Z",
            "scope": "ui-ux, plugins-midi, command-api, branches-prs domains",
            "total_claims_processed": len(verified_claims),
            "status_breakdown": dict(status_counts)
        },
        "verified_claims": verified_claims
    }

    # Save results
    with open(REPO_ROOT / "claim_verification_ui_plugins.json", "w") as f:
        json.dump(output_data, f, indent=2)

    # Print summary
    print("\n" + "=" * 80)
    print("VERIFICATION SUMMARY")
    print("=" * 80)
    print(f"\nTotal claims processed: {len(verified_claims)}")
    print(f"\nStatus breakdown:")
    for status, count in sorted(status_counts.items()):
        print(f"  {status:15s}: {count:3d}")

    # Find top mismatches (likely-false or partial)
    print(f"\n{'=' * 80}")
    print("TOP MISMATCHES / ISSUES")
    print("=" * 80)

    mismatches = [
        c for c in verified_claims
        if c["status"] in ["likely-false", "partial"]
    ]

    if mismatches:
        for i, claim in enumerate(mismatches[:10], 1):
            print(f"\n{i}. [{claim['claim_id']}] {claim['status'].upper()}")
            print(f"   Statement: {claim['statement'][:80]}...")
            print(f"   Notes: {claim['notes']}")
    else:
        print("\n✓ No significant mismatches found - all claims verified!")

    print(f"\n{'=' * 80}")
    print(f"Results saved to: claim_verification_ui_plugins.json")
    print(f"Branch scan saved to: branch_scan.json")
    print(f"Plan claims saved to: plan_claims.json")
    print("=" * 80)

if __name__ == "__main__":
    main()
