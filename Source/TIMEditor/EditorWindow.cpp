#include "EditorWindow.h"
#include "ui_EditorWindow.h"
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QSettings>

#include <QMessageBox>
#include <QFileDialog>
#include "SelectSkyboxDialog.h"
// #include "AssimpLoader.h"

using namespace tim;

EditorWindow::EditorWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::EditorWindow)
{
    ui->setupUi(this);
    this->setCentralWidget(nullptr);

    _mainRenderer = ui->glViewContainer->getRendererWindow()->getRenderer();

    this->tabifyDockWidget(ui->assetDockWidget, ui->resourceDockWidget);
    this->tabifyDockWidget(ui->sceneDockWidget, ui->meshDockWidget);

    ui->resourceWidget->viewport()->setAcceptDrops(true);
    ui->sceneEditorWidget->setLocalCB(ui->localRot, ui->localTrans);

    // Restore previous state
    QSettings settings("TIMEngine2", "TIMEditor");
    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
    const QByteArray winState = settings.value("windowState", QByteArray()).toByteArray();
    restoreGeometry(geometry);
    restoreState(winState);

    ui->meshEditorWidget->setMainRenderer(_mainRenderer);

    ui->meshEditorWidget->setResourceWidget(ui->resourceWidget);

    ui->sceneEditorWidget->setMainRenderer(_mainRenderer);
    ui->sceneEditorWidget->setMeshEditor(ui->meshEditorWidget);

    ui->assetViewWidget->setMeshEditorWidget(ui->meshEditorWidget);

    ui->resourceWidget->addDir(".");

    ui->statusBar->showMessage("Welcome !", 10000);

    connect(ui->glViewContainer, SIGNAL(F11_pressed()), ui->viewDockWidget, SLOT(switchFullScreen()));
    connect(ui->glViewContainer, SIGNAL(pressedMouseMoved(int,int)), ui->meshEditorWidget, SLOT(rotateEditedMesh(int,int)));
    connect(ui->glViewContainer, SIGNAL(clickInEditor(vec3,vec3,bool)), ui->sceneEditorWidget, SLOT(selectSceneObject(vec3,vec3,bool)));
    connect(ui->glViewContainer, SIGNAL(translateMouse(float,float,int)), ui->sceneEditorWidget, SLOT(translateMouse(float,float,int)));
    connect(ui->glViewContainer, SIGNAL(escapePressed()), ui->sceneEditorWidget, SLOT(cancelSelection()));
    connect(ui->glViewContainer, SIGNAL(deleteCurrent()), ui->sceneEditorWidget, SLOT(deleteCurrentObjects()));

    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(flushStatsLabel()));
    timer->start(300);

    connect(ui->meshEditorWidget, SIGNAL(saveMeshClicked()), this, SLOT(addMeshToAsset()));
    connect(ui->meshEditorWidget, SIGNAL(changeCurBaseModelName(QString)), ui->sceneEditorWidget, SLOT(changeBaseModelName(QString)));

    connect(ui->glViewContainer, SIGNAL(addAssetToScene(QString)), this, SLOT(addAssetToScene(QString)));
    connect(ui->glViewContainer, SIGNAL(addGeometryToScene(QString,QString)), this, SLOT(addGeometryToScene(QString,QString)));

    connect(ui->glViewContainer, SIGNAL(startEdit()), ui->sceneEditorWidget, SLOT(saveCurMeshTrans()));
    connect(ui->glViewContainer, SIGNAL(cancelEdit()), ui->sceneEditorWidget, SLOT(restoreCurMeshTrans()));
    connect(ui->glViewContainer, SIGNAL(stateChanged(int)), ui->sceneEditorWidget, SLOT(flushUiAccordingState(int)));
    connect(ui->sceneEditorWidget, SIGNAL(feedbackTransformation(QString)), this, SLOT(flushFeedbackTrans(QString)));

    connect(ui->actionGenerate_Specular_Probe, SIGNAL(triggered()), ui->sceneEditorWidget, SLOT(renderSpecularProbe()));

    _copySC = new QShortcut(QKeySequence("Ctrl+C"), ui->glViewContainer);
    connect(_copySC, SIGNAL(activated()), ui->sceneEditorWidget, SLOT(copyObject()));

    connect(ui->sceneEditorWidget, SIGNAL(editTransformation(int)), ui->glViewContainer, SLOT(enableTransformationMode(int)));

    loadParameter("editorParameter.txt");
}

