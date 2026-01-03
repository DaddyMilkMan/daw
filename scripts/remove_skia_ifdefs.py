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
    
    # Pattern 1: Remove conditional Skia includes (any Skia-related include)
    # Replace: #ifdef ZENITH_USE_SKIA\n#include ...\n#endif
    # With: #include ...
    pattern1 = r'#ifdef ZENITH_USE_SKIA\s*\n((?:#include\s+[^\n]+\n)+)#endif'
    matches1 = list(re.finditer(pattern1, content))
    if matches1:
        for match in reversed(matches1):
            content = content[:match.start()] + match.group(1) + content[match.end():]
            changed = True
        print(f"  - Removed conditional includes")
    
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
    
    # Pattern 4: Remove standalone #ifdef ZENITH_USE_SKIA guards around methods or members
    # This handles: #ifdef ZENITH_USE_SKIA\n  void drawFoo();\n  void drawBar();\n#endif
    pattern4 = r'#ifdef ZENITH_USE_SKIA\s*\n([^#]+?)\n#endif'
    matches4 = list(re.finditer(pattern4, content))
    if matches4:
        # Process in reverse to maintain positions
        for match in reversed(matches4):
            # Check if this is a reasonable block
            block_content = match.group(1).strip()
            # Allow blocks up to 10 lines (method declarations, member variables)
            if len(block_content.split('\n')) <= 10:
                content = content[:match.start()] + match.group(1) + content[match.end():]
                changed = True
        if changed:
            print(f"  - Removed standalone #ifdef guards")
    
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
    
    # Remove conditional Skia includes (same as header files)
    pattern1 = r'#ifdef ZENITH_USE_SKIA\s*\n((?:#include\s+[^\n]+\n)+)#endif'
    matches1 = list(re.finditer(pattern1, content))
    if matches1:
        for match in reversed(matches1):
            content = content[:match.start()] + match.group(1) + content[match.end():]
            changed = True
        print(f"  - Removed conditional includes")
    
    # Remove #ifdef ZENITH_USE_SKIA before drawSkia implementation
    pattern2 = r'#ifdef ZENITH_USE_SKIA\s*\n(void\s+\w+::drawSkia\(SkCanvas\s*\*\s*canvas\))'
    if re.search(pattern2, content):
        content = re.sub(pattern2, r'\1', content)
        changed = True
        print(f"  - Removed #ifdef before drawSkia implementation")
    
    # Remove #endif after complete drawSkia implementations
    # Look for: method implementation followed by }#endif
    pattern3 = r'(\w+::drawSkia\([^)]*\)[^{]*\{(?:[^{}]|\{[^{}]*\})*\})\s*#endif'
    if re.search(pattern3, content):
        content = re.sub(pattern3, r'\1', content)
        changed = True
        print(f"  - Removed #endif after drawSkia implementation")
    
    # Remove small #ifdef ZENITH_USE_SKIA blocks (similar to headers)
    pattern4 = r'#ifdef ZENITH_USE_SKIA\s*\n([^#]+?)\n#endif'
    matches4 = list(re.finditer(pattern4, content))
    if matches4:
        for match in reversed(matches4):
            block_content = match.group(1).strip()
            # Allow blocks up to 15 lines for cpp files (can have more implementation code)
            if len(block_content.split('\n')) <= 15:
                content = content[:match.start()] + match.group(1) + content[match.end():]
                changed = True
        if changed:
            print(f"  - Removed standalone #ifdef guards")
    
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
