"""
Automation Assistant Agent for AI-powered automation suggestions.

This module provides intelligent automation curve generation,
pattern learning, and context-aware automation recommendations.
"""

from typing import List, Optional, Dict, Any, Tuple, Callable
from dataclasses import dataclass, field
from enum import Enum
import math


class AutomationPattern(Enum):
    """Common automation pattern types."""
    FADE_IN = "fade_in"
    FADE_OUT = "fade_out"
    FILTER_SWEEP_UP = "filter_sweep_up"
    FILTER_SWEEP_DOWN = "filter_sweep_down"
    SIDECHAIN_PUMP = "sidechain_pump"
    PAN_LFO = "pan_lfo"
    TREMOLO = "tremolo"
    VOLUME_SWELL = "volume_swell"
    DELAY_FEEDBACK_BUILD = "delay_feedback_build"
    REVERB_SWELL = "reverb_swell"
    CUSTOM = "custom"


class CurveType(Enum):
    """Automation curve interpolation types."""
    LINEAR = "linear"
    EXPONENTIAL = "exponential"
    LOGARITHMIC = "logarithmic"
    S_CURVE = "s_curve"
    STEP = "step"


class ParameterType(Enum):
    """Types of automatable parameters."""
    VOLUME = "volume"
    PAN = "pan"
    FILTER_CUTOFF = "filter_cutoff"
    FILTER_RESONANCE = "filter_resonance"
    DELAY_MIX = "delay_mix"
    DELAY_FEEDBACK = "delay_feedback"
    REVERB_MIX = "reverb_mix"
    COMPRESSOR_THRESHOLD = "compressor_threshold"
    EQ_GAIN = "eq_gain"
    SEND_LEVEL = "send_level"
    PLUGIN_PARAM = "plugin_param"


@dataclass
class AutomationPoint:
    """A single automation point."""
    time: float  # Time in seconds
    value: float  # Normalized value 0.0-1.0
    curve_type: CurveType = CurveType.LINEAR


@dataclass
class AutomationCurve:
    """A complete automation curve."""
    parameter: ParameterType
    parameter_name: str
    points: List[AutomationPoint] = field(default_factory=list)
    pattern: AutomationPattern = AutomationPattern.CUSTOM
    
    @property
    def duration(self) -> float:
        """Get curve duration."""
        if not self.points:
            return 0.0
        return max(p.time for p in self.points) - min(p.time for p in self.points)
    
    def get_value_at(self, time: float) -> float:
        """Interpolate value at a specific time."""
        if not self.points:
            return 0.0
        
        # Sort points by time
        sorted_points = sorted(self.points, key=lambda p: p.time)
        
        # Before first point
        if time <= sorted_points[0].time:
            return sorted_points[0].value
        
        # After last point
        if time >= sorted_points[-1].time:
            return sorted_points[-1].value
        
        # Find surrounding points
        for i in range(len(sorted_points) - 1):
            p1, p2 = sorted_points[i], sorted_points[i + 1]
            if p1.time <= time <= p2.time:
                return self._interpolate(p1, p2, time)
        
        return sorted_points[-1].value
    
    def _interpolate(
        self, 
        p1: AutomationPoint, 
        p2: AutomationPoint, 
        time: float
    ) -> float:
        """Interpolate between two points."""
        if p2.time == p1.time:
            return p1.value
        
        t = (time - p1.time) / (p2.time - p1.time)
        
        if p1.curve_type == CurveType.LINEAR:
            return p1.value + (p2.value - p1.value) * t
        elif p1.curve_type == CurveType.EXPONENTIAL:
            return p1.value + (p2.value - p1.value) * (t ** 2)
        elif p1.curve_type == CurveType.LOGARITHMIC:
            return p1.value + (p2.value - p1.value) * math.sqrt(t)
        elif p1.curve_type == CurveType.S_CURVE:
            # Smooth S-curve using sine
            smooth_t = (1 - math.cos(t * math.pi)) / 2
            return p1.value + (p2.value - p1.value) * smooth_t
        elif p1.curve_type == CurveType.STEP:
            return p1.value if t < 0.5 else p2.value
        
        return p1.value


@dataclass
class AutomationSuggestion:
    """A suggested automation curve."""
    track_name: str
    curve: AutomationCurve
    context: str  # Why this is suggested
    priority: str  # "high", "medium", "low"
    section_type: Optional[str] = None  # intro, verse, chorus, etc.