EditorWindow::~EditorWindow()
{
    delete ui;
    delete _copySC;
}

void EditorWindow::closeEvent(QCloseEvent* event)
{
    QSettings settings("TIMEngine2", "TIMEditor");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    QMainWindow::closeEvent(event);
}

void EditorWindow::loadParameter(QString filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
        return;

    QTextStream in(&file);
    QString line;

    while(in.readLineInto(&line))
    {
        auto vec = StringUtils(line.toStdString()).splitWord('=');
        if(vec.size() == 2)
        {
            if(vec[0] == "mouseSensitivity")
            {
                float sens = StringUtils(vec[1]).toFloat();
                if(sens <= 0)
                    sens = 1;

                ui->glViewContainer->setMouseSensitivity(sens);
            }
        }
    }
}

QString EditorWindow::genTitle() const
{
    QString title = "TIMEditor - Scene " + QString::number(ui->sceneEditorWidget->activeScene()+1);
    if(!_savePath[ui->sceneEditorWidget->activeScene()].isEmpty())
        title += " - " + _savePath[ui->sceneEditorWidget->activeScene()];

    return title;
}

/** SLOTS **/

void EditorWindow::flushStatsLabel()
{
    QString stats;
    if (_mainRenderer) {
        stats += QString::number(_mainRenderer->getNumTriangleRendered()) + " triangles - " + QString::number(_mainRenderer->getNumDrawcalls()) + " drawcalls";
        ui->renderStats->setText(stats);
    } else {
        ui->renderStats->setText("No stats");
    }
}

void EditorWindow::EditorWindow::flushFeedbackTrans(QString str)
{
    ui->feedbackTrans->setText(str);
}

void EditorWindow::addResourceFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory",
                                                    ".",
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);
    ui->resourceWidget->addDir(dir);
}

void EditorWindow::addResourceFolderRec()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory",
                                                    ".",
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);
    ui->resourceWidget->addDir(dir, true);
}

void EditorWindow::on_actionClose_Context_triggered()
{
    ui->glViewContainer->closeContext();
}

void EditorWindow::on_actionAdd_folder_triggered()
{
    addResourceFolder();
}

void EditorWindow::on_actionAdd_folder_recursively_triggered()
{
    addResourceFolderRec();
}

void EditorWindow::on_actionSunDirection_triggered()
{
    int sceneIndex = _mainRenderer->getCurSceneIndex();
    if(!_mainRenderer->getScene(sceneIndex).globalLight.dirLights.empty())
    {
        interface::Pipeline::DirectionalLight l = _mainRenderer->getScene(sceneIndex).globalLight.dirLights[0];
        l.direction = _mainRenderer->getSceneView(sceneIndex).camera.dir - _mainRenderer->getSceneView(sceneIndex).camera.pos;
        _mainRenderer->setDirectionalLight(sceneIndex, l);

        if(sceneIndex > 0)
            ui->sceneEditorWidget->setSunDirection(sceneIndex-1, l.direction);
    }

}

void EditorWindow::on_actionSet_skybox_triggered()
{
    SelectSkyboxDialog dialog(this, ui->resourceWidget);
    dialog.exec();

    QList<QString> list = dialog.getSkydirPaths();

    if(list.empty()) return;

    _mainRenderer->addEvent([=](){
        _mainRenderer->setSkybox(_mainRenderer->getCurSceneIndex(), list);
    });

    if(_mainRenderer->getCurSceneIndex() > 0)
        ui->sceneEditorWidget->setSkybox(_mainRenderer->getCurSceneIndex()-1, list);
}

