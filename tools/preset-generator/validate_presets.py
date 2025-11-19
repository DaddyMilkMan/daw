#!/usr/bin/env python3
"""
Preset Library Validator
Validates the generated preset library for correctness
"""

import json
import sys
from pathlib import Path
from typing import Dict, List, Tuple


class PresetValidator:
    """Validates preset libraries"""

    def __init__(self, presets_dir: str):
        self.presets_dir = Path(presets_dir)
        self.errors = []
        self.warnings = []

        # Expected parameter ranges
        self.polysynth_params = {
            'osc_type': (0.0, 2.0),
            'filter_cutoff': (0.0, 1.0),
            'filter_resonance': (0.0, 1.0),
            'attack': (0.0, 1.0),
            'decay': (0.0, 1.0),
            'sustain': (0.0, 1.0),
            'release': (0.0, 1.0)
        }

        self.sampler_params = {
            'attack': (0.001, 5.0),
            'decay': (0.001, 5.0),
            'sustain': (0.0, 1.0),
            'release': (0.001, 10.0),
            'filter_cutoff': (0.0, 1.0),
            'filter_resonance': (0.0, 1.0),
            'tune': (-12.0, 12.0),
            'gain': (0.0, 2.0),
            'character': (0.0, 1.0)
        }

        # Valid categories and tags
        self.polysynth_categories = {'Bass', 'Lead', 'Pluck', 'Pad', 'Arp', 'FX', 'Keys'}
        self.sampler_categories = {'808', 'Drums', 'Keys', 'FX'}

        self.valid_tags = {
            'trap', '808', 'sub', 'bass', 'hip-hop', 'reese', 'dnb', 'dubstep',
            'edm', 'aggressive', 'donk', 'acid', 'techno', '303', 'deep', 'minimal',
            'square', 'retro', 'gameboy', 'wobble', 'soft', 'ambient', 'saw',
            'punchy', 'vintage', 'analog', 'gritty', 'distorted', 'sine', 'smooth',
            'clean', 'modern', 'dark', 'mysterious', 'lead', 'supersaw', 'festival',
            'mono', 'vocal', 'singing', 'expressive', 'glide', 'melodic', 'chiptune',
            'thick', 'trance', 'uplifting', 'euphoric', 'dirty', 'breathy', 'airy',
            'sync', 'sharp', 'cutting', 'classic', 'versatile', 'pluck', 'future-bass',
            'chords', 'bright', 'muted', 'short', 'percussive', 'bell', 'resonant',
            'gentle', 'arp', 'stab', 'accent', 'house', 'dance', 'hollow', 'tight',
            'pad', 'warm', 'lush', 'digital', 'deep', 'atmospheric', 'drone',
            'evolving', 'gentle', 'rich', 'ethereal', 'sweep', 'movement', 'dynamic',
            'wash', 'texture', 'gate', 'sequence', 'triplet', 'staccato', 'rhythmic',
            'pure', 'groovy', 'fx', 'riser', 'build', 'tension', 'down', 'transition',
            'fall', 'impact', 'hit', 'boom', 'whoosh', 'zap', 'laser', 'sci-fi',
            'background', 'glitch', 'keys', 'electric-piano', 'marimba', 'wood',
            'piano', 'vibes', 'mallet', 'jazz', 'music-box', 'delicate', 'whimsical',
            'drums', 'clap', 'snare', 'hi-hat', 'crisp', 'percussion', 'kick',
            'rim', 'lofi', 'dusty', 'electric', 'plucky', 'toy', 'cinematic',
            'vox', 'shot'
        }

    def error(self, msg: str):
        """Add error message"""
        self.errors.append(f"ERROR: {msg}")

    def warning(self, msg: str):
        """Add warning message"""
        self.warnings.append(f"WARNING: {msg}")

    def validate_parameter_value(self, param_id: str, value: float,
                                 valid_ranges: Dict[str, Tuple[float, float]],
                                 preset_id: str) -> bool:
        """Validate a parameter value is within range"""
        if param_id not in valid_ranges:
            self.error(f"Preset '{preset_id}': Unknown parameter '{param_id}'")
            return False

        min_val, max_val = valid_ranges[param_id]
        if not (min_val <= value <= max_val):
            self.error(
                f"Preset '{preset_id}': Parameter '{param_id}' value {value} "
                f"out of range [{min_val}, {max_val}]"
            )
            return False

        return True

    def validate_preset(self, preset: Dict, instrument: str) -> bool:
        """Validate a single preset"""
        preset_id = preset.get('id', 'UNKNOWN')
        valid = True

        # Check required fields
        required_fields = ['id', 'name', 'category', 'tags', 'parameters']
        for field in required_fields:
            if field not in preset:
                self.error(f"Preset '{preset_id}': Missing required field '{field}'")
                valid = False

        if not valid:
            return False

        # Validate category
        if instrument == 'zenith_poly_synth':
            if preset['category'] not in self.polysynth_categories:
                self.error(
                    f"Preset '{preset_id}': Invalid category '{preset['category']}' "
                    f"(valid: {self.polysynth_categories})"
                )
                valid = False
            param_ranges = self.polysynth_params
        else:
            if preset['category'] not in self.sampler_categories:
                self.error(
                    f"Preset '{preset_id}': Invalid category '{preset['category']}' "
                    f"(valid: {self.sampler_categories})"
                )
                valid = False
            param_ranges = self.sampler_params

        # Validate tags
        if not isinstance(preset['tags'], list):
            self.error(f"Preset '{preset_id}': Tags must be a list")
            valid = False
        else:
            for tag in preset['tags']:
                if tag not in self.valid_tags:
                    self.warning(f"Preset '{preset_id}': Unknown tag '{tag}'")

        # Validate parameters
        if not isinstance(preset['parameters'], dict):
            self.error(f"Preset '{preset_id}': Parameters must be a dict")
            valid = False
        else:
            for param_id, value in preset['parameters'].items():
                if not self.validate_parameter_value(param_id, value, param_ranges, preset_id):
                    valid = False

        return valid

    def validate_bank(self, bank_file: Path) -> Tuple[bool, int]:
        """Validate a preset bank file"""
        print(f"  Validating {bank_file.name}...")

        try:
            with open(bank_file, 'r') as f:
                bank_data = json.load(f)
        except json.JSONDecodeError as e:
            self.error(f"Failed to parse {bank_file}: {e}")
            return False, 0
        except Exception as e:
            self.error(f"Failed to load {bank_file}: {e}")
            return False, 0

        # Validate bank structure
        required_fields = ['instrument', 'bank_name', 'version', 'preset_count', 'presets']
        for field in required_fields:
            if field not in bank_data:
                self.error(f"Bank {bank_file.name}: Missing required field '{field}'")
                return False, 0

        instrument = bank_data['instrument']
        presets = bank_data['presets']

        # Validate preset count matches
        if len(presets) != bank_data['preset_count']:
            self.error(
                f"Bank {bank_file.name}: Preset count mismatch "
                f"(declared: {bank_data['preset_count']}, actual: {len(presets)})"
            )
            return False, len(presets)

        # Validate each preset
        valid = True
        for preset in presets:
            if not self.validate_preset(preset, instrument):
                valid = False

        return valid, len(presets)

    def validate_polysynth(self) -> Tuple[bool, int]:
        """Validate ZenithPolySynth presets"""
        print("\n=== Validating ZenithPolySynth Presets ===")
        polysynth_dir = self.presets_dir / "ZenithPolySynth"

        if not polysynth_dir.exists():
            self.error(f"ZenithPolySynth directory not found: {polysynth_dir}")
            return False, 0

        bank_files = list(polysynth_dir.glob("*_bank.json"))
        if not bank_files:
            self.error("No ZenithPolySynth bank files found")
            return False, 0

        total_presets = 0
        all_valid = True

        for bank_file in bank_files:
            valid, count = self.validate_bank(bank_file)
            total_presets += count
            if not valid:
                all_valid = False

        print(f"  Total presets validated: {total_presets}")
        return all_valid, total_presets

    def validate_sampler(self) -> Tuple[bool, int]:
        """Validate ZenithSampler presets"""
        print("\n=== Validating ZenithSampler Presets ===")
        sampler_dir = self.presets_dir / "ZenithSampler"

        if not sampler_dir.exists():
            self.error(f"ZenithSampler directory not found: {sampler_dir}")
            return False, 0

        bank_files = list(sampler_dir.glob("*_bank.json"))
        if not bank_files:
            self.error("No ZenithSampler bank files found")
            return False, 0

        total_presets = 0
        all_valid = True

        for bank_file in bank_files:
            valid, count = self.validate_bank(bank_file)
            total_presets += count
            if not valid:
                all_valid = False

        print(f"  Total presets validated: {total_presets}")
        return all_valid, total_presets

    def validate_index(self) -> bool:
        """Validate preset index"""
        print("\n=== Validating Preset Index ===")
        index_file = self.presets_dir / "preset_index.json"

        if not index_file.exists():
            self.error(f"Preset index not found: {index_file}")
            return False

        try:
            with open(index_file, 'r') as f:
                index = json.load(f)
        except Exception as e:
            self.error(f"Failed to load preset index: {e}")
            return False

        print(f"  ✓ Preset index loaded")
        print(f"    Total presets: {index.get('total_presets', 0)}")

        return True

    def run(self) -> bool:
        """Run validation"""
        print("="*60)
        print("PRESET LIBRARY VALIDATION")
        print("="*60)

        polysynth_valid, polysynth_count = self.validate_polysynth()
        sampler_valid, sampler_count = self.validate_sampler()
        index_valid = self.validate_index()

        total_presets = polysynth_count + sampler_count

        # Print results
        print("\n" + "="*60)
        print("VALIDATION RESULTS")
        print("="*60)

        if self.warnings:
            print(f"\nWarnings ({len(self.warnings)}):")
            for warning in self.warnings[:10]:  # Show first 10
                print(f"  {warning}")
            if len(self.warnings) > 10:
                print(f"  ... and {len(self.warnings) - 10} more warnings")

        if self.errors:
            print(f"\nErrors ({len(self.errors)}):")
            for error in self.errors[:10]:  # Show first 10
                print(f"  {error}")
            if len(self.errors) > 10:
                print(f"  ... and {len(self.errors) - 10} more errors")

        print(f"\nTotal Presets: {total_presets}")
        print(f"Minimum Required: 500")

        all_valid = polysynth_valid and sampler_valid and index_valid and len(self.errors) == 0

        if all_valid and total_presets >= 500:
            print("\n✓ VALIDATION PASSED!")
            print(f"  - All {total_presets} presets are valid")
            print(f"  - All parameters within range")
            print(f"  - All metadata correct")
            print(f"  - Preset count >= 500 ✓")
        else:
            print("\n✗ VALIDATION FAILED")
            if len(self.errors) > 0:
                print(f"  - {len(self.errors)} errors found")
            if total_presets < 500:
                print(f"  - Preset count ({total_presets}) below minimum (500)")

        print("="*60 + "\n")

        return all_valid and total_presets >= 500


def main():
    """Main entry point"""
    script_dir = Path(__file__).parent
    presets_dir = script_dir.parent.parent / "Content" / "Presets"

    validator = PresetValidator(str(presets_dir))
    success = validator.run()

    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
