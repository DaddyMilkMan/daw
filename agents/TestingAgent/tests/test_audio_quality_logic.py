import sys
import os
import unittest
import numpy as np

# Add repo root to path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../..')))

from agents.TestingAgent.testing_agent import TestingAgent

class TestAudioQualityLogic(unittest.TestCase):
    def setUp(self):
        self.agent = TestingAgent()
        # Ensure we are using 48kHz as defined in the agent's test method defaults
        self.sample_rate = 48000

    def test_thd_clean_sine(self):
        print("\n--- Test: Clean Sine (Low THD) ---")
        # Processor: Identity
        processor = lambda x: x
        metrics = self.agent.test_audio_quality(processor)
        print(f"Result: THD={metrics.thd_percent}%")
        self.assertLess(metrics.thd_percent, 0.001)

    def test_thd_distortion(self):
        print("\n--- Test: Cubic Distortion (High THD) ---")
        # Processor: Cubic soft clipping (adds odd harmonics)
        # f(x) = x - 0.1 * x^3
        # For x=sin(t), x^3 contains 3rd harmonic
        def distortion(x):
            return x - 0.1 * x**3

        metrics = self.agent.test_audio_quality(distortion)
        print(f"Result: THD={metrics.thd_percent}%")
        # Theoretical calculation could be done, but we just expect significant THD
        self.assertGreater(metrics.thd_percent, 1.0)

    def test_snr_clean(self):
        print("\n--- Test: Clean Signal (High SNR) ---")
        processor = lambda x: x
        metrics = self.agent.test_audio_quality(processor)
        print(f"Result: SNR={metrics.snr_db}dB")
        self.assertGreater(metrics.snr_db, 120.0)

    def test_snr_noisy(self):
        print("\n--- Test: Noisy Signal (Medium SNR) ---")
        def noisy(x):
            # Add white noise at approx -40dB level relative to full scale sine
            # Sine RMS = 0.707
            # Noise RMS = 0.01
            # SNR approx 20*log10(0.707/0.01) = 37dB
            noise = np.random.normal(0, 0.01, size=len(x))
            return x + noise

        metrics = self.agent.test_audio_quality(noisy)
        print(f"Result: SNR={metrics.snr_db}dB")
        self.assertTrue(30.0 < metrics.snr_db < 45.0, f"SNR {metrics.snr_db} out of expected range (30-45)")

    def test_frequency_response_flat(self):
        print("\n--- Test: Flat Response ---")
        processor = lambda x: x
        metrics = self.agent.test_audio_quality(processor)
        print(f"Result: Flat={metrics.frequency_response_flat}")
        self.assertTrue(metrics.frequency_response_flat)

    def test_frequency_response_lowpass(self):
        print("\n--- Test: Lowpass (Non-Flat Response) ---")
        # Simulate simple lowpass via moving average
        # This will roll off high frequencies significantly
        def lowpass(x):
            return np.convolve(x, np.ones(10)/10.0, mode='same')

        metrics = self.agent.test_audio_quality(lowpass)
        print(f"Result: Flat={metrics.frequency_response_flat}")
        self.assertFalse(metrics.frequency_response_flat)

    def test_artifacts_clean(self):
        print("\n--- Test: Artifacts Clean ---")
        processor = lambda x: x
        metrics = self.agent.test_audio_quality(processor)
        self.assertTrue(metrics.no_clicks_pops)

    def test_artifacts_click(self):
        print("\n--- Test: Click Artifact ---")
        def clicky(x):
            y = x.copy()
            if len(y) > 10000:
                y[10000] += 5.0 # Giant click
            return y

        metrics = self.agent.test_audio_quality(clicky)
        self.assertFalse(metrics.no_clicks_pops)

    def test_artifacts_dc(self):
        print("\n--- Test: DC Offset Artifact ---")
        def dc_offset(x):
            return x + 0.5 # Huge DC

        metrics = self.agent.test_audio_quality(dc_offset)
        self.assertFalse(metrics.no_clicks_pops)

if __name__ == '__main__':
    unittest.main()
