import re
import shutil
from pathlib import Path

print("=" * 50)
print("Updating CMakeLists.txt for Manual Skia")
print("=" * 50)
print()

# Paths
cmake_file = Path(r"C:\zenith\daw\zenith-core\CMakeLists.txt")
backup_file = Path(r"C:\zenith\daw\zenith-core\CMakeLists.txt.backup")

# Backup
print("Backing up original CMakeLists.txt...")
shutil.copy2(cmake_file, backup_file)
print(f"Backup saved: {backup_file}")
print()

# Read content
print("Reading CMakeLists.txt...")
content = cmake_file.read_text(encoding='utf-8')

# Pattern to find the Skia section
old_pattern = re.compile(
    r'# ={70,}\n'
    r'# Skia Rendering \(conditional\)\n'
    r'# ={70,}\n'
    r'\n'
    r'if\(ZENITH_ENABLE_SKIA\).*?'
    r'else\(\)\s*\n'
    r'    message\(STATUS "Zenith DAW: Skia rendering DISABLED \(using JUCE fallback\)"\)\s*\n'
    r'endif\(\)',
    re.DOTALL
)

# New section
new_section = """# ============================================================================
# Skia Rendering (conditional) - Manual Integration
# ============================================================================
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/SkiaManualIntegration.cmake)"""

# Replace
print("Replacing Skia section...")
new_content = old_pattern.sub(new_section, content)

if new_content == content:
    print()
    print("ERROR: Could not find Skia section to replace!")
    print()
    print("The CMakeLists.txt might already be updated, or the format has changed.")
    print("Please check manually.")
    input("\nPress Enter to exit...")
    exit(1)

# Write
print("Writing updated CMakeLists.txt...")
cmake_file.write_text(new_content, encoding='utf-8')

print()
print("=" * 50)
print("CMakeLists.txt Updated Successfully!")
print("=" * 50)
print()
print(f"Original saved as: {backup_file}")
print()
print("Next: Run build-with-manual-skia.bat")
print()
input("Press Enter to continue...")
