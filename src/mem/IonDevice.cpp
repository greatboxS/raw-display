#include "IonDevice.h"

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/ion.h>
#include <cerrno>
#include <cstring>
#include <iostream>

namespace evs {
namespace early {

static void deleteIONBufferHandle(BufferHandle *buf) {
    if (buf != nullptr) {
        IonDevice::freeBuffer(buf);
        delete buf;
    }
}

static void print_errno(const char *msg) {
    int e = errno;
    fprintf(stderr, "%s: %s (%d)\n", msg, strerror(e), e);
}

IonDevice::IonDevice(const std::string &devicePath)
    : m_fd(-1)
    , m_path(devicePath) {}

IonDevice::~IonDevice() {
    close();
}

int IonDevice::open() {
    if (m_fd >= 0) {
        return 0;
    }
    m_fd = ::open(m_path.c_str(), O_RDWR | O_CLOEXEC);
    if (m_fd < 0) {
        print_errno("IonDevice open failed");
        return -errno;
    }
    return 0;
}

void IonDevice::close() {
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

BufferHandlePtr IonDevice::allocate(size_t length,
                                    size_t alignment,
                                    unsigned int heapMask,
                                    unsigned int flags) {
    if (m_fd < 0) {
        print_errno("IonDevice not opened");
        return nullptr;
    }

    struct ion_allocation_data alloc_data = {};
    alloc_data.len = length;
    alloc_data.align = (alignment == 0)
                           ? static_cast<size_t>(sysconf(_SC_PAGESIZE))
                           : alignment;
    alloc_data.heap_id_mask = heapMask;
    alloc_data.flags = flags;

    if (ioctl(m_fd, ION_IOC_ALLOC, &alloc_data) < 0) {
        print_errno("ION_IOC_ALLOC failed");
        return nullptr;
    }

    void *virt = mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, alloc_data.fd, 0);
    if (virt == MAP_FAILED) {
        print_errno("IonDevice mmap failed");
        ::close(alloc_data.fd);
        return nullptr;
    }

    auto raw = new BufferHandle(alloc_data.fd, virt, 0, length, 0);
    raw->beginAccessFnc = IonDevice::syncBuffer;
    raw->endAccessFnc = IonDevice::syncBuffer;

    return BufferHandlePtr(raw, deleteIONBufferHandle);
}

int IonDevice::freeBuffer(BufferHandle *buf) {
    if (buf == nullptr) {
        return -EINVAL;
    }
    if (buf->virt) {
        munmap(buf->virt, buf->len);
    }
    if (buf->fd >= 0) {
        ::close(buf->fd);
    }
    return 0;
}

int IonDevice::syncBuffer(const BufferHandle *buf,
                          bool start,
                          int write_flags) {
    if (buf == nullptr) {
        return -EINVAL;
    }

    struct ion_sync_data sync_data = {};
    sync_data.fd = buf->fd;
    sync_data.flags = write_flags;

    int ret = ioctl(m_fd, ION_IOC_SYNC, &sync_data);
    if (ret < 0) {
        print_errno("ION_IOC_SYNC failed");
        return -errno;
    }
    return 0;
}

} // namespace early
} // namespace evs
