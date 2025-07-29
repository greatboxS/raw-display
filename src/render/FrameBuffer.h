#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <EGL/eglext.h>
#include <atomic>

namespace evs {
namespace early {

class FrameBuffer
{
public:
    enum class FrameState {
        READY,
        RENDERING,
        DISPLAYING,
        MAX,
    };
    explicit FrameBuffer();
    ~FrameBuffer();

    explicit FrameBuffer(const FrameBuffer &fb)
        : fbo(fb.fbo)
        , texture(fb.texture)
        , width(fb.width)
        , height(fb.height)
        , isInited(fb.isInited) {
    }

    explicit FrameBuffer(FrameBuffer &&fb)
        : fbo(fb.fbo)
        , texture(fb.texture)
        , width(fb.width)
        , height(fb.height)
        , isInited(fb.isInited) {
        fb.isInited = false;
    }

    FrameBuffer &operator=(const FrameBuffer &fb) {
        this->fbo = fb.fbo;
        this->texture = fb.texture;
        this->width = fb.width;
        this->height = fb.height;
        this->isInited = fb.isInited;
        return *this;
    }

    FrameBuffer &operator=(FrameBuffer &&fb) {
        this->fbo = fb.fbo;
        this->texture = fb.texture;
        this->width = fb.width;
        this->height = fb.height;
        this->isInited = fb.isInited;
        fb.isInited = false;
        return *this;
    }

    bool init(int width, int height);
    void bind();
    void unbind();
    void destroy();
    bool isinit() const { return isInited; }

    inline GLuint getTexture() const { return texture; }
    inline GLuint getFBO() const { return fbo; }

private:
    GLuint fbo = 0;
    GLuint texture = 0;
    int width = 0;
    int height = 0;
    bool isInited = false;
};

} // namespace early
} // namespace evs
#endif // FRAMEBUFFER_H