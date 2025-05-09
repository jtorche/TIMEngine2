
#include "FrameBufferRenderer.h"

#include "MemoryLoggerOn.h"
namespace tim
{
    using namespace core;
namespace interface
{
namespace pipeline
{

FrameBufferRenderer::FrameBufferRenderer(uint fbo) : Pipeline::TerminalNode()
{
    _stateDrawQuad.setCullFace(false);
    _stateDrawQuad.setDepthTest(false);
    _stateDrawQuad.setWriteDepth(false);
    _stateDrawQuad.setShader(renderer::drawQuadShader);
    _fbo = fbo;
}

void FrameBufferRenderer::prepare()
{
    if(!tryPrepare()) return;

    for(uint i=0 ; i<_input.size() ; ++i)
    {
        if(_input[i])
            _input[i]->prepare();
    }
}

void FrameBufferRenderer::render()
{
    if(!tryRender()) return;

    for(uint i=0 ; i<_input.size() ; ++i)
    {
        if(_input[i])
        {
            _input[i]->acquire(0);
            _input[i]->render();
        }
    }

    renderer::openGL.bindFrameBuffer(_fbo);

    _stateDrawQuad.bind();

    for(uint i=0 ; i<_input.size() ; ++i)
    {
        renderer::openGL.bindTextureSampler(renderer::textureSampler[renderer::TextureMode::NoFilter], i);
        renderer::openGL.bindTexture(_input[i]->buffer()->id(), GL_TEXTURE_2D, i);
    }

    renderer::quadMeshBuffers->draw(6, renderer::VertexMode::TRIANGLES, 1);

    renderer::openGL.finish();

    for(uint i=0 ; i<_input.size() ; ++i)
    {
        if(_input[i])
            _input[i]->release(0);
    }
}

}
}
}
