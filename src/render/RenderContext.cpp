#include "RenderContext.h"
#include "RenderUtil.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <EGL/eglext.h>

#ifdef DEBUG_TAG
#undef DEBUG_TAG
#define DEBUG_TAG "EarlyRender RenderContextX11"
#endif

#ifndef EGL_CONTEXT_VER
#define EGL_CONTEXT_VER (2)
#endif

namespace evs {
namespace early {

const char *vertexShaderSrc = R"(
    attribute vec2 aPos;
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
    }
)";

const char *fragmentShaderSrc = R"(
    precision mediump float;
    void main() {
        gl_FragColor = vec4(1.0, 0.5, 0.0, 1.0);
    }
)";

GLuint createShader(GLenum type, const char *src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        RENDER_ERROR("Shader compile error: %s\n", log);
    }
    return shader;
}

RenderContext::RenderContext(int width,
                                   int height,
                                   void *nativeDisplay,
                                   void *nativeWindow,
                                   EGLContext sharedContext)
    : m_nativeDisplay(nativeDisplay)
    , m_nativeWindow(nativeWindow)
    , m_display(EGL_NO_DISPLAY)
    , m_surface(EGL_NO_SURFACE)
    , m_context(EGL_NO_CONTEXT)
    , m_sharedContext(sharedContext)
    , m_width(width)
    , m_height(height) {
}

RenderContext::~RenderContext() {
    shutdown();
    m_nativeWindow = 0;
    m_display = EGL_NO_DISPLAY;
    m_surface = EGL_NO_SURFACE;
    m_context = EGL_NO_CONTEXT;
    m_sharedContext = EGL_NO_CONTEXT;
    m_width = 0;
    m_height = 0;
}

bool RenderContext::initContext() {
    bool ret = false;
    EGLConfig config;
    EGLint numConfigs;
    EGLint attr[] = {
        EGL_SURFACE_TYPE,
        EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,
        8,
        EGL_GREEN_SIZE,
        8,
        EGL_BLUE_SIZE,
        8,
        EGL_DEPTH_SIZE,
        16,
        EGL_NONE};
    EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, EGL_CONTEXT_VER, EGL_NONE};

    do {
        if (m_nativeDisplay != nullptr) {
            m_display = eglGetDisplay((EGLNativeDisplayType)m_nativeDisplay);
        } else {
            m_display = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL);
        }

        if (m_display == EGL_NO_DISPLAY) {
            RENDER_ERROR("Failed to get EGL display: 0x%0X\n", eglGetError());
            break;
        }

        EGLint major, minor;
        if (eglInitialize(m_display, &major, &minor) == false) {
            RENDER_ERROR("Failed to initialize EGL display: 0x%0X\n", eglGetError());
            break;
        }

        if (eglBindAPI(EGL_OPENGL_ES_API) == false) {
            RENDER_ERROR("eglBindAPI(EGL_OPENGL_ES_API) failed: 0x%0X\n", eglGetError());
            break;
        }

        if (!m_nativeWindow) {
            // Use PBUFFER if no native window is provided
            attr[1] = EGL_PBUFFER_BIT;
            attr[2] = EGL_NONE;
            attr[3] = EGL_NONE;
        }
        if (eglChooseConfig(m_display, attr, &config, 1, &numConfigs) == false) {
            RENDER_ERROR("Failed to choose EGL config: 0x%0X\n", eglGetError());
            break;
        }

        if (m_nativeWindow) {
            m_surface = eglCreateWindowSurface(m_display, config, (EGLNativeWindowType)m_nativeWindow, nullptr);
            if (m_surface == EGL_NO_SURFACE) {
                RENDER_WARN("eglCreateWindowSurface failed: 0x%0X\n", eglGetError());
                break;
            }
        } else {
            EGLint pbuffer_attribs[] = {
                EGL_WIDTH,
                m_width,
                EGL_HEIGHT,
                m_height,
                EGL_NONE};
            m_surface = eglCreatePbufferSurface(m_display, config, pbuffer_attribs);
            if (m_surface == EGL_NO_SURFACE) {
                RENDER_ERROR("eglCreatePbufferSurface failed: 0x%0X\n", eglGetError());
                break;
            }
        }

        m_context = eglCreateContext(m_display, config, m_sharedContext, &context_attribs[0]);
        if (m_context == EGL_NO_CONTEXT) {
            RENDER_ERROR("eglCreateContext failed: 0x%0X\n", eglGetError());
            break;
        }

        if (eglMakeCurrent(m_display, m_surface, m_surface, m_context) == false) {
            RENDER_ERROR("Failed to make current EGL context: 0x%0X\n", eglGetError());
            break;
        }
        ret = true;
    } while (false);

    if (ret == false) {
        /* Init failed, shutdown the init state */
        RENDER_DEBUG("Shutdown the invalid context\n");
        shutdown();
    }

    return ret;
}

bool RenderContext::makeCurrent() {
    if (eglMakeCurrent(m_display, m_surface, m_surface, m_context) == false) {
        RENDER_ERROR("Failed to make current EGL context: 0x%0X\n", eglGetError());
        return false;
    }
    return true;
}

void RenderContext::swapBuffers() {
    eglSwapBuffers(m_display, m_surface);
}

void RenderContext::shutdown() {
    if (m_display != EGL_NO_DISPLAY) {
        eglDestroySurface(m_display, m_surface);
        eglDestroyContext(m_display, m_context);
        eglTerminate(m_display);
    }
    m_display = EGL_NO_DISPLAY;
    m_surface = EGL_NO_SURFACE;
    m_context = EGL_NO_CONTEXT;
}

} // namespace early
} // namespace evs