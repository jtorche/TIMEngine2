#ifndef VRROOMDIALOG_H
#define VRROOMDIALOG_H

#include <QDialog>
#include "VRRoomTools.h"

class QLineEdit;
class QLabel;
class QDoubleSpinBox;
class QCheckBox;

/* Parameters of the VR room generation (VRRoomGraph walk + VRRoomWriter) */
class VRRoomDialog : public QDialog
{
    Q_OBJECT

public:
    struct Config
    {
        VRRoomGraph::Parameters graph;
        VRRoomWriter::Parameters writer;
        bool writeFiles = true;
    };

    explicit VRRoomDialog(QWidget* parent = nullptr);
    Config getConfig() const;

public slots:
    void accept() override;

private slots:
    void browseSceneFolder();
    void browseAssetFile();
    void refreshSceneInfo();

private:
    QLineEdit* _sceneFolder;
    QLineEdit* _assetFile;
    QLabel* _sceneInfo;
    QDoubleSpinBox* _roomSize;
    QDoubleSpinBox* _margin;
    QDoubleSpinBox* _zTolerance;
    QDoubleSpinBox* _thickness;
    QCheckBox* _ignoreBounds;
    QCheckBox* _writeFiles;

    void loadSettings();
    void saveSettings();
};

#endif // VRROOMDIALOG_H
