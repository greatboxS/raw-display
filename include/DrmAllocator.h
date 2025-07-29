#ifndef BUFFERALLOCATOR_H
#define BUFFERALLOCATOR_H

#include <cstdint>
#include <cstddef>

namespace evs {
namespace early {
namespace drm {

typedef struct {
    uint32_t fbId;
    uint32_t handle;
    uint32_t stride;
    uint32_t offset;
    size_t size;
    void *ptr;
} DrmBuffer;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t depth;
    int format;
    int flags;
    int offset;
    int pitch;
    size_t size;
} BufferInfo;

typedef enum {
    DRM_ALLOCATOR_MMAP,
    DRM_ALLOCATOR_HEAP_DMA,
#ifdef SUPPORT_ION_ALLOCATOR
    DRM_ALLOCATOR_ION,
#endif // SUPPORT_ION_ALLOCATOR
    DRM_ALLOCATOR_UNKNOWN
} AllocatorType;

/**
 * @brief Allocator for mmap buffers.
 * This allocator uses the DRM subsystem to allocate buffers using mmap.
 * It is suitable for use with DRM devices that support dumb buffers.
 * It provides a way to allocate buffers that can be used with DRM.
 * It is designed to work with the DRM subsystem, which allows for
 * efficient memory management and allocation of buffers.
 * It is particularly useful for allocating buffers that need to be shared
 * between different components or processes, such as video processing or
 * rendering.
 * It uses the DRM_IOCTL_MODE_CREATE_DUMB and DRM_IOCTL_MODE_MAP_DUMB
 */
class MMapAllocator
{
public:
    static DrmBuffer *allocate(int fd, const BufferInfo &info);
    static void release(int fd, DrmBuffer *buf);
    static int exposeHandleToFd(int fd, const DrmBuffer *buf);
};

/**
 * @brief Allocator for DMA heap buffers.
 * This allocator uses the DMA heap subsystem to allocate buffers.
 * It is suitable for use with DRM devices that support DMA heap allocations.
 * It provides a way to allocate buffers that can be used with DRM.
 * It is designed to work with the DMA heap subsystem, which allows for
 * efficient memory management and allocation of buffers.
 * It is particularly useful for allocating buffers that need to be shared
 * between different components or processes, such as video processing or
 * rendering.
 */
class HeapDMAAllocator
{
public:
    static DrmBuffer *allocate(int fd, const BufferInfo &info);
    static void release(int fd, DrmBuffer *buf);
    static int exposeHandleToFd(int fd, const DrmBuffer *buf);
};

#ifdef SUPPORT_ION_ALLOCATOR
/**
 * @brief Allocator for ION buffers.
 * It uses the ION_IOC_ALLOC and ION_IOC_MAP ioctls to allocate and map buffers.
 * It is designed to work with the ION subsystem, which allows for
 * efficient memory management and allocation of buffers.
 * It is particularly useful for allocating buffers that need to be shared
 * between different components or processes, such as video processing or
 * rendering.
 */
class IonAllocator
{
public:
    static DrmBuffer *allocate(int fd, const BufferInfo &info);
    static void release(int fd, DrmBuffer &buf);
    static int exposeHandleToFd(int fd, const DrmBuffer *buf);
};

#endif // SUPPORT_ION_ALLOCATOR

} // namespace drm
} // namespace early
} // namespace evs

#endif
