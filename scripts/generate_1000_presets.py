#!/usr/bin/env python3
"""
Generate 1000 categorized presets for Zenith instruments.

Expands existing preset banks for:
  - ZenithPolySynth (7 categories, scaled to 1000 total)
  - ZenithSampler (4 categories, scaled to 1000 total)

Updates:
  - Category bank JSON files
  - Master bank JSON files
  - Content/Presets/preset_index.json
  - Content/Presets/README.md
"""

from __future__ import annotations

import copy
import json
import re
from dataclasses import dataclass
from datetime import date
from pathlib import Path
from typing import Dict, List, Tuple
import random


REPO_ROOT = Path(__file__).resolve().parents[1]
PRESETS_ROOT = REPO_ROOT / "Content" / "Presets"


@dataclass(frozen=True)
class BankSpec:
    instrument: str
    category: str
    filename: str
    target_count: int


POLYSYNTH_TARGETS = {
    "Bass": 217,
    "Lead": 174,
    "Pluck": 145,
    "Pad": 145,
    "Arp": 116,
    "FX": 116,
    "Keys": 87,
}

SAMPLER_TARGETS = {
    "808": 250,
    "Drums": 250,
    "Keys": 250,
    "FX": 250,
}


POLYSYNTH_BANKS = [
    BankSpec("zenith_poly_synth", "Bass", "bass_bank.json", POLYSYNTH_TARGETS["Bass"]),
    BankSpec("zenith_poly_synth", "Lead", "lead_bank.json", POLYSYNTH_TARGETS["Lead"]),
    BankSpec("zenith_poly_synth", "Pluck", "pluck_bank.json", POLYSYNTH_TARGETS["Pluck"]),
    BankSpec("zenith_poly_synth", "Pad", "pad_bank.json", POLYSYNTH_TARGETS["Pad"]),
    BankSpec("zenith_poly_synth", "Arp", "arp_bank.json", POLYSYNTH_TARGETS["Arp"]),
    BankSpec("zenith_poly_synth", "FX", "fx_bank.json", POLYSYNTH_TARGETS["FX"]),
    BankSpec("zenith_poly_synth", "Keys", "keys_bank.json", POLYSYNTH_TARGETS["Keys"]),
]

