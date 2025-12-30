/**
 * @file SkiaLinkerFix.cpp
 * @brief Manual RTTI symbol definitions for Skia
 * 
 * This file provides dummy typeinfo symbols for Skia types that are missing
 * because Skia was built with -fno-rtti but the DAW is built with RTTI enabled.
 * 
 * This allows us to keep RTTI enabled for JUCE and the DAW while still
 * linking against a non-RTTI Skia library.
 * 
 * Mangled names: _ZTI followed by length-prefixed type name.
 * Use c++filt to verify: c++filt _ZTI8SkCanvas -> typeinfo for SkCanvas
 */

#ifndef _WIN32

#include <typeinfo>

// We need to define the typeinfo for polymorphic Skia types that we touch
// from our RTTI-enabled code.

extern "C" {
    // SkCanvas (8 chars)
    void* _ZTI8SkCanvas = (void*)0x1;
    
    // GrDirectContext (15 chars)
    void* _ZTI15GrDirectContext = (void*)0x1;

    // SkRuntimeEffect (15 chars)
    void* _ZTI15SkRuntimeEffect = (void*)0x1;
    
    // SkTypeface (10 chars)
    void* _ZTI10SkTypeface = (void*)0x1;
    
    // SkSurface (9 chars)
    void* _ZTI9SkSurface = nullptr;
    
    // SkImage (7 chars)
    void* _ZTI7SkImage = nullptr;
    
    // SkPicture (9 chars)
    void* _ZTI9SkPicture = nullptr;
    
    // SkShader (8 chars)
    void* _ZTI8SkShader = nullptr;
    
    // SkColorFilter (13 chars)
    void* _ZTI13SkColorFilter = nullptr;
    
    // SkMaskFilter (12 chars)
    void* _ZTI12SkMaskFilter = nullptr;
    
    // SkPathEffect (12 chars)
    void* _ZTI12SkPathEffect = nullptr;
    
    // SkBlender (9 chars)
    void* _ZTI9SkBlender = nullptr;

    // Additional types that appeared in linker errors
    void* _ZTIN4skgpu12BudgetedE = nullptr;
}

#endif