interface::XmlMeshAssetLoader::MeshElementModel convertEditorModel(MeshElement model)
{
    interface::XmlMeshAssetLoader::MeshElementModel out;
    out.color = vec3(model.color.red(), model.color.green(), model.color.blue()) / 255.f;
    out.geometry = model.geometry.toStdString();
    out.material = model.material;
    out.textureScale = model.textureScale;

    out.advanced = model.advanced;
    out.advancedShader = model.advancedShader.toStdString();
    out.useAdvanced = model.useAdvanced;
    out.castShadow = model.castShadow;
    out.cmAffected = model.cmAffected;

    for(int i=0 ; i<MeshElement::NB_TEXTURES ; ++i)
    {
        out.textures[i] = model.textures[i].toStdString();
    }
    out.type = 0;

    return out;
}

void EditorWindow::addMeshToAsset()
{
    if(ui->meshEditorWidget->currentMeshName().isEmpty())
    {
        QMessageBox::warning(ui->meshEditorWidget, "Mesh name is empty", "We can't save the mesh without a valid name.");
        return;
    }

    if(ui->meshEditorWidget->currentMesh().isEmpty())
    {
        QMessageBox::warning(ui->meshEditorWidget, "The mesh is empty", "The active mesh is empty.");
        return;
    }

    AssetViewWidget::Element elem;
    elem.materials = ui->meshEditorWidget->currentMesh();
    elem.name = ui->meshEditorWidget->currentMeshName();

    vector<interface::XmlMeshAssetLoader::MeshElementModel> model;
    for(int i=0 ; i<elem.materials.size() ; ++i)
        model.push_back(convertEditorModel(elem.materials[i]));

    _assetLoader.addModel(elem.name.toStdString(), model);

    ui->assetViewWidget->addElement(elem);
}

void EditorWindow::loadMeshAssets(QString filename)
{
    interface::XmlMeshAssetLoader loader;
    if(!loader.load(filename.toStdString()))
        return;

    _assetLoader.load(filename.toStdString());

    for(auto p : loader.allAssets())
    {
        AssetViewWidget::Element elem;
        elem.name = p.first.c_str();

        elem.materials = ui->meshEditorWidget->convertFromEngine(p.second);
        ui->assetViewWidget->addElement(elem);
    }
}

void EditorWindow::on_actionMesh_assets_triggered()
{
    QString file = QFileDialog::getSaveFileName(this, "Save mesh assets", ".");
    if(file.isEmpty())
        return;

    ui->assetViewWidget->exportMesh(file, "./");
}

void EditorWindow::on_actionMesh_assets_import_triggered()
{
    QString file = QFileDialog::getOpenFileName(this, "Import mesh assets", ".", "XML files (*.xml)");
    if(file.isEmpty())
        return;

    loadMeshAssets(file);
}

void EditorWindow::on_action_selectAE_triggered()
{
    _mainRenderer->lock();
    _mainRenderer->setCurSceneIndex(0);
    ui->glViewContainer->setEditMode(GLViewContainer::MESH_EDITOR);
    _mainRenderer->unlock();

    ui->sceneEditorWidget->cancelSelection();
    ui->meshEditorWidget->activeEditMode();

    setWindowTitle("Asset Editor");
}

void EditorWindow::on_actionScene_1_triggered()
{
    _mainRenderer->lock();
    _mainRenderer->setCurSceneIndex(1);
    ui->glViewContainer->setEditMode(GLViewContainer::SCENE_EDITOR);
    _mainRenderer->unlock();

    ui->meshEditorWidget->setEditedMesh(nullptr, nullptr, nullptr, "");
    ui->sceneEditorWidget->switchScene(0);

    setWindowTitle(genTitle());
}

void EditorWindow::on_actionScene_2_triggered()
{
    _mainRenderer->lock();
    _mainRenderer->setCurSceneIndex(2);
    ui->glViewContainer->setEditMode(GLViewContainer::SCENE_EDITOR);
    _mainRenderer->unlock();

    ui->meshEditorWidget->setEditedMesh(nullptr, nullptr, nullptr, "");
    ui->sceneEditorWidget->switchScene(1);

    setWindowTitle(genTitle());
}

