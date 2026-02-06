#!/usr/bin/env python3
"""
Verify ZenithPolySynth Presets

Checks:
- All presets have non-zero attack (to avoid clicks)
- Parameter values are in valid ranges
- Provides statistics and breakdown
"""

import json
from pathlib import Path
from typing import Dict, List, Any


class PresetVerifier:
    """Verifies preset quality and statistics"""

    MIN_ATTACK = 0.002  # Minimum attack to avoid clicks

    def __init__(self):
        self.warnings = []
        self.errors = []

    def verify_preset(self, preset: Dict[str, Any], file_name: str) -> bool:
        """
        Verify a single preset

        Returns:
            True if preset is valid
        """
        valid = True
        preset_name = preset.get('name', 'Unknown')

        # Check attack value
        attack = preset['parameters'].get('attack', 0.0)
        if attack < self.MIN_ATTACK:
            self.warnings.append(
                f"{file_name} / {preset_name}: Attack too low ({attack:.4f}) - may cause clicks"
            )
            valid = False

        # Check all parameter ranges
        for param_name, value in preset['parameters'].items():
            if not (0.0 <= value <= 1.0):
                self.errors.append(
                    f"{file_name} / {preset_name}: {param_name} out of range ({value})"
                )
                valid = False

        return valid

    def verify_bank(self, json_file: Path) -> Dict[str, Any]:
        """
        Verify a preset bank file

        Returns:
            Statistics dict
        """
        with open(json_file, 'r') as f:
            bank_data = json.load(f)

        stats = {
            'file': json_file.name,
            'total': len(bank_data['presets']),
            'valid': 0,
            'invalid': 0,
            'categories': {},
            'tags': {},
        }

        for preset in bank_data['presets']:
            if self.verify_preset(preset, json_file.name):
                stats['valid'] += 1
            else:
                stats['invalid'] += 1

            # Count categories
            category = preset.get('category', 'Unknown')
            stats['categories'][category] = stats['categories'].get(category, 0) + 1

            # Count tags
            for tag in preset.get('tags', []):
                stats['tags'][tag] = stats['tags'].get(tag, 0) + 1

        return stats


def main():
    """Main entry point"""
    script_dir = Path(__file__).parent
    repo_root = script_dir.parent
    presets_dir = repo_root / 'zenith-core' / 'Content' / 'Presets' / 'PolySynth'

    verifier = PresetVerifier()

    # Find all JSON files
    json_files = list(presets_dir.glob('*.json'))

    if not json_files:
        print(f"No JSON preset files found in {presets_dir}")
        return 1

    print("="*70)
    print("ZenithPolySynth Preset Verification Report")
    print("="*70)

    all_stats = []
    total_presets = 0
    total_valid = 0
    total_invalid = 0
    category_totals = {}
    tag_totals = {}

    # Verify each bank
    for json_file in sorted(json_files):
        stats = verifier.verify_bank(json_file)
        all_stats.append(stats)

        total_presets += stats['total']
        total_valid += stats['valid']
        total_invalid += stats['invalid']

        # Aggregate categories
        for cat, count in stats['categories'].items():
            category_totals[cat] = category_totals.get(cat, 0) + count

        # Aggregate tags
        for tag, count in stats['tags'].items():
            tag_totals[tag] = tag_totals.get(tag, 0) + count

        print(f"\n{json_file.name}:")
        print(f"  Total: {stats['total']}, Valid: {stats['valid']}, "
              f"Invalid: {stats['invalid']}")

    # Print summary
    print("\n" + "="*70)
    print("SUMMARY")
    print("="*70)
    print(f"Total Presets: {total_presets}")
    print(f"Valid: {total_valid}")
    print(f"Invalid: {total_invalid}")

    print(f"\nBreakdown by Category:")
    for cat, count in sorted(category_totals.items()):
        print(f"  {cat}: {count} presets")

    print(f"\nTop Tags:")
    for tag, count in sorted(tag_totals.items(), key=lambda x: x[1], reverse=True)[:15]:
        print(f"  {tag}: {count} presets")

    # Print warnings and errors
    if verifier.warnings:
        print("\n" + "="*70)
        print("WARNINGS")
        print("="*70)
        for warning in verifier.warnings:
            print(f"  ⚠ {warning}")

    if verifier.errors:
        print("\n" + "="*70)
        print("ERRORS")
        print("="*70)
        for error in verifier.errors:
            print(f"  ✗ {error}")

    # Final status
    print("\n" + "="*70)
    if total_invalid == 0 and not verifier.errors:
        print("✓ All presets verified successfully!")
    else:
        print(f"✗ Found {total_invalid} invalid presets")

    print("="*70)

    return 0 if total_invalid == 0 else 1


if __name__ == '__main__':
    import sys
    sys.exit(main())
