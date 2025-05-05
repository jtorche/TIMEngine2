#ifndef MESHRENDERER_H
#define MESHRENDERER_H

#include "GLState.h"
#include "DrawState.h"
#include "GpuBuffer.h"
#include "core/Camera.h"
#include "MeshBuffers.h"
#include "FrameParameter.h"

#include "MemoryLoggerOn.h"
namespace tim
{
    using namespace core;
namespace renderer
{
    class MeshRenderer
    {
    public:
        struct Stats {
            unsigned int _numTriangles = 0;
            unsigned int _numInstances = 0;
            unsigned int _numDrawCalls = 0;
        };

        MeshRenderer();
        ~MeshRenderer();

        void resetStats();
        const Stats& getStats() const { return _stats; }

        void bind() const;
        int draw(const vector<MeshBuffers*>&, const vector<mat4>&, const vector<DummyMaterial>& mat = {},
                 const vector<vector<uint>>& extraUbo = {}, const vector<bool>& useLOD = {}, bool useCameraUbo = true);

        void setDrawState(const DrawState&);

        FrameParameter& frameParameter();
        const FrameParameter& frameParameter() const;

    private:

#if defined(USE_VCPP)
        static const uint _maxUboMat4 = 1024;
#else
        const uint _maxUboMat4;
#endif

        DrawState _states;
        FrameParameter _parameter;
        Stats _stats;

        GenericVertexBuffer<int> _drawIdBuffer;
        VAO* _vao = nullptr;

#ifdef USE_SSBO_MODELS
        ShaderStorageBuffer<mat4> _modelBuffer;
#else
        UniformBuffer<mat4> _modelBuffer;

        UniformBuffer<DummyMaterial> _materialBuffer;
        GpuBuffer<IndirectDrawParmeter, GpuBufferPolicy::MultiDrawBuffer> _drawIndirectBuffer;
#endif
    };

    inline FrameParameter& MeshRenderer::frameParameter() { return _parameter; }
    inline const FrameParameter& MeshRenderer::frameParameter() const  { return _parameter; }

    inline void MeshRenderer::resetStats()
    {
        _stats._numTriangles = 0;
        _stats._numInstances = 0;
        _stats._numDrawCalls = 0;
    }

}
}
#include "MemoryLoggerOff.h"

#endif // FRAMEBUFFER_H
