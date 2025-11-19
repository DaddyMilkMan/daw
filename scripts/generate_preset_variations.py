#!/usr/bin/env python3
"""
Preset Variation Generator for ZenithPolySynth

This script reads JSON preset files and generates variations by:
- Tweaking parameter values slightly (±5-15%)
- Creating multiple variations per preset
- Maintaining musical coherence
- Outputting to both JSON and XML (.zpreset format)
"""

import json
import os
import sys
import random
from pathlib import Path
from typing import Dict, List, Any
import xml.etree.ElementTree as ET
from xml.dom import minidom


class PresetVariationGenerator:
    """Generates variations of synth presets"""

    # Parameter variation ranges (how much to vary each param)
    VARIATION_RANGES = {
        'osc_type': 0.0,  # Don't vary oscillator type
        'filter_cutoff': 0.12,
        'filter_resonance': 0.15,
        'attack': 0.1,
        'decay': 0.12,
        'sustain': 0.1,
        'release': 0.12
    }

    def __init__(self, seed: int = None):
        """Initialize generator with optional random seed"""
        if seed is not None:
            random.seed(seed)

    def clamp(self, value: float, min_val: float = 0.0, max_val: float = 1.0) -> float:
        """Clamp value to range"""
        return max(min_val, min(max_val, value))

    def vary_parameter(self, param_name: str, value: float, variation_strength: float = 1.0) -> float:
        """
        Vary a parameter value slightly

        Args:
            param_name: Name of parameter
            value: Original value
            variation_strength: Multiplier for variation amount (0.0-2.0)

        Returns:
            Varied parameter value
        """
        if param_name not in self.VARIATION_RANGES:
            return value

        variation_range = self.VARIATION_RANGES[param_name] * variation_strength

        if variation_range == 0.0:
            return value

        # Random variation within range
        delta = random.uniform(-variation_range, variation_range)
        new_value = value + delta

        # Ensure we stay in valid range
        new_value = self.clamp(new_value)

        # Special handling for attack: never go below 0.002 to avoid clicks
        if param_name == 'attack':
            new_value = max(0.002, new_value)

        return new_value

    def generate_variation(
        self,
        preset: Dict[str, Any],
        variation_num: int,
        variation_strength: float = 1.0
    ) -> Dict[str, Any]:
        """
        Generate a variation of a preset

        Args:
            preset: Original preset dict
            variation_num: Variation number (for naming)
            variation_strength: How much to vary (0.0-2.0)

        Returns:
            New preset dict with varied parameters
        """
        variation = preset.copy()
        variation['parameters'] = {}

        # Vary each parameter
        for param_name, param_value in preset['parameters'].items():
            variation['parameters'][param_name] = self.vary_parameter(
                param_name,
                param_value,
                variation_strength
            )

        # Update name and tags
        variation['name'] = f"{preset['name']} {variation_num:02d}"
        variation['tags'] = preset.get('tags', []).copy()
        if 'variation' not in variation['tags']:
            variation['tags'].append('variation')

        return variation

    def load_preset_bank(self, json_file: Path) -> Dict[str, Any]:
        """Load a JSON preset bank file"""
        with open(json_file, 'r') as f:
            return json.load(f)

    def save_preset_bank(self, data: Dict[str, Any], json_file: Path):
        """Save a JSON preset bank file"""
        with open(json_file, 'w') as f:
            json.dump(data, f, indent=2)

    def preset_to_xml(self, preset: Dict[str, Any]) -> str:
        """
        Convert a preset dict to JUCE ValueTree XML format (.zpreset)

        Args:
            preset: Preset dictionary

        Returns:
            XML string in JUCE ValueTree format
        """
        # Create root element
        root = ET.Element('InstrumentPreset')

        # Add basic properties
        root.set('id', preset.get('id', preset['name'].lower().replace(' ', '_')))
        root.set('name', preset['name'])
        root.set('instrumentId', 'zenith_poly_synth')
        root.set('author', preset.get('author', 'Factory'))
        root.set('description', preset.get('description', ''))
        root.set('version', preset.get('version', '1.0.0'))

        # Add tags
        if 'tags' in preset and preset['tags']:
            root.set('tags', ','.join(preset['tags']))

        # Add parameters
        params_elem = ET.SubElement(root, 'Parameters')
        for param_id, value in preset['parameters'].items():
            param_elem = ET.SubElement(params_elem, 'Param')
            param_elem.set('id', param_id)
            param_elem.set('value', str(value))

        # Add macros (if any)
        if 'macros' in preset and preset['macros']:
            macros_elem = ET.SubElement(root, 'Macros')
            for macro_id, value in preset['macros'].items():
                macro_elem = ET.SubElement(macros_elem, 'Macro')
                macro_elem.set('id', macro_id)
                macro_elem.set('value', str(value))

        # Pretty print
        xml_str = ET.tostring(root, encoding='unicode')
        dom = minidom.parseString(xml_str)
        return dom.toprettyxml(indent='  ')

    def save_preset_xml(self, preset: Dict[str, Any], output_file: Path):
        """Save a preset as XML (.zpreset file)"""
        xml_content = self.preset_to_xml(preset)
        with open(output_file, 'w') as f:
            f.write(xml_content)

    def generate_variations_for_bank(
        self,
        bank_file: Path,
        num_variations_per_preset: int = 1,
        variation_strength: float = 1.0,
        output_dir: Path = None
    ) -> List[Dict[str, Any]]:
        """
        Generate variations for all presets in a bank

        Args:
            bank_file: Path to JSON preset bank file
            num_variations_per_preset: How many variations to generate per preset
            variation_strength: Variation strength (0.0-2.0)
            output_dir: Optional output directory for individual .zpreset files

        Returns:
            List of all presets (originals + variations)
        """
        bank_data = self.load_preset_bank(bank_file)
        all_presets = []

        for preset in bank_data['presets']:
            # Add original
            all_presets.append(preset)

            # Generate variations
            for i in range(1, num_variations_per_preset + 1):
                variation = self.generate_variation(preset, i, variation_strength)
                all_presets.append(variation)

                # Optionally save individual .zpreset file
                if output_dir:
                    output_dir.mkdir(parents=True, exist_ok=True)
                    preset_filename = f"{variation['name'].replace(' ', '_')}.zpreset"
                    self.save_preset_xml(variation, output_dir / preset_filename)

        return all_presets


