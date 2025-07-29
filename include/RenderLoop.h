#ifndef RENDERLOOP_H
#define RENDERLOOP_H

#include "RenderContext.h"

#include <thread>
#include <atomic>

namespace evs {
namespace early {

class RendererAbstraction;

class RenderLoop
{
    RenderLoop(const RenderLoop &) = delete;
    RenderLoop &operator=(const RenderLoop &) = delete;
    RenderLoop(RenderLoop &&) = delete;
    RenderLoop &operator=(RenderLoop &&) = delete;

public:
    RenderLoop(RendererAbstraction *pl,
               RenderContext *context);
    virtual ~RenderLoop();

    int start();
    int stop();

protected:
    virtual void onRenderLoopStart() {}
    virtual void onRenderLoopStop() {}
    virtual void run();

    RenderContext *context() { return m_ct; }

private:
    std::atomic<bool> m_isRuning{false};
    RendererAbstraction *m_rd = nullptr;
    RenderContext *m_ct = nullptr;
    std::thread m_rdThread{};
};

} // namespace early
} // namespace evs
#endif