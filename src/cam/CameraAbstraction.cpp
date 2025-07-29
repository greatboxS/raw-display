#include "CameraAbstraction.h"
#include "CommonUtil.h"
#include <future>

namespace evs {
namespace early {

CameraAbstraction::CameraAbstraction()
    : m_cameraId(-1)
    , m_state(CameraState::UNINITIALIZED)
    , m_lastError(CameraError::NONE)
    , m_frameCallback(nullptr)
    , m_loopRunning(false)
    , m_loopExitRequested(false)
    , m_loopThread()
    , m_config()
    , m_retryCount(0)
    , m_param(nullptr) {
}

CameraAbstraction::~CameraAbstraction() {
}

int CameraAbstraction::id() const {
    return m_cameraId;
}

int CameraAbstraction::initCamera(int id) {
    m_lastError = CameraError::NONE;

    EARLY_DEBUG("CameraAbstraction::init: Initializing camera with id: %d\n", id);
    do {
        if (id < 0) {
            EARLY_ERROR("CameraAbstraction::init: Invalid camera id\n");
            m_lastError = CameraError::INIT_FAILED;
            break;
        }
        if (m_state != CameraState::UNINITIALIZED) {
            EARLY_DEBUG("CameraAbstraction::init: Camera is already initialized or in an invalid state (%d)\n", static_cast<int>(m_state.load()));
            break;
        }

        m_cameraId = id;
        m_lastError = CameraError::NONE;
        m_frameCallback = nullptr;
        m_loopRunning = false;
        m_loopExitRequested = false;

        m_lastError = static_cast<CameraError>(this->onInit());

        if (m_lastError == CameraError::NONE) {
            EARLY_DEBUG("CameraAbstraction::init: Camera initialized successfully\n");
            m_state = CameraState::INITIALIZED;
            m_retryCount = 0;

        } else {
            m_retryCount += 1;
            m_cameraId = -1;
            EARLY_ERROR("CameraAbstraction::init: Camera initialization failed, retry count: %d\n", m_retryCount);
            if (m_retryCount > INIT_RETRY_COUNT) {
                m_state = CameraState::ERROR;
                m_lastError = CameraError::INIT_FAILED;
            }
            break;
        }

    } while (false);

    return static_cast<int>(m_lastError.load());
}

void CameraAbstraction::deInitCamera() {
    EARLY_DEBUG("CameraAbstraction::deInit: Camera deInit\n");

    do {

        if (m_state == CameraState::UNINITIALIZED) {
            EARLY_DEBUG("CameraAbstraction::deInit: Camera is already uninitialized\n");
            break;
        }

        /* Stop the event loop first */
        exitFrameCaptureWorker();

        /* Reset the camera state */
        this->onDeInit();

        m_cameraId = -1;
        m_state = CameraState::UNINITIALIZED;
        m_lastError = CameraError::NONE;
        m_frameCallback = nullptr;
        m_param = nullptr;
    } while (false);
}

int CameraAbstraction::registerCameraEventCallback(CameraEventCallbackFnc callback) {
    m_lastError = CameraError::NONE;
    EARLY_DEBUG("CameraAbstraction::registerCameraEventCallback: Registering camera event callback\n");

    do {
        if (callback == nullptr) {
            EARLY_ERROR("CameraAbstraction::registerCameraEventCallback: Callback function is null\n");
            m_lastError = CameraError::INVALID_ARGUMENT;
            break;
        }

        m_cameraEventCallback = callback;

    } while (false);

    return static_cast<int>(m_lastError.load());
}

int CameraAbstraction::createFrameCaptureWorker(evs::early::CameraAbstraction::FrameCallbackFnc callback, void *param) {
    m_lastError = CameraError::NONE;
    EARLY_DEBUG("CameraAbstraction::createLoop: Creating camera loop thread\n");

    do {
        if (m_loopThread.joinable() == true) {
            EARLY_DEBUG("CameraAbstraction::createLoop: Camera loop thread already exists, exiting previous thread\n");
            m_lastError = CameraError::CREATE_LOOP_FAILED;
            break;
        }

        m_frameCallback = callback;
        m_param = param;
        m_loopThread = std::thread(onLoopThreadFunc, this);
        m_loopRunning.store(true);
    } while (false);

    return static_cast<int>(m_lastError.load());
}

int CameraAbstraction::exitFrameCaptureWorker() {
    int ret = 0;

    EARLY_DEBUG("CameraAbstraction::exitFrameCaptureWorker: Requesting loop exit\n");
    m_loopExitRequested.store(true);

    do {
        if (m_loopThread.joinable() == false) {
            EARLY_DEBUG("CameraAbstraction::exitFrameCaptureWorker: Loop thread is not initialized\n");
            ret = -1;
            break;
        }

        if (m_loopThread.joinable() == true) {
            EARLY_DEBUG("CameraAbstraction::exitFrameCaptureWorker: Waiting for loop thread to exit\n");
            m_loopThread.join();

            m_loopRunning.store(false);
            m_frameCallback = nullptr;
            m_param = nullptr;
        } else {
            EARLY_DEBUG("CameraAbstraction::exitFrameCaptureWorker: Loop thread is not joinable\n");
            ret = -1;
        }
    } while (false);
    return ret;
}

int CameraAbstraction::startPreview() {
    m_lastError = CameraError::NONE;

    EARLY_DEBUG("CameraAbstraction::startPreview: Starting camera preview\n");

    do {
        if ((m_state != CameraState::INITIALIZED) && (m_state != CameraState::STOP)) {
            EARLY_DEBUG("CameraAbstraction::startPreview: Camera is not initialized or already running\n");
            m_lastError = CameraError::START_PREVIEW_FAILED;
            break;
        }

        m_lastError = static_cast<CameraError>(this->onStartPreview());

        if (m_lastError == CameraError::NONE) {
            EARLY_DEBUG("CameraAbstraction::startPreview: Camera preview started successfully\n");
            m_state = CameraState::RUNNING;
        } else {
            EARLY_ERROR("CameraAbstraction::stopPreview: Failed to stop camera preview, error: %d\n", static_cast<int>(m_lastError.load()));
            m_lastError = CameraError::STOP_PREVIEW_FAILED;
        }

    } while (false);

    return static_cast<int>(m_lastError.load());
}

void CameraAbstraction::stopPreview() {
    EARLY_DEBUG("CameraAbstraction::stopPreview: Stopping camera preview\n");

    do {
        if (m_state != CameraState::RUNNING) {
            EARLY_DEBUG("CameraAbstraction::stopPreview: Camera is not running, cannot stop preview\n");
            break;
        }

        m_lastError = static_cast<CameraError>(this->onStopPreview());

        if (m_lastError == CameraError::NONE) {
            EARLY_DEBUG("CameraAbstraction::startPreview: Camera preview stopped successfully\n");
            m_state = CameraState::STOP;
        } else {
            EARLY_ERROR("CameraAbstraction::stopPreview: Failed to stop camera preview, error: %d\n", static_cast<int>(m_lastError.load()));
            m_lastError = CameraError::STOP_PREVIEW_FAILED;
        }

    } while (false);
}

void CameraAbstraction::setState(CameraState state) {
    m_state.store(state);
}
void CameraAbstraction::setError(CameraError error) {
    m_lastError.store(error);
}

void CameraAbstraction::onLoopThreadFunc(CameraAbstraction *camera) {
    evs::early::CameraFrame *frame = nullptr;

    EARLY_DEBUG("CameraAbstraction::onLoopThreadFunc: Starting camera loop thread\n");
    do {
        if (camera->m_loopExitRequested == true) {
            EARLY_DEBUG("CameraAbstraction::onLoopThreadFunc: Loop exit requested, exiting loop thread\n");
            break;
        }

        if (camera->m_state == CameraState::RUNNING) {
            frame = camera->getFrame();
            if (camera->m_frameCallback != nullptr) {
                camera->m_frameCallback(camera, frame, camera->m_param);
            } else {
                EARLY_DEBUG("CameraAbstraction::onLoopThreadFunc: No frame callback set, skipping frame processing\n");
            }
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

    } while (camera->m_loopExitRequested == false);

    EARLY_DEBUG("CameraAbstraction::onLoopThreadFunc: Exiting camera loop thread\n");
}

} // namespace early
} // namespace evs