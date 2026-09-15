#include "SceneLightingDialog.h"
#include "ui_SceneLightingDialog.h"
#include <QColorDialog>
#include <algorithm>

using namespace tim::core;

SceneLightingDialog::SceneLightingDialog(const Parameters& parameters, bool hasSun, QWidget* parent) :
    QDialog(parent),
    ui(new Ui::SceneLightingDialog),
    _parameters(parameters)
{
    ui->setupUi(this);

    // Spin boxes round to their decimals: a value is only taken from a spin box once the user edits it,
    // so untouched parameters keep their exact value
    setSunColorUi();
    ui->ambientDiffuse->setValue(_parameters.ambientDiffuseScale);
    ui->ambientSpecular->setValue(_parameters.ambientSpecularScale);

    QDoubleSpinBox* sunColor[3] = {ui->sunR, ui->sunG, ui->sunB};
    for(int i=0 ; i<3 ; ++i)
    {
        connect(sunColor[i], &QDoubleSpinBox::valueChanged, this, [this, i](double value){
            _parameters.sunColor[i] = static_cast<float>(value);
            setSunColorUi();
            emit parametersChanged(_parameters);
        });
    }

    connect(ui->ambientDiffuse, &QDoubleSpinBox::valueChanged, this, [this](double value){
        _parameters.ambientDiffuseScale = static_cast<float>(value);
        emit parametersChanged(_parameters);
    });
    connect(ui->ambientSpecular, &QDoubleSpinBox::valueChanged, this, [this](double value){
        _parameters.ambientSpecularScale = static_cast<float>(value);
        emit parametersChanged(_parameters);
    });

    ui->sunColorLabel->setEnabled(hasSun);
    ui->sunR->setEnabled(hasSun);
    ui->sunG->setEnabled(hasSun);
    ui->sunB->setEnabled(hasSun);
    ui->pickSunColor->setEnabled(hasSun);
}

SceneLightingDialog::~SceneLightingDialog()
{
    delete ui;
}

void SceneLightingDialog::on_pickSunColor_clicked()
{
    // The picker is limited to [0,1], a brighter sun keeps its intensity
    vec3 color = _parameters.sunColor;
    float intensity = std::max({color.x(), color.y(), color.z(), 1.f});
    color /= intensity;

    QColor picked = QColorDialog::getColor(QColor::fromRgbF(color.x(), color.y(), color.z()), this, "Sun color");
    if(!picked.isValid())
        return;

    _parameters.sunColor = vec3(picked.redF(), picked.greenF(), picked.blueF()) * intensity;
    setSunColorUi();
    emit parametersChanged(_parameters);
}

void SceneLightingDialog::setSunColorUi()
{
    QDoubleSpinBox* sunColor[3] = {ui->sunR, ui->sunG, ui->sunB};
    for(int i=0 ; i<3 ; ++i)
    {
        if(float(sunColor[i]->value()) == _parameters.sunColor[i])
            continue;
        sunColor[i]->blockSignals(true);
        sunColor[i]->setValue(_parameters.sunColor[i]);
        sunColor[i]->blockSignals(false);
    }

    vec3 color = _parameters.sunColor;
    color /= std::max({color.x(), color.y(), color.z(), 1.f});
    QColor swatch = QColor::fromRgbF(std::clamp(color.x(), 0.f, 1.f), std::clamp(color.y(), 0.f, 1.f), std::clamp(color.z(), 0.f, 1.f));
    ui->pickSunColor->setStyleSheet(QString("background-color: %1;").arg(swatch.name()));
}
