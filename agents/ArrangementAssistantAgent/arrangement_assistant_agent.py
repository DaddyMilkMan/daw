"""
Arrangement Assistant Agent for AI-powered arrangement suggestions.

This module provides automated song structure analysis, section detection,
and arrangement recommendations based on genre conventions.
"""

from typing import List, Optional, Dict, Any, Tuple
from dataclasses import dataclass, field
from enum import Enum
import math


class SectionType(Enum):
    """Types of song sections."""
    INTRO = "intro"
    VERSE = "verse"
    PRE_CHORUS = "pre_chorus"
    CHORUS = "chorus"
    BRIDGE = "bridge"
    OUTRO = "outro"
    DROP = "drop"
    BREAKDOWN = "breakdown"
    BUILD = "build"
    SOLO = "solo"
    HOOK = "hook"
    UNKNOWN = "unknown"


class Genre(Enum):
    """Supported music genres."""
    POP = "pop"
    EDM = "edm"
    ROCK = "rock"
    HIP_HOP = "hip_hop"
    RNB = "rnb"
    COUNTRY = "country"
    JAZZ = "jazz"
    CLASSICAL = "classical"
    ELECTRONIC = "electronic"
    UNKNOWN = "unknown"


@dataclass
class Section:
    """Represents a detected song section."""
    section_type: SectionType
    start_time: float  # seconds
    end_time: float    # seconds
    confidence: float  # 0.0-1.0
    energy_level: float  # 0.0-1.0
    
    @property
    def duration(self) -> float:
        """Get section duration in seconds."""
        return self.end_time - self.start_time
    
    @property
    def duration_bars(self) -> float:
        """Get approximate duration in bars (at 120 BPM, 4/4)."""
        return self.duration / 2.0  # 2 seconds per bar at 120 BPM


@dataclass
class ArrangementSuggestion:
    """A suggestion for improving arrangement."""
    description: str
    priority: str  # "high", "medium", "low"
    affected_sections: List[int]  # indices into section list
    reason: str
    action: str  # What to do


@dataclass
class TransitionSuggestion:
    """A suggestion for improving a transition between sections."""
    from_section_idx: int
    to_section_idx: int
    suggestion: str
    priority: str
    techniques: List[str]  # Suggested techniques


@dataclass
class ArrangementVariation:
    """A generated arrangement variation."""
    name: str
    description: str
    section_order: List[SectionType]
    estimated_duration: float
    energy_curve: List[float]


@dataclass
class ArrangementAnalysisResult:
    """Complete analysis result for song arrangement."""
    sections: List[Section] = field(default_factory=list)
    detected_genre: Genre = Genre.UNKNOWN
    genre_confidence: float = 0.0
    tempo_bpm: float = 120.0
    key_signature: str = "C major"
    time_signature: str = "4/4"
    suggestions: List[ArrangementSuggestion] = field(default_factory=list)
    transition_suggestions: List[TransitionSuggestion] = field(default_factory=list)
    variations: List[ArrangementVariation] = field(default_factory=list)
    structure_score: float = 0.0  # 0.0-100.0
    analysis_notes: List[str] = field(default_factory=list)
    
    @property
    def total_duration(self) -> float:
        """Get total song duration."""
        if not self.sections:
            return 0.0
        return max(s.end_time for s in self.sections)
    
    @property
    def section_count(self) -> int:
        """Get number of detected sections."""
        return len(self.sections)


