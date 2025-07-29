#include "UploadTexture.h"
#include "RenderUtil.h"
#include <stdint.h>

#ifdef DEBUG_TAG
#undef DEBUG_TAG
#define DEBUG_TAG "EarlyRender UploadTexture"
#endif

namespace evs {
namespace early {

bool UploadTexture::onInit(int width, int height) {
    bool success = true;
    if (outputFB.init(width, height) == false) {
        RENDER_ERROR("Initialize frame buffer failed\n");
        success = false;
    }
    return success;
}

UploadTexture::UploadTexture()
    : Renderable("UploadTexture")
    , pixelData(nullptr)
    , imageWidth(0)
    , imageHeight(0) {
}

void UploadTexture::setImageData(const void *pixels, int width, int height) {
    pixelData = pixels;
    imageWidth = width;
    imageHeight = height;
}

void UploadTexture::onRender() {
    if ((pixelData == nullptr)
        || (imageWidth == 0)
        || (imageHeight == 0)) {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, outputFB.getTexture());
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imageWidth, imageHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
}

void UploadTexture::onDestroy() {
    outputFB.destroy();
}

} // namespace early
} // namespace evs
