// qcarcam.c - Stub QCarCam API Implementation for Android 14+

#include "qcarcam.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>

// --- Static data ---
static uint8_t dummy_frame_data[640 * 480 * 4];  // Dummy YUV frame data for testing

void generateTestImage(uint8_t *buffer, int width, int height) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = (y * width + x) * 4;
            buffer[i + 0] = rand() % 256;
            buffer[i + 1] = rand() % 256;
            buffer[i + 2] = rand() % 256;
            buffer[i + 3] = 255;//rand() % 256;
        }
    }
}

// --- QCarCam API Implementations ---

/**
 * Initialize the QCarCam system (CameraX or Camera2 under the hood)
 * 
 * @return QCARCAM_SUCCESS on success, QCARCAM_FAILURE on failure
 */
int qcarcam_initialize(void) {
    // This function should initialize CameraX or Camera2 internally
    // For now, it’s a stub returning success.
    printf("QCarCam system initialized (CameraX or Camera2 under the hood)\n");
    return QCARCAM_SUCCESS;
}

/**
 * Shutdown the QCarCam system (release resources)
 */
void qcarcam_shutdown(void) {
    // Stub: No-op for testing purposes.
    printf("QCarCam system shutdown (resources released)\n");
}

/**
 * Open a camera session
 * 
 * @param camera_id ID of the camera (e.g., back camera, front camera)
 * @return A session handle, or QCARCAM_INVALID_SESSION if the camera is invalid
 */
qcarcam_session_t qcarcam_open(int camera_id) {
    // CameraX or Camera2 would open a session here
    // Just return a camera ID as a session handle for now.
    if (camera_id < QCARCAM_MAX_CAMERAS) {
        printf("Opened camera session %d\n", camera_id);
        return camera_id;  // Return the session ID (camera ID)
    }
    return QCARCAM_INVALID_SESSION;  // Invalid session
}

/**
 * Close a camera session
 * 
 * @param session The session to close
 * @return QCARCAM_SUCCESS on success, QCARCAM_FAILURE on failure
 */
int qcarcam_close(qcarcam_session_t session) {
    if (session >= 0) {
        printf("Closed camera session %d\n", session);
        return QCARCAM_SUCCESS;
    }
    return QCARCAM_FAILURE;
}

/**
 * Start streaming from a camera (CameraX or Camera2 configuration)
 * 
 * @param session The session to start
 * @param config  Configuration for the camera (resolution, FPS, format)
 * @return QCARCAM_SUCCESS on success, QCARCAM_FAILURE on failure
 */
int qcarcam_start(qcarcam_session_t session, const qcarcam_config_t* config) {
    if (session < 0 || !config) {
        return QCARCAM_FAILURE;
    }
    // CameraX or Camera2 would configure the session with the provided config
    printf("Started streaming for camera session %d with config: %dx%d @ %d FPS\n", 
           session, config->width, config->height, config->fps);
    return QCARCAM_SUCCESS;
}

/**
 * Stop streaming from the camera
 * 
 * @param session The session to stop
 * @return QCARCAM_SUCCESS on success, QCARCAM_FAILURE on failure
 */
int qcarcam_stop(qcarcam_session_t session) {
    if (session >= 0) {
        printf("Stopped streaming for camera session %d\n", session);
        return QCARCAM_SUCCESS;
    }
    return QCARCAM_FAILURE;
}

/**
 * Get a frame (stub returns dummy data)
 * 
 * @param session The session to get the frame from
 * @param frame   The frame data (stubbed)
 * @return QCARCAM_SUCCESS on success, QCARCAM_FAILURE on failure
 */
int qcarcam_get_frame(qcarcam_session_t session, qcarcam_frame_t* frame) {
    if (!frame || session < 0) {
        return QCARCAM_FAILURE;
    }
    usleep(16600);
    frame->data = dummy_frame_data;  // Set pointer to dummy frame
    frame->size = sizeof(dummy_frame_data);  // Set frame size
    generateTestImage(frame->data, 500, 500);
    // printf("Got frame of size %zu for session %d\n", frame->size, session);
    return QCARCAM_SUCCESS;
}
