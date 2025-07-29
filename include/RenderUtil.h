#ifndef RENDERUTIL_H
#define RENDERUTIL_H

#define DEBUG_TAG "EarlyRender"

#ifdef DEBUG_
#include <stdio.h>
#define RENDER_DEBUG(...) printf("- [DBG] - " DEBUG_TAG ": " __VA_ARGS__)
#define RENDER_INFO(...)  printf("- [INF] - " DEBUG_TAG ": " __VA_ARGS__)
#define RENDER_WARN(...)  printf("- [WRN] - " DEBUG_TAG ": " __VA_ARGS__)
#define RENDER_ERROR(...) printf("- [ERR] - " DEBUG_TAG ": " __VA_ARGS__)
#define RENDER_FATAL(...) printf("- [FAT] - " DEBUG_TAG ": " __VA_ARGS__)
#else

#include <stdio.h>
#define RENDER_DEBUG(...)
#define RENDER_INFO(...)
#define RENDER_WARN(...)
#define RENDER_ERROR(...) printf("- [ERR] - " DEBUG_TAG ": " __VA_ARGS__)
#define RENDER_FATAL(...) printf("- [FAT] - " DEBUG_TAG ": " __VA_ARGS__)
#endif

namespace evs {
namespace early {

}
} // namespace evs
#endif // RENDERUTIL_H