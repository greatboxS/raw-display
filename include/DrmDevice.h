#ifndef DRMDEVICE_H
#define DRMDEVICE_H

#include "DrmAllocator.h"

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <mutex>
#include <condition_variable>

namespace evs {
namespace early {
namespace drm {

typedef enum class __ConnectorType {
    Unknown,
    VGA,
    HDMI,
    DVI,
    DisplayPort,
    Composite,
    SVIDEO,
} ConnectorType;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t refreshRate;
    std::string name;
} DrmModeInfo;

typedef struct {
    uint32_t id;
    uint32_t bufferId;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    bool enabled;
} DrmCrtcInfo;

typedef struct {
    uint32_t id;
    DrmCrtcInfo crtc;
    uint32_t possibleCrtcs;
} DrmEncoderInfo;

typedef struct {
    uint32_t id;
    ConnectorType type;
    uint32_t typeId;
    bool connected;
    std::string name;
    DrmEncoderInfo encoder;
} DrmConnectorInfo;

typedef struct {
    uint32_t id;
    uint32_t crtcId;
    uint32_t possibleCrtcs;
    std::vector<uint32_t> formats;
} DrmPlaneInfo;

typedef struct {
    std::string driverName;
    std::string cardName;
    std::string busInfo;
} DrmCardInfo;

class DrmDevice
{
public:
    typedef struct {
        std::mutex mtx;
        std::condition_variable cv;
        int flags{0};
        int fd{-1};
        int idx{-1};
        uint64_t lastFlipTimeUs{0};
        float fps{0.0f};
    } FlipEventObj;

public:
    explicit DrmDevice(int id, AllocatorType allocatorType = AllocatorType::DRM_ALLOCATOR_MMAP)
        : m_fd{-1}
        , m_allocatorType{allocatorType}
        , m_cardId{id}
        , m_cardInfo{}
        , m_connectors{}
        , m_encoders{}
        , m_crtcs{}
        , m_planes{}
        , m_buffers{nullptr, nullptr}
        , m_crtcId{0}
        , m_connectorId{0}
        , m_modelPtr{nullptr}
        , m_width{0}
        , m_height{0}
        , m_bpp{0}
        , m_format{0}
        , m_flags{0}
        , m_initialized{false} {
    }
    ~DrmDevice() {}

    bool open();
    void close();

    bool initDisplay(uint32_t connectorId,
                     uint32_t crtcId,
                     uint32_t width,
                     uint32_t height,
                     uint32_t bpp = 32U,
                     uint32_t format = 0U,
                     uint32_t flags = 0U);

    bool initDisplay(const DrmConnectorInfo &connectorInfo,
                     uint32_t bbp = 32U,
                     uint32_t format = 0U,
                     uint32_t flags = 0U);

    bool deInitDisplay();

    bool isInitialized() const { return m_initialized; }

    uint8_t *getDrawBuffer();
    
    bool setModeCrtc(const DrmBuffer *buffer);

    bool flipBuffer(bool useVSync = true);

    void waitFlipEvent();

    inline bool isOpen() const { return (m_fd >= 0); }
    inline int fd() const { return m_fd; }

    const DrmCardInfo &getCardInfo() const { return m_cardInfo; }
    const std::unordered_map<uint32_t, DrmConnectorInfo> &getConnectors() const { return m_connectors; }
    const std::unordered_map<uint32_t, DrmEncoderInfo> &getEncoders() const { return m_encoders; }
    const std::unordered_map<uint32_t, DrmCrtcInfo> &getCrtcs() const { return m_crtcs; }
    const std::unordered_map<uint32_t, DrmPlaneInfo> &getPlanes() const { return m_planes; }

    inline std::string getCardName() const { return m_cardInfo.cardName; }
    inline std::string getDriverName() const { return m_cardInfo.driverName; }
    inline std::string getBusInfo() const { return m_cardInfo.busInfo; }
    inline FlipEventObj &getFlipEventObj() { return m_flipEventObj; }

