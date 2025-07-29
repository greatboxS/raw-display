#ifndef RENDERCONTEXT_H
#define RENDERCONTEXT_H

#include <EGL/egl.h>

namespace evs {
namespace early {

class RenderContext
{
public:
    RenderContext(int width,
                  int height,
                  void *nativeDisplay = nullptr,
                  void *nativeWindow = nullptr,
                  EGLContext sharedContext = EGL_NO_CONTEXT);
    virtual ~RenderContext();

    bool isInitialized() const { return (eglGetCurrentContext() != EGL_NO_CONTEXT); }
    virtual bool initContext();
    virtual bool makeCurrent();
    virtual void swapBuffers();
    virtual void shutdown();

    int width() { return m_width; }
    int height() { return m_height; }

    EGLDisplay eglDisplay() const { return m_display; }
    EGLSurface eglSurface() const { return m_surface; }
    EGLContext eglContext() const { return m_context; }

private:
    void *m_nativeDisplay = nullptr;
    void *m_nativeWindow = nullptr;
    EGLDisplay m_display = EGL_NO_DISPLAY;
    EGLSurface m_surface = EGL_NO_SURFACE;
    EGLContext m_context = EGL_NO_CONTEXT;
    EGLContext m_sharedContext = EGL_NO_CONTEXT;
    int m_width = 0;
    int m_height = 0;
};

} // namespace early
} // namespace evs
#endif // RENDERCONTEXT_H