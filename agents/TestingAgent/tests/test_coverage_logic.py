import unittest
import sys
from pathlib import Path
from unittest.mock import MagicMock, patch

# Add parent directory to path to import testing_agent
sys.path.append(str(Path(__file__).parent.parent))

from testing_agent import GcovParser, TestingAgent, CoverageReport

class TestGcovParser(unittest.TestCase):
    def test_parse_simple_coverage(self):
        content = """
        -:    0:Source:test.cpp
        -:    1:#include <iostream>
        1:    2:int main() {
    #####:    3:    return 1;
        1:    4:}
        """
        parser = GcovParser()
        stats = parser.parse_file(content)

        self.assertEqual(stats['lines_total'], 3) # lines 2, 3, 4
        self.assertEqual(stats['lines_covered'], 2) # lines 2, 4
        self.assertEqual(stats['branches_total'], 0)
        self.assertAlmostEqual(stats['file_coverage'], 66.666, places=2)

    def test_parse_branch_coverage(self):
        content = """
        1:    4:    if (argc > 1) {
branch  0 taken 0 (fallthrough)
branch  1 taken 1
    #####:    5:        std::cout << "taken";
        -:    6:    }
        """
        parser = GcovParser()
        stats = parser.parse_file(content)

        self.assertEqual(stats['branches_total'], 2)
        self.assertEqual(stats['branches_covered'], 1)

class TestCoverageReport(unittest.TestCase):
    @patch('pathlib.Path.exists')
    @patch('pathlib.Path.rglob')
    @patch('subprocess.run')
    @patch('builtins.print') # Suppress print
    def test_generate_report_no_artifacts(self, mock_print, mock_run, mock_rglob, mock_exists):
        agent = TestingAgent()
        mock_exists.return_value = True # build dir exists
        mock_rglob.return_value = [] # no gcno files

        report = agent.generate_coverage_report(Path("build"))

        self.assertEqual(report.lines_total, 0)

    @patch('pathlib.Path.exists')
    @patch('pathlib.Path.rglob')
    @patch('subprocess.run')
    @patch('pathlib.Path.read_text')
    @patch('pathlib.Path.unlink') # Mock unlink to avoid deletion error
    @patch('builtins.open', new_callable=unittest.mock.mock_open)
    def test_generate_report_success(self, mock_file, mock_unlink, mock_read_text, mock_run, mock_rglob, mock_exists):
        agent = TestingAgent()
        mock_exists.return_value = True

        # Mock file system
        gcno_path = MagicMock(spec=Path)
        gcno_path.name = "test.gcno"
        gcno_path.parent = Path("/tmp")

        gcda_path = MagicMock(spec=Path)

        # When rglob called
        mock_rglob.side_effect = [[gcno_path], [gcda_path]]

        # Mock subprocess gcov output
        mock_run.return_value.returncode = 0
        mock_run.return_value.stdout = "Creating 'test.cpp.gcov'"

        # Mock gcov file read
        mock_read_text.return_value = """
        1:    1:int main() {}
        """

        report = agent.generate_coverage_report(Path("build"))

        self.assertEqual(report.lines_total, 1)
        self.assertEqual(report.lines_covered, 1)

if __name__ == '__main__':
    unittest.main()
