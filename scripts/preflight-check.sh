#!/bin/bash
# =============================================================================
# PREFLIGHT CHECK FOR 5-STAR NAVIGATION
# =============================================================================
# Run this BEFORE any refactoring to validate preconditions
# =============================================================================

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

ERRORS=0
WARNINGS=0

echo "🔍 PREFLIGHT CHECK: 5-Star Navigation"
echo "======================================"
echo ""

# =============================================================================
# 1. Git State
# =============================================================================
echo -e "${BLUE}1. Checking Git State${NC}"
echo "---------------------"

if ! git rev-parse --git-dir > /dev/null 2>&1; then
    echo -e "${RED}❌ ERROR: Not a git repository${NC}"
    exit 1
fi

# Check for uncommitted changes
if ! git diff --quiet HEAD; then
    echo -e "${RED}❌ ERROR: You have uncommitted changes${NC}"
    echo "   Commit or stash before running migration scripts"
    git status --short | head -10
    ((ERRORS++))
else
    echo -e "${GREEN}✅ Git working directory clean${NC}"
fi

# Check we're on a feature branch (not main/master)
BRANCH=$(git branch --show-current)
if [[ "$BRANCH" == "main" ]] || [[ "$BRANCH" == "master" ]]; then
    echo -e "${YELLOW}⚠️  WARNING: You are on $BRANCH branch${NC}"
    echo "   Recommended: Create a feature branch first"
    echo "   git checkout -b refactor/navigation-cleanup"
    ((WARNINGS++))
else
    echo -e "${GREEN}✅ On branch: $BRANCH${NC}"
fi

# Check for recent commits
RECENT_COMMITS=$(git log --oneline -5 2>/dev/null | wc -l)
if [ "$RECENT_COMMITS" -eq 0 ]; then
    echo -e "${YELLOW}⚠️  WARNING: No recent commits found${NC}"
    ((WARNINGS++))
fi

# =============================================================================
# 2. Build System
# =============================================================================
echo ""
echo -e "${BLUE}2. Checking Build System${NC}"
echo "------------------------"

# Check CMake exists
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}❌ ERROR: CMake not found${NC}"
    ((ERRORS++))
else
    CMAKE_VERSION=$(cmake --version | head -1)
    echo -e "${GREEN}✅ Found: $CMAKE_VERSION${NC}"
fi

# Check for CMakeLists.txt
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}❌ ERROR: CMakeLists.txt not found in root${NC}"
    ((ERRORS++))
else
    echo -e "${GREEN}✅ CMakeLists.txt exists${NC}"
fi

# Check for build directory
if [ -d "build" ]; then
    echo -e "${GREEN}✅ Build directory exists${NC}"
else
    echo -e "${YELLOW}⚠️  WARNING: No build directory (will be created)${NC}"
    ((WARNINGS++))
fi

# Count CMake files
CMAKE_COUNT=$(find cmake modules apps -name "*.cmake" -o -name "CMakeLists.txt" 2>/dev/null | grep -v external | wc -l)
echo "   Found $CMAKE_COUNT CMake configuration files"

# =============================================================================
# 3. Source Files
# =============================================================================
echo ""
echo -e "${BLUE}3. Checking Source Files${NC}"
echo "------------------------"

# Count source files
CPP_COUNT=$(find modules apps -name "*.cpp" 2>/dev/null | wc -l)
H_COUNT=$(find modules apps -name "*.h" 2>/dev/null | wc -l)
echo "   C++ files: $CPP_COUNT"
echo "   Header files: $H_COUNT"

# Check for files with problematic patterns
echo ""
echo "   Checking for problematic patterns..."

# Files with relative includes
RELATIVE_INCLUDES=$(grep -r '#include.*\.\./' modules --include="*.h" --include="*.cpp" 2>/dev/null | wc -l)
if [ "$RELATIVE_INCLUDES" -gt 0 ]; then
    echo "   - Relative includes (../): $RELATIVE_INCLUDES occurrences"
fi

# Duplicate filenames
DUPLICATES=$(find modules -name "*.h" -not -path "*/deprecated/*" | xargs -I {} basename {} | sort | uniq -d | wc -l)
if [ "$DUPLICATES" -gt 0 ]; then
    echo "   - Duplicate header names: $DUPLICATES (will be renamed)"
