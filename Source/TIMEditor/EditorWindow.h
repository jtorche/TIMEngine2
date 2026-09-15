#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include <QMainWindow>
#include <QShortcut>
#include "interface/XmlMeshAssetLoader.h"
#include "MeshElement.h"
#include "SceneEditorWidget.h"
#include "VRRoomTools.h"

namespace Ui {
class EditorWindow;
}

class EditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit EditorWindow(QWidget *parent = 0);
    ~EditorWindow();

    void closeEvent(QCloseEvent* event) override;

protected:

private:
    Ui::EditorWindow *ui;
    MainRenderer* _mainRenderer = nullptr;
    tim::interface::XmlMeshAssetLoader _assetLoader;

    QShortcut* _copySC;
    QString _savePath[SceneEditorWidget::NB_SCENE];

    VRRoomGraph _vrRoomGraph;              // last portal graph walk (VR Room menu)
    VRRoomWriter::Parameters _vrRoomParam; // room size and asset used by the last walk
    bool _vrRoomGraphBuilt = false;

    QString genTitle() const;
    void selectActiveScene();

    void loadParameter(QString);
    bool checkEditorEmpty();
    bool checkSceneEmpty(int sceneIndex);
    void aggregatePortals(const QList<VRRoomGraph::Placement>&, const QString& what);
    void showTextReport(const QString& title, const QString& text);

public slots:
    void addResourceFolder();
    void addResourceFolderRec();
    void addMeshToAsset();
    void loadMeshAssets(QString);

private slots:
    void on_actionClose_Context_triggered();
    void on_actionAdd_folder_triggered();
    void on_actionAdd_folder_recursively_triggered();
    void on_actionSet_skybox_triggered();
    void on_actionMesh_assets_triggered();
    void on_actionMesh_assets_import_triggered();
    void on_actionLoad_collada_triggered();
    void on_action_selectAE_triggered();
    void on_actionScene_1_triggered();
    void on_actionScene_2_triggered();
    void on_actionScene_3_triggered();
    void on_actionScene_4_triggered();
    void on_actionSave_triggered();
    void on_actionLoad_triggered();
    void on_actionImport_scene_triggered();
    void on_actionSave_As_triggered();
    void on_actionNew_triggered();
    void on_actionSunDirection_triggered();
    void on_actionSunColorAmbient_triggered();

    void on_actionRaw_Data_triggered();
    void on_actionRemove_Spec_Probe_triggered();
    void on_actionRemove_last_Spec_Probe_triggered();
    void on_actionRegenerate_Spec_Probe_triggered();

    void on_actionShow_Spec_Probes_triggered();

    void on_actionGenerate_VR_Rooms_triggered();
    void on_actionAggregate_Exit_Portals_triggered();
    void on_actionAggregate_Entrance_Portals_triggered();

    void addAssetToScene(QString);
    void addGeometryToScene(QString, QString);

    void flushFeedbackTrans(QString);
    void flushStatsLabel();

};

#endif // EDITORWINDOW_H
