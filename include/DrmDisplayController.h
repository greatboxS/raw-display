#ifndef DRMDISPLAYCONTROLLER_H
#define DRMDISPLAYCONTROLLER_H

#include "DrmAllocator.h"
#include "DrmDevice.h"

#include <cstddef>
#include <memory>
#include <string>
#include <drm/drm_fourcc.h>
namespace evs {
namespace early {
namespace drm {

class DrmController
{
    DrmController(const DrmController &) = delete;
    DrmController &operator=(const DrmController &) = delete;
    DrmController(DrmController &&) = delete;
    DrmController &operator=(DrmController &&) = delete;

public:
    DrmController(int cardId = 0);
    ~DrmController();

    bool init(size_t width, size_t height, uint8_t bpp, size_t stride, int format = 0, int flags = 0);
    bool deInit();

    bool isInit() const { return m_device.isOpen(); }

    size_t width() const { return m_width; }
    size_t height() const { return m_height; }
    int bpp() const { return m_bpp; }
    int format() const { return m_format; }
    int flags() const { return m_flags; }
    size_t stride() const { return m_stride; }
    size_t size() const { return m_size; }
    size_t offset() const { return m_offset; }
    int pitch() const { return m_pitch; }

    const DrmDevice &device() const { return m_device; }

    void drawFrame();
    void syncFrame();
    void swapFrame();

private:
    static constexpr size_t MAX_BUFFER_COUNT = 2U;  // Maximum number of buffers in the ring
    static constexpr size_t DEFAULT_WIDTH = 1920U;  // Default width for buffers
    static constexpr size_t DEFAULT_HEIGHT = 1080U; // Default height for buffers
    static constexpr int DEFAULT_BPP = 32;          // Default bits per pixel
    static constexpr int DEFAULT_FORMAT = DRM_FORMAT_XRGB8888; // Default format for buffers
    static constexpr int DEFAULT_FLAGS = 0;         // Default flags for buffer creation
    static constexpr size_t DEFAULT_STRIDE = 0U;    // Default stride in bytes
    static constexpr size_t DEFAULT_SIZE = 0U;      // Default size in bytes
    static constexpr size_t DEFAULT_OFFSET = 0U;    // Default offset in bytes
    static constexpr int DEFAULT_PITCH = 0;         // Default pitch in bytes
    static constexpr int DEFAULT_CARD_ID = 0;       // Default card ID
    static constexpr int INVALID_BUFFER_INDEX = -1; // Invalid buffer index
    static constexpr int INVALID_BUFFER_ID = -1;    // Invalid buffer ID
    static constexpr int INVALID_FORMAT = -1;       // Invalid format
    static constexpr int INVALID_FLAGS = -1;        // Invalid flags for buffer creation
    static constexpr size_t INVALID_STRIDE = 0U;    // Invalid stride in bytes
    static constexpr size_t INVALID_SIZE = 0U;      // Invalid size in bytes
    static constexpr size_t INVALID_OFFSET = 0U;    // Invalid offset in bytes
    static constexpr int INVALID_PITCH = 0;         // Invalid pitch in bytes
    static constexpr int INVALID_CARD_ID = -1;      // Invalid card ID
    DrmDevice m_device;
    std::vector<DrmBuffer *> m_buffers;     // Vector of buffers
    size_t m_currentBufferId = INVALID_BUFFER_ID; // Current buffer ID
    size_t m_bufferCount = 0;
    size_t m_currentBufferIndex = 0;
    size_t m_width;
    size_t m_height;
    int m_bpp = 32;                        // bits per pixel
    int m_format = 0;                      // default format
    int m_flags = 0;                       // flags for buffer creation
    size_t m_stride = 0;                   // stride in bytes
    size_t m_size = 0;                     // total size in bytes
    size_t m_offset = 0;                   // offset in bytes
    int m_pitch = 0;                       // pitch in bytes
    DrmConnectorInfo m_connectedConnector; // Connector information
};

} // namespace drm
} // namespace early
} // namespace evs
#endif