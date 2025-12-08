import re

# Read the file
with open(r'c:\zenith\daw\apps\desktop\Source\ui\PianoRollComponent.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Find and remove empty "Missing Stub" sections (header + empty)
lines_to_remove = set()
i = 0
while i < len(lines) - 3:
    # Check for section header pattern
    if '//==' in lines[i] and 'Missing' in lines[i+1] and '//==' in lines[i+2]:
        # Check if followed by another section header or empty lines then another header
        j = i + 3
        while j < len(lines) and lines[j].strip() == '':
            j += 1
        
        # If next non-empty line is another section header, remove this empty section
        if j < len(lines) and '//==' in lines[j]:
            print(f"Removing empty section at lines {i+1}-{j}")
            for k in range(i, j):
                lines_to_remove.add(k)
            i = j
            continue
    i += 1

print(f"\nRemoving {len(lines_to_remove)} lines of empty section headers")

# Write cleaned file
with open(r'c:\zenith\daw\apps\desktop\Source\ui\PianoRollComponent.cpp', 'w', encoding='utf-8') as f:
    for i, line in enumerate(lines):
        if i not in lines_to_remove:
            f.write(line)

print("Cleanup complete!")
