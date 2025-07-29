// qcarcam.h - QCarCam API for Android 14+ (Stubbed Version)

#ifndef QCARCAM_H
#define QCARCAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

// --- Constants ---
#define QCARCAM_SUCCESS         0
#define QCARCAM_FAILURE         -1
#define QCARCAM_INVALID_SESSION -2
#define QCARCAM_MAX_CAMERAS     4

// --- Types ---
typedef int32_t qcarcam_session_t;

typedef struct {
    int width;
    int height;
    int fps;
    int format; // Placeholder for pixel format enum
} qcarcam_config_t;

typedef struct {
    uint8_t *data;
    size_t size;
} qcarcam_frame_t;

// --- QCarCam API Functions ---

/**
 * Initialize the QCarCam system (Initialize CameraX or Camera2 under the hood)
 *
 * @return QCARCAM_SUCCESS on success, QCARCAM_FAILURE on failure
 */
int qcarcam_initialize(void);

/**
 * Shutdown the QCarCam system (Release resources)
 */
void qcarcam_shutdown(void);

/**
 * Open a camera session
 *
 * @param camera_id ID of the camera (e.g., back camera, front camera)
 * @return A session handle, or QCARCAM_INVALID_SESSION if the camera is invalid
 */
qcarcam_session_t qcarcam_open(int camera_id);

/**
 * Close a camera session
 *
 * @param session The session to close
 * @return QCARCAM_SUCCESS on success, QCARCAM_FAILURE on failure
 */
int qcarcam_close(qcarcam_session_t session);

/**
 * Start streaming from a camera (Configure CameraX or Camera2 session)
 *
 * @param session The session to start
 * @param config  Configuration for the camera (resolution, FPS, format)
 * @return QCARCAM_SUCCESS on success, QCARCAM_FAILURE on failure
 */
int qcarcam_start(qcarcam_session_t session, const qcarcam_config_t *config);

/**
 * Stop streaming from the camera
 *
 * @param session The session to stop
 * @return QCARCAM_SUCCESS on success, QCARCAM_FAILURE on failure
 */
int qcarcam_stop(qcarcam_session_t session);

/**
 * Get a frame (stub returns dummy data)
 *
 * @param session The session to get the frame from
 * @param frame   The frame data (stubbed)
 * @return QCARCAM_SUCCESS on success, QCARCAM_FAILURE on failure
 */
int qcarcam_get_frame(qcarcam_session_t session, qcarcam_frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif // QCARCAM_H
