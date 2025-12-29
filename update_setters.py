#!/usr/bin/env python3

import re

# Read the Settings.h file
with open("/home/micah/Desktop/zenith/daw/apps/desktop/Source/Settings.h", "r") as f:
    content = f.read()

# Pattern to match setters
setter_pattern = r'void (\w+)\(([^)]+)\) \{ if \(([^_]+)_ != ([^)]+)\) \{ \2 = \3; save\(\); sendChangeMessage\(\); \} \}'

# Replace all setters
def replace_setters(match):
    method_name = match.group(1)
    param_type = match.group(2)
    member_name = match.group(3)
    param_name = match.group(4)
    
    return f'''void {method_name}({param_type}) {{
        setWithBroadcast([&]() {{
            if ({member_name}_ != {param_name}) {{
                {member_name}_ = {param_name};
                save();
                return true;
            }}
            return false;
        }});
    }}'''

# Apply replacement
new_content = re.sub(setter_pattern, replace_setters, content)

# Write back
with open("/home/micah/Desktop/zenith/daw/apps/desktop/Source/Settings.h", "w") as f:
    f.write(new_content)

print("Updated all setters to be thread-safe")