# Genre-specific arrangement templates
GENRE_TEMPLATES: Dict[Genre, List[SectionType]] = {
    Genre.POP: [
        SectionType.INTRO,
        SectionType.VERSE,
        SectionType.PRE_CHORUS,
        SectionType.CHORUS,
        SectionType.VERSE,
        SectionType.PRE_CHORUS,
        SectionType.CHORUS,
        SectionType.BRIDGE,
        SectionType.CHORUS,
        SectionType.OUTRO
    ],
    Genre.EDM: [
        SectionType.INTRO,
        SectionType.BUILD,
        SectionType.DROP,
        SectionType.BREAKDOWN,
        SectionType.BUILD,
        SectionType.DROP,
        SectionType.OUTRO
    ],
    Genre.ROCK: [
        SectionType.INTRO,
        SectionType.VERSE,
        SectionType.CHORUS,
        SectionType.VERSE,
        SectionType.CHORUS,
        SectionType.SOLO,
        SectionType.CHORUS,
        SectionType.OUTRO
    ],
    Genre.HIP_HOP: [
        SectionType.INTRO,
        SectionType.VERSE,
        SectionType.HOOK,
        SectionType.VERSE,
        SectionType.HOOK,
        SectionType.VERSE,
        SectionType.HOOK,
        SectionType.OUTRO
    ],
}


