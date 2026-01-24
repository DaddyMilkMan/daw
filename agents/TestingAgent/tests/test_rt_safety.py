import unittest
import sys
import os
from pathlib import Path
import tempfile

# Add parent directory to path to import testing_agent
sys.path.append(str(Path(__file__).parent.parent))

from testing_agent import TestingAgent, TestStatus, TestType

class TestRTSafety(unittest.TestCase):
    def setUp(self):
        self.agent = TestingAgent()
        self.temp_dir = tempfile.TemporaryDirectory()
        self.temp_path = Path(self.temp_dir.name)

    def tearDown(self):
        self.temp_dir.cleanup()

    def create_dummy_file(self, filename, content):
        path = self.temp_path / filename
        path.write_text(content, encoding='utf-8')
        return path

    def test_detect_malloc(self):
        content = """
        void processBlock(AudioBuffer& buffer) {
            void* ptr = malloc(1024);
        }
        """
        path = self.create_dummy_file("unsafe_malloc.cpp", content)
        results = self.agent.validate_rt_safety([path])

        # Current implementation groups all allocations under "Allocation"
        self.assertTrue(any("Allocation" in r.error_message for r in results),
                        "Should detect malloc in processBlock")

    def test_detect_mutex(self):
        content = """
        void processBlock(AudioBuffer& buffer) {
            std::lock_guard<std::mutex> lock(mtx);
        }
        """
        path = self.create_dummy_file("unsafe_mutex.cpp", content)
        results = self.agent.validate_rt_safety([path])

        self.assertTrue(any("Lock" in r.error_message for r in results),
                        "Should detect mutex lock in processBlock")

    def test_detect_rt_safe_tag_violation(self):
        content = """
        // RT-SAFE
        void explicitSafeFunction() {
            int* p = new int[10];
        }
        """
        path = self.create_dummy_file("unsafe_tag.cpp", content)
        results = self.agent.validate_rt_safety([path])

        self.assertTrue(any("Allocation" in r.error_message for r in results),
                        "Should detect new in tagged RT-SAFE function")

    def test_safe_code(self):
        content = """
        void processBlock(AudioBuffer& buffer) noexcept {
            float x = 0.0f;
            for(int i=0; i<buffer.getNumSamples(); ++i) {
                x += 1.0f;
            }
        }
        """
        path = self.create_dummy_file("safe.cpp", content)
        results = self.agent.validate_rt_safety([path])

        self.assertEqual(len(results), 0, f"Should not report errors for safe code. Found: {results}")

    def test_missing_noexcept(self):
        content = """
        void processBlock(AudioBuffer& buffer) {
            // Safe code but missing noexcept
        }
        """
        path = self.create_dummy_file("missing_noexcept.cpp", content)
        results = self.agent.validate_rt_safety([path])

        found = any("noexcept" in r.error_message for r in results)
        self.assertTrue(found, "Should detect missing noexcept specifier")

    def test_intervening_declaration(self):
        content = """
        // RT-SAFE
        void helper() noexcept; // Intervening declaration
        void unsafe() {
            // This function is missing noexcept
        }
        """
        path = self.create_dummy_file("intervening.cpp", content)
        results = self.agent.validate_rt_safety([path])

        # Should detect missing noexcept in unsafe()
        found = any("noexcept" in r.error_message for r in results)
        self.assertTrue(found, "Should detect missing noexcept specifier even with intervening declaration")

if __name__ == '__main__':
    unittest.main()
