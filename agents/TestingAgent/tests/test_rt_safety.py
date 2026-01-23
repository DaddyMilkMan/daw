
import unittest
import sys
import tempfile
import os
from pathlib import Path
from unittest.mock import MagicMock, patch

# Add parent directory to path to import testing_agent
sys.path.append(str(Path(__file__).parent.parent))

from testing_agent import TestingAgent, TestStatus

class TestRTSafety(unittest.TestCase):
    def test_rt_safety_violations(self):
        # Create a dummy C++ file with unsafe operations
        cpp_content = """
        #include <vector>
        #include <mutex>
        #include <iostream>
        #include <juce_core/juce_core.h>

        class AudioProcessor {
        public:
            // Trigger: processBlock
            void processBlock(float* buffer, int numSamples) {
                // Unsafe: Allocation
                std::vector<float> temp;
                temp.resize(numSamples); // EXPECT: Container Mutation

                // Unsafe: New
                float* f = new float[100]; // EXPECT: Allocation
                delete[] f; // EXPECT: Allocation

                // Unsafe: Lock
                std::mutex mtx; // EXPECT: Lock
                std::lock_guard<std::mutex> lock(mtx); // EXPECT: Lock

                juce::CriticalSection cs; // EXPECT: Lock
                juce::ScopedLock sl(cs); // EXPECT: Lock
                juce::ReadWriteLock rwl; // EXPECT: Lock

                // Unsafe: I/O
                std::cout << "Processing" << std::endl; // EXPECT: I/O
                printf("Processing\\n"); // EXPECT: I/O
                juce::Logger::writeToLog("Log"); // EXPECT: I/O

                // Unsafe: JUCE Data Structures
                juce::String s = "hello"; // EXPECT: String/ValueTree Usage
                juce::ValueTree v("Type"); // EXPECT: String/ValueTree Usage
                juce::var var = 1; // EXPECT: String/ValueTree Usage

                // Unsafe: Container clear
                temp.clear(); // EXPECT: Container Mutation
                temp.erase(temp.begin()); // EXPECT: Container Mutation

                // Safe: RT-SAFE ignore
                std::cout << "Debug" << std::endl; // NOLINT
                juce::String s2 = "ignored"; // RT-SAFE-IGNORE

                // Unsafe: Flow Control
                try { // EXPECT: Flow Control
                    throw 1; // EXPECT: Flow Control
                } catch (...) {} // EXPECT: Flow Control
            }

            void otherFunction() {
                // This function is not RT-critical, so unsafe ops are fine
                std::vector<int> v;
                v.push_back(1);
            }

            // RT-SAFE
            void criticalFunction() {
                 malloc(100); // EXPECT: Allocation
            }
        };
        """

        with tempfile.NamedTemporaryFile(suffix=".cpp", delete=False, mode='w') as f:
            f.write(cpp_content)
            temp_path = Path(f.name)

        try:
            agent = TestingAgent()
            results = agent.validate_rt_safety(source_files=[temp_path])

            # We expect strict violation types
            expected_violations = {
                 "Container Mutation": 3, # resize, clear, erase
                 "Allocation": 3, # new, delete, malloc
                 "Lock": 5, # mutex, lock_guard, CriticalSection, ScopedLock, ReadWriteLock
                 "I/O": 3, # cout, printf, Logger
                 "String/ValueTree Usage": 3, # String, ValueTree, var
                 "Flow Control": 3, # try, throw, catch
            }

            total_expected = sum(expected_violations.values())
            self.assertEqual(len(results), total_expected)

            found_counts = {k: 0 for k in expected_violations}

            for res in results:
                found = False
                for v_type in expected_violations:
                    if v_type in res.error_message:
                        found_counts[v_type] += 1
                        found = True
                        break
                self.assertTrue(found, f"Unknown violation message: {res.error_message}")

            for v_type, count in expected_violations.items():
                self.assertEqual(found_counts[v_type], count, f"Mismatch for {v_type}")

        finally:
            if temp_path.exists():
                temp_path.unlink()

if __name__ == '__main__':
    unittest.main()
