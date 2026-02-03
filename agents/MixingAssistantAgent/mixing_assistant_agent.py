"""
Mixing Assistant Agent for AI-powered mixing suggestions.

This module provides automated mixing analysis and recommendations
for EQ, compression, panning, and level adjustments.
"""

from typing import List, Optional, Dict, Any, Tuple
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
import math


class IssueSeverity(Enum):
    """Severity levels for mixing issues."""
    INFO = "info"
    WARNING = "warning"
    CRITICAL = "critical"


class IssueCategory(Enum):
    """Categories of mixing issues."""
    FREQUENCY = "frequency"
    DYNAMICS = "dynamics"
    STEREO = "stereo"
    PHASE = "phase"
    LEVELS = "levels"


@dataclass
class FrequencyBand:
    """Represents a frequency band for EQ analysis."""
    low_freq: float
    high_freq: float
    name: str
    
    @property
    def center_freq(self) -> float:
        """Calculate geometric center frequency."""
        return math.sqrt(self.low_freq * self.high_freq)


@dataclass
class MixingIssue:
    """A detected mixing issue with recommendation."""
    category: IssueCategory
    severity: IssueSeverity
    description: str
    affected_tracks: List[str]
    recommendation: str
    frequency_range: Optional[Tuple[float, float]] = None


@dataclass
class EQSuggestion:
    """EQ adjustment suggestion for a track."""
    track_name: str
    frequency: float
    gain_db: float
    q_factor: float
    filter_type: str  # "peak", "lowshelf", "highshelf", "lowpass", "highpass"
    reason: str


@dataclass
class CompressionSuggestion:
    """Compression settings suggestion for a track."""
    track_name: str
    threshold_db: float
    ratio: float
    attack_ms: float
    release_ms: float
    makeup_gain_db: float
    reason: str


@dataclass
class PanningSuggestion:
    """Panning position suggestion for a track."""
    track_name: str
    pan_position: float  # -1.0 (left) to 1.0 (right)
    reason: str


@dataclass
class LevelSuggestion:
    """Level adjustment suggestion for a track."""
    track_name: str
    gain_db: float
    reason: str


@dataclass
class MixAnalysisResult:
    """Complete analysis result for a mix."""
    issues: List[MixingIssue] = field(default_factory=list)
    eq_suggestions: List[EQSuggestion] = field(default_factory=list)
    compression_suggestions: List[CompressionSuggestion] = field(default_factory=list)
    panning_suggestions: List[PanningSuggestion] = field(default_factory=list)
    level_suggestions: List[LevelSuggestion] = field(default_factory=list)
    overall_score: float = 0.0  # 0.0-100.0
    analysis_notes: List[str] = field(default_factory=list)
    
    @property
    def critical_issue_count(self) -> int:
        """Count critical severity issues."""
        return sum(1 for i in self.issues if i.severity == IssueSeverity.CRITICAL)
    
    @property
    def warning_count(self) -> int:
        """Count warning severity issues."""
        return sum(1 for i in self.issues if i.severity == IssueSeverity.WARNING)


@dataclass
class TrackAnalysis:
    """Analysis data for a single track."""
    track_name: str
    peak_db: float = 0.0
    rms_db: float = -20.0
    lufs: float = -14.0
    crest_factor_db: float = 12.0
    frequency_spectrum: Dict[str, float] = field(default_factory=dict)
    stereo_width: float = 1.0
    phase_correlation: float = 1.0


# Standard frequency bands for analysis
FREQUENCY_BANDS = [
    FrequencyBand(20, 60, "sub_bass"),
    FrequencyBand(60, 250, "bass"),
    FrequencyBand(250, 500, "low_mids"),
    FrequencyBand(500, 2000, "mids"),
    FrequencyBand(2000, 4000, "high_mids"),
    FrequencyBand(4000, 8000, "presence"),
    FrequencyBand(8000, 20000, "air"),
]


