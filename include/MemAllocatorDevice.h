#ifndef MEM_ALLOCATOR_DEVICE_H
#define MEM_ALLOCATOR_DEVICE_H

#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define USE_DMA_HEAP 1
#else
#define USE_ION 1
#endif

#if defined(USE_DMA_HEAP)
#include "DmaHeapDevice.h"
using MemDevice = evs::early::DmaHeapDevice;
#else
#include "IonDevice.h"
using MemDevice = evs::early::IonDevice;
#endif

#include <vector>

namespace evs {
namespace early {

class MemAllocatorDevice
{
public:
    explicit MemAllocatorDevice(const std::string &devicePath = "")
        : m_device(devicePath)
        , m_buffers{} {
    }
    ~MemAllocatorDevice() {}

    int open() {
        return m_device.open();
    }

    void close() {
        m_device.close();
    }

    void createBuffer(size_t count, size_t size) {
        for (size_t i = 0; i < count; i++) {
            m_buffers.emplace_back(m_device.allocate(size));
        }
    }

    void destroyBuffer() {
        for (auto &buf : m_buffers) {
            buf.reset();
        }
        m_buffers.clear();
    }

    const std::vector<std::shared_ptr<BufferHandle>> &getBuffers() const {
        return m_buffers;
    }

private:
    MemDevice m_device;
    std::vector<std::shared_ptr<BufferHandle>> m_buffers{};
};

} // namespace early
} // namespace evs
#endif // MEM_ALLOCATOR_DEVICE_H