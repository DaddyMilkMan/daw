#include "platform/skia_context.h"
#include <skia/include/gpu/GrDirectContext.h>
#include <skia/include/gpu/GrBackendSurface.h>
#include <skia/include/gpu/gl/GrGLInterface.h>

namespace platform {

SkiaContext& SkiaContext::instance() {
    static SkiaContext inst;
    return inst;
}

SkiaContext::SkiaContext() {
    // Attempt to create a GL-based GrDirectContext. On platforms where Metal/D3D
    // is preferred, this factory should be replaced by the appropriate backend.
    auto interface = GrGLMakeNativeInterface();
    grContext_.reset(GrDirectContext::MakeGL(interface).release());
}

SkiaContext::~SkiaContext() {
    if (grContext_)
        grContext_->releaseResourcesAndAbandonContext();
}

sk_sp<SkSurface> SkiaContext::makeWindowSurface(void* platformNativeHandle, int width, int height) {
    if (!grContext_)
        return nullptr;

    GrGLFramebufferInfo fbInfo;
    fbInfo.fFBOID = reinterpret_cast<uint32_t>(platformNativeHandle);
    // FIXME: format needs to be chosen per-platform
    fbInfo.fFormat = GL_RGBA8;

    GrBackendRenderTarget backendRT(width, height, /*samples*/0, /*stencil*/0, fbInfo);
    SkSurfaceProps props(0, kUnknown_SkPixelGeometry);
    return SkSurface::MakeFromBackendRenderTarget(grContext_.get(), backendRT,
        kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType, nullptr, &props);
}

} // namespace platform
