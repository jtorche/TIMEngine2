#include "MultipleSceneHelper.h"

#include "MemoryLoggerOn.h"
MultipleSceneHelper::MultipleSceneHelper(const interface::FullPipeline::Parameter& param, interface::FullPipeline& pipe)
    : _param(param), _pipeline(pipe)
{
    _combineSceneShader = interface::ShaderPool::instance().get("combineScene");
    _nbSceneUniformId = _combineSceneShader->uniformLocation("nbScene");
}

MultipleSceneHelper::~MultipleSceneHelper()
{
    freeCamera();
}

void MultipleSceneHelper::setEnableEdge(bool b, interface::Scene& sceneFrom, interface::MeshInstance* inst)
{
    vector<InternalEdge>& candidat = _graph[&sceneFrom];

    for(InternalEdge& e : candidat)
    {
        if(e.edge.portal == inst)
        {
            e.enabled = b;
        }
    }
}

void MultipleSceneHelper::setCrossableEdge(bool b, interface::Scene& sceneFrom, interface::MeshInstance* inst)
{
    vector<InternalEdge>& candidat = _graph[&sceneFrom];
    for(InternalEdge& e : candidat)
    {
        if(e.edge.portal == inst)
        {
            e.crossable = b;
        }
    }
}

void MultipleSceneHelper::setEdgeDrawDistance(float dist, interface::Scene& sceneFrom, interface::MeshInstance* inst)
{
    vector<InternalEdge>& candidat = _graph[&sceneFrom];
    for(InternalEdge& e : candidat)
    {
        if(e.edge.portal == inst)
        {
            e.drawDistance = dist;
        }
    }
}

void MultipleSceneHelper::setView(interface::View& view)
{
    _curCamera = &view;
}

void MultipleSceneHelper::setCurScene(interface::Scene& scene)
{
    if(_currentScene == &scene)
        return;

    rebuild(scene);
}

void MultipleSceneHelper::setStereoView(interface::View& eye1, interface::View& eye2)
{
    _curStereoCamera[0] = &eye1;
    _curStereoCamera[1] = &eye2;
}

void MultipleSceneHelper::registerDirLightView(interface::Scene* scene, interface::View* dirView)
{
    _dirLightView[scene] = dirView;
}

void MultipleSceneHelper::addEdge(interface::Scene *sceneFrom, interface::Scene *sceneTo,
             interface::MeshInstance* inst, interface::Geometry geom, interface::MeshInstance* dest)
{
    Edge e;
    e.destPortal = dest;
    e.portal = inst;
    e.portalGeom = geom;
    e.sceneFrom = sceneFrom;
    e.sceneTo = sceneTo;
    addEdge(e);
}

void MultipleSceneHelper::addEdge(Edge edge)
{
    if(edge.sceneFrom == nullptr || edge.sceneTo == nullptr || edge.portal == nullptr)
        return;

    if(edge.portalGeom.meshData() == nullptr)
        return;

    InternalEdge internEdge;
    internEdge.edge = edge;
    internEdge.portalBox = Box::computeBox(reinterpret_cast<const real*>(edge.portalGeom.meshData()->vData),
                                           edge.portalGeom.meshData()->nbVertex,
                                           sizeof(renderer::MeshData::DataType)/sizeof(real));

    internEdge.portalPlan = internEdge.portalBox.extractOptimalPlan();


    vector<InternalEdge>& add = _graph[edge.sceneFrom];
    add.push_back(internEdge);

    if(edge.sceneFrom == _currentScene)
    {
        constructEdge(internEdge);
    }

    interface::Mesh mesh = edge.portal->mesh();
    if(mesh.nbElements() > 0)
    {
        mesh.element(0).setCastShadow(false);
        mesh.element(0).setEnable(0);
    }
    edge.portal->setMesh(mesh);

}

void MultipleSceneHelper::rebuild(interface::Scene& scene)
{
    if(_currentScene)
    {
        vector<InternalEdge>& edgeToFlush = _graph[_currentScene];
        for(InternalEdge& e : edgeToFlush)
        {
            interface::Mesh m=e.edge.portal->mesh();
            if(m.nbElements() > 0)
                m.element(0).setEnable(0);
            e.edge.portal->setMesh(m);
        }
    }

    vector<InternalEdge>& add = _graph[&scene];

    _currentScene = &scene;
    _curNbEdge = 0;

    freeCamera();

    for(size_t i=0 ; i<add.size() ; ++i)
    {
        constructEdge(add[i]);
    }
}

