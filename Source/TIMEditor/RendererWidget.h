#ifndef RENDERERWIDGET_H
#define RENDERERWIDGET_H

#include <QGLWidget>
#include <QMouseEvent>
#include <QMessageBox>

namespace tim{
class MainRenderer;
}

class RendererWidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit RendererWidget(QWidget *parent = 0);

    void setMainRenderer(tim::MainRenderer* mr) { _renderer = mr; }

protected:
    tim::MainRenderer* _renderer = nullptr;

    virtual void glInit() override {}
    virtual void glDraw() override {}

    virtual void resizeGL(int, int) override {}
    virtual void resizeEvent(QResizeEvent *) override;

    virtual void initializeGL() override {}
    virtual void paintGL() override {}
    virtual void paintEvent(QPaintEvent *) override {}

    virtual void closeEvent(QCloseEvent *event) override;

    /*virtual void mousePressEvent(QMouseEvent * event) override {

    }*/
    /*virtual void mouseReleaseEvent() override;
    virtual void mouseDoubleClickEvent() override;
    virtual void mouseMoveEvent() override; */

signals:

public slots:
    void closeContext();
};

#endif // RENDERERWIDGET_H