@dataclass
class UserPreferenceProfile:
    """Learned user automation preferences."""
    preferred_curve_type: CurveType = CurveType.LINEAR
    fade_in_duration: float = 2.0  # seconds
    fade_out_duration: float = 4.0  # seconds
    filter_sweep_depth: float = 0.8  # 0.0-1.0
    sidechain_intensity: float = 0.5  # 0.0-1.0
    automation_density: float = 0.5  # How much automation user typically uses
    common_parameters: List[ParameterType] = field(default_factory=list)


@dataclass
class PatternLibrary:
    """Library of automation patterns."""
    patterns: Dict[AutomationPattern, List[AutomationPoint]] = field(
        default_factory=dict
    )


# Default pattern templates
DEFAULT_PATTERNS: Dict[AutomationPattern, List[Tuple[float, float, CurveType]]] = {
    AutomationPattern.FADE_IN: [
        (0.0, 0.0, CurveType.EXPONENTIAL),
        (1.0, 1.0, CurveType.LINEAR),
    ],
    AutomationPattern.FADE_OUT: [
        (0.0, 1.0, CurveType.EXPONENTIAL),
        (1.0, 0.0, CurveType.LINEAR),
    ],
    AutomationPattern.FILTER_SWEEP_UP: [
        (0.0, 0.1, CurveType.S_CURVE),
        (1.0, 0.9, CurveType.LINEAR),
    ],
    AutomationPattern.FILTER_SWEEP_DOWN: [
        (0.0, 0.9, CurveType.S_CURVE),
        (1.0, 0.1, CurveType.LINEAR),
    ],
    AutomationPattern.SIDECHAIN_PUMP: [
        (0.0, 1.0, CurveType.EXPONENTIAL),
        (0.1, 0.2, CurveType.EXPONENTIAL),
        (0.5, 1.0, CurveType.S_CURVE),
        (1.0, 1.0, CurveType.LINEAR),
    ],
    AutomationPattern.VOLUME_SWELL: [
        (0.0, 0.0, CurveType.S_CURVE),
        (0.5, 1.0, CurveType.S_CURVE),
        (1.0, 0.0, CurveType.LINEAR),
    ],
    AutomationPattern.REVERB_SWELL: [
        (0.0, 0.2, CurveType.LINEAR),
        (0.7, 0.2, CurveType.S_CURVE),
        (1.0, 0.8, CurveType.LINEAR),
    ],
}


