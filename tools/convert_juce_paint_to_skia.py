#!/usr/bin/env python3
"""
Convert common JUCE paint/repaint usages to calls into ui::SkiaPainter::paint.
This script performs best-effort, non-destructive edits: it writes a backup of each
file edited (.bak) and prints the files modified. Review changes before committing.

Usage: python3 tools/convert_juce_paint_to_skia.py [root-dir]
If root-dir is omitted, the repository root is used.
"""
import os
import re
import sys

ROOT = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
PATTERNS = [
    # match member paint overrides: void paint (juce::Graphics& g) { ... }
    (re.compile(r"void\s+paint\s*\(\s*juce::Graphics\s*&\s*\w+\s*\)\s*\{", re.MULTILINE), 'member_paint'),
    # match free functions taking juce::Graphics
    (re.compile(r"void\s+paint\s*\(\s*juce::Graphics\s*&\s*\w+\s*\)", re.MULTILINE), 'func_paint'),
    # match calls to repaint(); -> TODO: call window invalidate
    (re.compile(r"\brepaint\s*\(\s*\)\s*;"), 'repaint_call'),
    # match juce::Graphics type references
    (re.compile(r"\bjuce::Graphics\b"), 'juce_graphics_type'),
]

EXTS = ('.cpp', '.cc', '.c', '.h', '.hpp', '.ipp', '.inl')

modified = []

for dirpath, dirnames, filenames in os.walk(ROOT):
    # skip common generated/external dirs
    if any(ex in dirpath for ex in ['.git', 'build', 'out', '.vs', 'external', 'third_party', 'node_modules']):
        continue
    for fname in filenames:
        if not fname.endswith(EXTS):
            continue
        path = os.path.join(dirpath, fname)
        try:
            with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                text = f.read()
        except Exception:
            continue
        new_text = text
        changed = False

        # Replace member paint overrides with SkiaPainter TODO wrapper
        def replace_member(m):
            nonlocal changed
            changed = True
            return ('void paint() {\n'
                    '    // Converted from JUCE paint(juce::Graphics&).\n'
                    '    // TODO: translate drawing commands from JUCE to Skia here.\n'
                    '    // Use ui::SkiaPainter::paint(windowHandle, width, height) or draw directly to Skia canvas.\n'
                    '    // Original signature retained in backup.\n')

        new_text = re.sub(r"void\s+paint\s*\(\s*juce::Graphics\s*&\s*\w+\s*\)\s*\{", replace_member, new_text)

        # Replace repaint() calls with placeholder invalidate call
        if re.search(r"\brepaint\s*\(\s*\)\s*;", new_text):
            new_text = re.sub(r"\brepaint\s*\(\s*\)\s*;",
                              '/* converted: repaint() -> request UI invalidation for Skia */\nrequestSkiaRepaint();',
                              new_text)
            changed = True

        # Replace juce::Graphics type references with comment to guide manual conversion
        if 'juce::Graphics' in new_text:
            new_text = new_text.replace('juce::Graphics', '/* juce::Graphics -> use Skia canvas */')
            changed = True

        if changed and new_text != text:
            # backup
            bak = path + '.bak'
            try:
                with open(bak, 'w', encoding='utf-8') as f:
                    f.write(text)
                with open(path, 'w', encoding='utf-8') as f:
                    f.write(new_text)
                modified.append(path)
            except Exception as e:
                print('Failed to write file', path, e)

if modified:
    print('Modified files:')
    for p in modified:
        print(' -', p)
    print('\nIMPORTANT: Review .bak files for original contents. The script performs best-effort replacements.')
    sys.exit(0)
else:
    print('No candidate JUCE painting usages found or nothing modified.')
    sys.exit(0)
