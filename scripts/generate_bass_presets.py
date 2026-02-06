#!/usr/bin/env python3
"""
Bass Preset Generator for ZenithPolySynth
Generates comprehensive bass preset banks in JSON format
"""

import json
import os
import random
from typing import Dict, List

# ZenithPolySynth Parameters:
# - osc_type: 0.0 (Sine), 0.5 (Saw), 1.0 (Square)
# - filter_cutoff: 0.0-1.0
# - filter_resonance: 0.0-1.0
# - attack: 0.0-1.0 (seconds)
# - decay: 0.0-1.0 (seconds)
# - sustain: 0.0-1.0 (percentage)
# - release: 0.0-1.0 (seconds)

def create_preset(name: str, params: Dict[str, float], tags: List[str], description: str = "") -> Dict:
    """Create a preset dictionary"""
    return {
        "name": name,
        "category": "Bass",
        "description": description,
        "tags": tags,
        "parameters": params
    }

def generate_sub_808_presets() -> List[Dict]:
    """Generate sub/808 bass presets"""
    presets = []

    # Classic 808 variations
    presets.append(create_preset(
        "808 Sub Classic",
        {"osc_type": 0.0, "filter_cutoff": 0.25, "filter_resonance": 0.1,
         "attack": 0.001, "decay": 0.4, "sustain": 0.0, "release": 0.05},
        ["808", "sub", "trap", "classic"],
        "Classic 808 kick-bass with medium decay"
    ))

    presets.append(create_preset(
        "808 Long Tail",
        {"osc_type": 0.0, "filter_cutoff": 0.2, "filter_resonance": 0.15,
         "attack": 0.001, "decay": 0.8, "sustain": 0.0, "release": 0.1},
        ["808", "sub", "trap", "long"],
        "Long decay 808 for sustained bass hits"
    ))

    presets.append(create_preset(
        "808 Short Punch",
        {"osc_type": 0.0, "filter_cutoff": 0.3, "filter_resonance": 0.05,
         "attack": 0.001, "decay": 0.15, "sustain": 0.0, "release": 0.02},
        ["808", "sub", "trap", "short", "punchy"],
        "Tight punchy 808 for fast patterns"
    ))

    presets.append(create_preset(
        "808 Distorted",
        {"osc_type": 0.5, "filter_cutoff": 0.35, "filter_resonance": 0.6,
         "attack": 0.001, "decay": 0.45, "sustain": 0.1, "release": 0.08},
        ["808", "sub", "distorted", "aggressive"],
        "Slightly distorted 808 with resonance"
    ))

    presets.append(create_preset(
        "Sub Sine Clean",
        {"osc_type": 0.0, "filter_cutoff": 0.22, "filter_resonance": 0.0,
         "attack": 0.001, "decay": 0.35, "sustain": 0.0, "release": 0.03},
        ["sub", "clean", "sine"],
        "Clean sine sub bass"
    ))

    presets.append(create_preset(
        "Sub Deep",
        {"osc_type": 0.0, "filter_cutoff": 0.18, "filter_resonance": 0.05,
         "attack": 0.002, "decay": 0.5, "sustain": 0.05, "release": 0.1},
        ["sub", "deep", "dark"],
        "Deep rumbling sub bass"
    ))

    presets.append(create_preset(
        "808 Boom",
        {"osc_type": 0.0, "filter_cutoff": 0.28, "filter_resonance": 0.25,
         "attack": 0.001, "decay": 0.6, "sustain": 0.0, "release": 0.15},
        ["808", "boom", "impact"],
        "Boomy 808 with resonance peak"
    ))

    presets.append(create_preset(
        "Trap Sub",
        {"osc_type": 0.0, "filter_cutoff": 0.24, "filter_resonance": 0.12,
         "attack": 0.001, "decay": 0.38, "sustain": 0.0, "release": 0.06},
        ["trap", "sub", "808"],
        "Modern trap sub bass"
    ))

    presets.append(create_preset(
        "808 Sustained",
        {"osc_type": 0.0, "filter_cutoff": 0.26, "filter_resonance": 0.08,
         "attack": 0.01, "decay": 0.2, "sustain": 0.7, "release": 0.25},
        ["808", "sustained", "melodic"],
        "Sustained 808 for melodic basslines"
    ))

    presets.append(create_preset(
        "Sub Tight",
        {"osc_type": 0.0, "filter_cutoff": 0.3, "filter_resonance": 0.0,
         "attack": 0.001, "decay": 0.12, "sustain": 0.0, "release": 0.01},
        ["sub", "tight", "fast"],
        "Ultra tight sub for fast rolls"
    ))

    # Additional variations
    presets.append(create_preset(
        "808 Soft",
        {"osc_type": 0.0, "filter_cutoff": 0.2, "filter_resonance": 0.0,
         "attack": 0.005, "decay": 0.5, "sustain": 0.2, "release": 0.2},
        ["808", "soft", "gentle"],
        "Gentle 808 with soft attack"
    ))

    presets.append(create_preset(
        "Sub Rumble",
        {"osc_type": 0.0, "filter_cutoff": 0.15, "filter_resonance": 0.3,
         "attack": 0.01, "decay": 0.7, "sustain": 0.15, "release": 0.35},
        ["sub", "rumble", "earthquake"],
        "Deep rumbling sub with resonance"
    ))

    presets.append(create_preset(
        "808 Modern",
        {"osc_type": 0.0, "filter_cutoff": 0.32, "filter_resonance": 0.18,
         "attack": 0.001, "decay": 0.42, "sustain": 0.05, "release": 0.07},
        ["808", "modern", "trap"],
        "Modern 808 sound"
    ))

    presets.append(create_preset(
        "808 Vintage",
        {"osc_type": 0.0, "filter_cutoff": 0.22, "filter_resonance": 0.08,
         "attack": 0.002, "decay": 0.48, "sustain": 0.0, "release": 0.08},
        ["808", "vintage", "classic"],
        "Vintage 808 character"
    ))

    presets.append(create_preset(
        "Sub Punch",
        {"osc_type": 0.0, "filter_cutoff": 0.35, "filter_resonance": 0.2,
         "attack": 0.001, "decay": 0.25, "sustain": 0.0, "release": 0.03},
        ["sub", "punch", "impact"],
        "Punchy sub with attack"
    ))

    presets.append(create_preset(
        "808 808",
        {"osc_type": 0.0, "filter_cutoff": 0.27, "filter_resonance": 0.14,
         "attack": 0.001, "decay": 0.55, "sustain": 0.0, "release": 0.1},
        ["808", "classic", "standard"],
        "Standard 808 reference"
    ))

    presets.append(create_preset(
        "Sub Warm",
        {"osc_type": 0.0, "filter_cutoff": 0.28, "filter_resonance": 0.06,
         "attack": 0.003, "decay": 0.45, "sustain": 0.1, "release": 0.12},
        ["sub", "warm", "smooth"],
        "Warm smooth sub bass"
    ))

    presets.append(create_preset(
        "808 Crisp",
        {"osc_type": 0.0, "filter_cutoff": 0.38, "filter_resonance": 0.22,
         "attack": 0.001, "decay": 0.35, "sustain": 0.0, "release": 0.05},
        ["808", "crisp", "bright"],
        "Crisp 808 with higher cutoff"
    ))

    presets.append(create_preset(
        "Sub Dense",
        {"osc_type": 0.0, "filter_cutoff": 0.19, "filter_resonance": 0.28,
         "attack": 0.002, "decay": 0.65, "sustain": 0.08, "release": 0.18},
        ["sub", "dense", "thick"],
        "Dense thick sub bass"
    ))

    presets.append(create_preset(
        "808 Hybrid",
        {"osc_type": 0.25, "filter_cutoff": 0.3, "filter_resonance": 0.16,
         "attack": 0.001, "decay": 0.4, "sustain": 0.0, "release": 0.06},
        ["808", "hybrid", "modern"],
        "Hybrid sine-saw 808"
    ))

    return presets

