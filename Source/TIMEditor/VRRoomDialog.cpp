#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QFileInfo>
#include <QDir>

#include "VRRoomDialog.h"

namespace
{
    QDoubleSpinBox* meterSpinBox(QWidget* parent, double min, double max, double step, int decimals = 2)
    {
        QDoubleSpinBox* box = new QDoubleSpinBox(parent);
        box->setRange(min, max);
        box->setSingleStep(step);
        box->setDecimals(decimals);
        box->setSuffix(" m");
        return box;
    }
}

VRRoomDialog::VRRoomDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Generate VR rooms");
    setMinimumWidth(560);

    _sceneFolder = new QLineEdit(this);
    QPushButton* browseFolder = new QPushButton("...", this);
    browseFolder->setFixedWidth(30);
    QHBoxLayout* folderLayout = new QHBoxLayout;
    folderLayout->addWidget(_sceneFolder);
    folderLayout->addWidget(browseFolder);

    _sceneInfo = new QLabel(this);
    _sceneInfo->setWordWrap(true);

    _roomSize = meterSpinBox(this, 0.5, 50, 0.5);
    _margin = meterSpinBox(this, 0, 50, 0.1);
    _zTolerance = meterSpinBox(this, 0, 50, 0.1);
    _thickness = meterSpinBox(this, 0.001, 10, 0.01, 3);
    _ignoreBounds = new QCheckBox("Cross every portal of a scene, even outside the room bounds", this);
    _writeFiles = new QCheckBox("Write the vr_room_# objects into the scene files (previous ones are replaced)", this);

    _assetFile = new QLineEdit(this);
    QPushButton* browseAsset = new QPushButton("...", this);
    browseAsset->setFixedWidth(30);
    QHBoxLayout* assetLayout = new QHBoxLayout;
    assetLayout->addWidget(_assetFile);
    assetLayout->addWidget(browseAsset);

    QFormLayout* form = new QFormLayout;
    form->addRow("Scene folder", folderLayout);
    form->addRow("", _sceneInfo);
    form->addRow("Room size (N x N)", _roomSize);
    form->addRow("Portal reach margin", _margin);
    form->addRow("Vertical tolerance", _zTolerance);
    form->addRow("", _ignoreBounds);
    form->addRow("", _writeFiles);
    form->addRow("vr_room asset", assetLayout);
    form->addRow("Ground slab thickness", _thickness);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText("Generate");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &VRRoomDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &VRRoomDialog::reject);
    connect(browseFolder, &QPushButton::clicked, this, &VRRoomDialog::browseSceneFolder);
    connect(browseAsset, &QPushButton::clicked, this, &VRRoomDialog::browseAssetFile);
    connect(_sceneFolder, &QLineEdit::textChanged, this, &VRRoomDialog::refreshSceneInfo);
    connect(_writeFiles, &QCheckBox::toggled, _assetFile, &QLineEdit::setEnabled);
    connect(_writeFiles, &QCheckBox::toggled, browseAsset, &QPushButton::setEnabled);

    loadSettings();
    refreshSceneInfo();
}

VRRoomDialog::Config VRRoomDialog::getConfig() const
{
    Config config;
    config.graph.sceneFolder = _sceneFolder->text();
    config.graph.roomSize = float(_roomSize->value());
    config.graph.margin = float(_margin->value());
    config.graph.zTolerance = float(_zTolerance->value());
    config.graph.ignoreRoomBounds = _ignoreBounds->isChecked();
    config.writer.assetFile = _assetFile->text();
    config.writer.roomSize = config.graph.roomSize;
    config.writer.thickness = float(_thickness->value());
    config.writeFiles = _writeFiles->isChecked();
    return config;
}

void VRRoomDialog::loadSettings()
{
    QSettings settings("TIMEngine2", "TIMEditor");
    settings.beginGroup("VRRoom");

    QString folder = settings.value("sceneFolder").toString();
    if(folder.isEmpty() || !QDir(folder).exists())
        folder = QDir("scene").exists() ? QDir("scene").absolutePath() : QDir::currentPath();
    _sceneFolder->setText(folder);

    _assetFile->setText(settings.value("assetFile", "shape/vr_room.xml").toString());
    _roomSize->setValue(settings.value("roomSize", 3.0).toDouble());
    _margin->setValue(settings.value("margin", 0.5).toDouble());
    _zTolerance->setValue(settings.value("zTolerance", 1.0).toDouble());
    _thickness->setValue(settings.value("thickness", 0.05).toDouble());
    _ignoreBounds->setChecked(settings.value("ignoreBounds", false).toBool());
    _writeFiles->setChecked(settings.value("writeFiles", true).toBool());
    settings.endGroup();
}

void VRRoomDialog::saveSettings()
{
    QSettings settings("TIMEngine2", "TIMEditor");
    settings.beginGroup("VRRoom");
    settings.setValue("sceneFolder", _sceneFolder->text());
    settings.setValue("assetFile", _assetFile->text());
    settings.setValue("roomSize", _roomSize->value());
    settings.setValue("margin", _margin->value());
    settings.setValue("zTolerance", _zTolerance->value());
    settings.setValue("thickness", _thickness->value());
    settings.setValue("ignoreBounds", _ignoreBounds->isChecked());
    settings.setValue("writeFiles", _writeFiles->isChecked());
    settings.endGroup();
}

void VRRoomDialog::refreshSceneInfo()
{
    QStringList scenes, files;
    QString error;
    if(VRRoomGraph::readConfigScene(_sceneFolder->text(), scenes, files, error) && !scenes.isEmpty())
        _sceneInfo->setText("configScene.txt: " + QString::number(scenes.size()) + " scenes, root = " + scenes.first() + "\n" + scenes.join(", "));
    else
        _sceneInfo->setText(error.isEmpty() ? "configScene.txt lists no scene" : error);
}

void VRRoomDialog::accept()
{
    QStringList scenes, files;
    QString error;
    if(!VRRoomGraph::readConfigScene(_sceneFolder->text(), scenes, files, error) || scenes.isEmpty())
    {
        QMessageBox::warning(this, "Generate VR rooms", error.isEmpty() ? "configScene.txt lists no scene" : error);
        return;
    }
    if(_writeFiles->isChecked() && !QFileInfo::exists(_assetFile->text()))
    {
        QMessageBox::warning(this, "Generate VR rooms", "The vr_room asset file " + _assetFile->text() + " doesn't exist.");
        return;
    }

    saveSettings();
    QDialog::accept();
}

void VRRoomDialog::browseSceneFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Scene folder (with configScene.txt)", _sceneFolder->text(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(!dir.isEmpty())
        _sceneFolder->setText(dir);
}

void VRRoomDialog::browseAssetFile()
{
    QString file = QFileDialog::getOpenFileName(this, "vr_room mesh asset", _assetFile->text(), "XML files (*.xml)");
    if(!file.isEmpty())
        _assetFile->setText(QDir::current().relativeFilePath(file));
}
