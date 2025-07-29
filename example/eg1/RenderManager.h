#ifndef RENDERCONTEXTMANAGER_H
#define RENDERCONTEXTMANAGER_H

#include "RenderContext.h"
#include "RendererAbstraction.h"
#include "RenderLoop.h"

#include <memory>
#include <EGL/egl.h>

namespace evs {
namespace early {

class RenderManager
{
public:
    enum class ContextType {
        X11,
        WAYLAND,
        DRM,
        MAX,
    };
    explicit RenderManager(std::shared_ptr<RendererAbstraction> renderer);
    ~RenderManager();

    int init(int w, int h, ContextType type = ContextType::X11);
    int deinit();

    int makeCurrent();
    void swapBuffers();
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    int startRender();
    int stopRender();

    std::shared_ptr<RenderContext> ctx() { return m_context; }

private:
    std::shared_ptr<RenderContext> m_context = nullptr;
    std::shared_ptr<RendererAbstraction> m_renderer = nullptr;
    RenderLoop *m_renderLoop = nullptr;
    ContextType contextType = ContextType::MAX;
    EGLDisplay eglDisplay = EGL_NO_DISPLAY;
    EGLContext eglContext = EGL_NO_CONTEXT;
    EGLSurface eglSurface = EGL_NO_SURFACE;
    int width = 0;
    int height = 0;
};

} // namespace early
} // namespace evs

#endif // RENDERCONTEXTMANAGER_H