#include "RenderLoop.h"
#include "RenderUtil.h"
#include "RendererAbstraction.h"
#include "RenderContext.h"

#include <future>
#include <functional>

#ifdef DEBUG_TAG
#undef DEBUG_TAG
#define DEBUG_TAG "EarlyRender RenderLoop"
#endif

namespace evs {
namespace early {

RenderLoop::RenderLoop(RendererAbstraction *pl,
                       RenderContext *context)
    : m_isRuning(false)
    , m_rd(pl)
    , m_ct(context)
    , m_rdThread() {
}

RenderLoop::~RenderLoop() {
}

int RenderLoop::start() {
    int ret = 0;
    RENDER_DEBUG("Creating render loop thread\n");

    do {
        if (m_rdThread.joinable() == true) {
            RENDER_DEBUG("Camera loop thread already exists, exiting previous thread\n");
            ret = -1;
            break;
        }

        m_isRuning.store(true);
        m_rdThread = std::thread(std::bind(&RenderLoop::run, this));
    } while (false);
    return ret;
}

int RenderLoop::stop() {
    int ret = 0;

    RENDER_DEBUG("Requesting loop exit\n");
    do {
        if (m_rdThread.joinable() == false) {
            RENDER_DEBUG("Loop thread is not initialized\n");
            ret = -1;
            break;
        }

        m_isRuning.store(false);
        if (m_rdThread.joinable() == true) {
            RENDER_DEBUG("Waiting for loop thread to exit\n");
            m_rdThread.join();
        } else {
            RENDER_DEBUG("Loop thread is not joinable\n");
            ret = -1;
        }
    } while (false);
    return ret;
}

void RenderLoop::run() {
    RENDER_DEBUG("Starting render loop thread\n");

    onRenderLoopStart();

    while (m_isRuning.load() == true) {
        if (m_ct->isInitialized() == false) {
            RENDER_DEBUG("Render context is not initialized, initializing now\n");

            m_rd->deInitRenderer();
            m_ct->shutdown();

            if (m_ct->initContext() == false) {
                RENDER_ERROR("Failed to initialize render context\n");
                break;
            }

            if (m_rd->isInitialized() == false) {
                RENDER_DEBUG("Renderer is not initialized, initializing now\n");
                if (m_rd->initRederer() == false) {
                    RENDER_ERROR("Failed to initialize renderer\n");
                    break;
                }
            }
        }

        if (m_ct->makeCurrent() == false) {
            RENDER_ERROR("Failed to make render context current\n");
        }

        if (m_rd->nextFrameReady() == true) {
            if (m_rd->rendering() == false) {
                RENDER_DEBUG("Error while rendering a frame\n");
            }
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
    RENDER_DEBUG("Exiting render loop thread\n");

    onRenderLoopStop();
}

} // namespace early
} // namespace evs