def generate_reese_growl_presets() -> List[Dict]:
    """Generate Reese/growl bass presets"""
    presets = []

    presets.append(create_preset(
        "Reese Classic",
        {"osc_type": 0.5, "filter_cutoff": 0.45, "filter_resonance": 0.7,
         "attack": 0.01, "decay": 0.3, "sustain": 0.9, "release": 0.15},
        ["reese", "classic", "dnb"],
        "Classic detuned Reese bass"
    ))

    presets.append(create_preset(
        "Reese Dark",
        {"osc_type": 0.5, "filter_cutoff": 0.35, "filter_resonance": 0.65,
         "attack": 0.015, "decay": 0.25, "sustain": 0.85, "release": 0.2},
        ["reese", "dark", "deep"],
        "Dark Reese with low cutoff"
    ))

    presets.append(create_preset(
        "Reese Bright",
        {"osc_type": 0.5, "filter_cutoff": 0.6, "filter_resonance": 0.75,
         "attack": 0.005, "decay": 0.2, "sustain": 0.95, "release": 0.12},
        ["reese", "bright", "aggressive"],
        "Bright aggressive Reese"
    ))

    presets.append(create_preset(
        "Growl Heavy",
        {"osc_type": 0.5, "filter_cutoff": 0.42, "filter_resonance": 0.8,
         "attack": 0.008, "decay": 0.28, "sustain": 0.88, "release": 0.18},
        ["growl", "heavy", "aggressive"],
        "Heavy growling bass"
    ))

    presets.append(create_preset(
        "Growl Wobble",
        {"osc_type": 0.5, "filter_cutoff": 0.5, "filter_resonance": 0.85,
         "attack": 0.005, "decay": 0.15, "sustain": 0.92, "release": 0.1},
        ["growl", "wobble", "dubstep"],
        "Wobble-ready growl bass"
    ))

    presets.append(create_preset(
        "Reese Smooth",
        {"osc_type": 0.5, "filter_cutoff": 0.4, "filter_resonance": 0.55,
         "attack": 0.02, "decay": 0.35, "sustain": 0.8, "release": 0.25},
        ["reese", "smooth", "liquid"],
        "Smooth liquid Reese"
    ))

    presets.append(create_preset(
        "Reese Tight",
        {"osc_type": 0.5, "filter_cutoff": 0.48, "filter_resonance": 0.72,
         "attack": 0.002, "decay": 0.18, "sustain": 0.9, "release": 0.08},
        ["reese", "tight", "punchy"],
        "Tight punchy Reese"
    ))

    presets.append(create_preset(
        "Growl Nasty",
        {"osc_type": 0.5, "filter_cutoff": 0.55, "filter_resonance": 0.9,
         "attack": 0.003, "decay": 0.22, "sustain": 0.95, "release": 0.12},
        ["growl", "nasty", "brutal"],
        "Nasty brutal growl"
    ))

    presets.append(create_preset(
        "Reese Wide",
        {"osc_type": 0.5, "filter_cutoff": 0.52, "filter_resonance": 0.68,
         "attack": 0.012, "decay": 0.3, "sustain": 0.87, "release": 0.2},
        ["reese", "wide", "stereo"],
        "Wide stereo Reese"
    ))

    presets.append(create_preset(
        "Growl Gritty",
        {"osc_type": 0.5, "filter_cutoff": 0.46, "filter_resonance": 0.82,
         "attack": 0.006, "decay": 0.2, "sustain": 0.91, "release": 0.14},
        ["growl", "gritty", "dirty"],
        "Gritty dirty growl"
    ))

    presets.append(create_preset(
        "Reese Neurofunk",
        {"osc_type": 0.5, "filter_cutoff": 0.58, "filter_resonance": 0.78,
         "attack": 0.004, "decay": 0.16, "sustain": 0.93, "release": 0.09},
        ["reese", "neurofunk", "tech"],
        "Neurofunk style Reese"
    ))

    presets.append(create_preset(
        "Growl Sub",
        {"osc_type": 0.5, "filter_cutoff": 0.38, "filter_resonance": 0.7,
         "attack": 0.01, "decay": 0.32, "sustain": 0.85, "release": 0.22},
        ["growl", "sub", "deep"],
        "Deep sub growl"
    ))

    presets.append(create_preset(
        "Reese Modulated",
        {"osc_type": 0.5, "filter_cutoff": 0.5, "filter_resonance": 0.75,
         "attack": 0.007, "decay": 0.24, "sustain": 0.89, "release": 0.16},
        ["reese", "modulated", "evolving"],
        "Evolving modulated Reese"
    ))

    presets.append(create_preset(
        "Growl Mid",
        {"osc_type": 0.5, "filter_cutoff": 0.54, "filter_resonance": 0.8,
         "attack": 0.005, "decay": 0.19, "sustain": 0.9, "release": 0.11},
        ["growl", "mid", "presence"],
        "Mid-range growl with presence"
    ))

    presets.append(create_preset(
        "Reese Vintage",
        {"osc_type": 0.5, "filter_cutoff": 0.43, "filter_resonance": 0.62,
         "attack": 0.015, "decay": 0.28, "sustain": 0.82, "release": 0.2},
        ["reese", "vintage", "classic"],
        "Vintage Reese character"
    ))

    presets.append(create_preset(
        "Growl Scream",
        {"osc_type": 0.5, "filter_cutoff": 0.62, "filter_resonance": 0.88,
         "attack": 0.002, "decay": 0.14, "sustain": 0.94, "release": 0.08},
        ["growl", "scream", "intense"],
        "Screaming intense growl"
    ))

    presets.append(create_preset(
        "Reese Rolling",
        {"osc_type": 0.5, "filter_cutoff": 0.47, "filter_resonance": 0.7,
         "attack": 0.008, "decay": 0.26, "sustain": 0.86, "release": 0.18},
        ["reese", "rolling", "dnb"],
        "Rolling drum and bass Reese"
    ))

    presets.append(create_preset(
        "Growl Thick",
        {"osc_type": 0.5, "filter_cutoff": 0.44, "filter_resonance": 0.76,
         "attack": 0.01, "decay": 0.3, "sustain": 0.88, "release": 0.2},
        ["growl", "thick", "fat"],
        "Thick fat growl"
    ))

    presets.append(create_preset(
        "Reese Square",
        {"osc_type": 0.75, "filter_cutoff": 0.48, "filter_resonance": 0.68,
         "attack": 0.009, "decay": 0.27, "sustain": 0.84, "release": 0.17},
        ["reese", "square", "hollow"],
        "Square-wave Reese variation"
    ))

    presets.append(create_preset(
        "Growl Filter",
        {"osc_type": 0.5, "filter_cutoff": 0.5, "filter_resonance": 0.92,
         "attack": 0.004, "decay": 0.2, "sustain": 0.92, "release": 0.1},
        ["growl", "filter", "resonant"],
        "High resonance filter growl"
    ))

    return presets