class MixingAssistantAgent:
    """
    AI-powered mixing assistant for analysis and recommendations.
    
    Analyzes multi-track audio to detect issues and provide
    professional mixing suggestions for EQ, compression, panning, and levels.
    """

    def __init__(self):
        """Initialize Mixing Assistant Agent."""
        self.frequency_bands = FREQUENCY_BANDS
        self.target_lufs = -14.0  # Standard streaming target
        self.headroom_db = -1.0   # Target headroom
        
        print("MixingAssistantAgent initialized")

    def analyze_mix(
        self,
        tracks: Dict[str, List[List[float]]],
        sample_rate: int,
        track_metadata: Optional[Dict[str, Dict[str, Any]]] = None
    ) -> MixAnalysisResult:
        """
        Analyze a complete mix for issues and generate suggestions.
        
        Args:
            tracks: Dictionary of track_name -> audio_data [channels][samples]
            sample_rate: Audio sample rate
            track_metadata: Optional metadata per track (instrument type, role)
            
        Returns:
            MixAnalysisResult with issues and suggestions
        """
        print(f"Analyzing mix: {len(tracks)} tracks at {sample_rate}Hz")
        
        result = MixAnalysisResult()
        
        if not tracks:
            result.analysis_notes.append("No tracks to analyze")
            return result
        
        # Analyze each track
        track_analyses = {}
        for track_name, audio_data in tracks.items():
            analysis = self._analyze_track(track_name, audio_data, sample_rate)
            track_analyses[track_name] = analysis
        
        # Detect frequency collisions
        frequency_issues = self._detect_frequency_collisions(track_analyses)
        result.issues.extend(frequency_issues)
        
        # Detect phase issues
        phase_issues = self._detect_phase_issues(track_analyses)
        result.issues.extend(phase_issues)
        
        # Detect level issues
        level_issues = self._detect_level_issues(track_analyses)
        result.issues.extend(level_issues)
        
        # Detect dynamics issues
        dynamics_issues = self._detect_dynamics_issues(track_analyses)
        result.issues.extend(dynamics_issues)
        
        # Generate EQ suggestions
        result.eq_suggestions = self._generate_eq_suggestions(
            track_analyses, result.issues
        )
        
        # Generate compression suggestions
        result.compression_suggestions = self._generate_compression_suggestions(
            track_analyses
        )
        
        # Generate panning suggestions
        result.panning_suggestions = self._generate_panning_suggestions(
            track_analyses, track_metadata
        )
        
        # Generate level suggestions
        result.level_suggestions = self._generate_level_suggestions(
            track_analyses
        )
        
        # Calculate overall score
        result.overall_score = self._calculate_mix_score(result)
        
        # Add summary notes
        result.analysis_notes.append(
            f"Analyzed {len(tracks)} tracks"
        )
        result.analysis_notes.append(
            f"Found {len(result.issues)} issues "
            f"({result.critical_issue_count} critical, {result.warning_count} warnings)"
        )
        
        print(f"Analysis complete: score={result.overall_score:.1f}/100")
        return result

    def _analyze_track(
        self,
        track_name: str,
        audio_data: List[List[float]],
        sample_rate: int
    ) -> TrackAnalysis:
        """Analyze a single track."""
        analysis = TrackAnalysis(track_name=track_name)
        
        if not audio_data or not audio_data[0]:
            return analysis
        
        # Calculate peak level
        all_samples = [abs(s) for ch in audio_data for s in ch]
        peak = max(all_samples) if all_samples else 0.001
        analysis.peak_db = 20 * math.log10(peak) if peak > 0 else -96.0
        
        # Calculate RMS level
        rms = math.sqrt(sum(s**2 for s in all_samples) / len(all_samples))
        analysis.rms_db = 20 * math.log10(rms) if rms > 0 else -96.0
        
        # Calculate crest factor
        analysis.crest_factor_db = analysis.peak_db - analysis.rms_db
        
        # Estimate LUFS (simplified)
        analysis.lufs = analysis.rms_db - 0.5  # Approximation
        
        # Calculate stereo width (if stereo)
        if len(audio_data) >= 2:
            analysis.stereo_width = self._calculate_stereo_width(audio_data)
            analysis.phase_correlation = self._calculate_phase_correlation(audio_data)
        
        # Analyze frequency content (placeholder)
        analysis.frequency_spectrum = self._analyze_frequency_content(
            audio_data, sample_rate
        )
        
        return analysis

    def _calculate_stereo_width(
        self, 
        audio_data: List[List[float]]
    ) -> float:
        """Calculate stereo width (0.0=mono, 1.0=full stereo)."""
        if len(audio_data) < 2:
            return 0.0
        
        left = audio_data[0]
        right = audio_data[1]
        
        # Calculate side/mid ratio
        side_energy = sum((l - r)**2 for l, r in zip(left, right))
        mid_energy = sum((l + r)**2 for l, r in zip(left, right))
        
        if mid_energy == 0:
            return 1.0
        
        return min(1.0, math.sqrt(side_energy / mid_energy))

    def _calculate_phase_correlation(
        self, 
        audio_data: List[List[float]]
    ) -> float:
        """Calculate stereo phase correlation (-1.0 to 1.0)."""
        if len(audio_data) < 2:
            return 1.0
        
        left = audio_data[0]
        right = audio_data[1]
        
        # Pearson correlation coefficient
        n = len(left)
        if n == 0:
            return 1.0
        
        sum_l = sum(left)
        sum_r = sum(right)
        sum_lr = sum(l * r for l, r in zip(left, right))
        sum_l2 = sum(l**2 for l in left)
        sum_r2 = sum(r**2 for r in right)
        
        numerator = n * sum_lr - sum_l * sum_r
        denominator = math.sqrt((n * sum_l2 - sum_l**2) * (n * sum_r2 - sum_r**2))
        
        if denominator == 0:
            return 1.0
        
        return numerator / denominator

    def _analyze_frequency_content(
        self,
        audio_data: List[List[float]],
        sample_rate: int
    ) -> Dict[str, float]:
        """Analyze frequency content per band (placeholder)."""
        # TODO: Implement FFT-based frequency analysis
        # This is a placeholder that returns estimated values
        
        result = {}
        for band in self.frequency_bands:
            # Placeholder: would use FFT to get actual band energy
            result[band.name] = -20.0  # dB placeholder
        
        return result

    def _detect_frequency_collisions(
        self,
        track_analyses: Dict[str, TrackAnalysis]
    ) -> List[MixingIssue]:
        """Detect frequency masking/collision between tracks."""
        issues = []
        
        # TODO: Implement proper frequency collision detection
        # This would compare frequency spectra between tracks
        # and identify overlapping energy that causes masking
        
        # Placeholder: check for tracks with similar energy in same bands
        track_list = list(track_analyses.keys())
        for i, track1 in enumerate(track_list):
            for track2 in track_list[i+1:]:
                # Simplified collision check
                analysis1 = track_analyses[track1]
                analysis2 = track_analyses[track2]
                
                # Check if both tracks have significant low-mid energy
                if (analysis1.frequency_spectrum.get("low_mids", -40) > -25 and
                    analysis2.frequency_spectrum.get("low_mids", -40) > -25):
                    issues.append(MixingIssue(
                        category=IssueCategory.FREQUENCY,
                        severity=IssueSeverity.WARNING,
                        description=f"Potential frequency collision in low-mids",
                        affected_tracks=[track1, track2],
                        recommendation="Apply EQ cuts to separate frequencies",
                        frequency_range=(250, 500)
                    ))
        
        return issues

    def _detect_phase_issues(
        self,
        track_analyses: Dict[str, TrackAnalysis]
    ) -> List[MixingIssue]:
        """Detect phase correlation issues."""
        issues = []
        
        for track_name, analysis in track_analyses.items():
            if analysis.phase_correlation < 0.5:
                issues.append(MixingIssue(
                    category=IssueCategory.PHASE,
                    severity=IssueSeverity.WARNING,
                    description=f"Low phase correlation ({analysis.phase_correlation:.2f})",
                    affected_tracks=[track_name],
                    recommendation="Check for phase issues, consider mono compatibility"
                ))
            elif analysis.phase_correlation < 0:
                issues.append(MixingIssue(
                    category=IssueCategory.PHASE,
                    severity=IssueSeverity.CRITICAL,
                    description=f"Phase cancellation detected ({analysis.phase_correlation:.2f})",
                    affected_tracks=[track_name],
                    recommendation="Flip polarity or align phase to fix cancellation"
                ))
        
        return issues

    def _detect_level_issues(
        self,
        track_analyses: Dict[str, TrackAnalysis]
    ) -> List[MixingIssue]:
        """Detect level and headroom issues."""
        issues = []
        
        for track_name, analysis in track_analyses.items():
            # Check for clipping
            if analysis.peak_db > -0.1:
                issues.append(MixingIssue(
                    category=IssueCategory.LEVELS,
                    severity=IssueSeverity.CRITICAL,
                    description=f"Track is clipping (peak: {analysis.peak_db:.1f}dB)",
                    affected_tracks=[track_name],
                    recommendation="Reduce gain to avoid digital clipping"
                ))
            
            # Check for insufficient headroom
            elif analysis.peak_db > self.headroom_db:
                issues.append(MixingIssue(
                    category=IssueCategory.LEVELS,
                    severity=IssueSeverity.WARNING,
                    description=f"Low headroom (peak: {analysis.peak_db:.1f}dB)",
                    affected_tracks=[track_name],
                    recommendation=f"Leave at least {-self.headroom_db}dB headroom"
                ))
            
            # Check for very quiet tracks
            if analysis.rms_db < -40:
                issues.append(MixingIssue(
                    category=IssueCategory.LEVELS,
                    severity=IssueSeverity.INFO,
                    description=f"Track is very quiet (RMS: {analysis.rms_db:.1f}dB)",
                    affected_tracks=[track_name],
                    recommendation="Consider increasing gain or check if intentional"
                ))
        
        return issues

    def _detect_dynamics_issues(
        self,
        track_analyses: Dict[str, TrackAnalysis]
    ) -> List[MixingIssue]:
        """Detect dynamics/compression issues."""
        issues = []
        
        for track_name, analysis in track_analyses.items():
            # Check for over-compression (low crest factor)
            if analysis.crest_factor_db < 6:
                issues.append(MixingIssue(
                    category=IssueCategory.DYNAMICS,
                    severity=IssueSeverity.WARNING,
                    description=f"Track may be over-compressed (crest: {analysis.crest_factor_db:.1f}dB)",
                    affected_tracks=[track_name],
                    recommendation="Reduce compression for more dynamic range"
                ))
            
            # Check for too much dynamic range
            elif analysis.crest_factor_db > 20:
                issues.append(MixingIssue(
                    category=IssueCategory.DYNAMICS,
                    severity=IssueSeverity.INFO,
                    description=f"Wide dynamic range (crest: {analysis.crest_factor_db:.1f}dB)",
                    affected_tracks=[track_name],
                    recommendation="Consider compression for more consistent levels"
                ))
        
        return issues

    def _generate_eq_suggestions(
        self,
        track_analyses: Dict[str, TrackAnalysis],
        issues: List[MixingIssue]
    ) -> List[EQSuggestion]:
        """Generate EQ adjustment suggestions."""
        suggestions = []
        
        # Generate suggestions based on detected issues
        for issue in issues:
            if issue.category == IssueCategory.FREQUENCY and issue.frequency_range:
                freq_center = math.sqrt(issue.frequency_range[0] * issue.frequency_range[1])
                
                # Suggest cut on one of the affected tracks
                if issue.affected_tracks:
                    suggestions.append(EQSuggestion(
                        track_name=issue.affected_tracks[0],
                        frequency=freq_center,
                        gain_db=-3.0,
                        q_factor=1.5,
                        filter_type="peak",
                        reason=issue.description
                    ))
        
        # Add general suggestions based on track analysis
        for track_name, analysis in track_analyses.items():
            # High-pass filter suggestion for non-bass instruments
            # (Would check track_metadata in real implementation)
            if analysis.frequency_spectrum.get("sub_bass", -40) > -20:
                suggestions.append(EQSuggestion(
                    track_name=track_name,
                    frequency=80.0,
                    gain_db=0.0,
                    q_factor=0.7,
                    filter_type="highpass",
                    reason="Remove unnecessary sub-bass to clean up mix"
                ))
        
        return suggestions

    def _generate_compression_suggestions(
        self,
        track_analyses: Dict[str, TrackAnalysis]
    ) -> List[CompressionSuggestion]:
        """Generate compression settings suggestions."""
        suggestions = []
        
        for track_name, analysis in track_analyses.items():
            # Suggest compression for tracks with high dynamic range
            if analysis.crest_factor_db > 15:
                suggestions.append(CompressionSuggestion(
                    track_name=track_name,
                    threshold_db=analysis.peak_db - 10,
                    ratio=3.0,
                    attack_ms=10.0,
                    release_ms=100.0,
                    makeup_gain_db=3.0,
                    reason=f"Reduce dynamic range (current crest: {analysis.crest_factor_db:.1f}dB)"
                ))
        
        return suggestions

    def _generate_panning_suggestions(
        self,
        track_analyses: Dict[str, TrackAnalysis],
        track_metadata: Optional[Dict[str, Dict[str, Any]]] = None
    ) -> List[PanningSuggestion]:
        """Generate panning position suggestions."""
        suggestions = []
        
        # TODO: Implement intelligent panning based on:
        # - Track instrument type
        # - Frequency content
        # - Existing panning in the mix
        # - Genre conventions
        
        # Placeholder: suggest centered low-frequency content
        for track_name, analysis in track_analyses.items():
            if analysis.frequency_spectrum.get("bass", -40) > -15:
                suggestions.append(PanningSuggestion(
                    track_name=track_name,
                    pan_position=0.0,
                    reason="Bass-heavy content should be centered for mono compatibility"
                ))
        
        return suggestions

    def _generate_level_suggestions(
        self,
        track_analyses: Dict[str, TrackAnalysis]
    ) -> List[LevelSuggestion]:
        """Generate level adjustment suggestions."""
        suggestions = []
        
        for track_name, analysis in track_analyses.items():
            # Suggest gain reduction for hot tracks
            if analysis.peak_db > -3:
                reduction = analysis.peak_db - (-6)
                suggestions.append(LevelSuggestion(
                    track_name=track_name,
                    gain_db=-reduction,
                    reason=f"Reduce level for proper headroom (peak: {analysis.peak_db:.1f}dB)"
                ))
            
            # Suggest gain increase for quiet tracks
            elif analysis.rms_db < -35:
                suggestions.append(LevelSuggestion(
                    track_name=track_name,
                    gain_db=10.0,
                    reason=f"Increase level (current RMS: {analysis.rms_db:.1f}dB)"
                ))
        
        return suggestions

    def _calculate_mix_score(self, result: MixAnalysisResult) -> float:
        """Calculate overall mix quality score (0-100)."""
        score = 100.0
        
        # Deduct for issues
        score -= result.critical_issue_count * 15
        score -= result.warning_count * 5
        score -= sum(1 for i in result.issues if i.severity == IssueSeverity.INFO) * 1
        
        return max(0.0, min(100.0, score))


