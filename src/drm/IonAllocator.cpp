#include "DrmAllocator.h"
#include "CommonUtil.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/dma-buf.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <stdexcept>
#include <cstring>

#include <linux/ion.h>

#ifdef DEBUG_TAG
#undef DEBUG_TAG
#define DEBUG_TAG "EarlyDisplay IonAllocator"
#endif

namespace evs {
namespace early {
namespace drm {

static constexpr char *ION_DEVICE = "/dev/ion";
static constexpr int ION_HEAP_TYPE_SYSTEM = 1;

DrmBuffer *IonAllocator::allocate(int drmFd, const BufferInfo &info) {
    int ioFd = -1;
    int dmaBufFd = -1;
    uint32_t stride = 0;
    uint32_t handle = 0;
    uint32_t fbId = 0;
    uint32_t offset = 0;
    size_t size = 0;
    struct ion_allocation_data alloc = {};
    struct ion_fd_data fdData = {};
    struct drm_prime_handle prime = {};
    DrmBuffer *buf = nullptr;

    do {
        ionFd = open(ION_DEVICE, O_RDWR);
        if (ionFd < 0) {
            EARLY_ERROR("Failed to open %s\n", ION_DEVICE);
            break;
        }
        stride = info.width * (info.bpp / 8);
        size = stride * info.height;

        alloc.len = size;
        alloc.align = 0;
        alloc.heap_id_mask = 1 << ION_HEAP_TYPE_SYSTEM;
        alloc.flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
        alloc.fd = -1;

        if (ioctl(ionFd, ION_IOC_ALLOC, &alloc) < 0) {
            close(ionFd);
            EARLY_ERROR("ION_IOC_ALLOC failed\n");
            break;
        }

        fdData.handle = alloc.handle;

        if (ioctl(ionFd, ION_IOC_MAP, &fdData) < 0) {
            close(ionFd);
            EARLY_ERROR("ION_IOC_MAP failed\n");
            break;
        }

        dmaBufFd = fdData.fd;
        prime.fd = dmaBufFd;
        prime.flags = DRM_CLOEXEC | DRM_RDWR;

        if (drmIoctl(drmFd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime) != 0) {
            close(dmaBufFd);
            close(ionFd);
            EARLY_ERROR("DRM_IOCTL_PRIME_FD_TO_HANDLE failed\n");
            break;
        }

        handle = prime.handle;
        fbId = 0;

        if (info.format > 0) {
            if (drmModeAddFB2(drmFd, info.width, info.height, static_cast<uint32_t>(info.format), &handle, &stride, &offset, &fbId, 0) != 0) {
                EARLY_ERROR("Failed to add framebuffer with format\n");
                break;
            }
        } else {
            if (drmModeAddFB(drmFd, info.width, info.height, info.depth, info.bpp, stride, handle, &fbId) != 0) {
                EARLY_ERROR("Failed to add framebuffer without format\n");
                break;
            }
        }

        void *map = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, dmaBufFd, 0);
        if (map == MAP_FAILED) {
            close(dmaBufFd);
            close(ionFd);
            EARLY_ERROR("mmap failed\n");
            break;
        }

        struct ion_handle_data freeData = {};
        freeData.handle = alloc.handle;
        ioctl(ionFd, ION_IOC_FREE, &freeData);
        close(ionFd);
        close(dmaBufFd);

        buf = new (std::nothrow) DrmBuffer();
        if (buf == nullptr) {
            EARLY_ERROR("Failed to allocate memory for DrmBuffer\n");
            munmap(map, size);
            break;
        }

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

void IonAllocator::release(int drmFd, DrmBuffer *buf) {
    if (buf == nullptr) {
        EARLY_ERROR("Buffer is null, cannot release\n");
        return;
    }
    if (buf->ptr && buf->size > 0) {
        munmap(buf->ptr, buf->size);
    }
    if (buf->fbId) {
        drmModeRmFB(drmFd, buf->fbId);
    }
    if (buf->handle) {
        struct drm_gem_close req = {};
        req.handle = buf->handle;
        drmIoctl(drmFd, DRM_IOCTL_GEM_CLOSE, &req);
    }
    delete buf;
}

int IonAllocator::exposeHandleToFd(int fd, const DrmBuffer *buf) {
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