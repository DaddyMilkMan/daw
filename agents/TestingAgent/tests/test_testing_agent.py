import unittest
import sys
import numpy as np
from pathlib import Path
from dataclasses import dataclass

# Add parent directory to path to import testing_agent
sys.path.append(str(Path(__file__).parent.parent))

from testing_agent import TestingAgent, AudioQualityMetrics

class PerfectGainProcessor:
    def process(self, input_signal: np.ndarray) -> np.ndarray:
        return input_signal.copy()

class GainProcessor:
    def __init__(self, gain_db):
        self.gain = 10 ** (gain_db / 20.0)

    def process(self, input_signal: np.ndarray) -> np.ndarray:
        return input_signal * self.gain

class SoftClipperProcessor:
    def process(self, input_signal: np.ndarray) -> np.ndarray:
        # Simple soft clipping: tanh
        # Drive it a bit to ensure THD
        drive = 2.0
        return np.tanh(input_signal * drive)

class NoisyProcessor:
    def __init__(self, noise_level_db):
        self.noise_amp = 10 ** (noise_level_db / 20.0)

    def process(self, input_signal: np.ndarray) -> np.ndarray:
        noise = np.random.normal(0, self.noise_amp, input_signal.shape)
        return input_signal + noise

class FilterProcessor:
    """Simulates a non-flat frequency response (Low Pass)"""
    def process(self, input_signal: np.ndarray) -> np.ndarray:
        # Simple IIR Low Pass Filter
        output = np.zeros_like(input_signal)
        alpha = 0.1 # Very low cutoff for clear effect
        last = 0.0
        for i in range(len(input_signal)):
            last = last + alpha * (input_signal[i] - last)
            output[i] = last
        return output

class TestAudioQuality(unittest.TestCase):
    def setUp(self):
        self.agent = TestingAgent()

    def test_perfect_processor(self):
        processor = PerfectGainProcessor()
        metrics = self.agent.test_audio_quality(processor.process)

        print(f"\nPerfect Processor Metrics: {metrics}")

        # Expect near 0 THD
        self.assertLess(metrics.thd_percent, 0.01)
        # Expect high SNR (essentially infinite, but limited by float precision)
        self.assertGreater(metrics.snr_db, 120.0)
        # Expect flat response
        self.assertTrue(metrics.frequency_response_flat)
        # Expect no artifacts
        self.assertTrue(metrics.no_clicks_pops)

    def test_soft_clipper_thd(self):
        processor = SoftClipperProcessor()
        metrics = self.agent.test_audio_quality(processor.process)

        print(f"\nSoft Clipper Metrics: {metrics}")

        # Expect significant THD (tanh introduces odd harmonics)
        self.assertGreater(metrics.thd_percent, 1.0)
        # It's a non-linear process, but deterministic, so SNR should still be high (no random noise)
        self.assertGreater(metrics.snr_db, 80.0)

    def test_noisy_processor_snr(self):
        # Inject noise at -60dB
        processor = NoisyProcessor(-60.0)
        metrics = self.agent.test_audio_quality(processor.process)

        print(f"\nNoisy Processor Metrics: {metrics}")

        # SNR should be around 60dB (Signal is around 0dBFS)
        # Allow some margin
        self.assertGreater(metrics.snr_db, 50.0)
        self.assertLess(metrics.snr_db, 70.0)

    def test_filter_frequency_response(self):
        processor = FilterProcessor()
        metrics = self.agent.test_audio_quality(processor.process)

        print(f"\nFilter Processor Metrics: {metrics}")

        # Should NOT be flat
        self.assertFalse(metrics.frequency_response_flat)

if __name__ == '__main__':
    unittest.main()
