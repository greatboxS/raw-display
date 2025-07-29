#include "DrmController.h"
#include "CommonUtil.h"

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <memory>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <sys/mman.h>
#include <deque>

#ifdef DEBUG_TAG
#undef DEBUG_TAG
#define DEBUG_TAG "EarlyDisplay Controller"
#endif

#define USE_HEAPDMA_ALLOCATOR

namespace evs::early::drm {
DrmController::DrmController(int cardId)
    : m_device(cardId)
    , m_width(DEFAULT_WIDTH)
    , m_height(DEFAULT_HEIGHT)
    , m_bpp(DEFAULT_BPP)
    , m_format(INVALID_FORMAT)
    , m_flags(INVALID_FLAGS)
    , m_stride(INVALID_STRIDE)
    , m_size(INVALID_SIZE)
    , m_offset(DEFAULT_OFFSET)
    , m_pitch(DEFAULT_PITCH) {
}

DrmController::~DrmController() {
}

bool DrmController::init(size_t width, size_t height, uint8_t bpp, size_t stride, int format, int flags) {
    bool success = false;
    m_width = width;
    m_height = height;
    m_bpp = bpp;
    m_format = format;
    m_flags = flags;
    m_stride = stride;
    m_size = width * height * (bpp / 8U);

    do {
        if (m_size == 0) {
            EARLY_ERROR("Invalid buffer size: width=%zu, height=%zu, bpp=%u\n", width, height, bpp);
            break;
        }
        if (m_stride == 0) {
            m_stride = width * (bpp / 8);
            EARLY_DEBUG("Stride not specified, calculated as width * (bpp / 8): %zu\n", m_stride);
        }

        if (m_stride < width * (bpp / 8)) {
            EARLY_WARN("Invalid stride: %zu, must be at least width * (bpp / 8): %zu\n", m_stride, width * (bpp / 8));
        }

        m_offset = 0;
        m_pitch = stride;

        if (m_device.open() == false) {
            EARLY_ERROR("Failed to open DRM device.\n");
            break;
        }
        m_device.queryAllDeviceInfo();
        for (auto &connector : m_device.getConnectors()) {
            if (connector.second.connected == true) {
                m_connectedConnector = connector.second;
                break;
            }
        }

        EARLY_DEBUG("DRM device initialized successfully: width=%zu, height=%zu, bpp=%u, stride=%zu, format=%d, flags=%d\n",
                    m_width,
                    m_height,
                    m_bpp,
                    m_stride,
                    m_format,
                    m_flags);

        BufferInfo info = {
            static_cast<uint32_t>(m_width),
            static_cast<uint32_t>(m_height),
            static_cast<uint8_t>(m_bpp),
            0,
            m_format,
            m_flags,
            static_cast<int>(m_offset),
            static_cast<int>(m_stride),
            m_size};
        int fd = m_device.fd();

        for (uint32_t i = 0; i < MAX_BUFFER_COUNT; ++i) {
            DrmBuffer *buffer = nullptr;
#if defined(USE_HEAPDMA_ALLOCATOR)
            buffer = HeapDMAAllocator::allocate(fd, info);
#elif defined(USE_ION_ALLOCATOR)
            buffer = IonAllocator::allocate(fd, info);
#else
            buffer = MMapAllocator::allocate(fd, info);
#endif
            if (buffer == nullptr) {
                EARLY_ERROR("Failed to allocate buffer using MMapAllocator\n");
                break;
            }

            m_buffers.emplace_back(buffer);
            EARLY_DEBUG("Allocated buffer %u: fbId=%u, handle=0x%x, size=%zu, stride=%u, offset=%u\n",
                        i,
                        buffer->fbId,
                        buffer->handle,
                        buffer->size,
                        buffer->stride,
                        buffer->offset);
        }
        if (m_buffers.empty()) {
            EARLY_ERROR("Failed to allocate any buffers.\n");
            break;
        }
        success = true;
    } while (false);

    if (success == false) {
        EARLY_ERROR("Failed to initialize DRM display controller with width=%zu, height=%zu, bpp=%u, stride=%zu, format=%d, flags=%d\n",
                    width,
                    height,
                    bpp,
                    stride,
                    format,
                    flags);
        deInit();
    }

    return success;
}

bool DrmController::deInit() {
    bool success = false;

    m_connectedConnector = DrmConnectorInfo();
    m_width = DEFAULT_WIDTH;
    m_height = DEFAULT_HEIGHT;
    m_bpp = DEFAULT_BPP;
    m_format = INVALID_FORMAT;
    m_flags = INVALID_FLAGS;
    m_stride = INVALID_STRIDE;
    m_size = INVALID_SIZE;
    m_offset = DEFAULT_OFFSET;
    m_pitch = DEFAULT_PITCH;

    if (m_device.isOpen()) {
        for (auto buffer : m_buffers) {
            if (buffer) {
                MMapAllocator::release(m_device.fd(), buffer);
            }
        }
        m_buffers.clear();
        m_device.close();
        success = true;
    } else {
        EARLY_ERROR("DRM device is not open, cannot deinitialize.\n");
    }

    return success;
}

void DrmController::drawFrame() {
    m_currentBufferId = (m_currentBufferId ^ 1);
    m_currentBufferIndex = m_currentBufferId % m_buffers.size();
    auto &currentBuffer = m_buffers[m_currentBufferIndex];

    if (currentBuffer == nullptr) {
        EARLY_ERROR("Current buffer is null, cannot draw frame.\n");
        return;
    }

    EARLY_DEBUG("Drawing frame on buffer %zu with fbId=%u, handle=0x%x, size=%zu, stride=%u, offset=%u\n",
                m_currentBufferIndex,
                currentBuffer->fbId,
                currentBuffer->handle,
                currentBuffer->size,
                currentBuffer->stride,
                currentBuffer->offset);

    // Implement frame drawing logic here
    EARLY_DEBUG("Drawing frame on DRM display controller.\n");
}

void DrmController::syncFrame() {
    if (!isInit()) {
        EARLY_ERROR("Display controller is not initialized, cannot sync frame.\n");
        return;
    }

    // Implement frame synchronization logic here
    EARLY_DEBUG("Synchronizing frame on DRM display controller.\n");
}

void DrmController::swapFrame() {
    if (!isInit()) {
        EARLY_ERROR("Display controller is not initialized, cannot swap frame.\n");
        return;
    }

    // Implement frame swapping logic here
    EARLY_DEBUG("Swapping frame on DRM display controller.\n");
}

} // namespace evs::early::drm