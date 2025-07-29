#ifndef RENDERPIPELINE_H
#define RENDERPIPELINE_H

#include "Renderable.h"
#include "RenderContext.h"
#include <vector>
#include <memory>
#include <mutex>

namespace evs {
namespace early {

class RendererAbstraction
{
    friend class RenderManager;
public:
    RendererAbstraction(RenderContext *ctx = nullptr);
    virtual ~RendererAbstraction() = default;

    void addRenderJob(std::shared_ptr<Renderable> job);
    void clearRenderJob();

    bool isInitialized() const { return m_init; }
    virtual bool initRederer();
    virtual void deInitRenderer();
    virtual bool rendering();
    
    virtual bool addFrame(void *) = 0;
    virtual bool nextFrameReady() = 0;
protected:
    RenderContext *m_context{nullptr};
    std::vector<std::shared_ptr<Renderable>> m_renderJobs{};
    std::mutex m_mtx;
    int m_state = 0;
    bool m_init{false};

};

} // namespace early
} // namespace evs

#endif // RENDERPIPELINE_H