fi

# =============================================================================
# 4. Test Infrastructure
# =============================================================================
echo ""
echo -e "${BLUE}4. Checking Test Infrastructure${NC}"
echo "-------------------------------"

TEST_DIR="apps/desktop/Source/tests"
if [ -d "$TEST_DIR" ]; then
    TEST_COUNT=$(find "$TEST_DIR" -name "*.cpp" | wc -l)
    echo "   Tests in $TEST_DIR: $TEST_COUNT"
else
    echo -e "${YELLOW}⚠️  WARNING: Test directory not found: $TEST_DIR${NC}"
    ((WARNINGS++))
fi

# Check for module test directories
for module in zenith_core zenith_ui zenith_dsp zenith_network zenith_commands; do
    if [ -d "modules/$module/tests" ]; then
        COUNT=$(find "modules/$module/tests" -name "*.cpp" 2>/dev/null | wc -l)
        echo "   - modules/$module/tests: $COUNT tests"
    fi
done

# =============================================================================
# 5. Dependencies
# =============================================================================
echo ""
echo -e "${BLUE}5. Checking Dependencies${NC}"
echo "------------------------"

# Check for external dependencies
if [ -d "external/JUCE" ]; then
    echo -e "${GREEN}✅ JUCE found${NC}"
else
    echo -e "${YELLOW}⚠️  JUCE not found (will be fetched during build)${NC}"
fi

if [ -d "external/vcpkg" ]; then
    echo -e "${GREEN}✅ vcpkg found${NC}"
else
    echo -e "${YELLOW}⚠️  vcpkg not found${NC}"
fi

# =============================================================================
# 6. IDE/Editor Files
# =============================================================================
echo ""
echo -e "${BLUE}6. Checking IDE Configuration${NC}"
echo "-----------------------------"

# VS Code
if [ -d ".vscode" ]; then
    echo "   VS Code: .vscode/ exists"
fi

# CLion
if [ -d ".idea" ]; then
    echo "   CLion: .idea/ exists"
fi

# Visual Studio
VCPROJ_COUNT=$(find . -name "*.vcxproj" -not -path "./external/*" 2>/dev/null | wc -l)
if [ "$VCPROJ_COUNT" -gt 0 ]; then
    echo "   Visual Studio: $VCPROJ_COUNT .vcxproj files"
fi

echo ""
echo "   ⚠️  Note: IDE indexes will need rebuilding after refactoring"

# =============================================================================
# 7. Disk Space
# =============================================================================
echo ""
echo -e "${BLUE}7. Checking Resources${NC}"
echo "---------------------"

# Check disk space
if command -v df &> /dev/null; then
    AVAILABLE=$(df . | tail -1 | awk '{print $4}')
    AVAILABLE_GB=$((AVAILABLE / 1024 / 1024))
    if [ "$AVAILABLE_GB" -lt 5 ]; then
        echo -e "${YELLOW}⚠️  WARNING: Low disk space (${AVAILABLE_GB}GB available)${NC}"
        ((WARNINGS++))
    else
        echo "   Disk space: ${AVAILABLE_GB}GB available"
    fi
fi

# =============================================================================
# 8. Summary
# =============================================================================
echo ""
echo "======================================"
echo "           SUMMARY"
echo "======================================"

if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo -e "${GREEN}✅ ALL CHECKS PASSED${NC}"
    echo ""
    echo "You are ready to proceed with 5-star navigation refactoring!"
    echo ""
    echo "Recommended next steps:"
    echo "  1. Run: python3 scripts/safe-rename.py --dry-run"
    echo "  2. Review proposed changes"
    echo "  3. Run: python3 scripts/safe-rename.py --execute"
    echo "  4. Build and verify: cmake --build build"
    exit 0
elif [ $ERRORS -eq 0 ]; then
    echo -e "${YELLOW}⚠️  PASSED WITH WARNINGS${NC}"
    echo "   Errors: $ERRORS"
    echo "   Warnings: $WARNINGS"
    echo ""
    echo "You can proceed, but review warnings above."
    exit 0
else
    echo -e "${RED}❌ FAILED${NC}"
    echo "   Errors: $ERRORS"
    echo "   Warnings: $WARNINGS"
    echo ""
    echo "Please fix errors before proceeding."
    exit 1
fi
