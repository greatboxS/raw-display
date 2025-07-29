#include "DrmDevice.h"
#include "CommonUtil.h"

#include <unordered_map>
#include <memory>
#include <fcntl.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <cstring>
#include <thread>
#include <iostream>

#ifdef DEBUG_TAG
#undef DEBUG_TAG
#define DEBUG_TAG "EarlyDisplay DrmDevice"
#endif

#define QUERY_CONNCECTORS (1U)
#define QUERY_ENCODERS    (2U)
#define QUERY_CRTCS       (4U)
#define QUERY_PLANES      (8U)
#define QUERY_ALL         (QUERY_CONNCECTORS | QUERY_ENCODERS | QUERY_CRTCS | QUERY_PLANES)

namespace evs {
namespace early {
namespace drm {

static std::string connectorTypeToString(uint32_t type) {
    static const std::unordered_map<uint32_t, const char *> g_connectors = {
        {DRM_MODE_CONNECTOR_VGA, "VGA"},
        {DRM_MODE_CONNECTOR_DVII, "DVI-I"},
        {DRM_MODE_CONNECTOR_DVID, "DVI-D"},
        {DRM_MODE_CONNECTOR_DVIA, "DVI-A"},
        {DRM_MODE_CONNECTOR_Composite, "Composite"},
        {DRM_MODE_CONNECTOR_SVIDEO, "S-Video"},
        {DRM_MODE_CONNECTOR_LVDS, "LVDS"},
        {DRM_MODE_CONNECTOR_Component, "Component"},
        {DRM_MODE_CONNECTOR_9PinDIN, "DIN"},
        {DRM_MODE_CONNECTOR_DisplayPort, "DisplayPort"},
        {DRM_MODE_CONNECTOR_HDMIA, "HDMI-A"},
        {DRM_MODE_CONNECTOR_HDMIB, "HDMI-B"},
        {DRM_MODE_CONNECTOR_TV, "TV"},
        {DRM_MODE_CONNECTOR_eDP, "eDP"},
        {DRM_MODE_CONNECTOR_VIRTUAL, "Virtual"},
        {DRM_MODE_CONNECTOR_DSI, "DSI"},
        {DRM_MODE_CONNECTOR_DPI, "DPI"}};

    std::string str = "Unknown";
    auto it = g_connectors.find(type);
    if (it != g_connectors.end()) {
        str = std::string(it->second);
    }
    return str;
}

bool DrmDevice::open() {
    bool success = false;
    std::string path = "/dev/dri/card" + std::to_string(m_cardId);
    do {
        m_fd = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
        if (m_fd < 0) {
            EARLY_ERROR("Failed to open DRM device at %s: %s\n", path.c_str(), strerror(errno));
            break;
        }
        success = true;
    } while (false);
    return success;
}

void DrmDevice::close() {
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

bool DrmDevice::initDisplay(uint32_t connectorId,
                            uint32_t crtcId,
                            uint32_t width,
                            uint32_t height,
                            uint32_t bpp,
                            uint32_t format,
                            uint32_t flags) {
    bool success = true;

    do {
        if (m_initialized == true) {
            EARLY_INFO("The Drm device already initialized, return\n");
            success = false;
            break;
        }
        for (uint8_t i = 0U; i < 2U; ++i) {
            if (m_allocatorType == AllocatorType::DRM_ALLOCATOR_MMAP) {
                m_buffers[i] = DrmDevice::createBuffer<MMapAllocator>(m_fd, width, height, bpp, 0U, flags, format);
            } else if (m_allocatorType == AllocatorType::DRM_ALLOCATOR_HEAP_DMA) {
                m_buffers[i] = DrmDevice::createBuffer<HeapDMAAllocator>(m_fd, width, height, bpp, 0U, flags, format);
#ifdef SUPPORT_ION_ALLOCATOR
            } else if (m_allocatorType == AllocatorType::DRM_ALLOCATOR_ION) {
                m_buffers[i] = DrmDevice::createBuffer<IonAllocator>(m_fd, width, height, bpp, 0U, stride, flags, format);
#endif
            } else {
                EARLY_ERROR("Unsupported allocator type %d for display initialization.\n", static_cast<int>(m_allocatorType));
                success = false;
                break;
            }
            if (m_buffers[i] == nullptr) {
                EARLY_ERROR("Failed to allocate buffer %u for display initialization.\n", i);
                success = false;
                break;
            }
        }

        if (success == false) {
            deInitDisplay();
            break;
        }

        auto iter = m_connectors.find(connectorId);
        if (iter == m_connectors.end()) {
            success = false;
            EARLY_ERROR("Connector %u not found in the device.\n", connectorId);
            break;
        }

        uint32_t connector = connectorId;
        drmModeCrtcPtr crtc = drmModeGetCrtc(m_fd, crtcId);
        if (crtc == nullptr) {
            EARLY_ERROR("Failed to get CRTC %u: %s\n", crtcId, strerror(errno));
            success = false;
            break;
        }

        if (crtc->mode_valid == 0) {
            drmModeFreeCrtc(crtc);
            EARLY_ERROR("CRTC %u has no valid mode.\n", crtcId);
            success = false;
            break;
        }

        if (m_modelPtr != nullptr) {
            if (memcmp(m_modelPtr, &crtc->mode, sizeof(drmModeModeInfo)) != 0) {
                delete static_cast<drmModeModeInfo *>(m_modelPtr);
            }
        } else {
            m_modelPtr = new drmModeModeInfo;
            memcpy(m_modelPtr, &crtc->mode, sizeof(drmModeModeInfo));
        }
        drmModeFreeCrtc(crtc);

        DrmConnectorInfo connectorInfo{};
        auto conn = drmModeGetConnectorCurrent(m_fd, connectorId);
        getConnectorInfo(conn, connectorInfo);
        drmModeFreeConnector(conn);

        if (drmModeSetCrtc(m_fd,
                           crtcId,
                           m_buffers[0]->fbId,
                           0,
                           0,
                           &connector,
                           1,
                           static_cast<drmModeModeInfo *>(m_modelPtr))
            != 0) {
            EARLY_ERROR("Failed to set to fd %d with CRTC %u with connector %u, fbid %u, %s\n",
                        m_fd,
                        crtcId,
                        connector,
                        m_buffers[0]->fbId,
                        strerror(errno));
            success = false;
            break;
        }

        m_connectorId = connectorId;
        m_crtcId = crtcId;
        m_width = width;
        m_height = height;
        m_bpp = bpp;
        m_format = format;
        m_flags = flags;
        m_flipEventObj.idx = 0;
        m_flipEventObj.flags = 0;
        m_initialized = true;
        m_bkConnector = connectorInfo;
    } while (false);
    return success;
}

bool DrmDevice::initDisplay(const DrmConnectorInfo &connectorInfo,
                            uint32_t bpp,
                            uint32_t format,
                            uint32_t flags) {
    bool success = true;
    do {
        if ((connectorInfo.id == 0)
            || (connectorInfo.encoder.id == 0)
            || (connectorInfo.encoder.crtc.id == 0)) {
            EARLY_ERROR("Invalid connector %u, or encoder %u or CRTC %u ID.\n",
                        connectorInfo.id,
                        connectorInfo.encoder.id,
                        connectorInfo.encoder.crtc.id);

            success = false;
            break;
        }

        if (initDisplay(connectorInfo.id,
                        connectorInfo.encoder.crtc.id,
                        connectorInfo.encoder.crtc.width,
                        connectorInfo.encoder.crtc.height,
                        bpp,
                        format,
                        flags)
            == false) {
            success = false;
            break;
        }
    } while (false);
    return success;
}

bool DrmDevice::deInitDisplay() {
    bool success = true;
    do {
        if (m_fd < 0) {
            success = false;
            break;
        }

        for (uint8_t i = 0U; i < 2U; ++i) {
            DrmBuffer *buffer = m_buffers[i];
            if (buffer == nullptr) {
                continue;
            }
            if (m_allocatorType == AllocatorType::DRM_ALLOCATOR_MMAP) {
                DrmDevice::removeBuffer<MMapAllocator>(m_fd, buffer);
            } else if (m_allocatorType == AllocatorType::DRM_ALLOCATOR_HEAP_DMA) {
                DrmDevice::removeBuffer<HeapDMAAllocator>(m_fd, buffer);
#ifdef SUPPORT_ION_ALLOCATOR
            } else if (m_allocatorType == AllocatorType::DRM_ALLOCATOR_ION) {
                DrmDevice::removeBuffer<IonAllocator>(m_fd, buffer);
#endif
            } else {
                success = false;
                EARLY_ERROR("Unsupported allocator type %d for display de-initialization.\n", static_cast<int>(m_allocatorType));
                break;
            }
            m_buffers[i] = nullptr;
        }

        if (m_modelPtr != nullptr) {
            delete static_cast<drmModeModeInfo *>(m_modelPtr);
            m_modelPtr = nullptr;
        }

        m_crtcId = 0U;
        m_connectorId = 0U;
        m_width = 0U;
        m_height = 0U;
        m_bpp = 0U;
        m_format = 0U;
        m_flags = 0U;
        m_buffers[0U] = nullptr;
        m_buffers[1U] = nullptr;
        m_flipEventObj.flags = -1;
        m_flipEventObj.idx = -1;
        m_initialized = false;
        success = true;
    } while (false);
    return success;
}

uint8_t *DrmDevice::getDrawBuffer() {
    DrmBuffer *buffer = activeBuffer();
    return static_cast<uint8_t *>(buffer ? buffer->ptr : nullptr);
}

bool DrmDevice::setModeCrtc(const DrmBuffer *buffer) {
    return (drmModeSetCrtc(m_fd, m_crtcId, buffer->fbId, 0, 0, &m_connectorId, 1, static_cast<drmModeModeInfo *>(m_modelPtr)) == 0);
}

bool DrmDevice::flipBuffer(bool useVSync) {
    bool success = true;
    DrmBuffer *buffer = activeBuffer();
    do {
        if (buffer == nullptr) {
            success = false;
            break;
        }
        if (buffer->fbId == 0) {
            success = false;
            break;
        }

        {
            std::unique_lock<std::mutex> lock(m_flipEventObj.mtx);
            m_flipEventObj.flags = 1;
        }
        int ret = drmModePageFlip(m_fd, m_crtcId, buffer->fbId, useVSync ? DRM_MODE_PAGE_FLIP_EVENT : 0, this);
        if (ret < 0) {
            EARLY_ERROR("Failed to flip buffer %u on CRTC %u: %s\n", buffer->fbId, m_crtcId, strerror(errno));
            success = false;
            break;
        }

        m_flipEventObj.idx ^= 1;
    } while (false);

    return success;
}

void DrmDevice::pageFlipHandler(int fd,
                                unsigned int sequence,
                                unsigned int tv_sec,
                                unsigned int tv_usec,
                                void *user_data) {
    // EARLY_DEBUG("Page flip event: sequence=%u, sec=%u, usec=%u, %u\n", sequence, tv_sec, tv_usec, getppid());
    // std::thread::id tid = std::this_thread::get_id();
    // EARLY_DEBUG("Page flip event handled in thread id: %lu\n", static_cast<unsigned long>(tid));
    // std::cout << "Page flip event: sequence=" << sequence
    //           << ", sec=" << tv_sec
    //           << ", usec=" << tv_usec
    //           << ", thread id: " << tid << std::endl;
    DrmDevice *device = static_cast<DrmDevice *>(user_data);
    FlipEventObj &flipEventObj = device->getFlipEventObj();

    // Get current time in microseconds
    uint64_t currentTimeUs = static_cast<uint64_t>(tv_sec) * 1000000 + tv_usec;
    {
        std::unique_lock<std::mutex> lock(flipEventObj.mtx);

        // Calculate FPS
        if (flipEventObj.lastFlipTimeUs != 0) {
            uint64_t deltaUs = currentTimeUs - flipEventObj.lastFlipTimeUs;
            if (deltaUs > 0) {
                float fps = 1.0e6f / deltaUs;
                // EARLY_DEBUG("Estimated FPS: %.2f\n", fps);
                flipEventObj.fps = fps;
            }
        }

        // Update flip tracking
        flipEventObj.lastFlipTimeUs = currentTimeUs;
        flipEventObj.flags = 0;
        flipEventObj.idx ^= 1;
        flipEventObj.cv.notify_all();
    }
}

void DrmDevice::waitFlipEvent() {
    drmEventContext evctx = {};
    evctx.version = DRM_EVENT_CONTEXT_VERSION;
    evctx.page_flip_handler = DrmDevice::pageFlipHandler;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_fd, &fds);

    while (m_flipEventObj.flags > 0) {
        if (select(m_fd + 1, &fds, nullptr, nullptr, nullptr) > 0) {
            if (FD_ISSET(m_fd, &fds)) {
                drmHandleEvent(m_fd, &evctx);
            }
        }
    }

    std::unique_lock<std::mutex> lock(m_flipEventObj.mtx);
    if (m_flipEventObj.flags) {
        m_flipEventObj.cv.wait(lock, [this]() { return m_flipEventObj.flags == 0; });
    }
}

void DrmDevice::queryDeviceName() {
    drmVersion *version = drmGetVersion(m_fd);
    m_cardInfo = {};
    if (version) {
        m_cardInfo.driverName = std::string(version->name, version->name_len);
        m_cardInfo.cardName = std::string(version->desc, version->desc_len);
        m_cardInfo.busInfo = std::string(version->date, version->date_len);
        drmFreeVersion(version);
    }
}

void DrmDevice::queryDeviceConnectors() {
    queryDeviceInfo(QUERY_CONNCECTORS);
}

void DrmDevice::queryDeviceEncoders() {
    queryDeviceInfo(QUERY_ENCODERS);
}

void DrmDevice::queryCrtcs() {
    queryDeviceInfo(QUERY_CRTCS);
}

void DrmDevice::queryPlanes() {
    drmModePlaneRes *planeRes = drmModeGetPlaneResources(m_fd);
    if (planeRes == nullptr) {
        return;
    }

    for (uint32_t i = 0; i < planeRes->count_planes; ++i) {
        drmModePlane *plane = drmModeGetPlane(m_fd, planeRes->planes[i]);
        if (plane == nullptr) {
            continue;
        }

        DrmPlaneInfo info;
        info.id = plane->plane_id;
        info.crtcId = plane->crtc_id;
        info.possibleCrtcs = plane->possible_crtcs;
        for (uint32_t j = 0; j < plane->count_formats; ++j) {
            info.formats.push_back(plane->formats[j]);
        }
        m_planes[plane->plane_id] = info;
        drmModeFreePlane(plane);
    }

    drmModeFreePlaneResources(planeRes);
}

void DrmDevice::queryAllDeviceInfo() {
    queryDeviceName();
    drmModeRes *resources = drmModeGetResources(m_fd);
    if (resources == nullptr) {
        EARLY_ERROR("Failed to get DRM resources: %s\n", strerror(errno));
        return;
    }
    queryDeviceInfo(QUERY_ALL);
    drmModeFreeResources(resources);
}

void DrmDevice::resetDeviceInfo() {
    m_connectors.clear();
    m_encoders.clear();
    m_crtcs.clear();
    m_planes.clear();
    m_cardInfo = {};
}

void DrmDevice::queryDeviceInfo(uint32_t flags) {

    drmModeRes *resources = drmModeGetResources(m_fd);
    if (resources == nullptr) {
        return;
    }
    if (flags & QUERY_CONNCECTORS) {
        getConnectedConnectors(resources);
    }
    if (flags & QUERY_ENCODERS) {
        queryDeviceEncoders(resources);
    }
    if (flags & QUERY_CRTCS) {
        queryCrtcs(resources);
    }
    if (flags & QUERY_PLANES) {
        queryPlanes();
    }
    drmModeFreeResources(resources);
}

void DrmDevice::queryDeviceEncoders(void *res) {
    drmModeRes *resources = static_cast<drmModeRes *>(res);

    m_encoders.clear();
    for (int i = 0; i < resources->count_encoders; ++i) {
        drmModeEncoder *enc = drmModeGetEncoder(m_fd, resources->encoders[i]);
        if (enc == nullptr) {
            continue;
        }

        DrmEncoderInfo encoder;
        encoder.id = enc->encoder_id;
        drmModeCrtc *crtc = drmModeGetCrtc(m_fd, enc->crtc_id);
        if (crtc != nullptr) {
            encoder.crtc.width = crtc->width;
            encoder.crtc.height = crtc->height;
            encoder.crtc.x = crtc->x;
            encoder.crtc.y = crtc->y;
            encoder.crtc.enabled = (crtc->mode_valid != 0);
            encoder.crtc.bufferId = crtc->buffer_id;
            drmModeFreeCrtc(crtc);
        }
        encoder.possibleCrtcs = enc->possible_crtcs;
        m_encoders[enc->encoder_id] = encoder;
        drmModeFreeEncoder(enc);
    }
}

void DrmDevice::queryCrtcs(void *res) {
    drmModeRes *resources = static_cast<drmModeRes *>(res);
    m_crtcs.clear();
    for (int i = 0; i < resources->count_crtcs; ++i) {
        DrmCrtcInfo crtc{};
        drmModeCrtc *drmCrtc = drmModeGetCrtc(m_fd, resources->crtcs[i]);
        if (drmCrtc != nullptr) {
            crtc.id = drmCrtc->crtc_id;
            crtc.bufferId = drmCrtc->buffer_id;
            crtc.x = drmCrtc->x;
            crtc.y = drmCrtc->y;
            crtc.width = drmCrtc->width;
            crtc.height = drmCrtc->height;
            crtc.enabled = (drmCrtc->mode_valid != 0);
            m_crtcs[drmCrtc->crtc_id] = crtc;
            drmModeFreeCrtc(drmCrtc);
        }
    }
}

void DrmDevice::getConnectedConnectors(void *resources) {
    drmModeRes *res = static_cast<drmModeRes *>(resources);
    for (int i = 0; i < res->count_connectors; ++i) {
        drmModeConnector *connector = drmModeGetConnector(m_fd, res->connectors[i]);
        if (connector == nullptr) {
            continue;
        }

        EARLY_DEBUG("Connector %d: type=%s, status=%d, encoder_id=%d\n",
                    connector->connector_id,
                    connectorTypeToString(connector->connector_type).c_str(),
                    connector->connection,
                    connector->encoder_id);

        if (connector->connection == DRM_MODE_CONNECTED) {
            DrmConnectorInfo connectorInfo{};
            getConnectorInfo(connector, connectorInfo);
            m_connectors[connector->connector_id] = connectorInfo;
        }
        drmModeFreeConnector(connector);
    }
}

void DrmDevice::getConnectorInfo(void *conn, DrmConnectorInfo &connectorInfo) {
    drmModeConnector *connector = static_cast<drmModeConnector *>(conn);
    connectorInfo.id = connector->connector_id;
    connectorInfo.type = static_cast<ConnectorType>(connector->connector_type);
    connectorInfo.typeId = connector->connector_type_id;
    connectorInfo.name = connectorTypeToString(connector->connector_type);
    connectorInfo.connected = true;

    drmModeEncoder *encoder = drmModeGetEncoder(m_fd, connector->encoder_id);
    if (encoder != nullptr) {
        connectorInfo.encoder.id = encoder->encoder_id;
        connectorInfo.encoder.possibleCrtcs = encoder->possible_crtcs;

        drmModeCrtc *crtc = drmModeGetCrtc(m_fd, encoder->crtc_id);
        if (crtc != nullptr) {
            connectorInfo.encoder.crtc.width = crtc->width;
            connectorInfo.encoder.crtc.height = crtc->height;
            connectorInfo.encoder.crtc.id = crtc->crtc_id;
            connectorInfo.encoder.crtc.x = crtc->x;
            connectorInfo.encoder.crtc.y = crtc->y;
            connectorInfo.encoder.crtc.enabled = (crtc->mode_valid != 0);
            connectorInfo.encoder.crtc.bufferId = crtc->buffer_id;
            drmModeFreeCrtc(crtc);
        }
        drmModeFreeEncoder(encoder);
    }
}

} // namespace drm
} // namespace early
} // namespace evs