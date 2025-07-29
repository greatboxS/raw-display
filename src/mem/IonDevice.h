#ifndef ION_DEVICE_H
#define ION_DEVICE_H

#include "BufferHandle.h"

#include <string>
#include <memory>
#include <cstdint>
#include <unistd.h>
#include <fcntl.h>

namespace evs {
namespace early {

class IonDevice
{
public:
    enum class ReadWriteFlags {
        SYNC_READ = 1 << 0,
        SYNC_WRITE = 1 << 1,
        SYNC_RW = SYNC_READ | SYNC_WRITE,
    };

    explicit IonDevice(const std::string &devPath = "/dev/ion");
    ~IonDevice();

    int open();
    void close();

    BufferHandlePtr allocate(size_t length,
                             size_t alignment = 0,
                             unsigned int heapMask = 1,
                             unsigned int flags = 0);

    static int freeBuffer(BufferHandle *buf);

    static int syncBuffer(const BufferHandle *buf,
                          bool start,
                          int rw_flags = (static_cast<int>(ReadWriteFlags::SYNC_RW)));

    bool isOpen() const { return m_fd >= 0; }
    const std::string &path() const { return m_path; }
    int fd() const { return m_fd; }

private:
    int m_fd = -1;
    std::string m_path;
};

} // namespace early
} // namespace evs
#endif // ION_DEVICE_H