def print_analysis_report(result: MixAnalysisResult) -> None:
    """Print a formatted analysis report."""
    print("\n" + "=" * 60)
    print("🎛️  MIXING ASSISTANT ANALYSIS REPORT")
    print("=" * 60)
    
    print(f"\nOverall Score: {result.overall_score:.1f}/100")
    print(f"Issues Found: {len(result.issues)}")
    print(f"  - Critical: {result.critical_issue_count}")
    print(f"  - Warnings: {result.warning_count}")
    
    if result.issues:
        print("\n📋 ISSUES:")
        for issue in result.issues:
            emoji = {"critical": "🚨", "warning": "⚠️", "info": "ℹ️"}
            print(f"  {emoji.get(issue.severity.value, '•')} [{issue.category.value}] {issue.description}")
            print(f"      Tracks: {', '.join(issue.affected_tracks)}")
            print(f"      Recommendation: {issue.recommendation}")
    
    if result.eq_suggestions:
        print("\n🎚️  EQ SUGGESTIONS:")
        for eq in result.eq_suggestions:
            print(f"  {eq.track_name}: {eq.filter_type} at {eq.frequency:.0f}Hz "
                  f"({eq.gain_db:+.1f}dB, Q={eq.q_factor:.1f})")
            print(f"      Reason: {eq.reason}")
    
    if result.compression_suggestions:
        print("\n🔊 COMPRESSION SUGGESTIONS:")
        for comp in result.compression_suggestions:
            print(f"  {comp.track_name}: {comp.ratio:.1f}:1 @ {comp.threshold_db:.1f}dB "
                  f"(A:{comp.attack_ms:.0f}ms R:{comp.release_ms:.0f}ms)")
            print(f"      Reason: {comp.reason}")
    
    if result.panning_suggestions:
        print("\n🔀 PANNING SUGGESTIONS:")
        for pan in result.panning_suggestions:
            pos = "center" if pan.pan_position == 0 else \
                  f"{abs(pan.pan_position)*100:.0f}% {'left' if pan.pan_position < 0 else 'right'}"
            print(f"  {pan.track_name}: {pos}")
            print(f"      Reason: {pan.reason}")
    
    if result.level_suggestions:
        print("\n📊 LEVEL SUGGESTIONS:")
        for level in result.level_suggestions:
            print(f"  {level.track_name}: {level.gain_db:+.1f}dB")
            print(f"      Reason: {level.reason}")
    
    print("\n" + "=" * 60)


