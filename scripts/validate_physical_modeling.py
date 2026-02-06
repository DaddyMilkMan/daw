#!/usr/bin/env python3

import os
import re

def validate_implementation():
    """Validate the physical modeling implementation"""
    
    print("Validating Physical Modeling Implementation...")
    print("=" * 50)
    
    # Check for required files
    required_files = [
        "modules/zenith_core/instruments/ZenithUltraSynth/physical_modeling/StringModel.h",
        "modules/zenith_core/instruments/ZenithUltraSynth/physical_modeling/StringModelVoice.cpp",
        "modules/zenith_core/instruments/ZenithUltraSynth/physical_modeling/WindModel.h",
        "modules/zenith_core/instruments/ZenithUltraSynth/physical_modeling/WindModelVoice.cpp",
        "modules/zenith_core/instruments/ZenithUltraSynth/physical_modeling/PercussionModel.h",
        "modules/zenith_core/instruments/ZenithUltraSynth/physical_modeling/PercussionModelVoice.cpp"
    ]
    
    missing_files = []
    for file_path in required_files:
        if not os.path.exists(file_path):
            missing_files.append(file_path)
        else:
            print(f"✓ Found: {file_path}")
    
    if missing_files:
        print("\nMissing files:")
        for file in missing_files:
            print(f"✗ Missing: {file}")
        return False
    
    # Check for implementation content
    implementation_files = [
        "modules/zenith_core/instruments/ZenithUltraSynth/physical_modeling/StringModelVoice.cpp",
        "modules/zenith_core/instruments/ZenithUltraSynth/physical_modeling/WindModelVoice.cpp",
        "modules/zenith_core/instruments/ZenithUltraSynth/physical_modeling/PercussionModelVoice.cpp"
    ]
    
    required_methods = [
        "noteOn",
        "noteOff",
        "process",
        "isActive",
        "reset"
    ]
    
    for file_path in implementation_files:
        print(f"\nChecking implementation in {file_path}")
        
        with open(file_path, 'r') as f:
            content = f.read()
        
        # Check for required methods
        for method in required_methods:
            if f"{method}(" in content:
                print(f"✓ Method: {method}")
            else:
                print(f"✗ Missing: {method}")
        
        # Check for MPE support
        if "setPitchBend" in content and "setPressure" in content:
            print("✓ MPE support")
        else:
            print("✗ MPE support missing")
        
        # Check for audio processing
        if "process(" in content and "AudioBuffer" in content:
            print("✓ Audio processing")
        else:
            print("✗ Audio processing missing")
        
        # Check for error handling
        if "try {" in content and "catch" in content:
            print("✓ Error handling")
        else:
            print("✗ Error handling missing")
        
        # Check for performance optimization
        if "simd" in content.lower() or "optimiz" in content.lower():
            print("✓ Performance optimization")
        else:
            print("⚠ Performance optimization not found")
    
    # Check test files
    print("\nChecking test files...")
    test_files = [
        "tests/physical_modeling/PhysicalModelingTest.h",
        "tests/physical_modeling/PhysicalModelingTest.cpp",
        "tests/physical_modeling/TestRunner.cpp"
    ]
    
    for file_path in test_files:
        if os.path.exists(file_path):
            print(f"✓ Found: {file_path}")
        else:
            print(f"✗ Missing: {file_path}")
    
    # Check documentation
    doc_file = "modules/zenith_core/instruments/ZenithUltraSynth/physical_modeling/README.md"
    if os.path.exists(doc_file):
        print(f"\n✓ Documentation: {doc_file}")
        
        # Check documentation content
        with open(doc_file, 'r') as f:
            doc_content = f.read()
        
        if "StringModel" in doc_content and "WindModel" in doc_content and "PercussionModel" in doc_content:
            print("✓ All models documented")
        else:
            print("✗ Documentation incomplete")
    else:
        print(f"\n✗ Documentation missing: {doc_file}")
    
    print("\nValidation completed!")
    return True

if __name__ == "__main__":
    validate_implementation()
