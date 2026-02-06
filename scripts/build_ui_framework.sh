#!/bin/bash

# Build UI Framework Integration Script
# This script builds and integrates the UI framework components

set -e

echo "Building UI Framework Integration..."

# Navigate to the project directory
cd "$(dirname "$0")/.."

# Create build directory
mkdir -p build/ui_framework

# Copy new UI framework files to appropriate locations
echo "Copying UI framework files..."

# Copy validation framework
cp -r modules/zenith_ui/ui/validation/ build/ui_framework/

# Copy error handling framework
cp -r modules/zenith_ui/ui/framework/UIErrorHandler.* build/ui_framework/

# Copy toast notification system
cp -r modules/zenith_ui/ui/controls/ToastNotificationManager.* build/ui_framework/

# Copy keyboard navigation system
cp -r modules/zenith_ui/ui/framework/TabOrderManager.* build/ui_framework/
cp -r modules/zenith_ui/ui/framework/KeyboardShortcutManager.* build/ui_framework/

# Update CMakeLists.txt to include new files
echo "Updating CMakeLists.txt..."

cat >> CMakeLists.txt << 'EOF'

# UI Framework Integration
set(UI_FRAMEWORK_SOURCES
    ui/validation/Validator.cpp
    ui/validation/Validator.h
    ui/validation/ValidationDecorator.cpp
    ui/validation/ValidationDecorator.h
    ui/validation/ValidationError.h
    ui/framework/UIErrorHandler.cpp
    ui/framework/UIErrorHandler.h
    ui/controls/ToastNotificationManager.cpp
    ui/controls/ToastNotificationManager.h
    ui/framework/TabOrderManager.cpp
    ui/framework/TabOrderManager.h
    ui/framework/KeyboardShortcutManager.cpp
    ui/framework/KeyboardShortcutManager.h
)

# Add to main UI library
target_sources(zenith_ui_unified PRIVATE ${UI_FRAMEWORK_SOURCES})

# Include directories
target_include_directories(zenith_ui_unified PRIVATE
    ui/validation
    ui/framework
    ui/controls
)

# Dependencies
target_link_libraries(zenith_ui_unified PRIVATE
    zenith_core
    zenith_ui_unified
)

EOF

echo "UI framework files copied and CMake updated."

# Build the project
echo "Building project with UI framework..."
mkdir -p build
cd build
cmake ..
make -j$(nproc)

# Run tests if available
if [ -f "ZenithTests" ]; then
    echo "Running UI framework tests..."
    ./ZenithTests --gtest_filter="UIFramework*"
fi

echo "UI framework build completed successfully!"

# Check for any errors
if [ $? -eq 0 ]; then
    echo "✅ UI framework integration successful!"
else
    echo "❌ UI framework integration failed!"
    exit 1
fi