def generate_analog_house_presets() -> List[Dict]:
    """Generate analog-style house bass presets"""
    presets = []

    presets.append(create_preset(
        "House Classic",
        {"osc_type": 0.5, "filter_cutoff": 0.5, "filter_resonance": 0.4,
         "attack": 0.001, "decay": 0.15, "sustain": 0.3, "release": 0.08},
        ["house", "classic", "analog"],
        "Classic house bass sound"
    ))

    presets.append(create_preset(
        "House Deep",
        {"osc_type": 0.5, "filter_cutoff": 0.42, "filter_resonance": 0.35,
         "attack": 0.002, "decay": 0.18, "sustain": 0.4, "release": 0.12},
        ["house", "deep", "warm"],
        "Deep warm house bass"
    ))

    presets.append(create_preset(
        "House Acid",
        {"osc_type": 0.5, "filter_cutoff": 0.55, "filter_resonance": 0.75,
         "attack": 0.001, "decay": 0.12, "sustain": 0.2, "release": 0.06},
        ["house", "acid", "resonant"],
        "Acid house bass with high resonance"
    ))

    presets.append(create_preset(
        "Techno Stab",
        {"osc_type": 0.5, "filter_cutoff": 0.6, "filter_resonance": 0.5,
         "attack": 0.001, "decay": 0.1, "sustain": 0.15, "release": 0.05},
        ["techno", "stab", "punchy"],
        "Punchy techno stab bass"
    ))

    presets.append(create_preset(
        "House Warm",
        {"osc_type": 0.5, "filter_cutoff": 0.45, "filter_resonance": 0.3,
         "attack": 0.003, "decay": 0.2, "sustain": 0.45, "release": 0.15},
        ["house", "warm", "smooth"],
        "Warm smooth house bass"
    ))

    presets.append(create_preset(
        "Techno Drive",
        {"osc_type": 0.5, "filter_cutoff": 0.58, "filter_resonance": 0.6,
         "attack": 0.001, "decay": 0.14, "sustain": 0.25, "release": 0.07},
        ["techno", "drive", "aggressive"],
        "Driving techno bass"
    ))

    presets.append(create_preset(
        "House Funk",
        {"osc_type": 0.5, "filter_cutoff": 0.52, "filter_resonance": 0.45,
         "attack": 0.002, "decay": 0.16, "sustain": 0.35, "release": 0.1},
        ["house", "funk", "groovy"],
        "Funky house bass"
    ))

    presets.append(create_preset(
        "Techno Hard",
        {"osc_type": 0.5, "filter_cutoff": 0.62, "filter_resonance": 0.7,
         "attack": 0.001, "decay": 0.11, "sustain": 0.18, "release": 0.05},
        ["techno", "hard", "industrial"],
        "Hard industrial techno bass"
    ))

    presets.append(create_preset(
        "House Pluck",
        {"osc_type": 0.5, "filter_cutoff": 0.65, "filter_resonance": 0.42,
         "attack": 0.001, "decay": 0.13, "sustain": 0.22, "release": 0.06},
        ["house", "pluck", "bright"],
        "Plucky bright house bass"
    ))

    presets.append(create_preset(
        "Techno Sub",
        {"osc_type": 0.5, "filter_cutoff": 0.38, "filter_resonance": 0.25,
         "attack": 0.001, "decay": 0.2, "sustain": 0.5, "release": 0.15},
        ["techno", "sub", "deep"],
        "Deep sub techno bass"
    ))

    presets.append(create_preset(
        "House Vintage",
        {"osc_type": 0.5, "filter_cutoff": 0.48, "filter_resonance": 0.38,
         "attack": 0.003, "decay": 0.17, "sustain": 0.38, "release": 0.12},
        ["house", "vintage", "retro"],
        "Vintage house bass sound"
    ))

    presets.append(create_preset(
        "Techno Minimal",
        {"osc_type": 0.5, "filter_cutoff": 0.5, "filter_resonance": 0.35,
         "attack": 0.001, "decay": 0.15, "sustain": 0.28, "release": 0.08},
        ["techno", "minimal", "clean"],
        "Minimal techno bass"
    ))

    presets.append(create_preset(
        "House Chicago",
        {"osc_type": 0.5, "filter_cutoff": 0.54, "filter_resonance": 0.48,
         "attack": 0.002, "decay": 0.14, "sustain": 0.32, "release": 0.09},
        ["house", "chicago", "classic"],
        "Chicago house style bass"
    ))

    presets.append(create_preset(
        "Techno Berlin",
        {"osc_type": 0.5, "filter_cutoff": 0.56, "filter_resonance": 0.55,
         "attack": 0.001, "decay": 0.13, "sustain": 0.24, "release": 0.07},
        ["techno", "berlin", "dark"],
        "Berlin techno bass"
    ))

    presets.append(create_preset(
        "House Disco",
        {"osc_type": 0.5, "filter_cutoff": 0.58, "filter_resonance": 0.4,
         "attack": 0.002, "decay": 0.18, "sustain": 0.42, "release": 0.13},
        ["house", "disco", "funky"],
        "Disco-influenced house bass"
    ))

    presets.append(create_preset(
        "Techno Peak",
        {"osc_type": 0.5, "filter_cutoff": 0.64, "filter_resonance": 0.68,
         "attack": 0.001, "decay": 0.1, "sustain": 0.16, "release": 0.04},
        ["techno", "peak", "intense"],
        "Peak-time techno bass"
    ))

    presets.append(create_preset(
        "House Detroit",
        {"osc_type": 0.5, "filter_cutoff": 0.51, "filter_resonance": 0.43,
         "attack": 0.002, "decay": 0.16, "sustain": 0.33, "release": 0.11},
        ["house", "detroit", "classic"],
        "Detroit house bass"
    ))

    presets.append(create_preset(
        "Techno Rolling",
        {"osc_type": 0.5, "filter_cutoff": 0.53, "filter_resonance": 0.52,
         "attack": 0.001, "decay": 0.14, "sustain": 0.26, "release": 0.08},
        ["techno", "rolling", "groovy"],
        "Rolling techno bassline"
    ))

    presets.append(create_preset(
        "House Garage",
        {"osc_type": 0.5, "filter_cutoff": 0.57, "filter_resonance": 0.46,
         "attack": 0.001, "decay": 0.15, "sustain": 0.3, "release": 0.09},
        ["house", "garage", "uk"],
        "UK garage style bass"
    ))

    presets.append(create_preset(
        "Techno Hypnotic",
        {"osc_type": 0.5, "filter_cutoff": 0.49, "filter_resonance": 0.5,
         "attack": 0.002, "decay": 0.17, "sustain": 0.35, "release": 0.12},
        ["techno", "hypnotic", "trance"],
        "Hypnotic techno bass"
    ))

    return presets

