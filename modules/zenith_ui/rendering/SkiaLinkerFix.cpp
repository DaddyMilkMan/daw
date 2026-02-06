/*
    This file is part of Zenith DAW - A Digital Audio Workstation for Linux

    Copyright (C) 2025 Micah Cooley <micahcooley@protonmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// SkiaLinkerFix.cpp

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
