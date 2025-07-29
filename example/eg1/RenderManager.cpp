#include "RenderManager.h"
#include "RenderUtil.h"
#include <EGL/eglext.h>
#include <GL/gl.h>
#include <cassert>
#include <vector>

#ifndef SUPPORT_CONTEXT_X11
#define SUPPORT_CONTEXT_X11
#endif

#ifdef SUPPORT_CONTEXT_X11
#include "RenderContext.h"
#endif

#ifdef SUPPORT_CONTEXT_DRM
#include "RenderContextDRM.h"
#endif

#ifdef SUPPORT_CONTEXT_WAYLAND
#include "RenderContextWayland.h"
#endif

#ifdef DEBUG_TAG
#undef DEBUG_TAG
#define DEBUG_TAG "EarlyRender RenderContextManager"
#endif

namespace evs {
namespace early {

RenderManager::RenderManager(std::shared_ptr<RendererAbstraction> renderer)
    : m_context(nullptr)
    , m_renderer(std::move(renderer))
    , m_renderLoop(nullptr)
    , contextType(ContextType::MAX)
    , eglDisplay(EGL_NO_DISPLAY)
    , eglContext(EGL_NO_CONTEXT)
    , eglSurface(EGL_NO_SURFACE)
    , width(0)
    , height(0) {
}

RenderManager::~RenderManager() {
}

int RenderManager::init(int w, int h, ContextType type) {
    int ret = 0;

    do {
        if (m_context != nullptr) {
            RENDER_WARN("Context already init\n");
            ret = -1;
            break;
        }
        width = w;
        height = h;
        contextType = type;
        if (contextType == ContextType::X11) {
#ifdef SUPPORT_CONTEXT_X11
            Display *x_display = nullptr;
            Window root = 0;
            Window x_win = 0;
            XSetWindowAttributes swa;

            m_context = std::make_shared<RenderContext>();
            x_display = XOpenDisplay(nullptr);
            if (x_display == nullptr) {
                RENDER_ERROR("Failed to open X display\n");
                ret = -1;
                break;
            }

            root = DefaultRootWindow(x_display);
            swa.event_mask = ExposureMask | PointerMotionMask | KeyPressMask;
            x_win = XCreateWindow(x_display, root, 0, 0, width, height, 0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask, &swa);
            XMapWindow(x_display, x_win);
            XStoreName(x_display, x_win, "EGL + OpenGL ES2 Triangle");

            ret = (m_context->init(width, height, x_display, (void *)x_win) ? 0 : -1);
#else
            RENDER_ERROR("X11 is not supporting on your build, please check the build option\n");
            ret = -1;
            break;
#endif
        } else if (contextType == ContextType::DRM) {
#ifdef SUPPORT_CONTEXT_DRM
            m_context = std::make_shared<RenderContextDRM>();
#else
            RENDER_ERROR("DRM is not supporting on your build, please check the build option\n");
            ret = -1;
            break;
#endif
        } else if (contextType == ContextType::WAYLAND) {
#ifdef SUPPORT_CONTEXT_WAYLAND
            m_context = std::make_shared<RenderContextWayland>();
#else
            RENDER_ERROR("Wayland is not supporting on your build, please check the build option\n");
            ret = -1;
            break;
#endif
        } else {
            RENDER_ERROR("Invalid m_context type, %d\n", static_cast<int>(contextType));
            ret = -1;
            break;
        }

        m_renderLoop = new (std::nothrow) RenderLoop(m_renderer.get());
        if (m_renderLoop == nullptr) {
            RENDER_ERROR("Can not allocate render pipe line\n");
            if (deinit() == -1) {
                RENDER_ERROR("Error while de-initialize the render manager\n");
            }
            ret = -1;
            break;
        }

        /* Set context to renderer class */
        if (m_renderer != nullptr) {
            m_renderer->m_context = m_context;
        }

        RENDER_DEBUG("Initialize successfully\n");
    } while (false);

    return ret;
}

int RenderManager::deinit() {
    int ret = -1;

    if (m_renderLoop != nullptr) {
        m_renderLoop->stop();
        delete m_renderLoop;
        m_renderLoop = nullptr;
    }

    if (m_renderer != nullptr) {
        m_renderer->deInitRenderer();
        m_renderer.reset();
    }
    if (m_context != nullptr) {
        m_context->shutdown();
        m_context.reset();
    }
    return ret;
}

int RenderManager::makeCurrent() {
    int ret = -1;
    if (m_context != nullptr) {
        ret = (m_context->makeCurrent() ? 0 : -1);
    }
    return ret;
}

void RenderManager::swapBuffers() {
    m_context->swapBuffers();
}

int RenderManager::startRender() {
    int ret = -1;
    if (m_renderLoop != nullptr) {
        ret = m_renderLoop->start();
    }
    return ret;
}

int RenderManager::stopRender() {
    int ret = -1;
    if (m_renderLoop != nullptr) {
        ret = m_renderLoop->stop();
    }
    return ret;
}

} // namespace early
} // namespace evs