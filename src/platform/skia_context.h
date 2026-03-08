#pragma once

#include <memory>
#include <skia/include/core/SkSurface.h>
#include <skia/include/gpu/GrDirectContext.h>

namespace platform {

class SkiaContext {
public:
    static SkiaContext& instance();

    GrDirectContext* grContext() const noexcept { return grContext_.get(); }
    sk_sp<SkSurface> makeWindowSurface(void* platformNativeHandle, int width, int height);

private:
    SkiaContext();
    ~SkiaContext();

    std::unique_ptr<GrDirectContext> grContext_;
};

} // namespace platform