def main():
    """Main entry point"""
    # Setup paths
    script_dir = Path(__file__).parent
    repo_root = script_dir.parent
    presets_dir = repo_root / 'zenith-core' / 'Content' / 'Presets' / 'PolySynth'

    # Create generator
    generator = PresetVariationGenerator(seed=42)

    # Process all JSON preset banks
    json_files = list(presets_dir.glob('*.json'))

    if not json_files:
        print(f"No JSON preset files found in {presets_dir}")
        return 1

    print(f"Found {len(json_files)} preset bank files")

    total_presets = 0

    for json_file in sorted(json_files):
        print(f"\nProcessing {json_file.name}...")

        # Load original bank
        bank_data = generator.load_preset_bank(json_file)
        original_count = len(bank_data['presets'])

        print(f"  Original presets: {original_count}")

        # Determine variation count based on category
        # For pads/drones: fewer variations (they're already numerous)
        # For keys: more variations (to reach our target)
        if 'Pads' in json_file.name or 'Drones' in json_file.name:
            variations_per_preset = 0  # No variations for pads/drones initially
        elif 'Keys' in json_file.name:
            variations_per_preset = 1  # 1 variation per key preset
        else:
            variations_per_preset = 1

        # Generate variations
        if variations_per_preset > 0:
            all_presets = generator.generate_variations_for_bank(
                json_file,
                num_variations_per_preset=variations_per_preset,
                variation_strength=0.8
            )

            # Save expanded bank
            output_file = json_file.parent / f"{json_file.stem}_Expanded.json"
            generator.save_preset_bank({'presets': all_presets}, output_file)

            print(f"  Generated {len(all_presets)} total presets (with variations)")
            print(f"  Saved to {output_file.name}")
            total_presets += len(all_presets)
        else:
            print(f"  Keeping original {original_count} presets (no variations)")
            total_presets += original_count

    print(f"\n{'='*60}")
    print(f"Total presets generated: {total_presets}")
    print(f"{'='*60}")

    # If we haven't hit 150+, let's add more variations
    if total_presets < 150:
        print(f"\nGenerating additional variations to reach 150+ presets...")

        # Add variations to pads
        pad_files = [f for f in json_files if 'Pads' in f.name]

        for json_file in pad_files[:2]:  # Just vary the first two pad banks
            print(f"\nAdding variations to {json_file.name}...")
            all_presets = generator.generate_variations_for_bank(
                json_file,
                num_variations_per_preset=1,
                variation_strength=0.7
            )

            output_file = json_file.parent / f"{json_file.stem}_Expanded.json"
            generator.save_preset_bank({'presets': all_presets}, output_file)

            print(f"  Generated {len(all_presets)} total presets")
            total_presets += len(all_presets) - len(generator.load_preset_bank(json_file)['presets'])

        print(f"\nNew total: {total_presets} presets")

    return 0


if __name__ == '__main__':
    sys.exit(main())