class ArrangementAssistantAgent:
    """
    AI-powered arrangement assistant for structure analysis and suggestions.
    
    Analyzes song structure, detects sections, compares to genre conventions,
    and generates arrangement improvement suggestions and variations.
    """

    def __init__(self):
        """Initialize Arrangement Assistant Agent."""
        self.genre_templates = GENRE_TEMPLATES
        self.min_section_duration = 4.0  # Minimum section length in seconds
        self.max_repetitions = 4  # Max times a section should repeat
        
        print("ArrangementAssistantAgent initialized")

    def analyze_arrangement(
        self,
        audio_data: List[List[float]],
        sample_rate: int,
        tempo_bpm: Optional[float] = None,
        genre_hint: Optional[Genre] = None
    ) -> ArrangementAnalysisResult:
        """
        Analyze song arrangement and detect sections.
        
        Args:
            audio_data: Audio [channels][samples]
            sample_rate: Audio sample rate
            tempo_bpm: Known tempo (auto-detect if not provided)
            genre_hint: Hint for genre (auto-detect if not provided)
            
        Returns:
            ArrangementAnalysisResult with sections and suggestions
        """
        print(f"Analyzing arrangement: {len(audio_data)} channels at {sample_rate}Hz")
        
        result = ArrangementAnalysisResult()
        
        if not audio_data or not audio_data[0]:
            result.analysis_notes.append("No audio data to analyze")
            return result
        
        # Calculate duration
        duration = len(audio_data[0]) / sample_rate
        
        # Detect or use provided tempo
        result.tempo_bpm = tempo_bpm if tempo_bpm else self._detect_tempo(
            audio_data, sample_rate
        )
        
        # Detect or use provided genre
        result.detected_genre = genre_hint if genre_hint else self._detect_genre(
            audio_data, sample_rate
        )
        
        # Detect sections
        result.sections = self._detect_sections(
            audio_data, sample_rate, result.tempo_bpm
        )
        
        # Generate suggestions
        result.suggestions = self._generate_arrangement_suggestions(
            result.sections, result.detected_genre
        )
        
        # Generate transition suggestions
        result.transition_suggestions = self._generate_transition_suggestions(
            result.sections
        )
        
        # Generate arrangement variations
        result.variations = self._generate_variations(
            result.sections, result.detected_genre
        )
        
        # Calculate structure score
        result.structure_score = self._calculate_structure_score(
            result.sections, result.detected_genre
        )
        
        # Add analysis notes
        result.analysis_notes.append(
            f"Detected {len(result.sections)} sections over {duration:.1f} seconds"
        )
        result.analysis_notes.append(
            f"Tempo: {result.tempo_bpm:.1f} BPM, Genre: {result.detected_genre.value}"
        )
        
        print(f"Analysis complete: {len(result.sections)} sections, "
              f"score={result.structure_score:.1f}/100")
        return result

    def _detect_tempo(
        self,
        audio_data: List[List[float]],
        sample_rate: int
    ) -> float:
        """Detect tempo from audio (placeholder)."""
        # TODO: Implement beat detection algorithm
        # Could use:
        # - Onset detection
        # - Autocorrelation
        # - FFT-based tempo estimation
        # - Machine learning models
        
        # Placeholder: return common tempo
        return 120.0

    def _detect_genre(
        self,
        audio_data: List[List[float]],
        sample_rate: int
    ) -> Genre:
        """Detect music genre from audio (placeholder)."""
        # TODO: Implement genre classification
        # Could use:
        # - Audio feature extraction (MFCCs, spectral features)
        # - Pre-trained neural network classifier
        # - Rule-based heuristics
        
        # Placeholder: return unknown
        return Genre.UNKNOWN

    def _detect_sections(
        self,
        audio_data: List[List[float]],
        sample_rate: int,
        tempo_bpm: float
    ) -> List[Section]:
        """
        Detect song sections from audio.
        
        Uses energy analysis, spectral changes, and repetition detection
        to identify section boundaries and types.
        """
        sections = []
        duration = len(audio_data[0]) / sample_rate
        
        # Calculate energy curve
        energy_curve = self._calculate_energy_curve(audio_data, sample_rate)
        
        # Find section boundaries using energy changes
        boundaries = self._find_section_boundaries(energy_curve, sample_rate, tempo_bpm)
        
        # Classify each section
        for i, (start, end) in enumerate(boundaries):
            section_energy = self._get_section_energy(energy_curve, start, end, sample_rate)
            section_type = self._classify_section(
                i, len(boundaries), section_energy, start, end, duration
            )
            
            sections.append(Section(
                section_type=section_type,
                start_time=start,
                end_time=end,
                confidence=0.75,  # Placeholder confidence
                energy_level=section_energy
            ))
        
        return sections

    def _calculate_energy_curve(
        self,
        audio_data: List[List[float]],
        sample_rate: int,
        window_size_ms: float = 50.0
    ) -> List[float]:
        """Calculate RMS energy curve over time."""
        window_samples = int(sample_rate * window_size_ms / 1000)
        mono = audio_data[0]  # Use first channel
        
        energy = []
        for i in range(0, len(mono) - window_samples, window_samples):
            window = mono[i:i + window_samples]
            rms = math.sqrt(sum(s**2 for s in window) / len(window))
            energy.append(rms)
        
        return energy

    def _find_section_boundaries(
        self,
        energy_curve: List[float],
        sample_rate: int,
        tempo_bpm: float,
        window_size_ms: float = 50.0
    ) -> List[Tuple[float, float]]:
        """Find section boundaries from energy curve."""
        if not energy_curve:
            return [(0.0, 1.0)]  # Default single section
        
        # Calculate bar duration
        bar_duration = 60.0 / tempo_bpm * 4  # 4 beats per bar
        
        # Convert to time
        window_duration = window_size_ms / 1000
        
        # Find significant energy changes
        boundaries = []
        current_start = 0.0
        
        # Simplified boundary detection: look for significant energy changes
        threshold = 0.15  # Energy change threshold
        
        for i in range(1, len(energy_curve)):
            current_time = i * window_duration
            
            # Check for significant energy change
            if i > 0 and abs(energy_curve[i] - energy_curve[i-1]) > threshold:
                # Snap to bar boundary
                snapped_time = round(current_time / bar_duration) * bar_duration
                if snapped_time > current_start + self.min_section_duration:
                    boundaries.append((current_start, snapped_time))
                    current_start = snapped_time
        
        # Add final section
        final_time = len(energy_curve) * window_duration
        if final_time > current_start:
            boundaries.append((current_start, final_time))
        
        # Ensure we have at least one section
        if not boundaries:
            boundaries = [(0.0, final_time)]
        
        return boundaries

    def _get_section_energy(
        self,
        energy_curve: List[float],
        start_time: float,
        end_time: float,
        sample_rate: int,
        window_size_ms: float = 50.0
    ) -> float:
        """Get average energy for a section."""
        window_duration = window_size_ms / 1000
        start_idx = int(start_time / window_duration)
        end_idx = int(end_time / window_duration)
        
        if end_idx <= start_idx or start_idx >= len(energy_curve):
            return 0.5
        
        end_idx = min(end_idx, len(energy_curve))
        section_energy = energy_curve[start_idx:end_idx]
        
        if not section_energy:
            return 0.5
        
        avg = sum(section_energy) / len(section_energy)
        max_energy = max(energy_curve) if energy_curve else 1.0
        
        return avg / max_energy if max_energy > 0 else 0.5

    def _classify_section(
        self,
        section_idx: int,
        total_sections: int,
        energy: float,
        start_time: float,
        end_time: float,
        total_duration: float
    ) -> SectionType:
        """Classify a section based on position and energy."""
        # Position in song (0.0-1.0)
        position = (start_time + end_time) / 2 / total_duration
        
        # First section is likely intro
        if section_idx == 0:
            return SectionType.INTRO
        
        # Last section is likely outro
        if section_idx == total_sections - 1:
            return SectionType.OUTRO
        
        # High energy sections
        if energy > 0.75:
            return SectionType.CHORUS if position < 0.8 else SectionType.DROP
        
        # Medium energy sections
        if energy > 0.5:
            if position < 0.5:
                return SectionType.VERSE
            else:
                return SectionType.BRIDGE
        
        # Low energy sections
        if energy > 0.25:
            return SectionType.VERSE
        
        # Very low energy
        return SectionType.BREAKDOWN

    def _generate_arrangement_suggestions(
        self,
        sections: List[Section],
        genre: Genre
    ) -> List[ArrangementSuggestion]:
        """Generate suggestions for improving arrangement."""
        suggestions = []
        
        if not sections:
            return suggestions
        
        # Get template for genre
        template = self.genre_templates.get(genre, [])
        
        # Check for excessive repetition
        section_counts: Dict[SectionType, int] = {}
        for section in sections:
            section_counts[section.section_type] = section_counts.get(
                section.section_type, 0
            ) + 1
        
        for section_type, count in section_counts.items():
            if count > self.max_repetitions:
                suggestions.append(ArrangementSuggestion(
                    description=f"Consider adding variation to {section_type.value} sections",
                    priority="medium",
                    affected_sections=[i for i, s in enumerate(sections) 
                                      if s.section_type == section_type],
                    reason=f"{section_type.value} appears {count} times",
                    action="Add melodic or rhythmic variations between repetitions"
                ))
        
        # Check for missing common sections
        if template:
            existing_types = set(s.section_type for s in sections)
            for section_type in set(template):
                if section_type not in existing_types:
                    suggestions.append(ArrangementSuggestion(
                        description=f"Consider adding a {section_type.value} section",
                        priority="low",
                        affected_sections=[],
                        reason=f"Typical {genre.value} songs include a {section_type.value}",
                        action=f"Add a {section_type.value} section for genre consistency"
                    ))
        
        # Check for energy flow issues
        for i in range(1, len(sections)):
            prev = sections[i - 1]
            curr = sections[i]
            
            # Warn about large energy drops (except for breakdowns)
            if (prev.energy_level - curr.energy_level > 0.5 and 
                curr.section_type != SectionType.BREAKDOWN and
                curr.section_type != SectionType.OUTRO):
                suggestions.append(ArrangementSuggestion(
                    description="Large energy drop between sections",
                    priority="medium",
                    affected_sections=[i - 1, i],
                    reason=f"Energy drops from {prev.energy_level:.1%} to {curr.energy_level:.1%}",
                    action="Add transitional element or build-up between sections"
                ))
        
        return suggestions

    def _generate_transition_suggestions(
        self,
        sections: List[Section]
    ) -> List[TransitionSuggestion]:
        """Generate suggestions for section transitions."""
        suggestions = []
        
        for i in range(len(sections) - 1):
            curr = sections[i]
            next_sec = sections[i + 1]
            
            techniques = []
            
            # Suggest techniques based on section types
            if next_sec.section_type == SectionType.CHORUS:
                techniques = ["riser", "drum fill", "harmonic lift", "filter sweep up"]
            elif next_sec.section_type == SectionType.DROP:
                techniques = ["riser", "white noise sweep", "silence before drop"]
            elif next_sec.section_type == SectionType.BREAKDOWN:
                techniques = ["filter sweep down", "reverb tail", "gradual strip-down"]
            elif next_sec.section_type == SectionType.OUTRO:
                techniques = ["gradual fade", "reverb increase", "element removal"]
            else:
                techniques = ["drum fill", "chord change", "melodic motif"]
            
            suggestions.append(TransitionSuggestion(
                from_section_idx=i,
                to_section_idx=i + 1,
                suggestion=f"Transition from {curr.section_type.value} to {next_sec.section_type.value}",
                priority="medium",
                techniques=techniques
            ))
        
        return suggestions

    def _generate_variations(
        self,
        sections: List[Section],
        genre: Genre
    ) -> List[ArrangementVariation]:
        """Generate arrangement variations."""
        variations = []
        
        if not sections:
            return variations
        
        section_types = [s.section_type for s in sections]
        
        # Variation 1: Extended version (add repetitions)
        extended = list(section_types)
        if SectionType.CHORUS in extended:
            # Add extra chorus
            chorus_idx = extended.index(SectionType.CHORUS)
            extended.insert(chorus_idx + 1, SectionType.CHORUS)
        
        variations.append(ArrangementVariation(
            name="Extended",
            description="Extended version with additional chorus",
            section_order=extended,
            estimated_duration=sum(s.duration for s in sections) * 1.2,
            energy_curve=[s.energy_level for s in sections]
        ))
        
        # Variation 2: Radio edit (shorter)
        radio = [s for s in section_types 
                 if s not in [SectionType.SOLO, SectionType.BREAKDOWN]]
        
        variations.append(ArrangementVariation(
            name="Radio Edit",
            description="Shorter version for radio play",
            section_order=radio,
            estimated_duration=sum(s.duration for s in sections) * 0.8,
            energy_curve=[s.energy_level for s in sections if s.section_type in radio]
        ))
        
        # Variation 3: Instrumental
        instrumental = [s for s in section_types 
                       if s not in [SectionType.VERSE]]
        
        variations.append(ArrangementVariation(
            name="Instrumental",
            description="Version focused on instrumental sections",
            section_order=instrumental,
            estimated_duration=sum(s.duration for s in sections) * 0.7,
            energy_curve=[0.5] * len(instrumental)
        ))
        
        return variations

    def _calculate_structure_score(
        self,
        sections: List[Section],
        genre: Genre
    ) -> float:
        """Calculate arrangement structure score (0-100)."""
        if not sections:
            return 0.0
        
        score = 100.0
        
        # Penalize for very few sections
        if len(sections) < 4:
            score -= 15
        
        # Penalize for too many sections
        if len(sections) > 12:
            score -= 10
        
        # Check for typical structure elements
        section_types = set(s.section_type for s in sections)
        
        if SectionType.INTRO not in section_types:
            score -= 5
        if SectionType.OUTRO not in section_types:
            score -= 5
        if SectionType.CHORUS not in section_types and SectionType.DROP not in section_types:
            score -= 15
        
        # Check for good energy flow
        energy_changes = [
            abs(sections[i].energy_level - sections[i-1].energy_level)
            for i in range(1, len(sections))
        ]
        
        if energy_changes:
            avg_change = sum(energy_changes) / len(energy_changes)
            if avg_change > 0.4:  # Too much variation
                score -= 10
            elif avg_change < 0.1:  # Too flat
                score -= 10
        
        return max(0.0, min(100.0, score))


