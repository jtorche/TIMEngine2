
#include "DirLightShadowNode.h"

#include "MemoryLoggerOn.h"
namespace tim
{
    using namespace core;
namespace interface
{
namespace pipeline
{

DirLightShadowNode::DirLightShadowNode(renderer::MeshRenderer& meshDrawer)
    : Pipeline::DepthMapRendererNode(), _meshDrawer(meshDrawer), _buffer(renderer::texBufferPool)
{
    _defaultDrawState.setShader(renderer::depthPassShader);
    _defaultDrawState.setCullBackFace(true);

    const float arr[3] = {50,150,500};
    for(uint i=0 ; i<3 ; ++i)
    {
        _sizeOrtho[i] = vec3(arr[i], arr[i], 1000);
        _orthoMatrix[i] = mat4::Ortho(-_sizeOrtho[i].x(), _sizeOrtho[i].x(),
                                      -_sizeOrtho[i].y(), _sizeOrtho[i].y(),
                                      -_sizeOrtho[i].z(), _sizeOrtho[i].z());
    }
}

void DirLightShadowNode::acquire(int)
{
    if(_needUpdate)
    {
        _matrix.resize(_resolution.z());
        _needUpdate = false;

        renderer::TextureBufferPool::Key k;
        k.type = renderer::TextureBufferPool::Key::DEPTH_MAP_ARRAY;
        k.res = _resolution;
        _buffer.setParameter(k);
    }

    _buffer.acquire();
}

void DirLightShadowNode::release(int)
{
    _previousShadowMapRendered = _buffer.buffer(0);
    _buffer.release();
}

void DirLightShadowNode::setShadowLightRange(const vector<float>& r)
{
    for(uint i=0 ; i<std::min<uint>(renderer::MAX_SHADOW_MAP_LVL, (uint)r.size()) ; ++i)
    {
        _sizeOrtho[i] = vec3(r[i], r[i], 1000);
        _orthoMatrix[i] = mat4::Ortho(-_sizeOrtho[i].x(), _sizeOrtho[i].x(),
                                      -_sizeOrtho[i].y(), _sizeOrtho[i].y(),
                                      -_sizeOrtho[i].z(), _sizeOrtho[i].z());
    }

    if(_resolution.z() != std::min<uint>(renderer::MAX_SHADOW_MAP_LVL, (uint)r.size()))
    {
        _needUpdate = true;
        _resolution.z() = std::min<uint>(renderer::MAX_SHADOW_MAP_LVL, (uint)r.size());
    }
}

void DirLightShadowNode::setDepthMapResolution(uint res)
{
    if(_resolution.x() != res || _resolution.y() != res)
    {
        _resolution = {res,res,_resolution.z()};
        _needUpdate = true;
    }
}
void DirLightShadowNode::setSkipRenderLastCascadeIfPersistent(bool skip)
{
    _skipRenderLastCascadeIfPersistent = skip;
}


DirLightShadowNode::~DirLightShadowNode()
{

}

renderer::Texture* DirLightShadowNode::buffer(uint index) const
{
    return _buffer.buffer(index);
}

void DirLightShadowNode::prepare()
{
    if(!tryPrepare()) return;

    if(!_sceneView)
        return;


    for (uint i = 0; i < renderer::MAX_SHADOW_MAP_LVL; ++i) {
        _toDraw[i].clear();
        _useShadowLOD[i].clear();
    }

    for(size_t i=0 ; i<_meshInstanceSource.size() ; ++i)
    {
        if(!_meshInstanceSource[i]) continue;
        _meshInstanceSource[i]->prepare();

        for(uint j=0 ; j<_resolution.z() ; ++j)
        {
            const auto& culledMesh =_meshInstanceSource[i]->get(j);

            for(const MeshInstance& m : culledMesh)
            {
                for (uint i = 0; i < m.mesh().nbElements(); ++i) {
                    if (m.mesh().element(i).isEnable() && m.mesh().element(i).castShadow()) {
                        _toDraw[j].push_back({ &(m.mesh().element(i)), &(m.matrix()) });
                        _useShadowLOD[j].push_back(m.useShadowLOD());
                    }
                }
            }
        }
    }
}

void DirLightShadowNode::render()
{
    if(!tryRender()) return;

    if(!_sceneView)
        return;

    renderer::openGL.polygoneOffset(2.0, 2.0); // Z bias

    _counter++;

    for(size_t i=0 ; i<_resolution.z() ; ++i)
    {
        if (_skipRenderLastCascadeIfPersistent && i == (_resolution.z() - 1) && _previousShadowMapRendered == _buffer.buffer(0))
            continue; // skip rendering, the content of the last cascade can be used this frame

        _buffer.fbo()->attachDepthTexture(_buffer.buffer(0), i);
        _buffer.fbo()->bind();
        renderer::openGL.clearDepth();

        if(!_toDraw[i].empty())
        {
            vector<mat4> accMatr;
            vector<renderer::MeshBuffers*> accMesh;
            vector<bool> accUseIndexBufferLOD;

            for(uint index=0 ; index < _toDraw[i].size() ; ++index)
            {
                if(_toDraw[i][index].first->geometry().buffers() && !_toDraw[i][index].first->geometry().buffers()->isNull())
                {
                    renderer::MeshBuffers* pMeshBuffers = _toDraw[i][index].first->geometry().buffers();

                    if (pMeshBuffers->hasSecondaryIndexBuffer()) {
                        // Slightly shift the LOD to avoid self shadowing issue, this value should be mesh dependent
                        accMatr.push_back(_toDraw[i][index].second->translated(_sceneView->dirLightView.lightDir * 0.02f).transposed());
                    } else {
                        accMatr.push_back(_toDraw[i][index].second->transposed());
                    }

                    accMesh.push_back(pMeshBuffers);
                    accUseIndexBufferLOD.push_back(_useShadowLOD[i][index]);
                }
            }
            if(!accMesh.empty())
            {
                _meshDrawer.setDrawState(_defaultDrawState);

                mat4 viewMat = mat4::View(_sceneView->dirLightView.realPos[i], _sceneView->dirLightView.realPos[i] + _sceneView->dirLightView.lightDir,
                                          _sceneView->dirLightView.up);

                mat4 projView = _orthoMatrix[i] * viewMat;
                _matrix[i] = mat4::BIAS() * projView;
                _defaultDrawState.shader()->bind();
                _defaultDrawState.shader()->setUniform(projView,
                                                       _defaultDrawState.shader()->engineUniformId(renderer::Shader::PROJVIEW));

                _meshDrawer.draw(accMesh, accMatr, {}, {}, accUseIndexBufferLOD, false);
            }
        }

        _buffer.fbo()->unbind();
    }
    
    renderer::openGL.polygoneOffset(0.0, 0.0); // restore default
}

}
}
}
