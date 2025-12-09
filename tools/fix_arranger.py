
import os

path = r"c:\zenith\daw\apps\desktop\Source\ui\ArrangerComponent.cpp"

with open(path, 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Look for the pattern around line 1020
for i in range(1015, 1030):
    if i < len(lines):
        line = lines[i].rstrip()
        prev = lines[i-1].rstrip()
        
        # Pattern we hate:
        # prev: }
        # line: }
        # next: #endif
        
        if line.strip() == '}' and prev.strip() == '}' and lines[i+1].strip() == '#endif':
            print(f"Found double brace at line {i+1}: Removing it.")
            # Remove the line
            lines.pop(i)
            # Write back
            with open(path, 'w', encoding='utf-8') as f_out:
                f_out.writelines(lines)
            print("File updated.")
            exit(0)

print("Pattern not found.")
exit(1)
