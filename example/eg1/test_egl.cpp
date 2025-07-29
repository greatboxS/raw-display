#include "RenderContext.h"
#include "RVCController.h"

#include <X11/Xlib.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <gbm.h>

using namespace evs::early;

// Dummy function to create a test image (RGBA)
void generateTestImage(uint8_t *buffer, int width, int height) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = (y * width + x) * 4;
            buffer[i + 0] = rand() % 256;
            buffer[i + 1] = rand() % 256;
            buffer[i + 2] = rand() % 256;
            buffer[i + 3] = 255; // rand() % 256;
        }
    }
}

int main() {
    int width = 512;
    int height = 512;

    // Display *x_display = XOpenDisplay(nullptr);
    // if (!x_display) {
    //     std::cerr << "Failed to open X display\n";
    //     return -1;
    // }

    // Window root = DefaultRootWindow(x_display);
    // Window xwin = XCreateSimpleWindow(x_display, root, 0, 0, width, height, 1, 0, 0);
    // XMapWindow(x_display, xwin);
    // XStoreName(x_display, xwin, "Render Test");

    DrmDevice drmDevice(0, DRM_ALLOCATOR_MMAP);
    if (drmDevice.open() == false) {
        std::cerr << "Failed to open DRM device.\n";
        return -1;
    }

    drmDevice.queryAllDeviceInfo();
    if (drmDevice.getConnectors().empty()) {
        printf("No connectors found.\n");
        drmDevice.close();
        return -1;
    }
    // ConnectorInfo
    for (const auto &connector : drmDevice.getConnectors()) {
        const DrmConnectorInfo &info = connector.second;
        std::cout << "Connector ID: " << info.id
                  << ", Name: " << info.name
                //   << ", Type: " << info.type
                //   << ", Width: " << info.width
                //   << ", Height: " << info.height
                  << ", Modes: " << info.encoder.crtc.width
                  << ", Modes: " << info.encoder.crtc.height << std::endl;
    }

    DrmConnectorInfo connectorInfo = drmDevice.getConnectors().begin()->second;
    if (!drmDevice.initDisplay(connectorInfo, 32, DRM_FORMAT_ARGB8888, DRM_MODE_FLAG_PVSYNC)) {
        printf("Failed to initialize display.\n");
        drmDevice.close();
        return -1;
    }

    width = drmDevice.width();
    height = drmDevice.height();
    // === Initialize EGL/GL context wrapper ===
    std::unique_ptr<RenderContext> renderContext = std::unique_ptr<RenderContext>(new RenderContext(width, height, nullptr, (void *)nullptr, EGL_NO_CONTEXT));
    auto renderer = std::make_shared<RVCController>(renderContext.get());
    renderer->setDrmDisplay(&drmDevice);
    renderer->startRendering();
    renderer->startPreview();

#if 1
    int counter = 0;
    while (counter < 10000000) {
        // auto buffer = drmDevice.activeBuffer();
        // if (buffer) {
        //     drmDevice.flipBuffer(true);
        //     drmDevice.waitFlipEvent();
        //     // drmDevice.setModeCrtc(buffer);
        // }

        // counter++;
        // usleep(16666); // Sleep for ~16ms to simulate ~60 FPS
    }
#endif
    sleep(10);
    renderContext->shutdown();
    // XDestroyWindow(x_display, xwin);
    // XCloseDisplay(x_display);
    renderer->stopRendering();
    renderer->stopPreview();
    renderer.reset();
    drmDevice.deInitDisplay();
    drmDevice.close();
    return 0;
}