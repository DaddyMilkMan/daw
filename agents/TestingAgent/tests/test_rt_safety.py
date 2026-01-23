
import unittest
from pathlib import Path
import tempfile
import shutil
import os
import sys

# Add project root to sys.path so we can import agents
sys.path.append(str(Path(__file__).parents[3]))

from agents.TestingAgent.testing_agent import TestingAgent, TestStatus

class TestRTSafety(unittest.TestCase):
    def setUp(self):
        self.agent = TestingAgent()
        self.temp_dir = tempfile.mkdtemp()
        self.temp_path = Path(self.temp_dir)

    def tearDown(self):
        shutil.rmtree(self.temp_dir)

    def test_rt_safety_detection(self):
        cpp_content = """
        // RT-SAFE
        void safeFunction() {
            int x = 0;
        }

        void processBlock(float* buffer) {
            // Unsafe: Allocation (Container)
            std::vector<float> v;
            v.resize(100);

            // Unsafe: Lock
            std::mutex m;
            std::lock_guard<std::mutex> lock(m);

            // Unsafe: I/O
            std::cout << "Log" << std::endl;
            printf("Log");

            // Safe: Suppressed
            std::cout << "Debug" << std::endl; // NOLINT

            // Unsafe: String
            std::string s = "test";

            // Unsafe: MessageManagerLock
            juce::MessageManagerLock mmLock;

            // Unsafe: Threading
            std::async(std::launch::async, [](){});
            juce::Thread::launch([](){});

            // Unsafe: Raw Allocation
            float* ptr = (float*)malloc(1024);
            int* ptr2 = new int[10];
        }

        // This function is NOT real-time, so unsafe ops are ignored
        void loadFile() {
             std::vector<float> v;
             v.resize(1000);
        }
        """

        test_file = self.temp_path / "test_safety.cpp"
        test_file.write_text(cpp_content)

        results = self.agent.validate_rt_safety(source_files=[test_file])

        # Helper to find if a line triggered a specific violation
        def has_violation(line_num, violation_type):
            for r in results:
                # The name format is "filename::funcname::L<num>"
                if f"::L{line_num}" in r.name and violation_type in r.error_message:
                    return True
            return False

        # Verify detections
        # Note: Line numbers are based on the string above.
        # ...
        # Line 7: void processBlock... {
        # Line 8: // Unsafe: Allocation (Container)
        # Line 9: std::vector<float> v;
        # Line 10: v.resize(100);
        self.assertTrue(has_violation(10, "Container Mutation"), "Failed to detect Container Mutation")

        # Line 13: std::mutex m; -> Lock check flags 'mutex'
        self.assertTrue(has_violation(13, "Lock"), "Failed to detect std::mutex declaration")

        # Line 14: std::lock_guard...
        self.assertTrue(has_violation(14, "Lock"), "Failed to detect lock_guard")

        # Line 17: std::cout...
        self.assertTrue(has_violation(17, "I/O"), "Failed to detect std::cout")

        # Line 18: printf...
        self.assertTrue(has_violation(18, "I/O"), "Failed to detect printf")

        # Line 21: ... // NOLINT
        self.assertFalse(has_violation(21, "I/O"), "Suppression failed for NOLINT")

        # Line 24: std::string s...
        self.assertTrue(has_violation(24, "String Usage"), "Failed to detect std::string")

        # Line 27: juce::MessageManagerLock mmLock;
        self.assertTrue(has_violation(27, "Lock"), "Failed to detect MessageManagerLock")

        # Line 30: std::async...
        self.assertTrue(has_violation(30, "Threading"), "Failed to detect std::async")

        # Line 31: juce::Thread::launch...
        self.assertTrue(has_violation(31, "Threading"), "Failed to detect juce::Thread::launch")

        # Line 34: malloc
        self.assertTrue(has_violation(34, "Allocation"), "Failed to detect malloc")

        # Line 35: new
        self.assertTrue(has_violation(35, "Allocation"), "Failed to detect new")

        # Verify context
        # loadFile is at line 39.
        # Line 41: v.resize(1000);
        self.assertFalse(has_violation(41, "Container Mutation"), "False positive in non-RT function")

if __name__ == '__main__':
    unittest.main()