def generate_donk_pluck_presets() -> List[Dict]:
    """Generate donk/pluck bass presets"""
    presets = []

    presets.append(create_preset(
        "Donk Classic",
        {"osc_type": 0.5, "filter_cutoff": 0.7, "filter_resonance": 0.8,
         "attack": 0.001, "decay": 0.08, "sustain": 0.0, "release": 0.02},
        ["donk", "pluck", "short"],
        "Classic donk bass sound"
    ))

    presets.append(create_preset(
        "Pluck Sharp",
        {"osc_type": 0.5, "filter_cutoff": 0.75, "filter_resonance": 0.65,
         "attack": 0.001, "decay": 0.1, "sustain": 0.0, "release": 0.03},
        ["pluck", "sharp", "bright"],
        "Sharp bright pluck"
    ))

    presets.append(create_preset(
        "Donk Heavy",
        {"osc_type": 0.5, "filter_cutoff": 0.65, "filter_resonance": 0.85,
         "attack": 0.001, "decay": 0.12, "sustain": 0.05, "release": 0.04},
        ["donk", "heavy", "impact"],
        "Heavy impact donk"
    ))

    presets.append(create_preset(
        "Pluck Tight",
        {"osc_type": 0.5, "filter_cutoff": 0.8, "filter_resonance": 0.7,
         "attack": 0.001, "decay": 0.07, "sustain": 0.0, "release": 0.02},
        ["pluck", "tight", "fast"],
        "Tight fast pluck"
    ))

    presets.append(create_preset(
        "Donk Resonant",
        {"osc_type": 0.5, "filter_cutoff": 0.68, "filter_resonance": 0.9,
         "attack": 0.001, "decay": 0.09, "sustain": 0.0, "release": 0.025},
        ["donk", "resonant", "ringing"],
        "Resonant ringing donk"
    ))

    presets.append(create_preset(
        "Pluck Soft",
        {"osc_type": 0.5, "filter_cutoff": 0.72, "filter_resonance": 0.6,
         "attack": 0.003, "decay": 0.11, "sustain": 0.1, "release": 0.05},
        ["pluck", "soft", "gentle"],
        "Soft gentle pluck"
    ))

    presets.append(create_preset(
        "Donk Bounce",
        {"osc_type": 0.5, "filter_cutoff": 0.73, "filter_resonance": 0.82,
         "attack": 0.001, "decay": 0.1, "sustain": 0.0, "release": 0.03},
        ["donk", "bounce", "groovy"],
        "Bouncy donk bass"
    ))

    presets.append(create_preset(
        "Pluck Snap",
        {"osc_type": 0.5, "filter_cutoff": 0.82, "filter_resonance": 0.68,
         "attack": 0.001, "decay": 0.06, "sustain": 0.0, "release": 0.015},
        ["pluck", "snap", "crisp"],
        "Snappy crisp pluck"
    ))

    presets.append(create_preset(
        "Donk Bass",
        {"osc_type": 0.5, "filter_cutoff": 0.62, "filter_resonance": 0.88,
         "attack": 0.001, "decay": 0.13, "sustain": 0.08, "release": 0.04},
        ["donk", "bass", "low"],
        "Low donk bass"
    ))

    presets.append(create_preset(
        "Pluck Melody",
        {"osc_type": 0.5, "filter_cutoff": 0.78, "filter_resonance": 0.55,
         "attack": 0.002, "decay": 0.12, "sustain": 0.15, "release": 0.06},
        ["pluck", "melody", "musical"],
        "Melodic pluck bass"
    ))

    return presets