void EditorWindow::on_actionScene_3_triggered()
{
    _mainRenderer->lock();
    _mainRenderer->setCurSceneIndex(3);
    ui->glViewContainer->setEditMode(GLViewContainer::SCENE_EDITOR);
    _mainRenderer->unlock();

    ui->meshEditorWidget->setEditedMesh(nullptr, nullptr, nullptr, "");
    ui->sceneEditorWidget->switchScene(2);

    setWindowTitle(genTitle());
}

void EditorWindow::on_actionScene_4_triggered()
{
    _mainRenderer->lock();
    _mainRenderer->setCurSceneIndex(4);
    ui->glViewContainer->setEditMode(GLViewContainer::SCENE_EDITOR);
    _mainRenderer->unlock();

    ui->meshEditorWidget->setEditedMesh(nullptr, nullptr, nullptr, "");
    ui->sceneEditorWidget->switchScene(3);

    setWindowTitle(genTitle());
}


void EditorWindow::on_actionLoad_collada_triggered()
{
#if 0
    QString file = QFileDialog::getOpenFileName(this, "Import mesh assets", ".", "Collada files (*.dae)");
    if(file.isEmpty())
        return;

    AssimpLoader loader;
    loader.load(file.toStdString());

    vector<AssimpLoader::Node> nodes = loader.nodes();

    if(_rendererThread->mainRenderer()->getCurSceneIndex() == 0)
    {
        switch(ui->sceneEditorWidget->activeScene())
        {
            case 0: on_actionScene_1_triggered(); break;
            case 1: on_actionScene_2_triggered(); break;
            case 2: on_actionScene_3_triggered(); break;
            case 3: on_actionScene_4_triggered(); break;
        }
    }

    for(const AssimpLoader::Node& elem : nodes)
    {
        AssetViewWidget::Element asset;
        if(ui->assetViewWidget->getElement(QString::fromStdString(elem.idName), &asset))
        {
            ui->sceneEditorWidget->addSceneObject(QString::fromStdString(elem.name), QString::fromStdString(elem.idName), asset.materials, elem.matrix);
        }
    }

    ui->statusBar->showMessage("Collada scene loaded", 1000 * 60 * 10);
#endif
}

void EditorWindow::on_actionSave_triggered()
{
    if(_savePath[ui->sceneEditorWidget->activeScene()].isEmpty())
        on_actionSave_As_triggered();
    else
        ui->sceneEditorWidget->exportScene(_savePath[ui->sceneEditorWidget->activeScene()],
                                           ui->sceneEditorWidget->activeScene());

    ui->statusBar->showMessage(QString("Scene saved : ") + QDateTime::currentDateTime().toString("hh:mm:ss"), 1000 * 60 * 10);
    setWindowTitle(genTitle());
}

void EditorWindow::on_actionLoad_triggered()
{
    QString file = QFileDialog::getOpenFileName(this, "Load scene", ".", "XML files (*.xml)");
    if(file.isEmpty())
        return;

    if(_mainRenderer->getCurSceneIndex() == 0)
    {
        switch(ui->sceneEditorWidget->activeScene())
        {
            case 0: on_actionScene_1_triggered(); break;
            case 1: on_actionScene_2_triggered(); break;
            case 2: on_actionScene_3_triggered(); break;
            case 3: on_actionScene_4_triggered(); break;
        }
    }

    ui->sceneEditorWidget->importScene(file, ui->sceneEditorWidget->activeScene());
    _savePath[ui->sceneEditorWidget->activeScene()] = file;

    ui->statusBar->showMessage("Scene loaded", 1000 * 60 * 10);
    setWindowTitle(genTitle());
}