void MultipleSceneHelper::extendPipeline(int size)
{
    for(int i=_nbExtraPipeline ; i<size ; ++i)
    {
        _pipeline.extendPipeline(_resolution, _param, i+1);
        _nbExtraPipeline = i+1;
    }

    for(size_t i=_extraCameras.size() ; i<static_cast<size_t>(size) ; ++i)
    {
        _extraCameras.push_back(nullptr);
        _extraStereoCameras[0].push_back(nullptr);
        _extraStereoCameras[1].push_back(nullptr);
    }

    TIM_ASSERT(NB_MAX_PIPELINE >= _nbExtraPipeline);
}

bool MultipleSceneHelper::update(interface::Scene*& sceneCrossed, mat4* offset_in, bool useLastShadowCascadeOptimization)
{
    if(!_curCamera)
        return false;

    bool enterNewScene=false;

    {
        vector<InternalEdge>& curEdges = _graph[_currentScene];

        for(size_t i=0 ; i<curEdges.size() ; ++i)
        {
            if(!curEdges[i].enabled || !curEdges[i].crossable)
                continue;

            Plan transformedPlan = curEdges[i].portalPlan.transformed(curEdges[i].edge.portal->matrix());
            if(transformedPlan.distance(_curCamera->camera.pos) > 0)
                transformedPlan = Plan(transformedPlan.plan() * -1);

            float d1 = transformedPlan.distance(_curCamera->camera.pos);
            float d2 = transformedPlan.distance(_lastCameraPos);

            if(d2 > 0 && d2-d1 < 0.2) // cross the plan
            {
                vec3 pInter = (_lastCameraPos*(-d1) + _curCamera->camera.pos*d2) / (-d1+d2);

                if(curEdges[i].portalBox.inside(curEdges[i].edge.portal->matrix().inverted()*pInter))
                {
                    sceneCrossed = curEdges[i].edge.sceneTo;
                    enterNewScene = true;

                    rebuild(*sceneCrossed);

                    mat4 offset = curEdges[i].edge.destPortal->matrix() * curEdges[i].edge.portal->matrix().inverted();
                    mat4 o_inv = offset.inverted();

                    _curCamera->offset(offset, o_inv);
                    if(offset_in)
                        *offset_in = offset;

                    if(_pipeline.isStereo())
                    {
                        _curStereoCamera[0]->offset(offset, o_inv);
                        _curStereoCamera[1]->offset(offset, o_inv);
                    }

                    interface::View* v = _dirLightView[sceneCrossed];
                    if(v)
                    {
                        _pipeline.setDirLightView(*v, 0);
                        v->dirLightView.camPos = _curCamera->camera.pos;
                    }

                    _pipeline.setScene(*sceneCrossed, 0);

                    break;
                }
            }
        }
    }

    _lastCameraPos = _curCamera->camera.pos;

    vector<InternalEdge>& curEdges = _graph[_currentScene];
    for (InternalEdge& e : curEdges) {
        e.finalDrawDecision = false;
    }

    TIM_ASSERT((int)curEdges.size() == _curNbEdge);

    vector<std::tuple<float, int>> optimizePortalLimit;
    for(int i=0 ; i<_curNbEdge ; ++i)
    {
        TIM_ASSERT(_extraCameras[i] != nullptr);

        if(!curEdges[i].enabled)
            continue;

        if(!optimizeExtraSceneRendering(curEdges[i]))
            continue;

        optimizePortalLimit.push_back(std::make_tuple((curEdges[i].edge.portal->volume().center()-_curCamera->camera.pos).length2(), i));
    }

    if((int)optimizePortalLimit.size() > _portalLimit)
    {
        std::sort(optimizePortalLimit.begin(), optimizePortalLimit.end(),
            [](std::tuple<float, int> const& a, std::tuple<float, int> const& b) {
                return std::get<0>(a) < std::get<0>(b);
        });
        optimizePortalLimit.resize(_portalLimit);
    }

    for (auto d_index : optimizePortalLimit) {
        curEdges[std::get<1>(d_index)].finalDrawDecision = true;
    }
    bool hasEdgeChangedFromPreviousFrame = false;
    for (InternalEdge& e : curEdges) {
        hasEdgeChangedFromPreviousFrame |= (e.previousDrawDecision != e.finalDrawDecision);
        e.previousDrawDecision = e.finalDrawDecision;
    }

    setupDecidedPortals(curEdges);

    for(int i=_curNbEdge ; i<_nbExtraPipeline ; ++i)
    {
        _pipeline.combineNode(0)->setEnableInput(i+2, false);
        if(_pipeline.isStereo())
            _pipeline.combineNode(1)->setEnableInput(i+2, false);
    }

    if (useLastShadowCascadeOptimization) {
        // Optimize shadow maps by not drawing all cascade1 all the time, alternate between current scene and scene view through the portals.
        bool mustRenderAllShadowMapCascades = enterNewScene || hasEdgeChangedFromPreviousFrame; // any new portals or scene change invalidate the content of the shadow maps
        _alternateShadowMapRendering = (_alternateShadowMapRendering + 1) % 2;
        for (int i = 0; i < _curNbEdge + 1; ++i) {
            for (unsigned int j = 0; j < _pipeline.dirLightShadowNode(i).size(); ++j) {
                interface::pipeline::DirLightShadowNode* pShadowMapNode = _pipeline.dirLightShadowNode(i)[j];
                pShadowMapNode->setSkipRenderLastCascadeIfPersistent(mustRenderAllShadowMapCascades || (i == 0 && _alternateShadowMapRendering == 0) || (i > 0 && _alternateShadowMapRendering == 1) ? false : true);
            }
        }
    }

    return enterNewScene;
}

