#ifndef RENDERPASSBASE_H
#define RENDERPASSBASE_H

#include "RenderContext.h"
#include "FrameBuffer.h"

#include <GLES2/gl2.h>
#include <string>
#include <memory>

namespace evs {
namespace early {

class Renderable
{
    friend class RendererAbstraction;

public:
    bool init(int width, int height, RenderContext *ctx) {
        ctxPtr = ctx;
        return onInit(width, height);
    }

    void execute() {
        onRender();
    }

    void destroy() {
        onDestroy();
    }

    FrameBuffer &getInputFB() {
        return inputFB;
    }

    void setInputFB(const FrameBuffer &fb) {
        inputFB = fb;
    }

    FrameBuffer &getOutputFB() {
        return outputFB;
    }

    explicit Renderable(const std::string &n)
        : inputFB{}
        , outputFB{}
        , name(n) {}
    virtual ~Renderable() = default;

protected:
    virtual bool onInit(int width, int height) = 0;
    virtual void onRender() = 0;
    virtual void onDestroy() = 0;

    GLuint compileShader(GLenum type, const char *src) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[256];
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    FrameBuffer inputFB{};
    FrameBuffer outputFB{};
    std::string name{""};
    RenderContext *ctxPtr = nullptr;
};

} // namespace early
} // namespace evs

#endif