void EditorWindow::on_actionSave_As_triggered()
{
    QString file = QFileDialog::getSaveFileName(this, "Save scene", ".", "XML Files (*.xml)");
    if(file.isEmpty())
        return;

    ui->sceneEditorWidget->exportScene(file, ui->sceneEditorWidget->activeScene());
    _savePath[ui->sceneEditorWidget->activeScene()] = file;

    ui->statusBar->showMessage(QString("Scene saved at ") + QDateTime::currentDateTime().toString("hh:mm:ss"), 1000 * 60 * 10);
    setWindowTitle(genTitle());
}

void EditorWindow::on_actionNew_triggered()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, "New scene", "Are you sure to clear the scene ?",
                                                              QMessageBox::Yes|QMessageBox::No);
    if(reply == QMessageBox::Yes)
        ui->sceneEditorWidget->clearScene(ui->sceneEditorWidget->activeScene());

    ui->statusBar->showMessage("New scene", 10000);

    _savePath[ui->sceneEditorWidget->activeScene()] = "";
    setWindowTitle(genTitle());
}

void EditorWindow::addAssetToScene(QString assetName)
{
    AssetViewWidget::Element asset;
    if(ui->assetViewWidget->getElement(assetName, &asset))
    {
        vec3 camPos = _mainRenderer->getSceneView(_mainRenderer->getCurSceneIndex()).camera.pos;
        vec3 camDir = _mainRenderer->getSceneView(_mainRenderer->getCurSceneIndex()).camera.dir;
        ui->sceneEditorWidget->addSceneObject("", assetName, asset.materials, mat3::IDENTITY(), camPos + (camDir-camPos).resize(2), vec3(1,1,1));

        ui->sceneEditorWidget->activateLastAdded();
    }
}

void EditorWindow::addGeometryToScene(QString geomPath, QString name)
{
    AssetViewWidget::Element asset;
    MeshElement elem;
    elem.color = QColor(255,255,255);
    elem.material = {0.5,0,0.15,0};
    elem.geometry = geomPath;
    asset.materials += elem;

    vec3 camPos = _mainRenderer->getSceneView(_mainRenderer->getCurSceneIndex()).camera.pos;
    vec3 camDir = _mainRenderer->getSceneView(_mainRenderer->getCurSceneIndex()).camera.dir;
    ui->sceneEditorWidget->addSceneObject("", name, asset.materials, mat3::IDENTITY(), camPos + (camDir-camPos).resize(2), vec3(1,1,1));

    ui->sceneEditorWidget->activateLastAdded();
}

void EditorWindow::on_actionRaw_Data_triggered()
{
    QString file = QFileDialog::getSaveFileName(this, "Save cubemap", ".", "RawImage Files (*.itim)");
    if(file.isEmpty())
        return;

    vec3 camPos = _mainRenderer->getSceneView(_mainRenderer->getCurSceneIndex()).camera.pos;
    _mainRenderer->addEvent( [=](){
        auto tex = _mainRenderer->renderCubemap(camPos, 1024, _mainRenderer->getCurSceneIndex(), 1);
        auto ptex = renderer::IndirectLightRenderer::processSkybox(tex, interface::ShaderPool::instance().get("processSpecularCubeMap"));
        renderer::Texture::exportTexture(ptex, file.toStdString(), 7);
        delete tex; delete ptex;

    });
}

void EditorWindow::on_actionRemove_Spec_Probe_triggered()
{
    ui->sceneEditorWidget->removeAllLightProbe();
}

void EditorWindow::on_actionRemove_last_Spec_Probe_triggered()
{
    ui->sceneEditorWidget->removeLastLightProbe();
}

void EditorWindow::on_actionRegenerate_Spec_Probe_triggered()
{
    ui->sceneEditorWidget->regenAllLightProb();
}

void EditorWindow::on_actionShow_Spec_Probes_triggered()
{
    ui->sceneEditorWidget->setShowSPecProbePreview(ui->actionShow_Spec_Probes->isChecked());
    if (ui->actionShow_Spec_Probes->isChecked()) {
        ui->sceneEditorWidget->createSpecProbePreview();
    } else {
        ui->sceneEditorWidget->removeSpecProbePreview();
    }
}
