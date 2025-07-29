#include "QualcommCamera.h"
#include "CommonUtil.h"
#include <thread>

int main(int /*argc*/, char const ** /*argv[]*/) {
    auto a = new evs::early::QualcommCamera();
    a->initCamera(0);
    if (a->getError() != evs::early::CameraError::NONE) {
        EARLY_ERROR("Failed to initialize camera: %d\n", static_cast<int>(a->getError()));
        delete a;
        return -1;
    } else {
        EARLY_DEBUG("Camera initialized successfully\n");
    }

    a->setConfig({1920, 1080, 30}); // Set camera configuration (width, height, fps, format)
    if (a->getError() != evs::early::CameraError::NONE) {
        EARLY_ERROR("Failed to set camera configuration: %d\n", static_cast<int>(a->getError()));
        a->deInitCamera();
        delete a;
        return -1;
    }

    a->createFrameCaptureWorker();
    if (a->getError() != evs::early::CameraError::NONE) {
        EARLY_ERROR("Failed to create camera loop: %d\n", static_cast<int>(a->getError()));
        a->deInitCamera();
        delete a;
        return -1;
    }
    EARLY_DEBUG("Camera loop created successfully\n");
    a->startPreview();

    if (a->getError() != evs::early::CameraError::NONE) {
        EARLY_ERROR("Failed to start camera preview: %d\n", static_cast<int>(a->getError()));
        a->deInitCamera();
        delete a;
        return -1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));

    a->stopPreview();
    if (a->getError() != evs::early::CameraError::NONE) {
        EARLY_ERROR("Failed to stop camera preview: %d\n", static_cast<int>(a->getError()));
    }

    delete a;
    return 0;
}
