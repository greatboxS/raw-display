#include "DrmAllocator.h"
#include "CommonUtil.h"

#include <drm/drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <sys/mman.h>
#include <cstring>
#include <stdexcept>
#include <fcntl.h>

#ifdef DEBUG_TAG
#undef DEBUG_TAG
#define DEBUG_TAG "EarlyDisplay MMapAllocator"
#endif

namespace evs {
namespace early {
namespace drm {

DrmBuffer *MMapAllocator::allocate(int drmFd, const BufferInfo &info) {
    struct drm_mode_create_dumb creq = {};
    struct drm_mode_map_dumb mreq = {};
    DrmBuffer *buf = nullptr;

    do {
        if (info.flags > 0) {
            creq.flags = info.flags;
        }
        creq.width = info.width;
        creq.height = info.height;
        creq.bpp = info.bpp;
        if (drmIoctl(drmFd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) < 0) {
            EARLY_ERROR("Failed to create dumb buffer\n");
            break;
        }

        uint32_t handle = creq.handle;
        uint32_t stride = creq.pitch;
        size_t size = creq.size;
        uint32_t fbId = 0;
        uint32_t offset = 0;
        struct drm_mode_destroy_dumb dreq = {};
        dreq.handle = handle;

        if (info.format > 0) {
            if (drmModeAddFB2(drmFd, info.width, info.height, static_cast<uint32_t>(info.format), &handle, &stride, &offset, &fbId, 0) != 0) {
                EARLY_ERROR("Failed to add framebuffer with format %u\n", info.format);
                drmModeRmFB(drmFd, fbId);
                if (drmIoctl(drmFd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq) != 0) {
                    EARLY_ERROR("Failed to destroy dumb buffer after failed framebuffer addition\n");
                }
                break;
            }
        } else {
            if (drmModeAddFB(drmFd, info.width, info.height, info.depth, info.bpp, stride, handle, &fbId) != 0) {
                EARLY_ERROR("Failed to add framebuffer without format\n");
                drmModeRmFB(drmFd, fbId);
                if (drmIoctl(drmFd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq) != 0) {
                    EARLY_ERROR("Failed to destroy dumb buffer after failed framebuffer addition\n");
                }
                break;
            }
        }

        mreq.handle = handle;
        if (drmIoctl(drmFd, DRM_IOCTL_MODE_MAP_DUMB, &mreq) != 0) {
            EARLY_ERROR("Failed to map dumb buffer\n");
            drmModeRmFB(drmFd, fbId);
            if (drmIoctl(drmFd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq) != 0) {
                EARLY_ERROR("Failed to destroy dumb buffer after failed framebuffer addition\n");
            }
            break;
        }

        void *map = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, drmFd, mreq.offset);
        if (map == MAP_FAILED) {
            map = nullptr;
            EARLY_ERROR("mmap failed\n");
            drmModeRmFB(drmFd, fbId);
            if (drmIoctl(drmFd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq) != 0) {
                EARLY_ERROR("Failed to destroy dumb buffer after mmap failure\n");
            }
            break;
        }

        buf = new (std::nothrow) DrmBuffer();
        if (buf == nullptr) {
            EARLY_ERROR("Failed to allocate memory for DrmBuffer\n");
            munmap(map, size);
            drmModeRmFB(drmFd, fbId);
            if (drmIoctl(drmFd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq) != 0) {
                EARLY_ERROR("Failed to destroy dumb buffer after DrmBuffer allocation failure\n");
            }
            break;
        }

        std::memset(buf, 0, sizeof(DrmBuffer));
        std::memset(map, 0, size);
        buf->fbId = fbId;
        buf->ptr = map;
        buf->size = size;
        buf->handle = handle;
        buf->stride = stride;
        buf->offset = mreq.offset;

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

void MMapAllocator::release(int drmFd, DrmBuffer *buf) {
    if (buf == nullptr) {
        EARLY_ERROR("Buffer is null, cannot release\n");
        return;
    }
    if (buf->ptr) {
        munmap(buf->ptr, buf->size);
    }
    if (buf->fbId) {
        drmModeRmFB(drmFd, buf->fbId);
    }
    if (buf->handle) {
        drm_mode_destroy_dumb dreq = {};
        dreq.handle = buf->handle;
        drmIoctl(drmFd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
    }
    delete buf;
}

int MMapAllocator::exposeHandleToFd(int fd, const DrmBuffer *buf) {
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