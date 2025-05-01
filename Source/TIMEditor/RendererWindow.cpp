#include "RendererWindow.h"
#include "MainRenderer.h"
#include "RendererThread.h"

#include <QWidget>
#include <QApplication>

const static bool cUseRenderThread = true;

RendererWindow::RendererWindow() : QWindow()
{
	setSurfaceType(QWindow::OpenGLSurface);
	create();

	_renderThread = new RendererThread(this);
	_renderThread->start();

	while (!_renderThread->isInitialized()) {
		qApp->processEvents();
	}

	_renderer = _renderThread->mainRenderer();
}

RendererWindow::~RendererWindow()
{
	_renderThread->stop();
	_renderThread->wait(3000);

	delete _renderThread;
	_renderThread = nullptr;
}