    void queryDeviceName();
    void queryDeviceConnectors();
    void queryDeviceEncoders();
    void queryCrtcs();
    void queryPlanes();
    void queryAllDeviceInfo();
    void resetDeviceInfo();

    inline int cardId() const { return m_cardId; }
    inline uint32_t crtcId() const { return m_crtcId; }
    inline uint32_t connectorId() const { return m_connectorId; }
    inline uint32_t width() const { return m_width; }
    inline uint32_t height() const { return m_height; }
    inline uint32_t bpp() const { return m_bpp; }
    inline uint32_t format() const { return m_format; }
    inline uint32_t flags() const { return m_flags; }
    inline int activeBufferIndex() const { return m_flipEventObj.idx; }
    inline DrmBuffer *activeBuffer() const {
        return (m_flipEventObj.idx >= 0 && m_flipEventObj.idx < 2) ? m_buffers[m_flipEventObj.idx] : nullptr;
    }
    inline DrmBuffer *buffer(int index) const {
        return (index >= 0 && index < 2) ? m_buffers[index] : nullptr;
    }
    inline DrmBuffer *buffer() const {
        return (m_flipEventObj.idx >= 0 && m_flipEventObj.idx < 2) ? m_buffers[m_flipEventObj.idx] : nullptr;
    }
    inline DrmBuffer *buffer0() const { return m_buffers[0U]; }
    inline DrmBuffer *buffer1() const { return m_buffers[1U]; }

    inline void setBufferPtr(int index, void *ptr) { m_buffers[index]->ptr = ptr; }

    template <typename Allocator>
    static DrmBuffer *createBuffer(int fd,
                                   uint32_t width,
                                   uint32_t height,
                                   uint32_t bpp = 32,
                                   uint32_t offset = 0,
                                   uint32_t flags = 0,
                                   uint32_t format = 0) {
        BufferInfo info = {};
        info.width = width;
        info.height = height;
        info.bpp = bpp;
        info.depth = 24U;
        info.format = format;
        info.flags = flags;
        return Allocator::allocate(fd, info);
    }

    template <typename Allocator>
    static void removeBuffer(int fd, DrmBuffer *buffer) {
        return Allocator::release(fd, buffer);
    }

private:
    void queryDeviceInfo(uint32_t flags);
    void queryDeviceEncoders(void *res);
    void queryCrtcs(void *res);
    void getConnectedConnectors(void *resources);
    void getConnectorInfo(void *conn, DrmConnectorInfo &connector);

    static void pageFlipHandler(int fd,
                                unsigned int sequence,
                                unsigned int tv_sec,
                                unsigned int tv_usec,
                                void *user_data);

    int m_fd{-1};
    AllocatorType m_allocatorType{AllocatorType::DRM_ALLOCATOR_MMAP};
    int m_cardId{-1};
    DrmCardInfo m_cardInfo{};
    std::unordered_map<uint32_t, DrmConnectorInfo> m_connectors{};
    std::unordered_map<uint32_t, DrmEncoderInfo> m_encoders{};
    std::unordered_map<uint32_t, DrmCrtcInfo> m_crtcs{};
    std::unordered_map<uint32_t, DrmPlaneInfo> m_planes{};
    DrmBuffer *m_buffers[2]{nullptr, nullptr};
    uint32_t m_crtcId{0};
    uint32_t m_connectorId{0};
    void *m_modelPtr{nullptr};
    uint32_t m_width{0};
    uint32_t m_height{0};
    uint32_t m_bpp{0};
    uint32_t m_format{0};
    uint32_t m_flags{0};
    bool m_initialized{false};
    DrmConnectorInfo m_initConnector{};
    DrmConnectorInfo m_bkConnector{};
    FlipEventObj m_flipEventObj{};
};

} // namespace drm
} // namespace early
} // namespace evs

#endif