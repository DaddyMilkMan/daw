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

// Static dummy variable to provide a unique, valid address for non-null symbols
static char dummy_typeinfo = 0;

extern "C" {
    // SkCanvas (8 chars)
    void* _ZTI8SkCanvas = &dummy_typeinfo;
    
    // GrDirectContext (15 chars)
    void* _ZTI15GrDirectContext = &dummy_typeinfo;

    // SkRuntimeEffect (15 chars)
    void* _ZTI15SkRuntimeEffect = &dummy_typeinfo;
    
    // SkTypeface (10 chars)
    void* _ZTI10SkTypeface = &dummy_typeinfo;
    
    // SkSurface (9 chars)
    void* _ZTI9SkSurface = &dummy_typeinfo;
    
    // SkImage (7 chars)
    void* _ZTI7SkImage = &dummy_typeinfo;
    
    // SkPicture (9 chars)
    void* _ZTI9SkPicture = &dummy_typeinfo;
    
    // SkShader (8 chars)
    void* _ZTI8SkShader = &dummy_typeinfo;
    
    // SkColorFilter (13 chars)
    void* _ZTI13SkColorFilter = &dummy_typeinfo;
    
    // SkMaskFilter (12 chars)
    void* _ZTI12SkMaskFilter = &dummy_typeinfo;
    
    // SkPathEffect (12 chars)
    void* _ZTI12SkPathEffect = &dummy_typeinfo;
    
    // SkBlender (9 chars)
    void* _ZTI9SkBlender = &dummy_typeinfo;

    // Additional types that appeared in linker errors
    void* _ZTIN4skgpu12BudgetedE = &dummy_typeinfo;
}

#endif