SAMPLER_BANKS = [
    BankSpec("zenith_sampler", "808", "808_bank.json", SAMPLER_TARGETS["808"]),
    BankSpec("zenith_sampler", "Drums", "drums_bank.json", SAMPLER_TARGETS["Drums"]),
    BankSpec("zenith_sampler", "Keys", "keys_bank.json", SAMPLER_TARGETS["Keys"]),
    BankSpec("zenith_sampler", "FX", "fx_bank.json", SAMPLER_TARGETS["FX"]),
]


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def save_json(path: Path, data: dict) -> None:
    with path.open("w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
        f.write("\n")


def slugify(value: str) -> str:
    value = value.strip().lower()
    value = re.sub(r"[^a-z0-9]+", "_", value)
    return value.strip("_")


def collect_param_bounds(presets: List[dict]) -> Dict[str, Tuple[float, float]]:
    bounds: Dict[str, List[float]] = {}
    for preset in presets:
        for key, val in preset.get("parameters", {}).items():
            if isinstance(val, (int, float)):
                bounds.setdefault(key, []).append(float(val))

    result: Dict[str, Tuple[float, float]] = {}
    for key, values in bounds.items():
        min_v = min(values)
        max_v = max(values)
        if min_v == max_v:
            pad = 0.05 if abs(min_v) < 1.0 else abs(min_v) * 0.05
            min_v -= pad
            max_v += pad
        result[key] = (min_v, max_v)
    return result


def clamp(val: float, min_v: float, max_v: float) -> float:
    if val < min_v:
        return min_v
    if val > max_v:
        return max_v
    return val


def vary_parameters(
    params: Dict[str, float],
    bounds: Dict[str, Tuple[float, float]],
    rng: random.Random,
    fixed_keys: set,
) -> Dict[str, float]:
    varied = {}
    for key, val in params.items():
        if key in fixed_keys:
            varied[key] = val
            continue

        min_v, max_v = bounds.get(key, (val - 0.1, val + 0.1))
        span = max_v - min_v
        jitter = span * 0.12
        if jitter == 0.0:
            jitter = 0.05 if abs(val) < 1.0 else abs(val) * 0.05

        new_val = val + rng.uniform(-jitter, jitter)

        # Parameter-specific hard clamps for safety
        if key in {"sustain", "filter_cutoff", "filter_resonance", "character"}:
            new_val = clamp(new_val, 0.0, 1.0)
        elif key in {"attack", "decay", "release"}:
            lower = 0.0
            upper = max(1.5, max_v * 1.1)
            new_val = clamp(new_val, lower, upper)
        elif key in {"gain", "global_gain"}:
            new_val = clamp(new_val, 0.0, 1.2)
        else:
            new_val = clamp(new_val, min_v, max_v)

        varied[key] = round(new_val, 3)
    return varied


def expand_bank(
    bank_data: dict,
    target_count: int,
    seed: int,
    fixed_keys: set,
) -> dict:
    presets = list(bank_data.get("presets", []))
    if len(presets) >= target_count:
        bank_data["preset_count"] = len(presets)
        bank_data["presets"] = presets
        return bank_data

    bounds = collect_param_bounds(presets)
    rng = random.Random(seed)
    existing_ids = {p["id"] for p in presets}
    existing_names = {p["name"] for p in presets}

    base_count = len(presets)
    counter = 1
    while len(presets) < target_count:
        base = rng.choice(presets[:base_count])
        base_name = base["name"]
        base_id = base["id"]

        new_id = f"{base_id}_x{counter:03d}"
        while new_id in existing_ids:
            counter += 1
            new_id = f"{base_id}_x{counter:03d}"

        new_name = f"{base_name} X{counter:03d}"
        while new_name in existing_names:
            counter += 1
            new_name = f"{base_name} X{counter:03d}"

        new_preset = copy.deepcopy(base)
        new_preset["id"] = new_id
        new_preset["name"] = new_name
        new_preset["description"] = f"Generated variation of {base_name}"
        new_preset["parameters"] = vary_parameters(
            base["parameters"], bounds, rng, fixed_keys
        )

        tags = list(dict.fromkeys(base.get("tags", []) + ["generated", "expanded"]))
        new_preset["tags"] = tags

        presets.append(new_preset)
        existing_ids.add(new_id)
        existing_names.add(new_name)
        counter += 1

    bank_data["preset_count"] = len(presets)
    bank_data["presets"] = presets
    return bank_data


def rebuild_master_bank(
    instrument_dir: Path,
    instrument_id: str,
    bank_specs: List[BankSpec],
) -> dict:
    all_presets = []
    for spec in bank_specs:
        bank = load_json(instrument_dir / spec.filename)
        all_presets.extend(bank["presets"])

    master = {
        "instrument": instrument_id,
        "bank_name": "Master Bank",
        "version": "1.0",
        "preset_count": len(all_presets),
        "presets": all_presets,
    }
    return master


def update_preset_index(
    path: Path,
    poly_counts: Dict[str, int],
    sampler_counts: Dict[str, int],
) -> None:
    data = load_json(path)
    data["generated"] = date.today().isoformat()

    poly_total = sum(poly_counts.values())
    sampler_total = sum(sampler_counts.values())
    data["total_presets"] = poly_total + sampler_total

    data["instruments"]["zenith_poly_synth"]["total"] = poly_total
    data["instruments"]["zenith_poly_synth"]["categories"] = poly_counts
    data["instruments"]["zenith_sampler"]["total"] = sampler_total
    data["instruments"]["zenith_sampler"]["categories"] = sampler_counts

    save_json(path, data)


def update_presets_readme(
    path: Path,
    poly_counts: Dict[str, int],
    sampler_counts: Dict[str, int],
) -> None:
    text = path.read_text(encoding="utf-8")

    poly_total = sum(poly_counts.values())
    sampler_total = sum(sampler_counts.values())
    total = poly_total + sampler_total

    replacements = {
        r"- \*\*Total Presets:\*\* \d+": f"- **Total Presets:** {total}",
        r"- \*\*ZenithPolySynth:\*\* \d+ presets": f"- **ZenithPolySynth:** {poly_total} presets",
        r"- \*\*ZenithSampler:\*\* \d+ presets": f"- **ZenithSampler:** {sampler_total} presets",
        r"## ZenithPolySynth Presets \(\d+ Total\)": f"## ZenithPolySynth Presets ({poly_total} Total)",
        r"## ZenithSampler Presets \(\d+ Total\)": f"## ZenithSampler Presets ({sampler_total} Total)",
    }

    for pattern, repl in replacements.items():
        text = re.sub(pattern, repl, text)

    for category, count in poly_counts.items():
        pattern = rf"\| \*\*{re.escape(category)}\*\* \| \d+ \|"
        text = re.sub(pattern, f"| **{category}** | {count} |", text)

    for category, count in sampler_counts.items():
        pattern = rf"\| \*\*{re.escape(category)}\*\* \| \d+ \|"
        text = re.sub(pattern, f"| **{category}** | {count} |", text)

    path.write_text(text, encoding="utf-8")


def main() -> int:
    poly_dir = PRESETS_ROOT / "ZenithPolySynth"
    sampler_dir = PRESETS_ROOT / "ZenithSampler"

    # Expand PolySynth banks
    for spec in POLYSYNTH_BANKS:
        bank_path = poly_dir / spec.filename
        bank_data = load_json(bank_path)
        expanded = expand_bank(
            bank_data,
            spec.target_count,
            seed=1337 + spec.target_count,
            fixed_keys={"osc_type"},
        )
        save_json(bank_path, expanded)

    # Expand Sampler banks
    for spec in SAMPLER_BANKS:
        bank_path = sampler_dir / spec.filename
        bank_data = load_json(bank_path)
        expanded = expand_bank(
            bank_data,
            spec.target_count,
            seed=2024 + spec.target_count,
            fixed_keys=set(),
        )
        save_json(bank_path, expanded)

    # Rebuild master banks
    poly_master = rebuild_master_bank(poly_dir, "zenith_poly_synth", POLYSYNTH_BANKS)
    save_json(poly_dir / "master_bank.json", poly_master)

    sampler_master = rebuild_master_bank(sampler_dir, "zenith_sampler", SAMPLER_BANKS)
    save_json(sampler_dir / "master_bank.json", sampler_master)

    # Update index + README
    update_preset_index(
        PRESETS_ROOT / "preset_index.json",
        POLYSYNTH_TARGETS,
        SAMPLER_TARGETS,
    )
    update_presets_readme(
        PRESETS_ROOT / "README.md",
        POLYSYNTH_TARGETS,
        SAMPLER_TARGETS,
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
