#ifndef COMMONUTIL_H
#define COMMONUTIL_H

#define DEBUG_TAG "EarlyBootCamera"

#ifdef DEBUG_
#include <stdio.h>
#define EARLY_DEBUG(...) printf("- [DBG] - " DEBUG_TAG ": " __VA_ARGS__)
#define EARLY_INFO(...)  printf("- [INF] - " DEBUG_TAG ": " __VA_ARGS__)
#define EARLY_WARN(...)  printf("- [WRN] - " DEBUG_TAG ": " __VA_ARGS__)
#define EARLY_ERROR(...) printf("- [ERR] - " DEBUG_TAG ": " __VA_ARGS__)
#define EARLY_FATAL(...) printf("- [FAT] - " DEBUG_TAG ": " __VA_ARGS__)
#else

#include <stdio.h>
#define EARLY_DEBUG(...)
#define EARLY_INFO(...)
#define EARLY_WARN(...)
#define EARLY_ERROR(...) printf("- [ERR] - " DEBUG_TAG ": " __VA_ARGS__)
#define EARLY_FATAL(...) printf("- [FAT] - " DEBUG_TAG ": " __VA_ARGS__)
#endif

#endif // COMMONUTIL_H