#ifndef SCENELIGHTINGDIALOG_H
#define SCENELIGHTINGDIALOG_H

#include <QDialog>
#include "core/Vector.h"

namespace Ui {
class SceneLightingDialog;
}

// Sun color and cubemap ambient scales of a scene, every edit is sent through parametersChanged for a live preview
class SceneLightingDialog : public QDialog
{
    Q_OBJECT

public:
    struct Parameters
    {
        tim::core::vec3 sunColor = {1,1,1};
        float ambientDiffuseScale = 1, ambientSpecularScale = 1;
    };

    explicit SceneLightingDialog(const Parameters& parameters, bool hasSun, QWidget* parent = nullptr);
    ~SceneLightingDialog();

    const Parameters& parameters() const { return _parameters; }

signals:
    void parametersChanged(const SceneLightingDialog::Parameters&);

private slots:
    void on_pickSunColor_clicked();

private:
    Ui::SceneLightingDialog *ui;
    Parameters _parameters;

    void setSunColorUi();
};

#endif // SCENELIGHTINGDIALOG_H
