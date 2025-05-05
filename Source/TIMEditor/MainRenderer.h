#ifndef MAINRENDERER_H
#define MAINRENDERER_H

#include "interface/FullPipeline.h"
#include "interface/ShaderPool.h"
#include "resource/MeshLoader.h"
#include "resource/AssetManager.h"
#include "interface/pipeline/pipeline.h"

#include <QMutex>
#include <QQueue>
#include <functional>
#include <QWaitCondition>
#include <QSurface>

#undef interface
#include "MemoryLoggerOn.h"
namespace tim{
class MainRenderer
{
public:
    MainRenderer(uivec2 res, QOpenGLContext*, bool useRenderThread);
    ~MainRenderer();

    void initRendering();
    void update(uint targetFbo);
    void close();

    float elapsedTime() const { return _time; }
    unsigned int getNumTriangleRendered() const { return _numTrianglesRendered; }
    unsigned int getNumDrawcalls() const { return _numDrawcalls; }

    interface::FullPipeline& pipeline() { return _pipeline; }
    void lock() const { if (m_useRenderThread) _mutex.lock(); }
    void unlock() const { if (m_useRenderThread) _mutex.unlock(); }
    void waitNoEvent();

    void updateSize(uivec2);

    void updateCamera_MeshEditor(int wheel);
    void updateCamera_SceneEditor(int wheel);

    tim::interface::View& getSceneView(int index) { return _view[index]; }

    tim::interface::Scene& getScene(int index) { return _scene[index]; }

    void setupScene(int, tim::interface::View&);

    int getCurSceneIndex() const { return _curScene; }
    void setCurSceneIndex(int index) { _curScene = index; }
    void setMoveDirection(int dir, bool v) { _moveDirection[dir] = v; }
    void setEnableDirection(bool m) { _enableMove = m; }
    void setSpeedBoost(bool b) { _speedBoost = b; }

    void addEvent(std::function<void()>);

    void setSkybox(int sceneIndex, QList<QString>);
    void setDirectionalLight(uint sceneIndex, const tim::interface::Pipeline::DirectionalLight&);

    const tim::interface::Mesh& lineMesh(uint index) const { return _lineMesh[index]; }
    const tim::interface::Mesh& specProbePreviewMesh() const { return _specProbeMesh; }

    renderer::Texture* renderCubemap(vec3 pos, uint resolution, uint sceneId, int mode=0, float farDist = 1000);
    void exportSkybox(renderer::Texture*, std::string filepath);

private:
    QOpenGLContext* _glContext;
    const bool m_useRenderThread;
    float _time = 0;
    float _totalTime = 0;

    bool _newSize=false;
    uivec2 _currentSize;

    interface::FullPipeline::Parameter _renderingParameter;
    interface::FullPipeline _pipeline;
    interface::pipeline::FrameBufferRenderer* _frameBufferRenderer = nullptr;
    mutable QRecursiveMutex _mutex;

    /* Shared state */
    static const int NB_SCENE=5;
    tim::interface::View _view[NB_SCENE];
    tim::interface::View _dirLightView[NB_SCENE];
    tim::interface::Scene _scene[NB_SCENE];

    int _curScene=0;
    bool _enableMove = false;
    bool _moveDirection[4] = {false};
    bool _speedBoost = false;

    mutable QMutex _eventMutex;
    QQueue<std::function<void()>> _events;
    QWaitCondition _waitNoEvent;

    unsigned int _numTrianglesRendered = 0;
    unsigned int _numDrawcalls = 0;

    /* gui elements */
    tim::interface::Mesh _lineMesh[3];
    tim::interface::Mesh _specProbeMesh;

    void resize();
    void updateCamera_SceneEditor();
};

}
#include "MemoryLoggerOff.h"

#endif // MAINRENDERER_H
