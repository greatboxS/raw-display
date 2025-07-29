#pragma once

#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/dma-heap.h>
#include <linux/dma-buf.h>
#include <cerrno>
#include <cstring>
#include <iostream>

namespace early {

class DmaHeapBuffer
{
public:
    explicit DmaHeapBuffer(const std::string &heapPath = "/dev/dma_heap/system")
        : heapPath(heapPath)
        , heapFd(-1)
        , bufFd(-1)
        , addr(nullptr)
        , size(0) {}

    ~DmaHeapBuffer() {}

    bool openHeap();
    void closeHeap();

    bool allocate(size_t length, unsigned int flags = 0);

    void *map();
    void unmap();

    void release();

    bool beginAccess(bool write = false);
    bool endAccess(bool write = false);

    int getFd() const { return bufFd; }
    size_t getSize() const { return size; }
    void *getAddr() const { return addr; }

private:
    std::string heapPath;
    int heapFd;
    int bufFd;
    void *addr;
    size_t size;
};

}