class AutomationAssistantAgent:
    """
    AI-powered automation assistant for intelligent curve generation.
    
    Learns user preferences, suggests context-aware automation,
    and generates common patterns for mixing and production.
    """

    def __init__(self):
        """Initialize Automation Assistant Agent."""
        self.user_profile = UserPreferenceProfile()
        self.pattern_library = self._initialize_patterns()
        self.learned_contexts: Dict[str, List[AutomationPattern]] = {}
        
        print("AutomationAssistantAgent initialized")

    def _initialize_patterns(self) -> PatternLibrary:
        """Initialize pattern library with defaults."""
        library = PatternLibrary()
        
        for pattern, points in DEFAULT_PATTERNS.items():
            library.patterns[pattern] = [
                AutomationPoint(time=t, value=v, curve_type=c)
                for t, v, c in points
            ]
        
        return library

    def generate_curve(
        self,
        pattern: AutomationPattern,
        duration: float,
        parameter: ParameterType = ParameterType.VOLUME,
        start_time: float = 0.0,
        intensity: float = 1.0
    ) -> AutomationCurve:
        """
        Generate an automation curve from a pattern.
        
        Args:
            pattern: The automation pattern to use
            duration: Duration of the curve in seconds
            parameter: Target parameter type
            start_time: Start time offset in seconds
            intensity: Intensity multiplier (0.0-1.0)
            
        Returns:
            Generated AutomationCurve
        """
        template_points = self.pattern_library.patterns.get(pattern, [])
        
        if not template_points:
            # Return empty curve for unknown patterns
            return AutomationCurve(
                parameter=parameter,
                parameter_name=parameter.value,
                points=[],
                pattern=pattern
            )
        
        # Scale points to duration and apply intensity
        scaled_points = []
        for point in template_points:
            scaled_points.append(AutomationPoint(
                time=start_time + point.time * duration,
                value=self._apply_intensity(point.value, intensity),
                curve_type=point.curve_type
            ))
        
        return AutomationCurve(
            parameter=parameter,
            parameter_name=parameter.value,
            points=scaled_points,
            pattern=pattern
        )

    def _apply_intensity(self, value: float, intensity: float) -> float:
        """Apply intensity to a value, keeping it closer to 0.5 for low intensity."""
        center = 0.5
        return center + (value - center) * intensity

    def generate_sidechain(
        self,
        duration: float,
        tempo_bpm: float,
        subdivision: str = "quarter",
        intensity: float = 0.5,
        start_time: float = 0.0
    ) -> AutomationCurve:
        """
        Generate sidechain pumping automation.
        
        Args:
            duration: Total duration in seconds
            tempo_bpm: Song tempo in BPM
            subdivision: "quarter", "eighth", "sixteenth"
            intensity: Pump intensity (0.0-1.0)
            start_time: Start time offset
            
        Returns:
            Sidechain automation curve
        """
        # Calculate beat duration
        beat_duration = 60.0 / tempo_bpm
        
        subdivisions = {
            "quarter": 1.0,
            "eighth": 0.5,
            "sixteenth": 0.25
        }
        
        cycle_duration = beat_duration * subdivisions.get(subdivision, 1.0)
        
        points = []
        current_time = start_time
        
        while current_time < start_time + duration:
            # Attack (quick drop)
            points.append(AutomationPoint(
                time=current_time,
                value=1.0,
                curve_type=CurveType.EXPONENTIAL
            ))
            points.append(AutomationPoint(
                time=current_time + cycle_duration * 0.1,
                value=1.0 - intensity,
                curve_type=CurveType.EXPONENTIAL
            ))
            # Release (slower rise)
            points.append(AutomationPoint(
                time=current_time + cycle_duration * 0.8,
                value=1.0,
                curve_type=CurveType.S_CURVE
            ))
            
            current_time += cycle_duration
        
        return AutomationCurve(
            parameter=ParameterType.VOLUME,
            parameter_name="Sidechain",
            points=points,
            pattern=AutomationPattern.SIDECHAIN_PUMP
        )

    def generate_filter_sweep(
        self,
        duration: float,
        direction: str = "up",
        curve_type: CurveType = CurveType.S_CURVE,
        start_value: float = 0.1,
        end_value: float = 0.9,
        start_time: float = 0.0
    ) -> AutomationCurve:
        """
        Generate filter sweep automation.
        
        Args:
            duration: Sweep duration in seconds
            direction: "up" or "down"
            curve_type: Interpolation curve type
            start_value: Starting filter value (0.0-1.0)
            end_value: Ending filter value (0.0-1.0)
            start_time: Start time offset
            
        Returns:
            Filter sweep automation curve
        """
        if direction == "down":
            start_value, end_value = end_value, start_value
        
        pattern = (AutomationPattern.FILTER_SWEEP_UP if direction == "up" 
                  else AutomationPattern.FILTER_SWEEP_DOWN)
        
        points = [
            AutomationPoint(
                time=start_time,
                value=start_value,
                curve_type=curve_type
            ),
            AutomationPoint(
                time=start_time + duration,
                value=end_value,
                curve_type=CurveType.LINEAR
            )
        ]
        
        return AutomationCurve(
            parameter=ParameterType.FILTER_CUTOFF,
            parameter_name="Filter Cutoff",
            points=points,
            pattern=pattern
        )

    def suggest_automation(
        self,
        track_name: str,
        section_type: str,
        tempo_bpm: float,
        section_duration: float,
        existing_automation: Optional[List[AutomationCurve]] = None
    ) -> List[AutomationSuggestion]:
        """
        Generate automation suggestions based on context.
        
        Args:
            track_name: Name of the track
            section_type: Type of section (intro, verse, chorus, etc.)
            tempo_bpm: Song tempo
            section_duration: Duration of the section
            existing_automation: Already present automation
            
        Returns:
            List of automation suggestions
        """
        suggestions = []
        
        # Section-specific suggestions
        if section_type == "intro":
            suggestions.append(AutomationSuggestion(
                track_name=track_name,
                curve=self.generate_curve(
                    AutomationPattern.FADE_IN,
                    min(section_duration, self.user_profile.fade_in_duration),
                    ParameterType.VOLUME
                ),
                context="Fade in at intro for smooth start",
                priority="high",
                section_type=section_type
            ))
            
            suggestions.append(AutomationSuggestion(
                track_name=track_name,
                curve=self.generate_filter_sweep(
                    section_duration * 0.8,
                    direction="up"
                ),
                context="Filter sweep to build energy into first section",
                priority="medium",
                section_type=section_type
            ))
        
        elif section_type == "outro":
            suggestions.append(AutomationSuggestion(
                track_name=track_name,
                curve=self.generate_curve(
                    AutomationPattern.FADE_OUT,
                    min(section_duration, self.user_profile.fade_out_duration),
                    ParameterType.VOLUME,
                    start_time=section_duration - self.user_profile.fade_out_duration
                ),
                context="Fade out at outro for smooth ending",
                priority="high",
                section_type=section_type
            ))
        
        elif section_type in ["chorus", "drop"]:
            # Suggest sidechain for rhythmic elements
            suggestions.append(AutomationSuggestion(
                track_name=track_name,
                curve=self.generate_sidechain(
                    section_duration,
                    tempo_bpm,
                    intensity=self.user_profile.sidechain_intensity
                ),
                context="Sidechain pumping for rhythmic energy",
                priority="medium",
                section_type=section_type
            ))
        
        elif section_type == "build":
            # Filter sweep for builds
            suggestions.append(AutomationSuggestion(
                track_name=track_name,
                curve=self.generate_filter_sweep(
                    section_duration,
                    direction="up",
                    curve_type=CurveType.EXPONENTIAL
                ),
                context="Filter sweep to build tension",
                priority="high",
                section_type=section_type
            ))
        
        elif section_type == "breakdown":
            suggestions.append(AutomationSuggestion(
                track_name=track_name,
                curve=self.generate_filter_sweep(
                    section_duration * 0.5,
                    direction="down"
                ),
                context="Filter sweep down for breakdown feel",
                priority="medium",
                section_type=section_type
            ))
            
            # Reverb swell
            suggestions.append(AutomationSuggestion(
                track_name=track_name,
                curve=self.generate_curve(
                    AutomationPattern.REVERB_SWELL,
                    section_duration,
                    ParameterType.REVERB_MIX
                ),
                context="Reverb swell for atmospheric breakdown",
                priority="low",
                section_type=section_type
            ))
        
        return suggestions

    def learn_from_automation(
        self,
        automation_data: List[AutomationCurve],
        context: str
    ) -> None:
        """
        Learn from user automation patterns.
        
        Args:
            automation_data: User's automation curves
            context: Context where automation was used
        """
        if not automation_data:
            return
        
        # Update common parameters
        for curve in automation_data:
            if curve.parameter not in self.user_profile.common_parameters:
                self.user_profile.common_parameters.append(curve.parameter)
        
        # Analyze curve characteristics
        for curve in automation_data:
            if curve.pattern == AutomationPattern.FADE_IN:
                self.user_profile.fade_in_duration = (
                    self.user_profile.fade_in_duration * 0.8 + curve.duration * 0.2
                )
            elif curve.pattern == AutomationPattern.FADE_OUT:
                self.user_profile.fade_out_duration = (
                    self.user_profile.fade_out_duration * 0.8 + curve.duration * 0.2
                )
            
            # Learn preferred curve type from first point
            if curve.points:
                self.user_profile.preferred_curve_type = curve.points[0].curve_type
        
        # Store context patterns
        detected_patterns = [c.pattern for c in automation_data]
        if context not in self.learned_contexts:
            self.learned_contexts[context] = []
        self.learned_contexts[context].extend(detected_patterns)
        
        print(f"Learned from {len(automation_data)} curves in context: {context}")

    def get_user_profile(self) -> Dict[str, Any]:
        """Get current user preference profile."""
        return {
            "preferred_curve_type": self.user_profile.preferred_curve_type.value,
            "fade_in_duration": self.user_profile.fade_in_duration,
            "fade_out_duration": self.user_profile.fade_out_duration,
            "filter_sweep_depth": self.user_profile.filter_sweep_depth,
            "sidechain_intensity": self.user_profile.sidechain_intensity,
            "automation_density": self.user_profile.automation_density,
            "common_parameters": [p.value for p in self.user_profile.common_parameters],
            "learned_contexts": len(self.learned_contexts)
        }

    def get_available_patterns(self) -> List[Dict[str, Any]]:
        """Get list of available automation patterns."""
        patterns = []
        for pattern in AutomationPattern:
            points = self.pattern_library.patterns.get(pattern, [])
            patterns.append({
                "name": pattern.value,
                "point_count": len(points),
                "has_template": len(points) > 0
            })
        return patterns


