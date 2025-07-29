#ifndef DMA_HEAP_DEVICE_H
#define DMA_HEAP_DEVICE_H

#include "BufferHandle.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <fcntl.h>

namespace evs {
namespace early {

class DmaHeapDevice
{
public:
    enum class ReadWriteFlags {
        SYNC_READ = 1 << 0,
        SYNC_WRITE = 1 << 1,
        SYNC_RW = SYNC_READ | SYNC_WRITE,
    };

    explicit DmaHeapDevice(const std::string &heapName = "/dev/dma_heap/system");
    ~DmaHeapDevice();

    int open();
    void close();

    BufferHandlePtr allocate(size_t length,
                             int fd_flags = O_CLOEXEC | O_RDWR,
                             uint32_t heap_flags = 0);

    static int freeBuffer(BufferHandle *buf);

    static int syncBuffer(const BufferHandle *buf,
                          bool start,
                          int write_flags = (static_cast<int>(ReadWriteFlags::SYNC_RW)));

    bool isOpen() const { return m_fd >= 0; };
    const std::string &path(void) const { return m_path; }
    int fd(void) const { return m_fd; }

private:
    int m_fd = -1;
    std::string m_path = "";
};

} // namespace early
} // namespace evs

#endif // DMA_HEAP_DEVICE_H
