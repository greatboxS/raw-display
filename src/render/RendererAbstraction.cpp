#include "RendererAbstraction.h"
#include "RenderUtil.h"

#ifdef DEBUG_TAG
#undef DEBUG_TAG
#define DEBUG_TAG "EarlyRender RendererAbstraction"
#endif

namespace evs {
namespace early {

RendererAbstraction::RendererAbstraction(RenderContext *ctx)
    : m_context(ctx)
    , m_renderJobs({})
    , m_state(-1) {}

void RendererAbstraction::addRenderJob(std::shared_ptr<Renderable> job) {
    std::unique_lock<std::mutex> lock(m_mtx);
    m_renderJobs.push_back(std::move(job));
}

void RendererAbstraction::clearRenderJob() {
    std::unique_lock<std::mutex> lock(m_mtx);
    m_renderJobs.clear();
}

bool RendererAbstraction::initRederer() {
    bool success = true;
    do {
        std::unique_lock<std::mutex> lock(m_mtx);
        if (m_init == true) {
            RENDER_WARN("Renderer is already initialized\n");
            success = false;
            break;
        }
        m_init = true;
        if (m_context == nullptr) {
            success = false;
            RENDER_ERROR("There is no context is set\n");
            break;
        }

        for (auto &job : m_renderJobs) {
            m_context->makeCurrent();
            if (!job->init(m_context->width(), m_context->height(), m_context)) {
                RENDER_ERROR("Initialize render job %s failed\n", job->name.c_str());
                success = false;
                break;
            }
        }
        m_state = -1;
    } while (false);
    return success;
}

void RendererAbstraction::deInitRenderer() {
    do {
        std::unique_lock<std::mutex> lock(m_mtx);
        if (m_init == false) {
            RENDER_WARN("Renderer is not initialized, nothing to deinitialize\n");
            break;
        }
        m_init = false;
        for (auto &job : m_renderJobs) {
            job->destroy();
        }
        m_renderJobs.clear();
        m_context->shutdown();
        m_context = nullptr;
        m_state = -1;
    } while(false);
}

bool RendererAbstraction::rendering() {
    m_state += 2;
    if (m_context->makeCurrent() == false) {
        RENDER_ERROR("Failed to make current context\n");
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(m_mtx);
        FrameBuffer prevFB{};
        for (auto &job : m_renderJobs) {
            job->setInputFB(prevFB);
            job->execute();
            prevFB = job->getOutputFB();
        }
    }

    // glFlush();
    glFinish();
    // m_context->swapBuffers();
    return true;
}

} // namespace early
} // namespace evs