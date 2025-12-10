import re
from collections import defaultdict

# Read the file
with open(r'c:\zenith\daw\apps\desktop\Source\ui\PianoRollComponent.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Broader pattern to match any PianoRollComponent member function
func_pattern = re.compile(r'^(?:std::vector<[^>]+>|void|bool|int|float|double|juce::\w+|PianoRollComponent::\w+)\s*\n?PianoRollComponent::(\w+)\s*\(')

# Also check for multiline definitions
func_pattern_multiline = re.compile(r'^PianoRollComponent::(\w+)\s*\(')

duplicated_funcs = [
    'getNoteExpression',
]

# Find function boundaries
brace_count = 0
in_function = False
current_func = None
func_start = 0
func_locations = defaultdict(list)

i = 0
while i < len(lines):
    line = lines[i]
    
    # Check for function start
    match = func_pattern.match(line) or func_pattern_multiline.match(line)
    if match and not in_function:
        current_func = match.group(1)
        # Check previous line for return type
        if i > 0 and 'std::vector' in lines[i-1]:
            func_start = i - 1
        else:
            func_start = i
        in_function = True
        brace_count = 0
    
    if in_function:
        brace_count += line.count('{') - line.count('}')
        if brace_count == 0 and '{' in ''.join(lines[func_start:i+1]):
            func_locations[current_func].append((func_start, i))
            in_function = False
            current_func = None
    
    i += 1

# Identify lines to remove
lines_to_remove = set()
for func_name in duplicated_funcs:
    locs = func_locations.get(func_name, [])
    if len(locs) > 1:
        print(f"{func_name}: keeping lines {locs[0][0]+1}-{locs[0][1]+1}, removing {len(locs)-1} duplicates")
        for start, end in locs[1:]:
            for line_num in range(start, end + 1):
                lines_to_remove.add(line_num)

print(f"\nTotal lines to remove: {len(lines_to_remove)}")

# Write cleaned file
with open(r'c:\zenith\daw\apps\desktop\Source\ui\PianoRollComponent.cpp', 'w', encoding='utf-8') as f:
    for i, line in enumerate(lines):
        if i not in lines_to_remove:
            f.write(line)

print("Cleanup complete!")