void MultipleSceneHelper::updateCameras()
{
    vector<InternalEdge>& edges = _graph[_currentScene];

    for(size_t i=0 ; i<edges.size() ; ++i)
    {
        if(edges[i].finalDrawDecision)
        {
            //*_extraCameras[i] = *_curCamera;

            mat4 offset = edges[i].edge.destPortal->matrix() * edges[i].edge.portal->matrix().inverted();
            mat4 inv_o = offset.inverted();
            //_extraCameras[i]->offset(offset, inv_o);
            //_extraCameras[i]->dirLightView.camPos = _extraCameras[i]->camera.pos;

            if(_pipeline.isStereo())
            {
                *_extraStereoCameras[0][i] = *_curStereoCamera[0];
                _extraStereoCameras[0][i]->offset(offset, inv_o);

                *_extraStereoCameras[1][i] = *_curStereoCamera[1];
                _extraStereoCameras[1][i]->offset(offset, inv_o);
            }
        }
    }
}

std::pair<interface::Scene*, interface::MeshInstance*> MultipleSceneHelper::closestPortal(const Sphere& sphere, mat4& offset)
{
    const vector<InternalEdge>& curEdges = _graph[_currentScene];

    for(size_t i=0 ; i<curEdges.size() ; ++i)
    {
        if(!curEdges[i].enabled)
            continue;

        Plan transformedPlan = curEdges[i].portalPlan.transformed(curEdges[i].edge.portal->matrix());
        float d = fabsf(transformedPlan.distance(sphere.center()));

        if(d < sphere.radius()) // candidat
        {
            vec3 pInter = transformedPlan.project(sphere.center());

            if(curEdges[i].portalBox.inside(curEdges[i].edge.portal->matrix().inverted()*pInter))
            {
                offset = curEdges[i].edge.destPortal->matrix() * curEdges[i].edge.portal->matrix().inverted();
                return {curEdges[i].edge.sceneTo, curEdges[i].edge.portal};
            }
        }
    }

    return {nullptr, nullptr};
}

std::pair<interface::Scene*, interface::MeshInstance*> MultipleSceneHelper::communicatingPortal(interface::Scene* sc, interface::MeshInstance* portal)
{
    const vector<InternalEdge>& curEdges = _graph[sc];

    for(size_t i=0 ; i<curEdges.size() ; ++i)
    {
       if(curEdges[i].edge.portal == portal)
       {
           return {curEdges[i].edge.sceneTo, curEdges[i].edge.destPortal};
       }
    }

    return {nullptr, nullptr};
}

int MultipleSceneHelper::hasCrossedPortal(vec3 p1, vec3 p2, interface::MeshInstance* portal, float radius)
{
    const vector<InternalEdge>& curEdges = _graph[_currentScene];

    for(size_t i=0 ; i<curEdges.size() ; ++i)
    {
        if(curEdges[i].edge.portal == portal)
        {
            if(!curEdges[i].enabled)
                return -1;

            Plan transformedPlan = curEdges[i].portalPlan.transformed(curEdges[i].edge.portal->matrix());

            float d1 = transformedPlan.distance(p1);
            float d2 = transformedPlan.distance(p2);
            float d = fabsf(d1);

            if(d < radius)
            {
                vec3 pInter = transformedPlan.project(p1);

                if(curEdges[i].portalBox.inside(curEdges[i].edge.portal->matrix().inverted()*pInter))
                {
                   if(d1*d2 <= 0) // crossed
                       return 1;
                   else return 0;
                }
                else
                    return -1;
            }
            else return -1;
        }
    }
    return -1;
}