def print_automation_suggestions(suggestions: List[AutomationSuggestion]) -> None:
    """Print formatted automation suggestions."""
    print("\n" + "=" * 60)
    print("🎚️  AUTOMATION ASSISTANT SUGGESTIONS")
    print("=" * 60)
    
    if not suggestions:
        print("\nNo suggestions available.")
        return
    
    for i, suggestion in enumerate(suggestions, 1):
        priority_emoji = {"high": "🔴", "medium": "🟡", "low": "🟢"}
        print(f"\n{i}. {suggestion.curve.pattern.value.replace('_', ' ').title()}")
        print(f"   {priority_emoji.get(suggestion.priority, '•')} Priority: {suggestion.priority}")
        print(f"   🎵 Track: {suggestion.track_name}")
        print(f"   📍 Section: {suggestion.section_type or 'N/A'}")
        print(f"   💡 Context: {suggestion.context}")
        print(f"   📊 Parameter: {suggestion.curve.parameter.value}")
        print(f"   ⏱️  Duration: {suggestion.curve.duration:.2f}s")
        print(f"   📍 Points: {len(suggestion.curve.points)}")
    
    print("\n" + "=" * 60)


def main():
    """Main entry point for automation assistant agent."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='AutomationAssistantAgent - AI Automation Suggestions'
    )
    parser.add_argument('--demo', action='store_true',
                       help='Run demo with sample suggestions')
    parser.add_argument('--info', action='store_true',
                       help='Show agent info')
    parser.add_argument('--list-patterns', action='store_true',
                       help='List available patterns')
    parser.add_argument('--profile', action='store_true',
                       help='Show user preference profile')
    
    args = parser.parse_args()
    
    agent = AutomationAssistantAgent()
    
    if args.info:
        print("\nAutomationAssistantAgent Information:")
        print("=" * 50)
        print("Purpose: AI-powered automation suggestions and learning")
        print("Capabilities:")
        print("  - Generate common automation curves")
        print("  - Context-aware suggestions based on section")
        print("  - Learn from user automation patterns")
        print("  - Sidechain and filter sweep generation")
        print("  - User preference profiling")
        print()
        return
    
    if args.list_patterns:
        print("\nAvailable Automation Patterns:")
        print("=" * 40)
        for pattern in agent.get_available_patterns():
            status = "✓" if pattern["has_template"] else "○"
            print(f"  {status} {pattern['name']:25} ({pattern['point_count']} points)")
        return
    
    if args.profile:
        print("\nUser Preference Profile:")
        print("=" * 40)
        profile = agent.get_user_profile()
        for key, value in profile.items():
            print(f"  {key}: {value}")
        return
    
    if args.demo:
        print("\nRunning demo with sample suggestions...")
        
        # Generate suggestions for different sections
        sections = [
            ("Lead Synth", "intro", 8.0),
            ("Bass", "chorus", 16.0),
            ("Pad", "build", 8.0),
            ("Lead Synth", "breakdown", 16.0),
            ("Master", "outro", 8.0),
        ]
        
        all_suggestions = []
        for track, section, duration in sections:
            suggestions = agent.suggest_automation(
                track_name=track,
                section_type=section,
                tempo_bpm=128.0,
                section_duration=duration
            )
            all_suggestions.extend(suggestions)
        
        print_automation_suggestions(all_suggestions)
        
        # Show profile
        print("\nCurrent User Profile:")
        profile = agent.get_user_profile()
        for key, value in profile.items():
            print(f"  {key}: {value}")
        
        return
    
    print("\nAutomationAssistantAgent ready")
    print("Use --demo to see sample suggestions")
    print("Use --list-patterns to see available patterns")
    print("Use --profile to see user preferences")
    print("Use --info for more information")


if __name__ == "__main__":
    main()