def print_arrangement_report(result: ArrangementAnalysisResult) -> None:
    """Print a formatted arrangement analysis report."""
    print("\n" + "=" * 60)
    print("🎵 ARRANGEMENT ASSISTANT ANALYSIS REPORT")
    print("=" * 60)
    
    print(f"\nStructure Score: {result.structure_score:.1f}/100")
    print(f"Detected Genre: {result.detected_genre.value}")
    print(f"Tempo: {result.tempo_bpm:.1f} BPM")
    print(f"Duration: {result.total_duration:.1f} seconds")
    print(f"Sections: {result.section_count}")
    
    if result.sections:
        print("\n📋 DETECTED SECTIONS:")
        for i, section in enumerate(result.sections):
            energy_bar = "█" * int(section.energy_level * 10)
            print(f"  {i+1}. [{section.start_time:6.1f}s - {section.end_time:6.1f}s] "
                  f"{section.section_type.value:12} | Energy: {energy_bar:10} "
                  f"({section.energy_level:.0%})")
    
    if result.suggestions:
        print("\n💡 ARRANGEMENT SUGGESTIONS:")
        for suggestion in result.suggestions:
            priority_emoji = {"high": "🔴", "medium": "🟡", "low": "🟢"}
            print(f"  {priority_emoji.get(suggestion.priority, '•')} {suggestion.description}")
            print(f"      Reason: {suggestion.reason}")
            print(f"      Action: {suggestion.action}")
    
    if result.transition_suggestions:
        print("\n🔀 TRANSITION SUGGESTIONS:")
        for trans in result.transition_suggestions[:5]:  # Show top 5
            print(f"  • {trans.suggestion}")
            print(f"      Try: {', '.join(trans.techniques[:3])}")
    
    if result.variations:
        print("\n🎼 ARRANGEMENT VARIATIONS:")
        for var in result.variations:
            print(f"  📀 {var.name}")
            print(f"      {var.description}")
            print(f"      Duration: ~{var.estimated_duration:.0f}s")
    
    print("\n" + "=" * 60)


