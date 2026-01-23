import unittest
import sys
import re
from pathlib import Path
from unittest.mock import MagicMock, patch

# Add parent directory to path to import testing_agent
sys.path.append(str(Path(__file__).parent.parent))

from testing_agent import TestingAgent, TestStatus, TestType

class TestRTSafety(unittest.TestCase):
    def setUp(self):
        self.agent = TestingAgent()

    def test_mask_comments_and_strings(self):
        source = 'code; // comment\n"string"; /* block */ code'
        masked = self.agent._mask_comments_and_strings(source)
        # Check that comments and strings are replaced by spaces but structure preserved
        # "code;           \n "      ";             code"
        self.assertEqual(len(masked), len(source))
        self.assertEqual(masked[0:5], "code;")
        self.assertEqual(masked[6:16].strip(), "") # comment area empty
        self.assertEqual(masked[18:24].strip(), "") # string content empty
        self.assertEqual(masked[27:35].strip(), "") # block comment empty
        self.assertTrue("code" in masked[36:])

    def test_mask_preserves_rt_safe(self):
        source = 'code; // RT-SAFE ignore'
        masked = self.agent._mask_comments_and_strings(source, preserve_rt_safe=True)
        self.assertIn("// RT-SAFE", masked)

        masked_no_preserve = self.agent._mask_comments_and_strings(source, preserve_rt_safe=False)
        self.assertNotIn("// RT-SAFE", masked_no_preserve)

    def test_extract_rt_function_bodies(self):
        source = """
        void processBlock(AudioBuffer& buffer) {
            // RT-SAFE
            buffer.clear();
        }

        void otherFunction() {
            unsafe();
        }

        void getNextAudioBlock() {
             critical();
        }
        """
        functions = self.agent._extract_rt_function_bodies(source)
        self.assertEqual(len(functions), 2)

        names = [f[0] for f in functions]
        self.assertIn("processBlock", names)
        self.assertIn("getNextAudioBlock", names)

    @patch('pathlib.Path.read_text')
    @patch('pathlib.Path.rglob')
    def test_validate_rt_safety_violations(self, mock_rglob, mock_read_text):
        # Mock file system
        file_path = MagicMock(spec=Path)
        file_path.name = "TestAudio.cpp"
        file_path.read_text.return_value = """
        void processBlock() {
            std::cout << "log";
            std::vector<int> v;
            v.push_back(1);
            v.clear();
            new int[5];
            // NOLINT new int[1];
        }
        """

        # When rglob called (mocking scanning source files)
        mock_rglob.return_value = [file_path]

        # We need to ensure the path doesn't look like a build artifact
        file_path.__str__.return_value = "src/TestAudio.cpp"

        results = self.agent.validate_rt_safety(source_files=[file_path])

        self.assertEqual(len(results), 4)

        # Verify types of violations found
        messages = [r.error_message for r in results]
        self.assertTrue(any("I/O" in m for m in messages))
        self.assertTrue(any("Container Mutation" in m for m in messages)) # push_back
        self.assertTrue(any("Container Mutation" in m for m in messages)) # clear
        self.assertTrue(any("Allocation" in m for m in messages)) # new

        # Ensure NOLINT prevented 5th error
        self.assertFalse(any("NOLINT" in m for m in messages))

    @patch('pathlib.Path.read_text')
    def test_validate_rt_safety_ignores_safe_functions(self, mock_read_text):
        file_path = MagicMock(spec=Path)
        file_path.name = "Safe.cpp"
        file_path.read_text.return_value = """
        void randomFunction() {
            new int[5]; // Unsafe but not in RT callback
        }
        """
        file_path.__str__.return_value = "src/Safe.cpp"

        results = self.agent.validate_rt_safety(source_files=[file_path])
        self.assertEqual(len(results), 0)

if __name__ == '__main__':
    unittest.main()
