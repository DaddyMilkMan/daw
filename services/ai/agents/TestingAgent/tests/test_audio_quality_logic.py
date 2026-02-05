
import unittest
import numpy as np
import scipy.signal as signal
import sys
from pathlib import Path

# Add parent directory to path to import testing_agent
sys.path.append(str(Path(__file__).parent.parent))

from testing_agent import TestingAgent, AudioQualityMetrics

class TestAudioQualityLogic(unittest.TestCase):
    def setUp(self):
        self.agent = TestingAgent()
        self.sample_rate = 48000

    def test_thd_pure_sine(self):
        """Verify THD is near zero for a pure sine wave."""
        # This uses the agent's internal helper directly if accessible, or via a dummy processor
        # Logic: Create a processor that just returns the input
        def pass_through(signal_in: np.ndarray) -> np.ndarray:
            return signal_in

        metrics = self.agent.test_audio_quality(pass_through)
        # Expect very low THD
        self.assertLess(metrics.thd_percent, 0.01)

    def test_thd_with_distortion(self):
        """Verify THD measurement with added harmonic."""
        # Inject 3rd harmonic at -20dB (10% amplitude)
        # -20dB = 0.1 amplitude relative to fundamental 1.0
        # THD ~ 10%

        def distort(signal_in: np.ndarray) -> np.ndarray:
            # Re-synthesize or add?
            # Ideally we want to test the measurement logic, so we can mock the internal generation
            # or rely on the agent sending a known sine.
            # But the agent generates its own sine.
            # If we want to test the agent's MEASUREMENT capability, we need to distort the agent's signal.

            # Simple soft clipping distortion to generate odd harmonics
            return np.tanh(signal_in * 1.5) / np.tanh(1.5)

        metrics = self.agent.test_audio_quality(distort)
        self.assertGreater(metrics.thd_percent, 1.0)

    def test_snr_pure(self):
        """Verify high SNR for clean signal."""
        def pass_through(signal_in: np.ndarray) -> np.ndarray:
            return signal_in

        metrics = self.agent.test_audio_quality(pass_through)
        self.assertGreater(metrics.snr_db, 80.0)

    def test_snr_noisy(self):
        """Verify SNR with added noise."""
        def add_noise(signal_in: np.ndarray) -> np.ndarray:
            # Add noise at -40dB
            noise = np.random.normal(0, 0.01, len(signal_in)).astype(np.float32)
            return signal_in + noise

        metrics = self.agent.test_audio_quality(add_noise)
        # Noise floor is approx -40dB (amplitude 0.01)
        # Signal is approx 0dB (amplitude 1.0)
        # SNR should be around 40dB
        self.assertGreater(metrics.snr_db, 30.0)
        self.assertLess(metrics.snr_db, 50.0)

    def test_frequency_response_flat(self):
        """Verify flat response."""
        def pass_through(signal_in: np.ndarray) -> np.ndarray:
            return signal_in

        metrics = self.agent.test_audio_quality(pass_through)
        self.assertTrue(metrics.frequency_response_flat)

    def test_frequency_response_lowpass(self):
        """Verify non-flat response (Low Pass Filter)."""
        def low_pass(signal_in: np.ndarray) -> np.ndarray:
            sos = signal.butter(4, 1000, 'lp', fs=self.sample_rate, output='sos')
            return signal.sosfilt(sos, signal_in).astype(np.float32)

        metrics = self.agent.test_audio_quality(low_pass)
        self.assertFalse(metrics.frequency_response_flat)

    def test_frequency_response_notch(self):
        """Verify non-flat response (Notch Filter at 1kHz)."""
        def notch(signal_in: np.ndarray) -> np.ndarray:
            # Notch at 1kHz Q=10
            # Note: The agent generates its own test signals.
            # If the agent uses a chirp, this will affect the 1kHz region.
            # If the agent uses 100Hz and 10kHz tones, this filter will have little effect (unity gain elsewhere).
            b, a = signal.iirnotch(1000, 10, self.sample_rate)
            return signal.lfilter(b, a, signal_in).astype(np.float32)

        metrics = self.agent.test_audio_quality(notch)
        self.assertFalse(metrics.frequency_response_flat, "Should detect 1kHz notch as not flat")

    def test_latency_compensation(self):
        """Verify metrics work even with latency (delayed signal)."""
        def delayed_pass_through(signal_in: np.ndarray) -> np.ndarray:
            # Delay by 1000 samples
            delay = 1000
            out = np.pad(signal_in, (delay, 0), mode='constant')[:len(signal_in)]
            return out

        metrics = self.agent.test_audio_quality(delayed_pass_through)
        # Should still be considered flat and low THD
        self.assertTrue(metrics.frequency_response_flat)
        self.assertLess(metrics.thd_percent, 0.01)

    def test_clicks_pops(self):
        """Verify click detection."""
        def inject_click(signal_in: np.ndarray) -> np.ndarray:
            out = signal_in.copy()
            # Inject click
            mid = len(out) // 2
            out[mid] += 0.9 # Large jump
            return out

        metrics = self.agent.test_audio_quality(inject_click)
        self.assertFalse(metrics.no_clicks_pops)

    def test_dc_offset(self):
        """Verify DC offset detection."""
        def inject_dc(signal_in: np.ndarray) -> np.ndarray:
            return signal_in + 0.1 # Significant DC

        metrics = self.agent.test_audio_quality(inject_dc)
        # Note: Depending on implementation, checking DC might fail on Sine waves if not averaged properly,
        # but the agent should handle it.
        # But wait, logic combines clicks and DC into `no_clicks_pops`.
        # The current `test_audio_quality` returns `no_clicks_pops` as a boolean.
        # I should probably split them or check the result.
        # Current logic: `no_clicks_pops=no_clicks and valid_dc`

        self.assertFalse(metrics.no_clicks_pops)

if __name__ == '__main__':
    unittest.main()
