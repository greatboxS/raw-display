#include "DrmAllocator.h"
#include "CommonUtil.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/dma-heap.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <stdexcept>
#include <cstring>
#include <drm/drm.h>

#ifdef DEBUG_TAG
#undef DEBUG_TAG
#define DEBUG_TAG "EarlyDisplay HeapDMAAllocator"
#endif

namespace evs {
namespace early {
namespace drm {

constexpr const char *DMA_HEAP_DEVICE = "/dev/dma_heap/system";

DrmBuffer *HeapDMAAllocator::allocate(int drmFd, const BufferInfo &info) {
    int heapFd = -1;
    int dmaFd = -1;
    uint32_t stride = 0;
    uint32_t handle = 0;
    uint32_t fbId = 0;
    uint32_t offset = 0;
    size_t size = 0;
    struct dma_heap_allocation_data alloc = {};
    struct drm_prime_handle prime = {};
    DrmBuffer *buf = nullptr;

    do {
        heapFd = open(DMA_HEAP_DEVICE, O_RDWR | O_CLOEXEC);
        if (heapFd < 0) {
            EARLY_ERROR("Failed to open %s\n", DMA_HEAP_DEVICE);
            break;
        }

        stride = info.width * (info.bpp / 8);
        size = stride * info.height;

        alloc.len = size;
        alloc.fd_flags = O_RDWR | O_CLOEXEC;
        alloc.heap_flags = 0;

        if (ioctl(heapFd, DMA_HEAP_IOCTL_ALLOC, &alloc) < 0) {
            close(heapFd);
            EARLY_ERROR("DMA_HEAP_IOCTL_ALLOC failed\n");
            break;
        }
        close(heapFd);

        dmaFd = alloc.fd;

        prime.fd = dmaFd;
        prime.flags = DRM_CLOEXEC | DRM_RDWR;

        if (drmIoctl(drmFd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime) != 0) {
            close(dmaFd);
            EARLY_ERROR("DRM_IOCTL_PRIME_FD_TO_HANDLE failed\n");
            break;
        }

        struct drm_gem_close req = {};
        handle = prime.handle;
        fbId = 0;

        if (info.format > 0) {
            if (drmModeAddFB2(drmFd, info.width, info.height, static_cast<uint32_t>(info.format), &handle, &stride, &offset, &fbId, 0) != 0) {
                EARLY_ERROR("Failed to add framebuffer with format\n");
                req.handle = handle;
                if (drmIoctl(drmFd, DRM_IOCTL_GEM_CLOSE, &req) != 0) {
                    EARLY_ERROR("Failed to close GEM handle after framebuffer addition failure\n");
                }
                close(dmaFd);
                break;
            }
        } else {
            if (drmModeAddFB(drmFd, info.width, info.height, info.depth, info.bpp, stride, handle, &fbId) != 0) {
                EARLY_ERROR("Failed to add framebuffer without format\n");
                req.handle = handle;
                if (drmIoctl(drmFd, DRM_IOCTL_GEM_CLOSE, &req) != 0) {
                    EARLY_ERROR("Failed to close GEM handle after framebuffer addition failure\n");
                }
                close(dmaFd);
                break;
            }
        }

        void *map = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, dmaFd, 0);
        if (map == MAP_FAILED) {
            map = nullptr;
            req.handle = handle;
            if (drmIoctl(drmFd, DRM_IOCTL_GEM_CLOSE, &req) != 0) {
                EARLY_ERROR("Failed to close GEM handle after mmap failure\n");
            }
            close(dmaFd);
            EARLY_ERROR("mmap failed\n");
            break;
        }

        buf = new (std::nothrow) DrmBuffer();
        if (buf == nullptr) {
            EARLY_ERROR("Failed to allocate memory for DrmBuffer\n");
            if (munmap(map, size) != 0) {
                EARLY_ERROR("munmap failed after DrmBuffer allocation failure\n");
            }
            req.handle = handle;
            if (drmIoctl(drmFd, DRM_IOCTL_GEM_CLOSE, &req) != 0) {
                EARLY_ERROR("Failed to close GEM handle after DrmBuffer allocation failure\n");
            }
            close(dmaFd);
            break;
        }
        close(dmaFd);

        memset(buf, 0, sizeof(DrmBuffer));
        memset(map, 0, size);
        buf->fbId = fbId;
        buf->ptr = map;
        buf->size = size;
        buf->handle = handle;
        buf->stride = stride;
        buf->offset = offset;
        EARLY_DEBUG("Allocated buffer: fbId=%u, handle=0x%x, size=%zu, stride=%u, offset=%u, ptr=%p\n",
                    buf->fbId,
                    buf->handle,
                    buf->size,
                    buf->stride,
                    buf->offset,
                    buf->ptr);
    } while (false);
    return buf;
}

void HeapDMAAllocator::release(int drmFd, DrmBuffer *buf) {
    struct drm_gem_close req = {};

    if (buf == nullptr) {
        EARLY_ERROR("Buffer is null, nothing to release\n");
        return;
    }
    if (buf->ptr && buf->size > 0) {
        munmap(buf->ptr, buf->size);
    }
    if (buf->fbId) {
        drmModeRmFB(drmFd, buf->fbId);
    }
    if (buf->handle) {
        req.handle = buf->handle;
        drmIoctl(drmFd, DRM_IOCTL_GEM_CLOSE, &req);
    }
    delete buf;
}

int HeapDMAAllocator::exposeHandleToFd(int fd, const DrmBuffer *buf) {
    int dmaFd = -1;
    do {
        if (buf == nullptr) {
            EARLY_ERROR("Invalid buffer or handle\n");
            break;
        }

        if (buf->handle == 0) {
            EARLY_ERROR("Buffer handle is zero, cannot expose DMA buffer\n");
            break;
        }

        // if (drmPrimeHandleToFD(fd, buf->handle, DRM_CLOEXEC | DRM_RDWR, &dmaFd) != 0) {
        //     EARLY_ERROR("drmPrimeHandleToFD failed for handle %u: %s\n", buf->handle, strerror(errno));
        //     break;
        // }
        // EARLY_DEBUG("Exposed DMA buffer handle %u to FD %d\n", buf->handle, dmaFd);

        struct drm_prime_handle prime = {};
        prime.handle = buf->handle;
        prime.flags = DRM_CLOEXEC | DRM_RDWR;
        if (drmIoctl(fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime) != 0) {
            EARLY_ERROR("DRM_IOCTL_PRIME_HANDLE_TO_FD failed: %s\n", strerror(errno));
            break;
        }
        dmaFd = prime.fd;
    } while (false);

    return dmaFd;
}

} // namespace drm
} // namespace early
} // namespace evs