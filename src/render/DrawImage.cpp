#include "DrawImage.h"
#include "RenderUtil.h"
#include "FrameBuffer.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


#ifdef DEBUG_TAG
#undef DEBUG_TAG
#define DEBUG_TAG "EarlyRender DrawImage"
#endif

namespace evs {
namespace early {

static const char* vsSrc = R"(
attribute vec2 aPos;
attribute vec2 aTex;
varying vec2 vTexCoord;
void main() {
    vTexCoord = aTex;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* fsSrc = R"(
precision mediump float;
varying vec2 vTexCoord;
uniform sampler2D uTexture;
void main() {
    gl_FragColor = texture2D(uTexture, vTexCoord);
}
)";


bool DrawImage::onInit(int w, int h) {
    bool success = true;
    const float quad[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,  // Bottom left
        1.0f, -1.0f, 1.0f, 0.0f,   // Bottom right
        -1.0f,  1.0f, 0.0f, 1.0f,  // Top left
        1.0f,  1.0f, 1.0f, 1.0f    // Top right
    };
    
    do {
        width = w;
        height = h;

        if (outputFB.init(w, h) == false) {
            RENDER_ERROR("Initialize frame buffer failed\n");
            success = false;
            break;
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
            success = false;
            break;
        }

        glDeleteShader(vs);
        glDeleteShader(fs);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    } while (false);
    return success;
}

void DrawImage::onRender() {
    if (inputFB.isinit() == false) {
        RENDER_WARN("Input fb is not initialized, skip execution for %s\n", name.c_str());
        return;
    }
    outputFB.bind();
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    GLint aPos = glGetAttribLocation(shaderProgram, "aPos");
    GLint aTex = glGetAttribLocation(shaderProgram, "aTex");
    if (aPos >= 0) {
        glEnableVertexAttribArray(aPos);
        glVertexAttribPointer(aPos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    }

    if (aTex >= 0) {
        glEnableVertexAttribArray(aTex);
        glVertexAttribPointer(aTex, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputFB.getTexture());
    glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(aPos);
    glDisableVertexAttribArray(aTex);
}

void DrawImage::onDestroy() {
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