def main():
    """Main entry point for arrangement assistant agent."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='ArrangementAssistantAgent - AI Arrangement Analysis'
    )
    parser.add_argument('--demo', action='store_true',
                       help='Run demo analysis with sample data')
    parser.add_argument('--info', action='store_true',
                       help='Show agent info')
    parser.add_argument('--list-genres', action='store_true',
                       help='List supported genres')
    
    args = parser.parse_args()
    
    agent = ArrangementAssistantAgent()
    
    if args.info:
        print("\nArrangementAssistantAgent Information:")
        print("=" * 50)
        print("Purpose: AI-powered arrangement analysis and suggestions")
        print("Capabilities:")
        print("  - Section detection (intro, verse, chorus, etc.)")
        print("  - Genre classification")
        print("  - Arrangement suggestions")
        print("  - Transition recommendations")
        print("  - Variation generation")
        print()
        return
    
    if args.list_genres:
        print("\nSupported Genres:")
        for genre in Genre:
            template = GENRE_TEMPLATES.get(genre, [])
            if template:
                structure = " → ".join(s.value for s in template)
                print(f"  {genre.value}: {structure}")
            else:
                print(f"  {genre.value}: (no template)")
        return
    
    if args.demo:
        # Generate demo audio with sample data
        import random
        random.seed(42)
        
        sample_rate = 44100
        duration = 180.0  # 3 minutes
        num_samples = int(sample_rate * duration)
        
        # Generate varying amplitude to simulate sections
        demo_audio = [[]]
        for i in range(num_samples):
            t = i / sample_rate
            # Create energy variations
            if t < 8:  # Intro
                amp = 0.2 + 0.1 * (t / 8)
            elif t < 32:  # Verse
                amp = 0.4
            elif t < 48:  # Chorus
                amp = 0.8
            elif t < 72:  # Verse
                amp = 0.4
            elif t < 88:  # Chorus
                amp = 0.8
            elif t < 104:  # Bridge
                amp = 0.5
            elif t < 120:  # Chorus
                amp = 0.85
            else:  # Outro
                amp = 0.3 * (1 - (t - 120) / 60)
            
            demo_audio[0].append(random.uniform(-amp, amp))
        
        print("\nRunning demo analysis on sample audio...")
        result = agent.analyze_arrangement(demo_audio, sample_rate, tempo_bpm=120)
        print_arrangement_report(result)
        return
    
    print("\nArrangementAssistantAgent ready")
    print("Use --demo to run a demo analysis")
    print("Use --list-genres to see supported genres")
    print("Use --info for more information")


if __name__ == "__main__":
    main()
