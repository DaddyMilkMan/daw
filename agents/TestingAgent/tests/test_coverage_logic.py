import unittest
import sys
from pathlib import Path
from unittest.mock import MagicMock, patch

# Add parent directory to path to import testing_agent
sys.path.append(str(Path(__file__).parent.parent))

from testing_agent import GcovParser

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
        # New assertion for lines_data
        self.assertEqual(stats['lines_data'][2], 1)
        self.assertEqual(stats['lines_data'][3], 0)
        self.assertEqual(stats['lines_data'][4], 1)
        self.assertEqual(stats['source_path'], "test.cpp")

    def test_parse_branch_coverage(self):
        content = """
        -:    0:Source:foo.cpp
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
        self.assertEqual(stats['source_path'], "foo.cpp")

if __name__ == '__main__':
    unittest.main()
