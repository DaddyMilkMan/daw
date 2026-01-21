import re
import os
from dataclasses import dataclass
from typing import List, Dict, Optional, Tuple
from pathlib import Path

@dataclass
class RTSafetyViolation:
    file_path: str
    function: str
    line_number: int  # Approximate, relative to function start or file
    violation_type: str
    matched_text: str
    context: str

class RTSafetyAnalyzer:
    """
    Static analyzer for Real-Time safety in C++ audio code.
    Performs surface-level scanning of critical audio callbacks (e.g., processBlock)
    to detect operations that are unsafe in a real-time thread.
    """

    # Patterns that are generally unsafe in RT threads
    UNSAFE_PATTERNS = [
        (r'\bnew\s+', "Heap Allocation (new)"),
        (r'\bdelete\s+', "Heap Deallocation (delete)"),
        (r'\bmalloc\(', "Heap Allocation (malloc)"),
        (r'\bcalloc\(', "Heap Allocation (calloc)"),
        (r'\brealloc\(', "Heap Allocation (realloc)"),
        (r'\bfree\(', "Heap Deallocation (free)"),
        (r'\bstd::lock_guard\b', "Blocking Synchronization (lock_guard)"),
        (r'\bstd::unique_lock\b', "Blocking Synchronization (unique_lock)"),
        (r'\bmutex\.lock\(', "Blocking Synchronization (mutex.lock)"),
        (r'\bstd::cout\b', "Blocking I/O (std::cout)"),
        (r'\bstd::cerr\b', "Blocking I/O (std::cerr)"),
        (r'\bprintf\(', "Blocking I/O (printf)"),
        (r'\bfprintf\(', "Blocking I/O (fprintf)"),
        (r'\bDBG\(', "Blocking I/O (DBG macro)"),
        (r'\.push_back\(', "Potential Allocation (vector.push_back)"),
        (r'\.reserve\(', "Potential Allocation (vector.reserve)"),
        (r'\.resize\(', "Potential Allocation (vector.resize)"),
        (r'\bstd::string\b', "Potential Allocation (std::string)"),
        (r'\bdynamic_cast<', "RTTI Overhead (dynamic_cast)"),
        (r'\bthrow\s+', "Exception (throw)"),
        (r'\btry\s*\{', "Exception Overhead (try/catch block)"),
        (r'\bsleep\(', "Blocking Call (sleep)"),
        (r'\bstd::this_thread::sleep_for', "Blocking Call (sleep_for)"),
    ]

    def __init__(self):
        pass

    def mask_comments_and_strings(self, source: str) -> str:
        """
        Replaces comments and string literals with spaces/placeholders
        to ensure structure parsing (brace counting) is robust.
        Keeps newlines to preserve line numbering.
        """
        out = list(source)
        n = len(source)
        i = 0

        while i < n:
            # Check for line comment //
            if i + 1 < n and source[i] == '/' and source[i+1] == '/':
                out[i] = ' '
                out[i+1] = ' '
                i += 2
                while i < n and source[i] != '\n':
                    out[i] = ' '
                    i += 1

            # Check for block comment /* ... */
            elif i + 1 < n and source[i] == '/' and source[i+1] == '*':
                out[i] = ' '
                out[i+1] = ' '
                i += 2
                while i + 1 < n and not (source[i] == '*' and source[i+1] == '/'):
                    if source[i] != '\n':
                        out[i] = ' '
                    i += 1
                if i + 1 < n:
                    out[i] = ' ' # *
                    out[i+1] = ' ' # /
                    i += 2

            # Check for string literal "..."
            elif source[i] == '"':
                # Don't mask the quote itself, but mask content
                # Actually, strictly for brace counting, we can leave quotes
                i += 1
                while i < n:
                    if source[i] == '"':
                        # Check for escaped quote \"
                        if source[i-1] == '\\' and source[i-2] != '\\':
                            # It's an escaped quote, mask it and continue
                            out[i] = ' '
                            i += 1
                            continue
                        else:
                            # End of string
                            i += 1
                            break
                    elif source[i] == '\n':
                        # String shouldn't span lines unless escaped, but for safety handle newline
                        break
                    else:
                        out[i] = ' '
                        i += 1

            # Check for char literal '...'
            elif source[i] == "'":
                i += 1
                while i < n:
                    if source[i] == "'":
                        if source[i-1] == '\\' and source[i-2] != '\\':
                             out[i] = ' '
                             i += 1
                             continue
                        else:
                            i += 1
                            break
                    elif source[i] == '\n':
                        break
                    else:
                        out[i] = ' '
                        i += 1
            else:
                i += 1

        return "".join(out)

    def extract_function_body(self, source: str, masked_source: str, func_name: str) -> List[Tuple[str, int]]:
        """
        Finds all occurrences of `func_name` and extracts their bodies.
        Returns a list of (body_source, start_line_number).
        """
        bodies = []

        # Simple heuristic to find function definition:
        # Look for "func_name" followed eventually by "{"
        # This regex is a starting point; it might match calls too, but
        # checking for "{" usually filters for definitions in valid C++.
        # We search in masked_source to ignore comments/strings.

        # Regex: boundary + func_name + args (...) + optional const/override + {
        # Note: Parsing C++ args with regex is hard. We'll look for:
        # func_name \s* \( ... \) ... {

        # We iterate manually to handle brace balancing for the function body

        search_start = 0
        while True:
            # Find the function name
            idx = masked_source.find(func_name, search_start)
            if idx == -1:
                break

            # Check if it's a whole word
            if (idx > 0 and masked_source[idx-1].isalnum()) or \
               (idx + len(func_name) < len(masked_source) and masked_source[idx+len(func_name)].isalnum()):
                search_start = idx + len(func_name)
                continue

            # Look ahead for opening brace '{'
            # We need to skip arguments (...).
            # If we find a semicolon ';' before '{', it's likely a declaration or call.

            brace_found = False
            brace_idx = -1
            semicolon_found = False

            curr = idx + len(func_name)
            while curr < len(masked_source):
                char = masked_source[curr]
                if char == '{':
                    brace_found = True
                    brace_idx = curr
                    break
                if char == ';':
                    semicolon_found = True
                    break
                curr += 1

            if semicolon_found or not brace_found:
                search_start = idx + len(func_name)
                continue

            # Found opening brace at brace_idx.
            # Now extract body using brace counting.
            stack = 1
            curr = brace_idx + 1
            while curr < len(masked_source) and stack > 0:
                if masked_source[curr] == '{':
                    stack += 1
                elif masked_source[curr] == '}':
                    stack -= 1
                curr += 1

            if stack == 0:
                # Extracted body
                end_idx = curr
                # We return the ORIGINAL source content for the body
                body_content = source[brace_idx:end_idx]

                # Calculate line number
                line_num = source.count('\n', 0, brace_idx) + 1

                bodies.append((body_content, line_num))

                search_start = end_idx
            else:
                # Unbalanced or EOF?
                search_start = brace_idx + 1

        return bodies

    def scan_code(self, body_source: str, start_line: int, file_path: str, function_name: str) -> List[RTSafetyViolation]:
        """
        Scans a specific code block for unsafe patterns.
        """
        violations = []

        # We should mask strings in the body before regex matching to avoid false positives
        # (e.g. print("Do not call new here"))
        # However, we want to report the original text in the violation.
        masked_body = self.mask_comments_and_strings(body_source)

        for pattern, description in self.UNSAFE_PATTERNS:
            regex = re.compile(pattern)
            for match in regex.finditer(masked_body):
                # Get line number relative to body start
                local_offset = match.start()
                local_line_offset = body_source.count('\n', 0, local_offset)
                absolute_line = start_line + local_line_offset

                # Extract snippet for context
                line_start = body_source.rfind('\n', 0, local_offset) + 1
                line_end = body_source.find('\n', local_offset)
                if line_end == -1: line_end = len(body_source)
                context_line = body_source[line_start:line_end].strip()

                # Double check we aren't matching inside a masked area (redundant if we regex on masked_body)
                # But masked_body replaced chars with spaces, so the match text might be spaces if we matched whitespace?
                # No, regexes look for words like "new", "malloc". In masked body, strings are spaces.
                # So we won't match "new" inside a string.

                violations.append(RTSafetyViolation(
                    file_path=str(file_path),
                    function=function_name,
                    line_number=absolute_line,
                    violation_type=description,
                    matched_text=match.group(0),
                    context=context_line
                ))

        return violations

    def analyze_file(self, file_path: Path) -> List[RTSafetyViolation]:
        """
        Analyzes a single file for RT safety in 'processBlock' methods.
        """
        try:
            source = file_path.read_text(encoding='utf-8', errors='ignore')
        except Exception as e:
            print(f"Failed to read {file_path}: {e}")
            return []

        masked_source = self.mask_comments_and_strings(source)
        violations = []

        # Detect 'processBlock' functions
        # Also could look for 'process', 'getNextAudioBlock'
        target_functions = ['processBlock', 'process', 'getNextAudioBlock', 'renderNextBlock']

        for func_name in target_functions:
            bodies = self.extract_function_body(source, masked_source, func_name)
            for body, start_line in bodies:
                v = self.scan_code(body, start_line, str(file_path), func_name)
                violations.extend(v)

        return violations

# Simple test if run directly
if __name__ == "__main__":
    import sys

    if len(sys.argv) < 2:
        print("Usage: python rt_analyzer.py <file_path>")
        sys.exit(1)

    analyzer = RTSafetyAnalyzer()
    path = Path(sys.argv[1])
    violations = analyzer.analyze_file(path)

    if violations:
        print(f"Found {len(violations)} potential RT safety violations:")
        for v in violations:
            print(f"  {v.file_path}:{v.line_number} - {v.violation_type}")
            print(f"    Context: {v.context}")
    else:
        print("No violations found.")
