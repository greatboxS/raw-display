#ifndef RVCRENDERER_H
#define RVCRENDERER_H

#include "RendererAbstraction.h"
#include "RenderLoop.h"
#include "UploadTexture.h"
#include "BlitToScreen.h"
#include "DrawImage.h"
#include "DrawGuidelines.h"
#include "QualcommCamera.h"
#include "DrmDevice.h"

#include <stdio.h>
#include <fcntl.h>
#include <drm/drm_fourcc.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <unistd.h>
#include <sys/mman.h>

#include <assert.h>
#include <string.h>

extern "C" {
PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES =
    (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR =
    (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
}

#ifdef DEBUG_TAG
#undef DEBUG_TAG
#define DEBUG_TAG "EarlyRender RVCController"
#endif

#include <condition_variable>

using namespace evs::early;
using namespace evs::early::drm;

struct texture_storage_metadata_t {
    int fourcc;
    EGLuint64KHR modifiers;
    EGLint stride;
    EGLint offset;
};
class RVCController : public RendererAbstraction
{
    RVCController(const RVCController &) = delete;
    RVCController &operator=(const RVCController &) = delete;
    RVCController(RVCController &&) = delete;
    RVCController &operator=(RVCController &&) = delete;

public:
    ~RVCController() override {
        camera.stopPreview();
        camera.deInitCamera();
        camera.exitFrameCaptureWorker();
        if (m_renderLoop != nullptr) {
            m_renderLoop->stop();
            m_renderLoop.reset();
        }
    };

    RVCController(RenderContext *ctx)
        : RendererAbstraction(ctx)
        , grabFrame(nullptr)
        , camera()
        , m_frameMutex()
        , m_frameCV()
        , m_uploadTexture(nullptr)
        , m_state{0}
        , m_renderLoop(nullptr)
        , m_drmDevice{nullptr} {

        m_renderLoop = std::make_unique<RenderLoop>(this, ctx);

        camera.initCamera(0);
        camera.createFrameCaptureWorker(RVCController::camera_frame_callback, this);

        m_uploadTexture = std::make_shared<UploadTexture>();
        m_blitTexture = std::make_shared<BlitToScreen>();
        this->addRenderJob(m_uploadTexture);
        this->addRenderJob(std::make_shared<DrawImage>());
        this->addRenderJob(std::make_shared<DrawGuidelines>());
        this->addRenderJob(m_blitTexture);
    }

    bool initRederer() override {
        RendererAbstraction::initRederer();
        initDisplay(m_drmDevice);
        return true;
    }

    bool rendering() override {
        RendererAbstraction::rendering();
        int idx = m_blitTexture->bufferIdx();
        return m_drmDevice->setModeCrtc(m_drmDevice->buffer(idx));
    }

    void setDrmDisplay(::drm::DrmDevice *drmDevice) {
        m_drmDevice = drmDevice;
    }

    bool initDisplay(DrmDevice *drmDevice) {
        bool success = false;
        if (drmDevice->isOpen()) {
            int dmaFd[2];
            dmaFd[0] = ::drm::HeapDMAAllocator::exposeHandleToFd(drmDevice->fd(), drmDevice->buffer0());
            dmaFd[1] = ::drm::HeapDMAAllocator::exposeHandleToFd(drmDevice->fd(), drmDevice->buffer1());
            if (dmaFd[0] < 0 || dmaFd[1] < 0) {
                printf("Failed to expose DMA buffer.\n");
                drmDevice->deInitDisplay();
                drmDevice->close();
            } else {
                for (int i = 0; i < 2; i++) {
                    // FrameBuffer &fb = m_blitTexture->getMapBuf()[i];
                    printf("Create EGLImage: fd=%d w=%d h=%d stride=%d format=%x\n",
                           dmaFd[i],
                           (int)drmDevice->width(),
                           (int)drmDevice->height(),
                           (int)drmDevice->buffer0()->stride,
                           DRM_FORMAT_ARGB8888);

                    // EGL: Create EGL image from the GL texture
                    // EGLImage image = eglCreateImage(m_context->eglDisplay(),
                    //                                 m_context->eglContext(),
                    //                                 EGL_GL_TEXTURE_2D,
                    //                                 (EGLClientBuffer)(uint64_t)fb.getTexture(),
                    //                                 NULL);
                    // assert(image != EGL_NO_IMAGE);

                    // // The next line works around an issue in radeonsi driver (fixed in master at the time of writing). If you are
                    // // having problems with texture rendering until the first texture update you can uncomment this line
                    // // glFlush();

                    // // EGL (extension: EGL_MESA_image_dma_buf_export): Get file descriptor (texture_dmabuf_fd) for the EGL image and get its
                    // // storage data (texture_storage_metadata)
                    // int texture_dmabuf_fd;
                    // struct texture_storage_metadata_t texture_storage_metadata;

                    // int num_planes;
                    // PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC eglExportDMABUFImageQueryMESA =
                    //     (PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC)eglGetProcAddress("eglExportDMABUFImageQueryMESA");
                    // EGLBoolean queried = eglExportDMABUFImageQueryMESA(m_context->eglDisplay(),
                    //                                                    image,
                    //                                                    &texture_storage_metadata.fourcc,
                    //                                                    &num_planes,
                    //                                                    &texture_storage_metadata.modifiers);
                    // assert(queried);
                    // assert(num_planes == 1);
                    // PFNEGLEXPORTDMABUFIMAGEMESAPROC eglExportDMABUFImageMESA =
                    //     (PFNEGLEXPORTDMABUFIMAGEMESAPROC)eglGetProcAddress("eglExportDMABUFImageMESA");
                    // EGLBoolean exported = eglExportDMABUFImageMESA(m_context->eglDisplay(),
                    //                                                image,
                    //                                                &texture_dmabuf_fd,
                    //                                                &texture_storage_metadata.stride,
                    //                                                &texture_storage_metadata.offset);
                    // assert(exported);
                    // struct drm_prime_handle prime = {};
                    // prime.fd = texture_dmabuf_fd;
                    // prime.flags = DRM_CLOEXEC | DRM_RDWR;

                    // if (drmIoctl(drmDevice->fd(), DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime) != 0) {
                    //     printf("DRM_IOCTL_PRIME_FD_TO_HANDLE failed\n");
                    // }

                    // uint32_t fd = 0;
                    // if (drmModeAddFB(drmDevice->fd(), drmDevice->width(), drmDevice->height(), 24, drmDevice->bpp(), texture_storage_metadata.stride, prime.handle, &fd) != 0) {
                    //     printf("Create FB failed %u %s\n",prime.handle, strerror(errno));
                    // }

                    // void *map = mmap(nullptr, drmDevice->buffer0()->size, PROT_READ | PROT_WRITE, MAP_SHARED, texture_dmabuf_fd, 0);
                    // if (map == MAP_FAILED) {
                    //     map = nullptr;
                    //     printf("mmap failed  %s\n", strerror(errno));
                    //     break;
                    // }
                    // drmDevice->setBufferPtr(i, map);

                    // EGLAttrib const attribute_list[] = {
                    //     EGL_WIDTH,
                    //     (int)drmDevice->width(),
                    //     EGL_HEIGHT,
                    //     (int)drmDevice->height(),
                    //     EGL_LINUX_DRM_FOURCC_EXT,
                    //     texture_storage_metadata.fourcc,
                    //     EGL_DMA_BUF_PLANE0_FD_EXT,
                    //     dmaFd[i],
                    //     EGL_DMA_BUF_PLANE0_OFFSET_EXT,
                    //     texture_storage_metadata.offset,
                    //     EGL_DMA_BUF_PLANE0_PITCH_EXT,
                    //     texture_storage_metadata.stride,
                    //     EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT,
                    //     (uint32_t)(texture_storage_metadata.modifiers & ((((uint64_t)1) << 33) - 1)),
                    //     EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT,
                    //     (uint32_t)((texture_storage_metadata.modifiers >> 32) & ((((uint64_t)1) << 33) - 1)),
                    //     EGL_NONE};
                    // image = eglCreateImage(m_context->eglDisplay(),
                    //                                 NULL,
                    //                                 EGL_LINUX_DMA_BUF_EXT,
                    //                                 (EGLClientBuffer)NULL,
                    //                                 attribute_list);

                    // EGLint attrs[] = {
                    //     EGL_WIDTH,
                    //     (int)drmDevice->width(),
                    //     EGL_HEIGHT,
                    //     (int)drmDevice->height(),
                    //     EGL_LINUX_DRM_FOURCC_EXT,
                    //     DRM_FORMAT_ARGB8888,
                    //     EGL_DMA_BUF_PLANE0_FD_EXT,
                    //     dmaFd[i],
                    //     EGL_DMA_BUF_PLANE0_OFFSET_EXT,
                    //     0,
                    //     EGL_DMA_BUF_PLANE0_PITCH_EXT,
                    //     (int)drmDevice->buffer0()->stride,
                    //     EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT,
                    //     (uint32_t)(DRM_FORMAT_MOD_LINEAR & 0xFFFFFFFF),
                    //     EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT,
                    //     (uint32_t)(DRM_FORMAT_MOD_LINEAR >> 32),
                    //     EGL_NONE};

                    // EGLImageKHR image = eglCreateImageKHR(m_context->eglDisplay(), EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attrs);
                    // if (image == EGL_NO_IMAGE_KHR) {
                    //     printf("Failed to create EGL image from DMA buffer %d %0X.\n", dmaFd[i], eglGetError());
                    // } else {
                    //     printf("EGL image created successfully from DMA buffer %d %0X.\n", dmaFd[i], eglGetError());
                    // }
                    // glBindTexture(GL_TEXTURE_2D, fb.getTexture());
                    // glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
                }
                m_drmDevice = drmDevice;
                success = true;
                printf("DRM device initialized successfully with DMA buffer %d, %d.\n", dmaFd[0], dmaFd[1]);
            }
        }
        return success;
    }

    void startRendering() {
        if (m_renderLoop != nullptr) {
            m_renderLoop->start();
        }
    }

    void stopRendering() {
        if (m_renderLoop != nullptr) {
            m_renderLoop->stop();
        }
    }

    void startPreview() {
        camera.startPreview();
    }

    void stopPreview() {
        camera.stopPreview();
    }

    void drawFrame() {
        m_drmDevice->flipBuffer(true);
        m_drmDevice->waitFlipEvent();
    }

    bool addFrame(void *frame) override {
        grabFrame = static_cast<CameraFrame *>(frame);
        if (grabFrame == nullptr) {
            return false;
        }

        std::unique_lock<std::mutex> lock(m_frameMutex);
        CameraBuffer &buffer = grabFrame->getBuffer();
        m_uploadTexture->setImageData(buffer.data, buffer.width, buffer.height);
        m_state = 0;
        m_frameCV.notify_all();
        return true;
    }

    bool nextFrameReady() override {
        bool success = false;
        std::unique_lock<std::mutex> lock(m_frameMutex);
        success = m_frameCV.wait_for(lock, std::chrono::milliseconds(1000), [this]() {
            return (m_state == 0);
        });

        if (success) {
            m_state += 1;
        } else {
            printf("No new frame received in the last 1000ms\n");
        }
        return success;
    }

private:
    static void camera_frame_callback(CameraAbstraction *, CameraFrame *frame, void *param) {
        RVCController *renderer = static_cast<RVCController *>(param);
        if (renderer != nullptr) {
            renderer->addFrame(frame);
        }
    }

    CameraFrame *grabFrame = nullptr;
    QualcommCamera camera;
    std::mutex m_frameMutex;
    std::condition_variable m_frameCV;
    std::shared_ptr<UploadTexture> m_uploadTexture = nullptr;
    std::shared_ptr<BlitToScreen> m_blitTexture = nullptr;
    std::atomic<int> m_state{0};
    std::unique_ptr<RenderLoop> m_renderLoop;
    ::drm::DrmDevice *m_drmDevice;
};

#endif // RVCRENDERER_H