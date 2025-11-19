#!/usr/bin/env python3
"""
Zenith Preset Generator
Generates comprehensive preset libraries for ZenithPolySynth and ZenithSampler
"""

import json
import os
import random
import sys
from pathlib import Path
from typing import Dict, List, Any
from dataclasses import dataclass, asdict


@dataclass
class PresetMetadata:
    """Metadata for a preset"""
    id: str
    name: str
    category: str
    tags: List[str]
    description: str = ""


class PresetGenerator:
    """Generates presets with variations"""

    def __init__(self, archetypes_file: str, output_dir: str):
        self.archetypes_file = archetypes_file
        self.output_dir = Path(output_dir)
        self.archetypes = {}
        self.generated_presets = {
            "zenith_poly_synth": {},
            "zenith_sampler": {}
        }
        self.preset_index = {
            "zenith_poly_synth": {},
            "zenith_sampler": {}
        }

    def load_archetypes(self):
        """Load archetype definitions"""
        with open(self.archetypes_file, 'r') as f:
            self.archetypes = json.load(f)
        print(f"✓ Loaded archetypes from {self.archetypes_file}")

    def sanitize_id(self, name: str, index: int = None) -> str:
        """Convert name to valid preset ID"""
        base_id = name.lower().replace(' ', '_').replace('-', '_')
        base_id = ''.join(c for c in base_id if c.isalnum() or c == '_')
        if index is not None:
            return f"{base_id}_{index:02d}"
        return base_id

    def create_variation(self, base_params: Dict[str, float],
                        param_ranges: Dict[str, Dict],
                        variation_amount: float = 0.1) -> Dict[str, float]:
        """Create a variation of base parameters

        Args:
            base_params: Base parameter values
            param_ranges: Parameter ranges and constraints
            variation_amount: Max variation percentage (0.1 = 10%)
        """
        varied = {}
        for param, value in base_params.items():
            if param not in param_ranges:
                varied[param] = value
                continue

            param_info = param_ranges[param]
            min_val = param_info['min']
            max_val = param_info['max']
            param_range = max_val - min_val

            # Calculate variation range (constrained)
            max_variation = param_range * variation_amount
            variation = random.uniform(-max_variation, max_variation)

            # Apply variation and clamp to valid range
            new_value = value + variation
            new_value = max(min_val, min(max_val, new_value))

            # Round to 3 decimal places
            varied[param] = round(new_value, 3)

        return varied

    def generate_polysynth_presets(self, variations_per_preset: int = 5):
        """Generate ZenithPolySynth presets"""
        print("\n=== Generating ZenithPolySynth Presets ===")

        synth_data = self.archetypes['zenith_poly_synth']
        param_ranges = synth_data['parameters']
        archetypes = synth_data['archetypes']

        all_presets = []
        category_counts = {}

        for category, presets in archetypes.items():
            print(f"\n  Category: {category}")
            category_presets = []

            for archetype in presets:
                # Add base archetype
                base_id = self.sanitize_id(archetype['name'])
                base_preset = {
                    "id": base_id,
                    "name": archetype['name'],
                    "category": category,
                    "tags": archetype['tags'],
                    "description": archetype['description'],
                    "parameters": archetype['params']
                }
                category_presets.append(base_preset)

                # Generate variations
                for i in range(1, variations_per_preset + 1):
                    var_id = self.sanitize_id(archetype['name'], i)
                    var_params = self.create_variation(
                        archetype['params'],
                        param_ranges,
                        variation_amount=0.08  # 8% variation
                    )

                    var_preset = {
                        "id": var_id,
                        "name": f"{archetype['name']} {i:02d}",
                        "category": category,
                        "tags": archetype['tags'],
                        "description": f"Variation of {archetype['name']}",
                        "parameters": var_params
                    }
                    category_presets.append(var_preset)

            all_presets.extend(category_presets)
            category_counts[category] = len(category_presets)
            print(f"    Generated {len(category_presets)} presets")

        self.generated_presets['zenith_poly_synth'] = all_presets
        self.preset_index['zenith_poly_synth'] = category_counts

        total = sum(category_counts.values())
        print(f"\n  Total ZenithPolySynth presets: {total}")
        return all_presets

    def generate_sampler_presets(self, variations_per_preset: int = 8):
        """Generate ZenithSampler presets"""
        print("\n=== Generating ZenithSampler Presets ===")

        sampler_data = self.archetypes['zenith_sampler']
        param_ranges = sampler_data['parameters']
        archetypes = sampler_data['archetypes']

        all_presets = []
        category_counts = {}

        for category, presets in archetypes.items():
            print(f"\n  Category: {category}")
            category_presets = []

            for archetype in presets:
                # Add base archetype
                base_id = self.sanitize_id(archetype['name'])
                base_preset = {
                    "id": base_id,
                    "name": archetype['name'],
                    "category": category,
                    "tags": archetype['tags'],
                    "description": archetype['description'],
                    "parameters": archetype['params']
                }
                category_presets.append(base_preset)

                # Generate variations
                for i in range(1, variations_per_preset + 1):
                    var_id = self.sanitize_id(archetype['name'], i)
                    var_params = self.create_variation(
                        archetype['params'],
                        param_ranges,
                        variation_amount=0.1  # 10% variation
                    )

                    var_preset = {
                        "id": var_id,
                        "name": f"{archetype['name']} {i:02d}",
                        "category": category,
                        "tags": archetype['tags'],
                        "description": f"Variation of {archetype['name']}",
                        "parameters": var_params
                    }
                    category_presets.append(var_preset)

            all_presets.extend(category_presets)
            category_counts[category] = len(category_presets)
            print(f"    Generated {len(category_presets)} presets")

        self.generated_presets['zenith_sampler'] = all_presets
        self.preset_index['zenith_sampler'] = category_counts

        total = sum(category_counts.values())
        print(f"\n  Total ZenithSampler presets: {total}")
        return all_presets

    def save_polysynth_presets(self):
        """Save ZenithPolySynth presets to JSON banks"""
        print("\n=== Saving ZenithPolySynth Presets ===")

        output_dir = self.output_dir / "ZenithPolySynth"
        output_dir.mkdir(parents=True, exist_ok=True)

        # Group by category
        by_category = {}
        for preset in self.generated_presets['zenith_poly_synth']:
            category = preset['category']
            if category not in by_category:
                by_category[category] = []
            by_category[category].append(preset)

        # Save each category to a separate bank file
        for category, presets in by_category.items():
            bank_file = output_dir / f"{category.lower()}_bank.json"

            bank_data = {
                "instrument": "zenith_poly_synth",
                "bank_name": f"{category} Bank",
                "category": category,
                "version": "1.0",
                "preset_count": len(presets),
                "presets": presets
            }

            with open(bank_file, 'w') as f:
                json.dump(bank_data, f, indent=2)

            print(f"  ✓ Saved {len(presets)} presets to {bank_file.name}")

        # Save master bank with all presets
        master_file = output_dir / "master_bank.json"
        master_data = {
            "instrument": "zenith_poly_synth",
            "bank_name": "Master Bank",
            "version": "1.0",
            "preset_count": len(self.generated_presets['zenith_poly_synth']),
            "presets": self.generated_presets['zenith_poly_synth']
        }

        with open(master_file, 'w') as f:
            json.dump(master_data, f, indent=2)

        print(f"  ✓ Saved master bank with {master_data['preset_count']} presets")

    def save_sampler_presets(self):
        """Save ZenithSampler presets to JSON banks"""
        print("\n=== Saving ZenithSampler Presets ===")

        output_dir = self.output_dir / "ZenithSampler"
        output_dir.mkdir(parents=True, exist_ok=True)

        # Group by category
        by_category = {}
        for preset in self.generated_presets['zenith_sampler']:
            category = preset['category']
            if category not in by_category:
                by_category[category] = []
            by_category[category].append(preset)

        # Save each category to a separate bank file
        for category, presets in by_category.items():
            bank_file = output_dir / f"{category.lower()}_bank.json"

            bank_data = {
                "instrument": "zenith_sampler",
                "bank_name": f"{category} Bank",
                "category": category,
                "version": "1.0",
                "preset_count": len(presets),
                "presets": presets
            }

            with open(bank_file, 'w') as f:
                json.dump(bank_data, f, indent=2)

            print(f"  ✓ Saved {len(presets)} presets to {bank_file.name}")

        # Save master bank with all presets
        master_file = output_dir / "master_bank.json"
        master_data = {
            "instrument": "zenith_sampler",
            "bank_name": "Master Bank",
            "version": "1.0",
            "preset_count": len(self.generated_presets['zenith_sampler']),
            "presets": self.generated_presets['zenith_sampler']
        }

        with open(master_file, 'w') as f:
            json.dump(master_data, f, indent=2)

        print(f"  ✓ Saved master bank with {master_data['preset_count']} presets")

    def generate_preset_index(self):
        """Generate master preset index"""
        print("\n=== Generating Preset Index ===")

        index = {
            "version": "1.0",
            "generated": "2025-11-18",
            "total_presets": (
                len(self.generated_presets['zenith_poly_synth']) +
                len(self.generated_presets['zenith_sampler'])
            ),
            "instruments": {
                "zenith_poly_synth": {
                    "name": "Zenith Poly Synth",
                    "total": len(self.generated_presets['zenith_poly_synth']),
                    "categories": self.preset_index['zenith_poly_synth']
                },
                "zenith_sampler": {
                    "name": "Zenith Sampler",
                    "total": len(self.generated_presets['zenith_sampler']),
                    "categories": self.preset_index['zenith_sampler']
                }
            }
        }

        index_file = self.output_dir / "preset_index.json"
        with open(index_file, 'w') as f:
            json.dump(index, f, indent=2)

        print(f"  ✓ Generated preset index: {index_file}")

        # Print summary
        print("\n" + "="*60)
        print("PRESET LIBRARY SUMMARY")
        print("="*60)

        for inst_id, inst_data in index['instruments'].items():
            print(f"\n{inst_data['name']}: {inst_data['total']} presets")
            for category, count in inst_data['categories'].items():
                print(f"  - {category}: {count}")

        print(f"\n{'='*60}")
        print(f"TOTAL PRESETS: {index['total_presets']}")
        print(f"{'='*60}\n")

        return index

    def run(self, polysynth_variations: int = 5, sampler_variations: int = 8):
        """Run the preset generator"""
        print("="*60)
        print("ZENITH PRESET GENERATOR")
        print("="*60)

        self.load_archetypes()
        self.generate_polysynth_presets(variations_per_preset=polysynth_variations)
        self.generate_sampler_presets(variations_per_preset=sampler_variations)

        self.save_polysynth_presets()
        self.save_sampler_presets()

        index = self.generate_preset_index()

        return index


def main():
    """Main entry point"""
    script_dir = Path(__file__).parent
    archetypes_file = script_dir / "archetypes.json"
    output_dir = script_dir.parent.parent / "Content" / "Presets"

    if not archetypes_file.exists():
        print(f"ERROR: Archetypes file not found: {archetypes_file}")
        sys.exit(1)

    generator = PresetGenerator(str(archetypes_file), str(output_dir))

    # Generate with custom variation counts to hit 500+ target
    # Polysynth: 69 archetypes * 6 (1 base + 5 variations) = 414
    # Sampler: 20 archetypes * 9 (1 base + 8 variations) = 180
    # Total: 594 presets
    index = generator.run(polysynth_variations=5, sampler_variations=8)

    if index['total_presets'] >= 500:
        print("✓ SUCCESS: Generated 500+ presets!")
    else:
        print(f"⚠ WARNING: Only generated {index['total_presets']} presets (target: 500+)")

    return 0


if __name__ == "__main__":
    sys.exit(main())
