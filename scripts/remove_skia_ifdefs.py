#!/usr/bin/env python3
"""
Script to remove #ifdef ZENITH_USE_SKIA conditional compilation
from files that already have Skia implementations.

This script:
1. Finds files with #ifdef ZENITH_USE_SKIA
2. Removes the conditional guards, keeping only the Skia code path
3. Removes JUCE Graphics fallback code
"""

import re
import sys
from pathlib import Path

def process_header_file(filepath):
    """Process a header file to remove JUCE Graphics conditionals."""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original_content = content
    changed = False
    
    # Pattern 1: Remove conditional Skia includes
    # Replace: #ifdef ZENITH_USE_SKIA\n#include <core/SkCanvas.h>\n...\n#endif
    # With: #include <core/SkCanvas.h>\n...
    pattern1 = r'#ifdef ZENITH_USE_SKIA\s*\n((?:#include\s+<(?:core|gpu)/[^>]+>\s*\n)+)#endif'
    if re.search(pattern1, content):
        content = re.sub(pattern1, r'\1', content)
        changed = True
        print(f"  - Removed conditional Skia includes")
    
    # Pattern 2: Remove conditional class inheritance
    # Replace: #ifdef ZENITH_USE_SKIA\nclass Foo : public SkiaComponent\n#else\nclass Foo : public juce::Component\n#endif
    # With: class Foo : public SkiaComponent
    pattern2 = r'#ifdef ZENITH_USE_SKIA\s*\n(class\s+\w+\s*:\s*public\s+SkiaComponent[^}]*)\n#else\s*\n(?:class\s+\w+\s*:\s*public\s+juce::[^}]*)\n#endif'
    if re.search(pattern2, content):
        content = re.sub(pattern2, r'\1', content)
        changed = True
        print(f"  - Removed conditional class inheritance")
    
    # Pattern 3: Remove conditional method declarations
    # Replace: #ifdef ZENITH_USE_SKIA\n  void drawSkia(SkCanvas* canvas);\n#else\n  void paint(juce::Graphics& g);\n#endif
    # With: void drawSkia(SkCanvas* canvas);
    pattern3 = r'#ifdef ZENITH_USE_SKIA\s*\n(\s*void\s+drawSkia\([^)]*\)[^;]*;)\s*\n#else\s*\n\s*void\s+paint\([^)]*\)[^;]*;\s*\n#endif'
    if re.search(pattern3, content):
        content = re.sub(pattern3, r'\1', content)
        changed = True
        print(f"  - Removed conditional drawSkia/paint declarations")
    
    # Pattern 4: Remove standalone #ifdef ZENITH_USE_SKIA guards around single items
    pattern4 = r'#ifdef ZENITH_USE_SKIA\s*\n([^#]+?)\n#endif'
    matches = list(re.finditer(pattern4, content))
    if matches:
        # Process in reverse to maintain positions
        for match in reversed(matches):
            # Check if this is a small block (likely a single line or declaration)
            block_content = match.group(1).strip()
            if len(block_content.split('\n')) <= 3:  # Small blocks only
                content = content[:match.start()] + match.group(1) + content[match.end():]
                changed = True
        if changed:
            print(f"  - Removed standalone #ifdef ZENITH_USE_SKIA guards")
    
    if changed:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        return True
    return False

def process_cpp_file(filepath):
    """Process a C++ file to remove JUCE Graphics fallback implementations."""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original_content = content
    changed = False
    
    # Pattern 1: Remove #else ... #endif blocks that contain JUCE Graphics paint() implementations
    # This is complex and risky, so we'll be conservative
    
    # Remove conditional Skia includes
    pattern1 = r'#ifdef ZENITH_USE_SKIA\s*\n((?:#include\s+<(?:core|gpu)/[^>]+>\s*\n)+)#endif'
    if re.search(pattern1, content):
        content = re.sub(pattern1, r'\1', content)
        changed = True
        print(f"  - Removed conditional Skia includes")
    
    # Remove #ifdef ZENITH_USE_SKIA before drawSkia implementation
    pattern2 = r'#ifdef ZENITH_USE_SKIA\s*\n(void\s+\w+::drawSkia\(SkCanvas\s*\*\s*canvas\))'
    if re.search(pattern2, content):
        content = re.sub(pattern2, r'\1', content)
        changed = True
        print(f"  - Removed #ifdef before drawSkia implementation")
    
    if changed:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        return True
    return False

def main():
    """Main entry point."""
    source_dir = Path("apps/desktop/Source")
    
    if not source_dir.exists():
        print(f"Error: {source_dir} not found")
        print("Run this script from the repository root")
        return 1
    
    # Find all header files with ZENITH_USE_SKIA
    header_files = []
    cpp_files = []
    
    for pattern in ["**/*.h", "**/*.cpp"]:
        for filepath in source_dir.glob(pattern):
            try:
                with open(filepath, 'r', encoding='utf-8') as f:
                    content = f.read()
                    if 'ZENITH_USE_SKIA' in content:
                        if filepath.suffix == '.h':
                            header_files.append(filepath)
                        else:
                            cpp_files.append(filepath)
            except Exception as e:
                print(f"Warning: Could not read {filepath}: {e}")
    
    print(f"Found {len(header_files)} header files with ZENITH_USE_SKIA")
    print(f"Found {len(cpp_files)} cpp files with ZENITH_USE_SKIA")
    print()
    
    # Process header files
    print("Processing header files...")
    header_count = 0
    for filepath in sorted(header_files):
        print(f"Checking {filepath.relative_to('apps/desktop/Source')}...")
        if process_header_file(filepath):
            header_count += 1
            print(f"  ✓ Modified")
        else:
            print(f"  - No changes")
    
    print()
    print("Processing cpp files...")
    cpp_count = 0
    for filepath in sorted(cpp_files):
        print(f"Checking {filepath.relative_to('apps/desktop/Source')}...")
        if process_cpp_file(filepath):
            cpp_count += 1
            print(f"  ✓ Modified")
        else:
            print(f"  - No changes")
    
    print()
    print(f"Summary: Modified {header_count} header files and {cpp_count} cpp files")
    print()
    print("NOTE: This script makes conservative changes.")
    print("Manual review and cleanup of JUCE Graphics fallback implementations is still needed.")
    print("Run 'git diff' to review changes before committing.")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
