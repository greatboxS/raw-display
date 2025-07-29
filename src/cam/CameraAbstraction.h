#ifndef CAMERAABTRACTION_H
#define CAMERAABTRACTION_H

#include "SignalWrapter.h"
#include <atomic>
#include <thread>
#include <memory>

#define INIT_RETRY_COUNT (5)

namespace evs {
namespace early {

/**
 * @enum CameraState
 * @brief Defines the possible states of the camera device.
 */
enum class CameraState {
    UNINITIALIZED, ///< Camera has not been initialized
    INITIALIZED,   ///< Camera is initialized but not streaming
    RUNNING,       ///< Camera stream is running
    STOP,          ///< Camera stream is stopped
    ERROR          ///< Camera is in an error state
};

/**
 * @enum CameraError
 * @brief Defines possible error codes for camera operations.
 */
enum class CameraError {
    NONE,                 ///< No error
    INIT_FAILED,          ///< Initialization failed
    CREATE_LOOP_FAILED,   ///< Failed to create the main loop
    EXIT_LOOP_FAILED,     ///< Failed to exit the main loop
    START_PREVIEW_FAILED, ///< Failed to start camera preview
    STOP_PREVIEW_FAILED,  ///< Failed to stop camera preview
    GET_FRAME_FAILED,     ///< Failed to retrieve frame
    SET_CONFIG_FAILED,    ///< Failed to set camera configuration
    GET_CONFIG_FAILED,    ///< Failed to get camera configuration
    INVALID_ARGUMENT,     ///< Invalid argument provided
    EGL_FAILED,           ///< EGL initialization failed
    EGL_DISPLAY_FAILED,   ///< EGL display initialization failed
    EGL_CONTEXT_FAILED,   ///< EGL context creation failed
    EGL_SURFACE_FAILED,   ///< EGL surface creation failed
    CONFIG_FAILED,        ///< Configuration failed
    STREAM_FAILED,        ///< Failed to start/stop stream
    FRAME_UNAVAILABLE,    ///< Frame not available
    DEVICE_DISCONNECTED,  ///< Camera device disconnected
    UNSUPPORTED,          ///< Operation not supported
    UNKNOWN               ///< Unknown error
};

/**
 * @struct CameraConfig
 * @brief Holds configuration parameters for the camera device.
 */
typedef struct CameraConfig_t {
    int width = 1920;   ///< Frame width in pixels
    int height = 1080;  ///< Frame height in pixels
    int framerate = 30; ///< Frame rate in frames per second
    // Add other configuration parameters as needed
} CameraConfig;

/**
 * @struct CameraBuffer
 * @brief Represents a buffer containing camera frame data.
 */
typedef struct CameraBuffer_t {
    int idx = 0;          ///< Buffer index
    void *data = nullptr; ///< Pointer to buffer data
    size_t size = 0;      ///< Size of the buffer in bytes
    int width = 0;        ///< Width of the frame in pixels
    int height = 0;       ///< Height of the frame in pixels
    int format = 0;       ///< Pixel format identifier
    // Add other buffer-related fields as needed
} CameraBuffer;

/**
 * @class CameraFrame
 * @brief Encapsulates a camera frame and its associated buffer.
 */
class CameraFrame
{
public:
    /**
     * @brief Default constructor.
     */
    CameraFrame() = default;

    /**
     * @brief Constructs a CameraFrame from a CameraBuffer.
     * @param buffer The buffer containing frame data.
     */
    explicit CameraFrame(const CameraBuffer &buffer)
        : mBuffer(buffer) {}

    /**
     * @brief Returns a const reference to the underlying CameraBuffer.
     * @return Const reference to CameraBuffer.
     */
    inline const CameraBuffer &getBuffer() const { return mBuffer; }

    /**
     * @brief Returns a reference to the underlying CameraBuffer.
     * @return Reference to CameraBuffer.
     */
    inline CameraBuffer &getBuffer() { return mBuffer; }

private:
    CameraBuffer mBuffer;
};

/**
 * @brief CameraAbstraction
 *
 */
class CameraAbstraction
{
protected:
    virtual int onInit() = 0;
    virtual void onDeInit() = 0;
    virtual int onStartPreview() = 0;
    virtual int onStopPreview() = 0;

public:
    /**
     * @brief Type definition for a callback function that handles camera events.
     *        The callback receives the camera ID and a pointer to the callback object.
     */
    using CameraEventCallbackFnc = void (*)(CameraAbstraction *, void *);
    using FrameCallbackFnc = void (*)(CameraAbstraction *, CameraFrame *, void *);

    explicit CameraAbstraction();
    virtual ~CameraAbstraction();

    int id() const;

    int initCamera(int);

    void deInitCamera();

    virtual int registerCameraEventCallback(CameraEventCallbackFnc callback);

    int createFrameCaptureWorker(FrameCallbackFnc callback = nullptr, void *param = nullptr);

    int exitFrameCaptureWorker();

    int startPreview();

    void stopPreview();

    virtual CameraFrame *getFrame() = 0;

    virtual int setConfig(const CameraConfig &config) = 0;

    virtual CameraConfig getConfig() const = 0;

    CameraState getState() const { return m_state.load(); }

    CameraError getError() const { return m_lastError.load(); }

protected:
    void setState(CameraState state);
    void setError(CameraError error);
    static void onLoopThreadFunc(CameraAbstraction *camera);

    int m_cameraId{-1};                                           ///< Unique identifier for the camera device
    std::atomic<CameraState> m_state{CameraState::UNINITIALIZED}; ///< Current state of the camera
    std::atomic<CameraError> m_lastError{CameraError::NONE};      ///< Last error encountered by the camera
    FrameCallbackFnc m_frameCallback{nullptr};                    ///< Callback for frame updates
    std::atomic<bool> m_loopRunning{false};                       ///< Flag indicating if the main loop is running
    std::atomic<bool> m_loopExitRequested{false};                 ///< Flag indicating if exit has been requested
    std::thread m_loopThread{};                                   ///< Thread for the main loop
    CameraConfig m_config{};                                      ///< Current camera configuration
    int m_retryCount{0};                                          ///< Retry count for operations
    CameraEventCallbackFnc m_cameraEventCallback{nullptr};        ///< Callback for camera events
    void *m_param{nullptr};                                       ///< Frame callback parameter
};
} // namespace early
} // namespace evs
#endif // CAMERAABTRACTION_H