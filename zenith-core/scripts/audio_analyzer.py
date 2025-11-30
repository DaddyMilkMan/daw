#!/usr/bin/env python3
"""
Audio Analyzer for Zenith DAW
Created: 2025-11-29
Author: Dr. Maya Rodriguez (Lead Integration)

Uses librosa to extract comprehensive audio features for AI analysis.
This script is called by the DAW to analyze exported audio files.
"""

import sys
import json
import numpy as np

try:
    import librosa
    import librosa.display
except ImportError:
    print(json.dumps({
        "error": "librosa not installed. Install with: pip install librosa",
        "success": False
    }))
    sys.exit(1)


def analyze_audio(audio_path):
    """
    Analyze audio file and extract comprehensive features.
    
    Args:
        audio_path: Path to audio file (WAV, MP3, etc.)
        
    Returns:
        Dictionary with audio features
    """
    try:
        # Load audio file
        y, sr = librosa.load(audio_path, sr=None, mono=False)
        
        # Convert to mono for analysis
        if len(y.shape) > 1:
            y_mono = librosa.to_mono(y)
            is_stereo = True
        else:
            y_mono = y
            is_stereo = False
        
        # Duration
        duration = librosa.get_duration(y=y_mono, sr=sr)
        
        # ======================================================================
        # LOUDNESS ANALYSIS
        # ======================================================================
        
        # RMS (Root Mean Square) - overall loudness
        rms = librosa.feature.rms(y=y_mono)[0]
        rms_db = librosa.amplitude_to_db(rms, ref=np.max)
        
        avg_rms_db = float(np.mean(rms_db))
        peak_rms_db = float(np.max(rms_db))
        
        # Peak amplitude
        peak_amplitude = float(np.max(np.abs(y_mono)))
        peak_db = float(librosa.amplitude_to_db(peak_amplitude))
        
        # Dynamic range
        dynamic_range = float(peak_rms_db - avg_rms_db)
        
        # ======================================================================
        # SPECTRAL ANALYSIS
        # ======================================================================
        
        # Spectral centroid (brightness)
        spectral_centroid = librosa.feature.spectral_centroid(y=y_mono, sr=sr)[0]
        avg_spectral_centroid = float(np.mean(spectral_centroid))
        
        # Spectral rolloff (high frequency content)
        spectral_rolloff = librosa.feature.spectral_rolloff(y=y_mono, sr=sr)[0]
        avg_spectral_rolloff = float(np.mean(spectral_rolloff))
        
        # Spectral bandwidth
        spectral_bandwidth = librosa.feature.spectral_bandwidth(y=y_mono, sr=sr)[0]
        avg_spectral_bandwidth = float(np.mean(spectral_bandwidth))
        
        # Zero crossing rate (noisiness)
        zcr = librosa.feature.zero_crossing_rate(y_mono)[0]
        avg_zcr = float(np.mean(zcr))
        
        # ======================================================================
        # FREQUENCY ANALYSIS
        # ======================================================================
        
        # Compute spectrogram
        D = np.abs(librosa.stft(y_mono))
        
        # Frequency bins
        freqs = librosa.fft_frequencies(sr=sr)
        
        # Average power per frequency band
        freq_bands = {
            "sub_bass": (20, 60),
            "bass": (60, 250),
            "low_mids": (250, 500),
            "mids": (500, 2000),
            "high_mids": (2000, 4000),
            "presence": (4000, 6000),
            "brilliance": (6000, 20000)
        }
        
        band_powers = {}
        for band_name, (low, high) in freq_bands.items():
            # Find frequency bins in this range
            band_mask = (freqs >= low) & (freqs <= high)
            band_power = np.mean(D[band_mask, :])
            band_powers[band_name] = float(librosa.amplitude_to_db(band_power))
        
        # Find dominant frequencies
        avg_spectrum = np.mean(D, axis=1)
        peak_freq_idx = np.argmax(avg_spectrum)
        dominant_frequency = float(freqs[peak_freq_idx])
        
        # ======================================================================
        # RHYTHM ANALYSIS
        # ======================================================================
        
        # Tempo detection
        tempo, beats = librosa.beat.beat_track(y=y_mono, sr=sr)
        tempo = float(tempo)
        
        # Beat strength
        onset_env = librosa.onset.onset_strength(y=y_mono, sr=sr)
        avg_beat_strength = float(np.mean(onset_env))
        
        # ======================================================================
        # STEREO ANALYSIS (if stereo)
        # ======================================================================
        
        stereo_info = {}
        if is_stereo:
            left = y[0]
            right = y[1]
            
            # Stereo width (correlation between channels)
            correlation = float(np.corrcoef(left, right)[0, 1])
            stereo_info["correlation"] = correlation
            stereo_info["width"] = "wide" if correlation < 0.7 else "narrow"
            
            # Balance (L/R level difference)
            left_rms = float(np.sqrt(np.mean(left**2)))
            right_rms = float(np.sqrt(np.mean(right**2)))
            balance = float((right_rms - left_rms) / (right_rms + left_rms + 1e-10))
            stereo_info["balance"] = balance  # -1 = left, 0 = center, 1 = right
        
        # ======================================================================
        # TIMBRE ANALYSIS
        # ======================================================================
        
        # MFCCs (Mel-frequency cepstral coefficients) - timbre
        mfccs = librosa.feature.mfcc(y=y_mono, sr=sr, n_mfcc=13)
        avg_mfccs = [float(np.mean(mfcc)) for mfcc in mfccs]
        
        # ======================================================================
        # COMPILE RESULTS
        # ======================================================================
        
        results = {
            "success": True,
            "file_info": {
                "path": audio_path,
                "duration_seconds": float(duration),
                "sample_rate": int(sr),
                "is_stereo": is_stereo
            },
            "loudness": {
                "average_rms_db": avg_rms_db,
                "peak_rms_db": peak_rms_db,
                "peak_amplitude_db": peak_db,
                "dynamic_range_db": dynamic_range,
                "headroom_db": float(0.0 - peak_db)  # How much room to 0dB
            },
            "spectral": {
                "centroid_hz": avg_spectral_centroid,
                "rolloff_hz": avg_spectral_rolloff,
                "bandwidth_hz": avg_spectral_bandwidth,
                "zero_crossing_rate": avg_zcr,
                "brightness": "bright" if avg_spectral_centroid > 3000 else "dark"
            },
            "frequency_bands": band_powers,
            "dominant_frequency_hz": dominant_frequency,
            "rhythm": {
                "tempo_bpm": tempo,
                "beat_strength": avg_beat_strength
            },
            "stereo": stereo_info if is_stereo else None,
            "timbre": {
                "mfcc_coefficients": avg_mfccs[:5]  # First 5 for brevity
            }
        }
        
        return results
        
    except Exception as e:
        return {
            "success": False,
            "error": str(e)
        }


def main():
    """Main entry point"""
    if len(sys.argv) < 2:
        print(json.dumps({
            "error": "Usage: python audio_analyzer.py <audio_file>",
            "success": False
        }))
        sys.exit(1)
    
    audio_path = sys.argv[1]
    
    # Analyze audio
    results = analyze_audio(audio_path)
    
    # Output as JSON
    print(json.dumps(results, indent=2))
    
    # Exit with appropriate code
    sys.exit(0 if results["success"] else 1)


if __name__ == "__main__":
    main()