void MultipleSceneHelper::constructEdge(const InternalEdge& edge)
{
    extendPipeline(_curNbEdge+1);

    _extraCameras[_curNbEdge] = new interface::View;

    if(_pipeline.isStereo())
    {
        _extraStereoCameras[0][_curNbEdge] = new interface::View;
        _extraStereoCameras[1][_curNbEdge] = new interface::View;
        _pipeline.setScene(*edge.edge.sceneTo, _curNbEdge+1);
        interface::View* tmp[2] = {_extraStereoCameras[0][_curNbEdge], _extraStereoCameras[1][_curNbEdge]};
        _pipeline.setStereoView(*_extraCameras[_curNbEdge], *tmp[0], *tmp[1], _curNbEdge+1);
    }
    else
    {
        /* _curNbEdge+1 because we have the main pipline at index 0 */
        _pipeline.setScene(*edge.edge.sceneTo, *_extraCameras[_curNbEdge], _curNbEdge+1);
    }

    _pipeline.setDirLightView(*_extraCameras[_curNbEdge], _curNbEdge+1);

    _curNbEdge++;
}

bool MultipleSceneHelper::optimizeExtraSceneRendering(MultipleSceneHelper::InternalEdge& edge)
{
    Box portalBox = edge.portalBox;
    mat4 portalMatrix = edge.edge.portal->matrix();
    //mat4 invPortalMatrix = portalMatrix.inverted();

    Frustum frust;
    Camera cam = _curCamera->camera;
//    cam.pos = invPortalMatrix * cam.pos;
//    cam.dir = invPortalMatrix * cam.dir;
//    cam.up = invPortalMatrix.down<1>() * cam.up;
    frust.buildCameraFrustum(_curCamera->camera);

    Sphere s(portalBox.toSphere());
    s.transform(portalMatrix);

    return frust.collide(s) && (s.center() - cam.pos).length() < edge.drawDistance;
}

void MultipleSceneHelper::optimizeFrustum(MultipleSceneHelper::InternalEdge& edge, int i)
{
    Box portalBox = edge.portalBox;
    mat4 portalMatrix = edge.edge.portal->matrix();

    mat4 offset = edge.edge.destPortal->matrix() * edge.edge.portal->matrix().inverted();

    Camera cam = _curCamera->camera;
    cam.dir = portalMatrix * portalBox.center();
    cam.pos = offset * cam.pos;
    cam.dir = offset * cam.dir;
    vec3 dirView = (cam.dir-cam.pos).normalized();

    float maxAngle = 0;
    float near = 99999;

    for(int x=0 ; x<2 ; ++x)for(int y=0 ; y<2 ; ++y)for(int z=0 ; z<2 ; ++z)
    {
        vec3 p = {portalBox.box()[0][x], portalBox.box()[1][y], portalBox.box()[2][z]};
        p = (portalMatrix * p);

        p = offset * p;
        float angle = acosf(dirView.dot((p-cam.pos).normalized()));
        maxAngle = std::max(maxAngle, angle);
        near = std::min(near, (p-cam.pos).length());
    }

    near = std::max(0.1f, near);

    if(_curCamera->camera.fov > toDeg(maxAngle)*2)
    {
        cam.clipDist.x() = near;
        cam.fov = toDeg(maxAngle)*2;
        _extraCameras[i]->camera = cam;
    }

    if(_pipeline.isStereo())
    {
        vec2 minV, maxV;
        mat4 projView = _curStereoCamera[0]->camera.raw_proj * _curStereoCamera[0]->camera.raw_view;
        bool useScissor = getScreenBoundingBox(portalBox, portalMatrix, projView, minV, maxV);

        for(auto ptr : _pipeline.deferredRendererNode(i+1, 0))
            ptr->setScissorTest(useScissor, minV, maxV-minV);

        projView = _curStereoCamera[1]->camera.raw_proj * _curStereoCamera[1]->camera.raw_view;
        useScissor = getScreenBoundingBox(portalBox, portalMatrix, projView, minV, maxV);

        for(auto ptr : _pipeline.deferredRendererNode(i+1, 1))
            ptr->setScissorTest(useScissor, minV, maxV-minV);
    }
}

