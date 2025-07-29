#include "QualcommCamera.h"
#include "CommonUtil.h"
#include <qcarcam.h>

namespace evs {
namespace early {
int QualcommCamera::onInit() {
    EARLY_DEBUG("QualcommCamera::onInit: Initializing camera with id: %d\n", m_cameraId);
    qcarcam_initialize();
    (void)qcarcam_open(m_cameraId);
    return static_cast<int>(CameraError::NONE);
}
void QualcommCamera::onDeInit() {
    EARLY_DEBUG("QualcommCamera::onDeInit: Deinitializing camera\n");
    qcarcam_shutdown();
}

int QualcommCamera::onStartPreview() {
    EARLY_DEBUG("QualcommCamera::onStartPreview: Starting camera preview\n");
    qcarcam_start(m_cameraId, nullptr);
    return static_cast<int>(CameraError::NONE);
}
int QualcommCamera::onStopPreview() {
    EARLY_DEBUG("QualcommCamera::onStopPreview: Stopping camera preview\n");
    qcarcam_stop(static_cast<qcarcam_session_t>(m_cameraId));
    return static_cast<int>(CameraError::NONE);
}

QualcommCamera::~QualcommCamera() {
    exitFrameCaptureWorker();
}

CameraFrame *QualcommCamera::getFrame() {
    static qcarcam_frame_t frame;
    static CameraFrame cameraFrame;
    cameraFrame.getBuffer().width = 500;
    cameraFrame.getBuffer().height = 500;
    // EARLY_DEBUG("QualcommCamera::getFrame: Retrieving camera frame1\n");
    qcarcam_get_frame(m_cameraId, &frame);
    cameraFrame.getBuffer().data = frame.data;
    // EARLY_DEBUG("QualcommCamera::getFrame: Retrieving camera frame2\n");
    return &cameraFrame;
}
int QualcommCamera::setConfig(const CameraConfig &config) {
    EARLY_DEBUG("QualcommCamera::setConfig: Setting camera configuration\n");
    m_config = config;
    return static_cast<int>(CameraError::NONE);
}

CameraConfig QualcommCamera::getConfig() const {
    EARLY_DEBUG("QualcommCamera::getConfig: Getting camera configuration\n");
    return m_config;
}

} // namespace early
} // namespace evs