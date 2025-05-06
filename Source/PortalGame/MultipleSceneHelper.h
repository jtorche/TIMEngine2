#ifndef MULTIPLESCENEHELPER_H
#define MULTIPLESCENEHELPER_H

#include "interface/FullPipeline.h"
#include "interface/MeshInstance.h"
using namespace tim;


#define NB_MAX_PIPELINE 30

class MultipleSceneHelper
{
public:
    struct Edge
    {
        interface::Geometry portalGeom;
        interface::MeshInstance *portal;
        interface::Scene *sceneFrom, *sceneTo;
        interface::MeshInstance* destPortal;
    };

    MultipleSceneHelper(const interface::FullPipeline::Parameter&, interface::FullPipeline&);
    ~MultipleSceneHelper();

    interface::FullPipeline& pipeline() { return _pipeline; }
    const interface::FullPipeline& pipeline() const { return _pipeline; }

    interface::Scene* curScene() const { return _currentScene; }

    void setResolution(uivec2 res) { _resolution = res; }

    void setCurScene(interface::Scene&);
    void setView(interface::View&);
    void setStereoView(interface::View&, interface::View&);
    void registerDirLightView(interface::Scene*, interface::View*);
    void addEdge(Edge);
    void addEdge(interface::Scene *sceneFrom, interface::Scene *sceneTo,
                 interface::MeshInstance*, interface::Geometry, interface::MeshInstance* dest = nullptr);

    bool update(interface::Scene*&, mat4* offset, bool useLastShadowCascadeOptimization);
    void rebuild(interface::Scene&);
    void extendPipeline(int);
    void updateCameras();

    std::pair<interface::Scene*, interface::MeshInstance*> closestPortal(const Sphere&, mat4&);
    int hasCrossedPortal(vec3, vec3, interface::MeshInstance*, float radius);
    std::pair<interface::Scene*, interface::MeshInstance*> communicatingPortal(interface::Scene*, interface::MeshInstance*);

    void setEnableEdge(bool b, interface::Scene&, interface::MeshInstance*);
    void setCrossableEdge(bool b, interface::Scene&, interface::MeshInstance*);
    void setEdgeDrawDistance(float, interface::Scene&, interface::MeshInstance*);
    void setPortalLimit(int l) { _portalLimit = l; }

private:
    struct InternalEdge
    {
        Edge edge;

        Plan portalPlan;
        Box portalBox;
        bool enabled = true;
        bool crossable = true;
        float drawDistance = 20;
        bool finalDrawDecision;
        bool previousDrawDecision = false;
    };

    uivec2 _resolution;
    interface::FullPipeline::Parameter _param;
    interface::FullPipeline& _pipeline;

    int _portalLimit = 2;
    int _nbExtraPipeline = 0;
    int _curNbEdge = 0;
    int _alternateShadowMapRendering = 0;

    renderer::Shader* _combineSceneShader;
    int _nbSceneUniformId = -1;

    interface::Scene* _currentScene = nullptr;
    interface::View* _curCamera = nullptr;
    interface::View* _curStereoCamera[2] = {nullptr, nullptr};
    vec3 _lastCameraPos;
    std::map<interface::Scene*, vector<InternalEdge>> _graph;
    vector<interface::View*> _extraCameras;
    vector<interface::View*> _extraStereoCameras[2];

    std::map<interface::Scene*, interface::View*> _dirLightView;

    void freeCamera();
    void constructEdge(const InternalEdge&);
    bool optimizeExtraSceneRendering(InternalEdge&);
    void optimizeFrustum(InternalEdge&, int i);

    void setupDecidedPortals(vector<InternalEdge>&);

    bool getScreenBoundingBox(const Box&, const mat4& boxMat, const mat4& projView, vec2& minV, vec2& maxV);

};

#endif // MULTIPLESCENEHELPER_H