def generate_variation(base_preset: Dict, variation_num: int) -> Dict:
    """Generate a variation of a base preset with slight parameter changes"""
    import copy
    preset = copy.deepcopy(base_preset)

    # Modify name
    preset["name"] = f"{base_preset['name']} V{variation_num}"

    # Add variation tag
    if "variation" not in preset["tags"]:
        preset["tags"].append("variation")

    # Slightly randomize parameters (±10%)
    for param in preset["parameters"]:
        if param != "osc_type":  # Don't randomize oscillator type
            original_value = preset["parameters"][param]
            variation = random.uniform(-0.1, 0.1)
            new_value = max(0.0, min(1.0, original_value + variation))
            preset["parameters"][param] = round(new_value, 3)

    return preset

def save_preset_bank(presets: List[Dict], filename: str):
    """Save a bank of presets to a JSON file"""
    output_dir = "zenith-core/Content/Presets/PolySynth"
    os.makedirs(output_dir, exist_ok=True)

    filepath = os.path.join(output_dir, filename)
    with open(filepath, 'w') as f:
        json.dump({"presets": presets}, f, indent=2)

    print(f"Created: {filepath} ({len(presets)} presets)")
    return filepath

def main():
    """Generate all bass preset banks"""
    print("Generating ZenithPolySynth Bass Preset Banks...")
    print("=" * 60)

    all_files = []
    total_presets = 0

    # Generate core preset categories
    print("\n1. Generating Sub/808 Bass Presets...")
    sub_808 = generate_sub_808_presets()
    file = save_preset_bank(sub_808, "Bass_Sub_808.json")
    all_files.append(file)
    total_presets += len(sub_808)

    print("\n2. Generating Reese/Growl Bass Presets...")
    reese_growl = generate_reese_growl_presets()
    file = save_preset_bank(reese_growl, "Bass_Reese_Growl.json")
    all_files.append(file)
    total_presets += len(reese_growl)

    print("\n3. Generating Analog House Bass Presets...")
    analog_house = generate_analog_house_presets()
    file = save_preset_bank(analog_house, "Bass_Analog_House.json")
    all_files.append(file)
    total_presets += len(analog_house)

    print("\n4. Generating Donk/Pluck Bass Presets...")
    donk_pluck = generate_donk_pluck_presets()
    file = save_preset_bank(donk_pluck, "Bass_Donk_Pluck.json")
    all_files.append(file)
    total_presets += len(donk_pluck)

    # Generate variations to reach ~150 presets
    print("\n5. Generating Preset Variations...")
    all_base_presets = sub_808 + reese_growl + analog_house + donk_pluck
    variations = []

    # Calculate how many variations we need
    target_total = 150
    variations_needed = target_total - total_presets

    if variations_needed > 0:
        # Select random presets and create variations
        random.seed(42)  # For reproducibility
        for i in range(variations_needed):
            base = random.choice(all_base_presets)
            variation = generate_variation(base, (i % 3) + 1)
            variations.append(variation)

        file = save_preset_bank(variations, "Bass_Variations.json")
        all_files.append(file)
        total_presets += len(variations)

    # Create preset index
    print("\n6. Creating Preset Index...")
    index = {
        "name": "ZenithPolySynth Bass Preset Bank",
        "version": "1.0.0",
        "category": "Bass",
        "total_presets": total_presets,
        "banks": [
            {"name": "Sub/808 Bass", "file": "Bass_Sub_808.json", "count": len(sub_808)},
            {"name": "Reese/Growl Bass", "file": "Bass_Reese_Growl.json", "count": len(reese_growl)},
            {"name": "Analog House Bass", "file": "Bass_Analog_House.json", "count": len(analog_house)},
            {"name": "Donk/Pluck Bass", "file": "Bass_Donk_Pluck.json", "count": len(donk_pluck)},
            {"name": "Bass Variations", "file": "Bass_Variations.json", "count": len(variations)}
        ],
        "tags": ["808", "sub", "trap", "reese", "growl", "house", "techno", "donk", "pluck", "fm", "analog"],
        "description": "Comprehensive bass preset collection for ZenithPolySynth featuring 808s, subs, Reese basses, growls, house basslines, and plucks"
    }

    index_file = "zenith-core/Content/Presets/PolySynth/Bass_Index.json"
    with open(index_file, 'w') as f:
        json.dump(index, f, indent=2)
    all_files.append(index_file)

    print(f"Created: {index_file}")

    # Summary
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)
    print(f"Total Preset Files Created: {len(all_files)}")
    print(f"Total Bass Presets: {total_presets}")
    print("\nFiles created:")
    for f in all_files:
        print(f"  - {f}")

    print("\n✓ Bass preset bank generation complete!")

if __name__ == "__main__":
    main()
