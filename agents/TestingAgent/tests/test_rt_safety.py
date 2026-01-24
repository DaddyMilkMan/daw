import unittest
import sys
import tempfile
import os
from pathlib import Path

# Add parent directory to path to import testing_agent
sys.path.append(str(Path(__file__).parent.parent))

from testing_agent import TestingAgent

class TestRTSafety(unittest.TestCase):
    def setUp(self):
        self.agent = TestingAgent()
        self.temp_dir = tempfile.TemporaryDirectory()
        self.temp_path = Path(self.temp_dir.name)

    def tearDown(self):
        self.temp_dir.cleanup()

    def _create_file(self, filename, content):
        path = self.temp_path / filename
        path.write_text(content, encoding='utf-8')
        return path

    def _scan_file(self, path):
        return self.agent.validate_rt_safety(source_files=[path])

    def test_detection_allocations(self):
        """Verify detection of various allocation patterns."""
        content = """
        void processBlock() {
            int* x = new int[10];           // Violation
            delete[] x;                     // Violation
            void* y = malloc(100);          // Violation
            free(y);                        // Violation
            auto z = std::make_unique<int>(5); // Violation
            std::vector<int> v;
            v.push_back(10);                // Violation
        }
        """
        path = self._create_file("allocations.cpp", content)
        results = self._scan_file(path)

        self.assertEqual(len(results), 6)
        self.assertTrue(any("new" in r.error_message for r in results))
        self.assertTrue(any("delete" in r.error_message for r in results))
        self.assertTrue(any("malloc" in r.error_message for r in results))
        self.assertTrue(any("free" in r.error_message for r in results))
        self.assertTrue(any("make_unique" in r.error_message for r in results))
        self.assertTrue(any("push_back" in r.error_message for r in results))

    def test_detection_locks(self):
        """Verify detection of locking primitives."""
        content = """
        // RT-SAFE
        void render() {
            std::mutex m;                   // Violation
            std::lock_guard<std::mutex> l(m); // Violation
            juce::CriticalSection cs;       // Violation
            const juce::ScopedLock sl(cs);  // Violation
        }
        """
        path = self._create_file("locks.cpp", content)
        results = self._scan_file(path)

        self.assertEqual(len(results), 4)

    def test_detection_io(self):
        """Verify detection of I/O operations."""
        content = """
        void processBlock(AudioBuffer& buffer) {
            std::cout << "Hello";           // Violation
            printf("Debug");                // Violation
            DBG("Juice debug");             // Violation
            juce::Logger::writeToLog("Log"); // Violation
        }
        """
        path = self._create_file("io.cpp", content)
        results = self._scan_file(path)

        self.assertEqual(len(results), 4)

    def test_detection_system_calls(self):
        """Verify detection of system calls (read/write/open)."""
        content = """
        void processBlock() {
            int fd = open("/tmp/test", 0); // Violation
            char buf[10];
            read(fd, buf, 10);             // Violation
            write(fd, buf, 10);            // Violation
            socket(0,0,0);                 // Violation

            // False positives check: simple variable access shouldn't trigger
            int read_val = 0;
            int write_val = 0;
        }
        """
        path = self._create_file("syscalls.cpp", content)
        results = self._scan_file(path)

        self.assertEqual(len(results), 4)
        msgs = [r.error_message for r in results]
        self.assertTrue(any("open" in m for m in msgs))
        self.assertTrue(any("read" in m for m in msgs))
        self.assertTrue(any("write" in m for m in msgs))
        self.assertTrue(any("socket" in m for m in msgs))

    def test_detection_flow_control(self):
        """Verify detection of unsafe flow control."""
        content = """
        void processBlock() {
            try {                           // Violation
                throw std::runtime_error("e"); // Violation
            } catch (...) {                 // Violation
            }
        }
        """
        path = self._create_file("flow.cpp", content)
        results = self._scan_file(path)
        self.assertEqual(len(results), 3)

    def test_detection_waiting(self):
        """Verify detection of wait/sleep."""
        content = """
        void processBlock() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Violation
            sleep(1);                       // Violation
        }
        """
        path = self._create_file("waiting.cpp", content)
        results = self._scan_file(path)
        self.assertEqual(len(results), 2)

    def test_masking_comments_strings(self):
        """Verify that comments and strings do not trigger violations."""
        content = """
        void processBlock() {
            // new int[10];  <-- comment should be safe
            /* malloc(100);  <-- block comment safe */
            const char* s = "std::cout << unsafe"; // string safe
            const char* c = "std::mutex"; // string safe
        }
        """
        path = self._create_file("masking.cpp", content)
        results = self._scan_file(path)

        if results:
            print(f"Masking failures: {[r.error_message for r in results]}")
        self.assertEqual(len(results), 0)

    def test_function_extraction(self):
        """Verify only marked/known functions are scanned."""
        content = """
        void safeInit() {
            new int[10]; // Should NOT be detected (not RT function)
        }

        // RT-SAFE
        void customRT() {
            new int[10]; // Should be detected
        }

        void processBlock() {
            malloc(10);  // Should be detected
        }
        """
        path = self._create_file("extraction.cpp", content)
        results = self._scan_file(path)

        self.assertEqual(len(results), 2)
        func_names = [r.name for r in results]

        self.assertTrue(any("processBlock" in n for n in func_names))
        self.assertFalse(any("safeInit" in n for n in func_names))
        self.assertTrue(any("// RT-SAFE" in n for n in func_names) or any("customRT" in n for n in func_names))

    def test_false_positives_names(self):
        """Verify that partial matches (substrings) do not trigger."""
        content = """
        void processBlock() {
            int renew = 0;      // 'new' inside 'renew'
            int freeme = 1;     // 'free' inside 'freeme'
            calloc_custom();    // 'calloc' inside name (if no boundary)
            mystd::cout << 1;   // 'std::cout' partial? (depends on regex)
        }
        """
        path = self._create_file("names.cpp", content)
        results = self._scan_file(path)
        if results:
             print(f"False Positives: {[r.error_message for r in results]}")
        self.assertEqual(len(results), 0)

    def test_missing_patterns_juce(self):
        """Verify detection of JUCE patterns."""
        content = """
        void processBlock() {
            juce::Array<int> arr;           // Violation
            juce::HashMap<int, int> map;    // Violation
            juce::MessageManagerLock mml;   // Violation
            juce::String s = "alloc";       // Violation
            s.formatted("%d", 1);           // Violation
            s.toStdString();                // Violation
        }
        """
        path = self._create_file("juce_missing.cpp", content)
        results = self._scan_file(path)

        self.assertGreaterEqual(len(results), 6)

        msgs = [r.error_message for r in results]
        self.assertTrue(any("juce::Array" in m for m in msgs))
        self.assertTrue(any("juce::HashMap" in m for m in msgs))
        self.assertTrue(any("MessageManagerLock" in m for m in msgs))
        self.assertTrue(any("juce::String" in m for m in msgs))
        self.assertTrue(any("formatted" in m for m in msgs))
        self.assertTrue(any("toStdString" in m for m in msgs))

    def test_missing_patterns_std_modern(self):
        """Verify detection of modern C++ unsafe patterns."""
        content = """
        void processBlock() {
            auto x = std::allocate_shared<int>(alloc, 1); // Smart Pointer
            std::function<void()> f = []{}; // Heavy Type
            std::atomic_wait(&atom, 1);     // Waiting
            std::format("{}", 1);           // Formatting
        }
        """
        path = self._create_file("std_missing.cpp", content)
        results = self._scan_file(path)

        self.assertEqual(len(results), 4)
        msgs = [r.error_message for r in results]
        self.assertTrue(any("allocate_shared" in m for m in msgs))
        self.assertTrue(any("function" in m for m in msgs))
        self.assertTrue(any("atomic_wait" in m for m in msgs))
        self.assertTrue(any("format" in m for m in msgs))

    def test_suppression(self):
        """Verify that suppression comments work."""
        content = """
        void processBlock() {
            new int[10]; // NOLINT
            malloc(10); // RT-SAFE-IGNORE
        }
        """
        path = self._create_file("suppression.cpp", content)
        results = self._scan_file(path)
        self.assertEqual(len(results), 0)

    def test_safe_code(self):
        content = """
        void processBlock(AudioBuffer& buffer) noexcept {
            float x = 0.0f;
            for(int i=0; i<buffer.getNumSamples(); ++i) {
                x += 1.0f;
            }
        }
        """
        path = self._create_file("safe.cpp", content)
        results = self._scan_file(path)

        self.assertEqual(len(results), 0, f"Should not report errors for safe code. Found: {results}")

    def test_missing_noexcept(self):
        content = """
        void processBlock(AudioBuffer& buffer) {
            // Safe code but missing noexcept
        }
        """
        path = self._create_file("missing_noexcept.cpp", content)
        results = self._scan_file(path)

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
        path = self._create_file("intervening.cpp", content)
        results = self._scan_file(path)

        # Should detect missing noexcept in unsafe()
        found = any("noexcept" in r.error_message for r in results)
        self.assertTrue(found, "Should detect missing noexcept specifier even with intervening declaration")

if __name__ == '__main__':
    unittest.main()