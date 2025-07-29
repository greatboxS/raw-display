#ifndef BLITTOSCREENPASS_H
#define BLITTOSCREENPASS_H

#include "Renderable.h"

namespace evs {
namespace early {

class BlitToScreen : public Renderable
{
public:
    explicit BlitToScreen()
        : Renderable("BlitToScreen")
        , shaderProgram(0)
        , vao(0)
        , vbo(0) {}

    bool onInit(int w, int h) override;
    void onRender() override;
    void onDestroy() override;

    inline int bufferIdx() const { return currentBuf; }

    FrameBuffer *getMapBuf() {
        return &mapedBuf[0];
    }

private:
    GLuint shaderProgram = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    int width = 0;
    int height = 0;
    FrameBuffer mapedBuf[2];
    int currentBuf = 0;
};

} // namespace early
} // namespace evs

#endif