#ifndef FrameBufferRenderer_NODE_H
#define FrameBufferRenderer_NODE_H

#include "interface/Pipeline.h"

#include "MemoryLoggerOn.h"
namespace tim
{
    using namespace core;
namespace interface
{
namespace pipeline
{
    class FrameBufferRenderer : public Pipeline::TerminalNode
    {
    public:
        FrameBufferRenderer(uint fbo = 0);
        ~FrameBufferRenderer() = default;

        void prepare() override;
        void render() override;

        void setTargetFrameBuffer(uint fbo) { _fbo = fbo; }

    private:
        renderer::DrawState _stateDrawQuad;
        uint _fbo;

    };
}
}
}
#include "MemoryLoggerOff.h"

#endif // FrameBufferRenderer_NODE_H
