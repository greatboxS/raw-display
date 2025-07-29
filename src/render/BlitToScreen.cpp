#include "BlitToScreen.h"
#include "RenderUtil.h"
#include "FrameBuffer.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#ifdef DEBUG_TAG
#undef DEBUG_TAG
#define DEBUG_TAG "EarlyRender BlitToScreen"
#endif

namespace evs {
namespace early {

static const char* vsSrc = R"(
attribute vec2 aPos;
attribute vec2 aTex;
varying vec2 vTex;
void main() {
    vTex = aTex;
    gl_Position = vec4(aPos, 0.0, 1.0);
})";

static const char* fsSrc = R"(
precision mediump float;
varying vec2 vTex;
uniform sampler2D uTex;
void main() {
    gl_FragColor = texture2D(uTex, vTex);
})";

bool BlitToScreen::onInit(int w, int h) {
    bool ret = false;
    const float quad[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,  // Bottom left
        1.0f, -1.0f, 1.0f, 0.0f,  // Bottom right
        -1.0f,  1.0f, 0.0f, 1.0f,  // Top left
        1.0f,  1.0f, 1.0f, 1.0f   // Top right
    };
    
    do {
        for (int i = 0; i < 2; i++) {
            if (mapedBuf[i].init(w, h) == false) {
                RENDER_ERROR("Initialize frame buffer failed\n");
                ret = false;
                break;
            }
        }

        GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
        GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
        GLint linked = 0;
        
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vs);
        glAttachShader(shaderProgram, fs);
        glBindAttribLocation(shaderProgram, 0, "aPos");
        glBindAttribLocation(shaderProgram, 1, "aTex");
        glLinkProgram(shaderProgram);
        
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linked);
        if (linked == false) {
            char log[512];
            glGetProgramInfoLog(shaderProgram, sizeof(log), nullptr, log);
            RENDER_ERROR("Shader link error: %s\n", log);
            break;
        }

        glDeleteShader(vs);
        glDeleteShader(fs);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        width = w;
        height = h;
        ret = true;
    } while (false);
    return ret;
}

void BlitToScreen::onRender() {
    if (inputFB.isinit() == false) {
        RENDER_WARN("Input fb is not initialized, skip execution for %s\n", name.c_str());
        return;
    }
    int currentIndex = currentBuf ^ 1;
    glBindFramebuffer(GL_FRAMEBUFFER, mapedBuf[currentIndex].getFBO());
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void *)(sizeof(float) * 2));
    glEnableVertexAttribArray(1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputFB.getTexture());

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    currentBuf = currentIndex;
}

void BlitToScreen::onDestroy() {
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