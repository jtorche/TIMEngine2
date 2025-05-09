
#include "DeferredRendererNode.h"

#include "MemoryLoggerOn.h"
namespace tim
{
    using namespace core;
    using namespace renderer;
namespace interface
{
namespace pipeline
{

renderer::Texture* DeferredRendererNode::buffer(uint index) const
{
    if(!_rendererEntity) return nullptr;

    if(index == 0)
        return _rendererEntity->lightContext()->buffer();
    else if(index-1 < 4) return _rendererEntity->deferredRenderer().buffer(index-1);
    else return nullptr;
}

void DeferredRendererNode::setScissorTest(bool b, vec2 coord, vec2 size)
{
    _useScissor = b;
    _coordScissor = coord;
    _sizeScissor = size;
}

void DeferredRendererNode::prepare()
{
    if(!tryPrepare()) return;

    _culledLight.clear();
    _toDraw.clear();

    for(size_t i=0 ; i<_meshInstanceSource.size() ; ++i)
    {
        if(!_meshInstanceSource[i]) continue;

        _meshInstanceSource[i]->prepare();
        const auto& culledMesh =_meshInstanceSource[i]->get();

        for(const MeshInstance& m : culledMesh)
        {
            for(uint i=0 ; i<m.mesh().nbElements() ; ++i)
                if(m.mesh().element(i).isEnable() == 2 || (!_isAux && m.mesh().element(i).isEnable() == 1))
                    _toDraw.push_back({&(m.mesh().element(i)), &(m.matrix()), &(m.attachedUBO()), m.useVisualLOD()});
        }
    }

    if(_globalLightInfo)
    {
        for(uint i=0 ; i<std::min(_dirLightDepthMapRenderer.size(),_globalLightInfo->dirLights.size()) ; ++i)
        {
            if(_dirLightDepthMapRenderer[i] && _globalLightInfo->dirLights[i].projectShadow)
                _dirLightDepthMapRenderer[i]->prepare();
        }
    }

    std::sort(_toDraw.begin(), _toDraw.end(), [](const ElementInstance& e1, const ElementInstance& e2)
        { return e1.elem->drawState() < e2.elem->drawState(); });

    for(size_t i=0 ; i<_lightInstanceSource.size() ; ++i)
    {
        if(!_lightInstanceSource[i]) continue;
        _lightInstanceSource[i]->prepare();
        const auto& vec = _lightInstanceSource[i]->get();
        _culledLight.insert(_culledLight.end(), vec.begin(), vec.end());
    }
}

void DeferredRendererNode::render()
{
    if(!tryRender()) return;

    if(_rendererEntity == nullptr)
        return;

    if(_sceneView)
        _meshDrawer.frameParameter().setCamera(_sceneView->camera);

    _rendererEntity->deferredRenderer().acquire();
    _rendererEntity->deferredRenderer().frameBuffer()->bind();

    openGL.clearDepth();
    openGL.clearColor(vec4::construct(0));

    openGL.scissorTest(_useScissor);
    if(_useScissor)
    {
        if(buffer(0))
        {
            uivec2 coord = {static_cast<uint>(buffer(0)->resolution().x() * _coordScissor.x()),
                            static_cast<uint>(buffer(0)->resolution().y() * _coordScissor.y())};
            uivec2 size = {static_cast<uint>(buffer(0)->resolution().x() * _sizeScissor.x()),
                           static_cast<uint>(buffer(0)->resolution().y() * _sizeScissor.y())};

            openGL.scissorParam(coord, size);
        }

    }

    for(int i=0 ; i<NB_CLIP_PLAN ; ++i)
        if(_useClipPlan[i]) glEnable(GL_CLIP_DISTANCE0+i);
        else glDisable(GL_CLIP_DISTANCE0+i);

    std::set<Shader*> alreadyUsed;

    if(!_toDraw.empty())
    {
        vector<mat4> accMatr;
        vector<renderer::MeshBuffers*> accMesh;
        vector<renderer::DummyMaterial> accMate;
        vector<vector<uint>> accExtraUbo;
        vector<bool> accUseIndexBufferLOD;
        renderer::DrawState curDrawState = _toDraw[0].elem->drawState();
        uint curIndex=0;

        for(curIndex=0 ; curIndex < _toDraw.size() ; ++curIndex)
        {
            if(curDrawState != _toDraw[curIndex].elem->drawState())
            {
                _meshDrawer.setDrawState(curDrawState);

                if(curDrawState.shader())
                {
                    if(alreadyUsed.find(curDrawState.shader()) == alreadyUsed.end())
                    {
                        alreadyUsed.insert(curDrawState.shader());
                        curDrawState.shader()->bind();
                        for(int i=0 ; i<NB_CLIP_PLAN ; ++i)
                        {
                            if(_useClipPlan[i])
                                curDrawState.shader()->setUniform(_clipPlan[i],
                                                                  curDrawState.shader()->engineUniformId(Shader::EngineUniform::CLIP_PLAN_0+i));
                        }
                    }
                }

                _meshDrawer.draw(accMesh, accMatr, accMate, accExtraUbo, accUseIndexBufferLOD);
                accMesh.resize(0);
                accMatr.resize(0);
                accMate.resize(0);
                accUseIndexBufferLOD.resize(0);
                accExtraUbo.resize(0);
                curDrawState = _toDraw[curIndex].elem->drawState();
            }

            if(_toDraw[curIndex].elem->geometry().buffers() && !_toDraw[curIndex].elem->geometry().buffers()->isNull())
            {
                accMatr.push_back(_toDraw[curIndex].matrix->transposed());
                accMesh.push_back(_toDraw[curIndex].elem->geometry().buffers());
                accMate.push_back(_toDraw[curIndex].elem->dummyMaterial());
                accExtraUbo.push_back(*(_toDraw[curIndex].extraUbo));
                accUseIndexBufferLOD.push_back(_toDraw[curIndex].useLOD);
            }
        }
        if(!accMesh.empty())
        {
            _meshDrawer.setDrawState(curDrawState);

            if(curDrawState.shader())
            {
                if(alreadyUsed.find(curDrawState.shader()) == alreadyUsed.end())
                {
                    alreadyUsed.insert(curDrawState.shader());
                    curDrawState.shader()->bind();
                    for(int i=0 ; i<NB_CLIP_PLAN ; ++i)
                    {
                        if(_useClipPlan[i])
                            curDrawState.shader()->setUniform(_clipPlan[i],
                                                              curDrawState.shader()->engineUniformId(Shader::EngineUniform::CLIP_PLAN_0+i));
                    }
                }
            }

            _meshDrawer.draw(accMesh, accMatr, accMate, accExtraUbo, accUseIndexBufferLOD);
        }
    }

    for(int i=0 ; i<NB_CLIP_PLAN ; ++i)
        if(_useClipPlan[i])
            glDisable(GL_CLIP_DISTANCE0+i);

    vector<LightContextRenderer::Light> lights(_culledLight.size());
    for(uint i=0 ; i<lights.size() ; ++i)
        lights[i] = _culledLight[i].get().get();

    _rendererEntity->lightContext()->acquire();

    renderer::Texture* globalPSkybox = nullptr;
    if(_globalLightInfo)
        globalPSkybox = _globalLightInfo->skybox.second;

    bool needCompute = true;//!_isAux;
    if(lights.empty())
        needCompute = false;

    if (_rendererEntity->lightRenderer() && needCompute)
        _rendererEntity->lightRenderer()->draw(lights, globalPSkybox);
    else
        _rendererEntity->lightContext()->clear();

    if(_globalLightInfo)
    {
        openGL.scissorTest(false);
        for(uint i=0 ; i<std::min(_dirLightDepthMapRenderer.size(),_globalLightInfo->dirLights.size()) ; ++i)
        {
            if(_dirLightDepthMapRenderer[i] && _globalLightInfo->dirLights[i].projectShadow)
            {
                _dirLightDepthMapRenderer[i]->acquire(0);
                _dirLightDepthMapRenderer[i]->render();
            }
        }
        openGL.scissorTest(_useScissor);

        vector<IndirectLightRenderer::Light> lights(_globalLightInfo->dirLights.size());
        for(uint i=0 ; i<_globalLightInfo->dirLights.size() ; ++i)
        {
            lights[i].direction = _globalLightInfo->dirLights.at(i).direction;
            lights[i].color = vec3(_globalLightInfo->dirLights.at(i).color);
            lights[i].depthMap = nullptr;
            if(_globalLightInfo->dirLights.at(i).projectShadow && i<_dirLightDepthMapRenderer.size() &&
               _dirLightDepthMapRenderer[i])
            {
                lights[i].depthMap = _dirLightDepthMapRenderer[i]->buffer(0);
                lights[i].matrix =    _dirLightDepthMapRenderer[i]->matrix();
            }
        }

//        if(_rendererEntity->envLightRenderer().isLocalReflexionEnabled())
//        {
//            if(!_copyToFBO)
//            {
//                renderer::Texture::GenTexParam param;
//                param.size = uivec3(_rendererEntity->lightContext()->resolution(), 0);
//                param.nbLevels = 1;
//                param.format = _rendererEntity->lightContext()->buffer()->format();
//                _copyBuffer = renderer::Texture::genTexture2D(param);

//                _copyToFBO = new renderer::FrameBuffer(_rendererEntity->lightContext()->resolution());
//                _copyToFBO->attachTexture(0, _copyBuffer);
//            }

//            openGL.scissorTest(false);
//            _rendererEntity->lightContext()->frameBuffer().copyTo(*_copyToFBO);
//            openGL.scissorTest(_useScissor);

//            _rendererEntity->envLightRenderer().setReflexionBuffer(_copyBuffer);
//        }

        _rendererEntity->envLightRenderer().setEnableGI(!needCompute);
        _rendererEntity->envLightRenderer().setSkybox(_globalLightInfo->skybox.first, _globalLightInfo->skybox.second);
        _rendererEntity->envLightRenderer().draw(lights);

        for(uint i=0 ; i<std::min(_dirLightDepthMapRenderer.size(),_globalLightInfo->dirLights.size()) ; ++i)
            if(_dirLightDepthMapRenderer[i] && _globalLightInfo->dirLights[i].projectShadow)
                _dirLightDepthMapRenderer[i]->release(0);
    }

    if(_rendererEntity->reflexionRenderer())
        _rendererEntity->reflexionRenderer()->draw();

    _rendererEntity->lightContext()->release();
    _rendererEntity->deferredRenderer().release();

    openGL.scissorTest(false);
}

void DeferredRendererNode::acquire(int index)
{
    if(_rendererEntity)
    {
        if(index == 0)
            _rendererEntity->lightContext()->acquire();
        else if(index > 0)
            _rendererEntity->deferredRenderer().acquire();
    }

    for(uint i=0 ; i<std::min(_dirLightDepthMapRenderer.size(),_globalLightInfo->dirLights.size()) ; ++i)
        if(_dirLightDepthMapRenderer[i] && _globalLightInfo->dirLights[i].projectShadow)
            _dirLightDepthMapRenderer[i]->acquire(0);
}

void DeferredRendererNode::release(int index)
{
    if(_rendererEntity)
    {
        if(index == 0)
            _rendererEntity->lightContext()->release();
        else if(index > 0)
            _rendererEntity->deferredRenderer().release();
    }

    for(uint i=0 ; i<std::min(_dirLightDepthMapRenderer.size(),_globalLightInfo->dirLights.size()) ; ++i)
        if(_dirLightDepthMapRenderer[i] && _globalLightInfo->dirLights[i].projectShadow)
            _dirLightDepthMapRenderer[i]->release(0);
}

void DeferredRendererNode::setRendererEntity(Pipeline::DeferredRendererEntity& entity)
{
    _rendererEntity = &entity;
}

}
}
}
