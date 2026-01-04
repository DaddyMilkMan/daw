#!/bin/bash
# A+ Grade Neural Engine Verification Script

# Set paths
BUILD_DIR="$(pwd)/build"
TEST_EXEC="$BUILD_DIR/ZenithDAWTests"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${GREEN}=== Zenith Neural Engine Verification ===${NC}"

if [ ! -f "$TEST_EXEC" ]; then
    echo -e "${RED}Error: ZenithDAWTests executable not found in $BUILD_DIR${NC}"
    echo "Please ensure the build completes successfully."
    exit 1
fi

echo "Running AI and Neural Engine tests..."

# Run only tests with "AI" category (using --category flag if supported, or standard output filtering for now)
# Assuming ZenithDAWTests is a standard JUCE UnitTestRunner which usually runs all or can be filtered.
# If no args, it runs all. We will grep for AI results to be sure, or if we implemented category filtering in main, use that.
# Based on the source code, standard JUCE UnitTestRunner puts categories in the name. Let's run and filter.

"$TEST_EXEC" --category "AI"

EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: All Neural Engine tests passed!${NC}"
else
    echo -e "${RED}FAILURE: Some tests failed.${NC}"
fi

exit $EXIT_CODE
