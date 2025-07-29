#ifndef UPLOADTEXTUREPASS_H
#define UPLOADTEXTUREPASS_H

#include "Renderable.h"
#include "FrameBuffer.h"

#define USED_FRAME_BUFFER_SIZE (2)

namespace evs {
namespace early {

class UploadTexture : public Renderable
{
public:
    explicit UploadTexture();
    void setImageData(const void *pixels, int width, int height);

protected:
    bool onInit(int width, int height) override;
    void onRender() override;
    void onDestroy() override;

private:
    const void *pixelData = nullptr;
    int imageWidth = 0;
    int imageHeight = 0;
};

} // namespace early
} // namespace evs
#endif // UPLOADTEXTUREPASS_H