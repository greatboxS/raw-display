#ifndef DRAWIMAGE_H
#define DRAWIMAGE_H

#include "Renderable.h"

namespace evs {
namespace early {

class DrawImage : public Renderable
{
public:
    explicit DrawImage()
        : Renderable("DrawImage")
        , shaderProgram(0)
        , vao(0)
        , vbo(0)
        , width(0)
        , height(0) {}

    bool onInit(int w, int h) override;
    void onRender() override;
    void onDestroy() override;

private:
    
    GLuint shaderProgram = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    int width = 0;
    int height = 0;
};

} // namespace early
} // namespace evs

#endif // DRAWIMAGE_H