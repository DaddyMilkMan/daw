import re
from collections import defaultdict

# Read the file
with open(r'c:\zenith\daw\apps\desktop\Source\ui\PianoRollComponent.cpp', 'r', encoding='utf-8') as f:
    content = f.read()
    lines = content.split('\n')

# Find all function definitions
func_pattern = r'^void PianoRollComponent::(\w+)\s*\('
duplicates = defaultdict(list)

for i, line in enumerate(lines, 1):
    match = re.match(func_pattern, line)
    if match:
        func_name = match.group(1)
        duplicates[func_name].append(i)

# Print duplicates
print("=== DUPLICATE FUNCTION ANALYSIS ===\n")
for func, line_nums in sorted(duplicates.items(), key=lambda x: -len(x[1])):
    if len(line_nums) > 1:
        print(f"{func}: {len(line_nums)} copies at lines {line_nums}")

print(f"\n=== TOTAL: {sum(len(v) for v in duplicates.values() if len(v) > 1)} function definitions are duplicated ===")