def main():
    """Main entry point for mixing assistant agent."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='MixingAssistantAgent - AI Mixing Analysis'
    )
    parser.add_argument('--demo', action='store_true',
                       help='Run demo analysis with sample data')
    parser.add_argument('--info', action='store_true',
                       help='Show agent info')
    
    args = parser.parse_args()
    
    agent = MixingAssistantAgent()
    
    if args.info:
        print("\nMixingAssistantAgent Information:")
        print("=" * 50)
        print("Purpose: AI-powered mixing analysis and suggestions")
        print("Analysis categories: frequency, dynamics, stereo, phase, levels")
        print("Suggestions: EQ, compression, panning, level adjustments")
        print()
        return
    
    if args.demo:
        # Generate demo tracks with sample data
        import random
        random.seed(42)
        
        sample_rate = 44100
        duration = 1.0  # seconds
        num_samples = int(sample_rate * duration)
        
        demo_tracks = {
            "kick": [[random.uniform(-0.8, 0.8) for _ in range(num_samples)] for _ in range(2)],
            "bass": [[random.uniform(-0.6, 0.6) for _ in range(num_samples)] for _ in range(2)],
            "vocals": [[random.uniform(-0.5, 0.5) for _ in range(num_samples)] for _ in range(2)],
            "synth": [[random.uniform(-0.4, 0.4) for _ in range(num_samples)] for _ in range(2)],
        }
        
        print("\nRunning demo analysis on sample tracks...")
        result = agent.analyze_mix(demo_tracks, sample_rate)
        print_analysis_report(result)
        return
    
    print("\nMixingAssistantAgent ready")
    print("Use --demo to run a demo analysis")
    print("Use --info for more information")


if __name__ == "__main__":
    main()
