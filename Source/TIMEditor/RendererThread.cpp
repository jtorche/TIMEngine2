#include "RendererThread.h"
#include "QtTextureLoader.h"
#include <QOpenglContext.h>

using namespace tim;

RendererThread::RendererThread(RendererWindow* rendererWindow) : _rendererWindow(rendererWindow), _init(false), _running(false) {

    _glContext = new QOpenGLContext();
    
    QSurfaceFormat format = rendererWindow->format();
    format.setProfile(QSurfaceFormat::CoreProfile);
#if defined(TIM_DEBUG)
    format.setOption(QSurfaceFormat::DebugContext);
#endif
    _glContext->setFormat(format);
    TIM_ASSERT(_glContext->create());

    // _glContext = _rendererWindow->context();
    _glContext->doneCurrent();
    _glContext->moveToThread(this);
}

bool RendererThread::isInitialized() const {
    return _init;
}

void RendererThread::run()
{
    _running = true;
    initContext();

    while (_running) {
        _main->update(0);
        _glContext->swapBuffers(_rendererWindow);
    }

    _main->close();
    delete _main;
}

void RendererThread::initContext()
{
    using namespace tim;
    TIM_ASSERT(_glContext->makeCurrent(_rendererWindow));

	_main = new MainRenderer({ (uint)_rendererWindow->width(), (uint)_rendererWindow->height() }, _glContext, true);
    _main->initRendering();
    _init = true;
}
