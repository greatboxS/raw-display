#include "DmaHeapDevice.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/dma-heap.h>
#include <linux/dma-buf.h>
#include <stdio.h>
#include <string.h>

namespace evs {
namespace early {

static void deleteDmaHeapBufferHandle(BufferHandle *buf) {
    if (buf != nullptr) {
        DmaHeapDevice::freeBuffer(buf);
        delete buf;
    }
}

static void print_errno(const char *msg) {
    int e;
    e = errno;
    fprintf(stderr, "%s: %s (%d)\n", msg, strerror(e), e);
}

DmaHeapDevice::DmaHeapDevice(const std::string &path)
    : m_fd(-1)
    , m_path(path) {}

DmaHeapDevice::~DmaHeapDevice() {
}

int DmaHeapDevice::open() {
    int ret = 0;
    int fd = ::open(this->m_path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        print_errno("open: open heap failed");
        ret = -errno;
    } else {
        this->m_fd = fd;
    }
    return ret;
}

void DmaHeapDevice::close(void) {
    if (this->m_fd >= 0) {
        ::close(this->m_fd);
        this->m_fd = -1;
    }
}

BufferHandlePtr DmaHeapDevice::allocate(size_t length, int fd_flags, uint32_t heap_flags) {
    struct dma_heap_allocation_data data;
    void *addr = nullptr;

    std::memset(&data, 0, sizeof(data));

    data.len = length;
    data.fd_flags = fd_flags;
    data.heap_flags = heap_flags;

    if (m_fd < 0) {
        return nullptr;
    }

    if (::ioctl(m_fd, DMA_HEAP_IOCTL_ALLOC, &data) < 0) {
        print_errno("DMA_HEAP_IOCTL_ALLOC failed");
        return nullptr;
    }

    addr = ::mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, data.fd, 0);
    if (addr == MAP_FAILED) {
        if (::close(data.fd) < 0) {
            print_errno("Close fd after mmap failed");
        }
        print_errno("mmap failed");
        return nullptr;
    }

    BufferHandle *raw = new BufferHandle(data.fd, -1, addr, 0, length);
    raw->beginAccessFnc = DmaHeapDevice::syncBuffer;
    raw->endAccessFnc = DmaHeapDevice::syncBuffer;

    return BufferHandlePtr(raw, deleteDmaHeapBufferHandle);
}

int DmaHeapDevice::syncBuffer(const BufferHandle *buf, bool start, int write_flags) {
    if (!buf || buf->fd < 0) {
        return -EINVAL;
    }

    struct dma_buf_sync sync;
    std::memset(&sync, 0, sizeof(sync));
    sync.flags = start ? DMA_BUF_SYNC_START | write_flags
                       : DMA_BUF_SYNC_END | write_flags;

    if (::ioctl(buf->fd, DMA_BUF_IOCTL_SYNC, &sync) < 0) {
        print_errno("DMA_BUF_IOCTL_SYNC failed");
        return -errno;
    }

    return 0;
}

int DmaHeapDevice::freeBuffer(BufferHandle *buf) {
    if (!buf) {
        return -EINVAL;
    }

    if (buf->virt != nullptr) {
        if (::munmap(buf->virt, buf->length) != 0) {
            print_errno("munmap failed");
        }
        buf->virt = nullptr;
    }

    if (buf->fd >= 0) {
        ::close(buf->fd);
        buf->fd = -1;
    }

    buf->handle = 0;
    buf->phys = 0;
    buf->length = 0;
    return 0;
}

} // namespace early
} // namespace evs