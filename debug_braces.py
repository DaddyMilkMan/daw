
import re

def analyze_braces(filepath):
    print(f"Analyzing {filepath}...")
    balance = 0
    namespace_depth = 0
    in_namespace = False
    
    with open(filepath, 'r') as f:
        lines = f.readlines()
        
    for i, line in enumerate(lines):
        line_num = i + 1
        stripped = line.strip()
        
        # Super basic comment removal (doesn't handle block comments fully correctly if inline, but good enough)
        if "//" in stripped:
            stripped = stripped.split("//")[0]
        
        # Check for namespace start
        if "namespace zenith" in line and "{" in line:
            in_namespace = True
            print(f"Namespace 'zenith' starts at line {line_num}")
        
        open_braces = stripped.count('{')
        close_braces = stripped.count('}')
        
        balance += open_braces
        balance -= close_braces
        
        if in_namespace and balance == 0:
            print(f"WARNING: Namespace/Global scope closed at line {line_num}")
            in_namespace = False # Reset to find next closure if any
            
        if balance < 0:
             print(f"ERROR: Negative balance at line {line_num}: {stripped}")
             
    print(f"Final Balance: {balance}")

analyze_braces('apps/desktop/Source/engine/Engine.cpp')