void MultipleSceneHelper::setupDecidedPortals(vector<InternalEdge>& edges)
{
    int pipIndex=0;

    for(size_t i=0 ; i<edges.size() ; ++i)
    {
        // i+2 because the 2 first inputs of the combined node are the "portal stencil" and the main scene result.
        _pipeline.combineNode(0)->setEnableInput(i+2, edges[i].finalDrawDecision);
        if(_pipeline.isStereo())
            _pipeline.combineNode(1)->setEnableInput(i+2, edges[i].finalDrawDecision);

        interface::Mesh mesh = edges[i].edge.portal->mesh();
        if(mesh.nbElements() == 0)
            continue;

        mesh.element(0).setEnable(edges[i].finalDrawDecision);
        mesh.element(0).setCastShadow(false);

        if(edges[i].finalDrawDecision)
        {
            Plan transformedPlan = edges[i].portalPlan.transformed(edges[i].edge.portal->matrix());
            if(transformedPlan.distance(_curCamera->camera.pos) > 0)
                transformedPlan = Plan(transformedPlan.plan() * -1);

            // A special variation of the gbuffer shader is used to use the material parameters as a plan equation.
            mesh.element(0).setMaterial(transformedPlan.plan());
            mesh.element(0).drawState().setCullFace(false);
            mesh.element(0).drawState().setShader(interface::ShaderPool::instance().get("portalShader"));
            // Same here, shader interpet as the index of the scene view through the portal
            mesh.element(0).setTextureScale(0.1 * pipIndex);

            ++pipIndex;

            /* setup cameras */

            *_extraCameras[i] = *_curCamera;

            interface::View* view = _dirLightView[edges[i].edge.sceneTo];
            if(view)
                _extraCameras[i]->dirLightView = view->dirLightView;

            mat4 offset = edges[i].edge.destPortal->matrix() * edges[i].edge.portal->matrix().inverted();
            mat4 inv_o = offset.inverted();
            _extraCameras[i]->offset(offset, inv_o);
            _extraCameras[i]->dirLightView.camPos = _extraCameras[i]->camera.pos;

            if(_pipeline.isStereo())
            {
                *_extraStereoCameras[0][i] = *_curStereoCamera[0];
                _extraStereoCameras[0][i]->offset(offset, inv_o);

                *_extraStereoCameras[1][i] = *_curStereoCamera[1];
                _extraStereoCameras[1][i]->offset(offset, inv_o);
            }

            /* setup cliplan */

            transformedPlan = transformedPlan.transformed(offset);

            for(auto ptr : _pipeline.deferredRendererNode(i+1, 0))
            {
                ptr->setClipPlan(transformedPlan.plan(), 0);
                ptr->setUseClipPlan(true, 0);
            }

            if(_pipeline.isStereo())
            {
                for(auto ptr : _pipeline.deferredRendererNode(i+1, 1))
                {
                    ptr->setClipPlan(transformedPlan.plan(), 0);
                    ptr->setUseClipPlan(true, 0);
                }
            }
        }

        edges[i].edge.portal->setMesh(mesh);
    }
}

bool MultipleSceneHelper::getScreenBoundingBox(const Box& box, const mat4& boxMat, const mat4& projView, vec2& minV, vec2& maxV)
{
    minV = {1,1};
    maxV = {-1,-1};

    bool everyThingIn = true;
    for(int x=0 ; x<2 ; ++x)for(int y=0 ; y<2 ; ++y)for(int z=0 ; z<2 ; ++z)
    {
        vec3 p = {box.box()[0][x], box.box()[1][y], box.box()[2][z]};
        p = (boxMat * p);
        vec4 projectedP = projView * vec4(p,1);

        if(projectedP.w() < 0)
            everyThingIn = false;

        projectedP /= fabs(projectedP.w());
        minV.x() = std::min(projectedP.x(), minV.x());
        minV.y() = std::min(projectedP.y(), minV.y());
        maxV.x() = std::max(projectedP.x(), maxV.x());
        maxV.y() = std::max(projectedP.y(), maxV.y());
    }

    if(everyThingIn)
    {
        minV.x() = std::max(-1.f,minV.x());
        minV.y() = std::max(-1.f,minV.y());
        maxV.x() = std::min( 1.f,maxV.x());
        maxV.y() = std::min( 1.f,maxV.y());

        minV = (minV * 0.5) + vec2(0.5,0.5);
        maxV = (maxV * 0.5) + vec2(0.5,0.5);

        if(minV.x() > maxV.x() || minV.y() > maxV.y())
        {
            minV = {0,0};
            maxV = {1,1};
            return false;
        }

        return true;
   }
   else
   {
       minV = {0,0};
       maxV = {1,1};
       return false;
   }
}

void MultipleSceneHelper::freeCamera()
{
    for(size_t i=0 ; i<_extraCameras.size() ; ++i)
    {
        delete _extraCameras[i];
        delete _extraStereoCameras[0][i];
        delete _extraStereoCameras[1][i];
    }

    _extraCameras.clear();
    _extraStereoCameras[0].clear();
    _extraStereoCameras[1].clear();
}


