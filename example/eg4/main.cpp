#include "MemAllocatorDevice.h"
#include <stdio.h>

int main(int argc, char const *argv[]) {
    evs::early::MemAllocatorDevice memDev("/dev/dma_heap/system");
    if (memDev.open() < 0) {
        return -1;
    }

    memDev.createBuffer(4, 1920 * 1080 * 4);
    memDev.close();

    for (const auto &buf : memDev.getBuffers()) {
        if (buf) {
            printf("fd=%d, virt=%p, phys=0x%lx, length=%zu\n",
                   buf->fd,
                   buf->virt,
                   static_cast<unsigned long>(buf->phys),
                   buf->length);

            std::unique_lock<std::mutex> lk(buf->mtx);
            if(buf->beginAccess(3) < 0) {
                printf("beginAccess failed\n");
            }
            if (buf->virt) {
                uint8_t *p = static_cast<uint8_t *>(buf->virt);
                for (size_t i = 0; i < buf->length; i++) {
                    p[i] = static_cast<uint8_t>(i);
                }
            }
            if (buf->endAccess(3) < 0) {
                printf("endAccess failed\n");
            }
        }
    }

    memDev.destroyBuffer();

    return 0;
}
