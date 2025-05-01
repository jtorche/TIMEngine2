#ifndef RENDERERTHREAD_H
#define RENDERERTHREAD_H

#include "core/core.h"
#include "renderer/renderer.h"
#include "resource/TextureLoader.h"
#include "MainRenderer.h"
#include "RendererWindow.h"

#include <QThread>
#include <QMutex>

class RendererThread : public QThread {
    Q_OBJECT
public:
    explicit RendererThread(RendererWindow* rendererWidget);
    bool isInitialized() const;

    void stop() { _running = false; }

    tim::MainRenderer* mainRenderer() { return _main; }

protected:
    virtual void run();

private:
    RendererWindow* _rendererWindow;
    QOpenGLContext* _glContext;
    bool _init;
    bool _running;

    tim::MainRenderer* _main = nullptr;

    void initContext();

};

#endif // RENDERERTHREAD_H
