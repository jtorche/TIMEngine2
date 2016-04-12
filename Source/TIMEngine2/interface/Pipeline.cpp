#include "Pipeline.h"

#include "MemoryLoggerOn.h"
namespace tim
{
    using namespace core;
namespace interface
{

Pipeline::DeferredRendererEntity& Pipeline::genDeferredRendererEntity(const uivec2& res)
{
    auto it = _deferredRendererEntity.find(res);
    if(it != _deferredRendererEntity.end())
        return *(it->second.get());
    else
    {
        DeferredRendererEntity* entity = new DeferredRendererEntity(res, _meshRenderer.frameState());
        _deferredRendererEntity[res] = std::unique_ptr<DeferredRendererEntity>(entity);
        return *entity;
    }
}

Pipeline::Pipeline()
{
    _meshRenderer.bind();
}

Pipeline::~Pipeline()
{
    for(uint i=0 ; i<_allProcessNodes.size() ; ++i)
    {
        delete _allProcessNodes[i];
    }
}

renderer::MeshRenderer& Pipeline::meshRenderer()
{
    return _meshRenderer;
}

void Pipeline::setOutputNode(Pipeline::TerminalNode& node)
{
    _outputNode = &node;
}

void Pipeline::prepare()
{
    for(uint i=0 ; i<_allProcessNodes.size() ; ++i)
        _allProcessNodes[i]->reset();

    if(_outputNode)
        _outputNode->prepare();
}

void Pipeline::render()
{
    if(_outputNode)
        _outputNode->render();

    renderer::openGL.execAllGLTask();
}

}
}