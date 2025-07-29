#ifndef BUFFER_HANDLE_H
#define BUFFER_HANDLE_H

#include <string>
#include <memory>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace evs {
namespace early {

struct BufferHandle {
    using BeginAccessFnc = int (*)(const BufferHandle *, bool, int);
    using EndAccessFnc = int (*)(const BufferHandle *, bool, int);

    int fd{-1};
    int handle{-1};
    void *virt{nullptr};
    uintptr_t phys{0U};
    size_t length{0U};
    BeginAccessFnc beginAccessFnc{nullptr};
    EndAccessFnc endAccessFnc{nullptr};
    std::mutex mtx;

    explicit BufferHandle(int _fd, int _handle, void *_virt, uintptr_t _phys, size_t _length)
        : fd(_fd)
        , handle(_handle)
        , virt(_virt)
        , phys(_phys)
        , length(_length) {
    }

    ~BufferHandle() {
    }

    BufferHandle(const BufferHandle &) = delete;
    BufferHandle &operator=(const BufferHandle &) = delete;
    BufferHandle(BufferHandle &&other) = delete;
    BufferHandle &operator=(BufferHandle &&other) = delete;

    int beginAccess(int flag = 0) {
        if (beginAccessFnc) {
            return beginAccessFnc(this, true, flag);
        }
        return -1;
    }
    int endAccess(int flag = 0) {
        if (endAccessFnc) {
            return endAccessFnc(this, false, flag);
        }
        return -1;
    }
};

using BufferHandlePtr = std::shared_ptr<BufferHandle>;

} // namespace early
} // namespace evs

#endif