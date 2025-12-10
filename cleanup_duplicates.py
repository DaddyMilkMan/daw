import re
from collections import defaultdict

# Read the file
with open(r'c:\zenith\daw\apps\desktop\Source\ui\PianoRollComponent.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Functions to de-duplicate (first occurrence is kept)
duplicated_funcs = [
    'generateVariation',
    'setExpressionLaneVisible', 
    'setNoteExpression',
    'setGhostNotesEnabled',
    'setGhostNoteOpacity',
    'addGhostClip',
    'removeGhostClip',
    'clearGhostClips',
    'refreshGhostNotes',
    'drawGhostNotes'
]

# Find function boundaries
func_pattern = re.compile(r'^void PianoRollComponent::(\w+)\s*\(')
brace_count = 0
in_function = False
current_func = None
func_start = 0
func_locations = defaultdict(list)  # func_name -> [(start, end), ...]

i = 0
while i < len(lines):
    line = lines[i]
    match = func_pattern.match(line)
    
    if match and not in_function:
        current_func = match.group(1)
        func_start = i
        in_function = True
        brace_count = 0
    
    if in_function:
        brace_count += line.count('{') - line.count('}')
        if brace_count == 0 and '{' in ''.join(lines[func_start:i+1]):
            # Function ended
            func_locations[current_func].append((func_start, i))
            in_function = False
            current_func = None
    
    i += 1

# Identify lines to remove (keep first occurrence only)
lines_to_remove = set()
for func_name in duplicated_funcs:
    locs = func_locations.get(func_name, [])
    if len(locs) > 1:
        print(f"{func_name}: keeping lines {locs[0][0]+1}-{locs[0][1]+1}, removing {len(locs)-1} duplicates")
        for start, end in locs[1:]:  # Skip first
            for line_num in range(start, end + 1):
                lines_to_remove.add(line_num)

# Also remove "Missing Stub" section headers that precede removed functions
stub_header = "// Missing Stub Implementations"
for i in range(len(lines) - 1):
    if stub_header in lines[i] and (i + 2) in lines_to_remove:
        # Remove the header block (usually 3 lines: comment, header, comment)
        if i >= 1 and '//==' in lines[i-1]:
            lines_to_remove.add(i - 1)
        lines_to_remove.add(i)
        if i + 1 < len(lines) and '//==' in lines[i+1]:
            lines_to_remove.add(i + 1)

print(f"\nTotal lines to remove: {len(lines_to_remove)}")
print(f"Original file: {len(lines)} lines")
print(f"After cleanup: {len(lines) - len(lines_to_remove)} lines")

# Write cleaned file
with open(r'c:\zenith\daw\apps\desktop\Source\ui\PianoRollComponent.cpp', 'w', encoding='utf-8') as f:
    for i, line in enumerate(lines):
        if i not in lines_to_remove:
            f.write(line)

print("\nCleanup complete!")
