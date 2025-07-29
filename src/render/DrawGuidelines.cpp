#include "DrawGuidelines.h"
#include "FrameBuffer.h"
#include "RenderUtil.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <chrono>

#ifdef DEBUG_TAG
#undef DEBUG_TAG
#define DEBUG_TAG "EarlyRender DrawGuidelines"
#endif

namespace evs {
namespace early {

static const char *vsSrc = R"(
attribute vec2 aPos;
varying vec2 vTex;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTex = (aPos + vec2(1.0)) * 0.5;
}
)";

const char *fsSrc = R"(
precision mediump float;
uniform vec4 uColor;
void main() {
    gl_FragColor = uColor;
}
)";

static float lineVerts[4] = {-0.5f, -0.5f, 0.5f, 0.5f}; // x1,y1,x2,y2
static float velocity[4] = {0.4f, 0.3f, -0.3f, 0.5f};   // dx1,dy1,dx2,dy2
static auto lastTime = std::chrono::steady_clock::now();

DrawGuidelines::~DrawGuidelines() {
}

bool DrawGuidelines::onInit(int w, int h) {
    bool success = true;

    do {
        GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
        GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
        shaderProgram = glCreateProgram();

        if (shaderProgram == 0) {
            RENDER_ERROR("Shader program is not valid!");
            success = false;
            break;
        }

        glAttachShader(shaderProgram, vs);
        glAttachShader(shaderProgram, fs);
        glBindAttribLocation(shaderProgram, 0, "aPos");
        glLinkProgram(shaderProgram);

        GLint linked;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linked);
        if (!linked) {
            char log[256];
            glGetProgramInfoLog(shaderProgram, sizeof(log), nullptr, log);
            RENDER_ERROR("Shader link error: %s\n", log);
            success = false;
            break;
        }

        glDeleteShader(vs);
        glDeleteShader(fs);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lineVerts), lineVerts, GL_DYNAMIC_DRAW);
    } while (false);
    return success;
}

static void updateLinePositon() {
    auto currentTime = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;
    for (int i = 0; i < 4; ++i) {
        lineVerts[i] += velocity[i] * dt;

        if (lineVerts[i] < -1.0f) {
            lineVerts[i] = -1.0f;
            velocity[i] = -velocity[i];
        } else if (lineVerts[i] > 1.0f) {
            lineVerts[i] = 1.0f;
            velocity[i] = -velocity[i];
        }
    }
}

void DrawGuidelines::onRender() {
    if (inputFB.isinit() == false) {
        RENDER_WARN("Input fb is not initialized, skip execution for %s\n", name.c_str());
        return;
    }
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    updateLinePositon();

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineVerts), lineVerts);
    glUseProgram(shaderProgram);

    GLint aPosLoc = glGetAttribLocation(shaderProgram, "aPos");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");
    if (aPosLoc >= 0) {
        glEnableVertexAttribArray(aPosLoc);
        glVertexAttribPointer(aPosLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    }

    if (colorLoc >= 0) {
        glUniform4f(colorLoc, 1.0f, 0.0f, 0.0f, 0.8f);
    }
    glDrawArrays(GL_LINES, 0, 2);

    if (aPosLoc >= 0) {
        glDisableVertexAttribArray(aPosLoc);
    }
    outputFB = inputFB;
}

void DrawGuidelines::onDestroy() {
    if (vbo > 0) {
        glDeleteBuffers(1, &vbo);
    }
    if (shaderProgram != 0) {
        glDeleteProgram(shaderProgram);
    }
    outputFB.destroy();
}

} // namespace early
} // namespace evs
