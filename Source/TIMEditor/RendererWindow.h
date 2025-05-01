#ifndef RENDERERWIDGET_H
#define RENDERERWIDGET_H

#include "core/Vector.h"
#include "renderer/renderer.h"

#include <QWindow>
#include <QMouseEvent>

class RendererThread;
class GLViewContainer;

namespace tim{
class MainRenderer;
}

class RendererWindow : public QWindow
{
    Q_OBJECT

public:
    explicit RendererWindow();
    ~RendererWindow();

    tim::MainRenderer* getRenderer() const { return _renderer; }

    virtual void resizeEvent(QResizeEvent* ev) override;

protected:
    RendererThread* _renderThread = nullptr;
    tim::MainRenderer* _renderer = nullptr;
};

#endif // RENDERERWIDGET_H
