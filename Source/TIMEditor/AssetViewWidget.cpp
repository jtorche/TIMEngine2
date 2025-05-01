#include "AssetViewWidget.h"
#include "core/StringUtils.h"
#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>

AssetViewWidget::AssetViewWidget(QWidget* parent) : QListWidget(parent), _meshIcon(":/icons/Icons/mesh.png")
{
}

void AssetViewWidget::addElement(const Element& e)
{
    for(ItemElement& elem : _items)
    {
        if(elem.elem.name == e.name)
        {
            elem.elem = e;
            return;
        }
    }

    _items += {e, getIcon(e)};
    auto ol = new QListWidgetItem(getIcon(e), e.name, this);
    ol->setSizeHint(QSize(100,100));
}

bool AssetViewWidget::getElement(QString name, Element* elemptr) const
{
    for(const ItemElement& elem : _items)
    {
        if(elem.elem.name == name)
        {
            *elemptr = elem.elem;
            return true;
        }
    }
    return false;
}

QIcon AssetViewWidget::getIcon(const Element&) const
{
    return _meshIcon;
}

void AssetViewWidget::exportMesh(QString filePath, QString relativeSource)
{
    QFile file(filePath);
    QDir destDir(relativeSource);
    if (file.open(QIODevice::WriteOnly))
    {
        QTextStream stream(&file);
        for(ItemElement& e : _items)
        {
            stream << "<MeshAsset name=\"" << e.elem.name << "\">\n";

            for(Material& m : e.elem.materials)
            {
                writeMaterial(m, stream, destDir);
            }

            stream << "</MeshAsset>\n\n";
        }
    }
}

void AssetViewWidget::writeMaterial(const Material& m, QTextStream& stream, QDir& destDir, QString prefix)
{
    stream << prefix << "<Element type=0>\n";
    stream << prefix << "\t<color>" << m.color.red() << "," << m.color.green() << "," << m.color.blue() << "</color>\n";
    stream << prefix << "\t<geometry>" << destDir.relativeFilePath(m.geometry) <<"</geometry>\n";
    stream << prefix << "\t<roughness>" << m.material[0] <<"</roughness>\n";
    stream << prefix << "\t<metallic>" << m.material[1] <<"</metallic>\n";
    stream << prefix << "\t<specular>" << m.material[2] <<"</specular>\n";
    stream << prefix << "\t<emissive>" << m.material[3] <<"</emissive>\n";
    stream << prefix << "\t<textureScale>" << m.textureScale <<"</textureScale>\n";
    stream << prefix << "\t<castShadow>" << m.castShadow <<"</castShadow>\n";
    stream << prefix << "\t<cmAffected>" << m.cmAffected <<"</cmAffected>\n";

    if(!m.textures[0].isEmpty())
        stream << prefix << "\t<diffuseTex>" << destDir.relativeFilePath(m.textures[0]) <<"</diffuseTex>\n";
    if(!m.textures[1].isEmpty())
        stream << prefix << "\t<normalTex>" << destDir.relativeFilePath(m.textures[1]) <<"</normalTex>\n";
    if(!m.textures[2].isEmpty())
        stream << prefix << "\t<materialTex>" << destDir.relativeFilePath(m.textures[2]) <<"</materialTex>\n";

    if(m.useAdvanced)
    {
        stream << prefix << "\t<Advanced>\n";
        stream << prefix << "\t\t<shader>" << m.advancedShader << "</shader>\n";
        stream << prefix << "\t\t<blend>" << m.advanced.blend() << "</blend>\n";
        stream << prefix << "\t\t<cullBackFace>" << m.advanced.cullBackFace() << "</cullBackFace>\n";
        stream << prefix << "\t\t<cullFace>" << m.advanced.cullFace() << "</cullFace>\n";
        stream << prefix << "\t\t<depthTest>" << m.advanced.depthTest() << "</depthTest>\n";
        stream << prefix << "\t\t<writeDepth>" << m.advanced.writeDepth() << "</writeDepth>\n";
        stream << prefix << "\t\t<priority>" << m.advanced.priority() << "</priority>\n";
        stream << prefix << "\t\t<blendEquation>" << QString::fromStdString(renderer::DrawState::toBlendEquationStr(m.advanced.blendEquation())) << "</blendEquation>\n";
        stream << prefix << "\t\t<primitive>" << QString::fromStdString(renderer::DrawState::toPrimitiveStr(m.advanced.primitive())) << "</primitive>\n";
        stream << prefix << "\t\t<depthFunc>" << QString::fromStdString(renderer::DrawState::toComparFuncStr(m.advanced.depthFunc())) << "</depthFunc>\n";
        stream << prefix << "\t\t<blendFunc f1=\"" << QString::fromStdString(renderer::DrawState::toBlendFuncStr(m.advanced.blendFunc()[0])) <<
                            "\" f2=\"" << QString::fromStdString(renderer::DrawState::toBlendFuncStr(m.advanced.blendFunc()[1])) << "\" />\n";

        stream << prefix << "\t</Advanced>\n";
    }

    stream << prefix << "</Element>\n";
}

void AssetViewWidget::onItemDoubleClicked(QListWidgetItem* item)
{
    for(ItemElement& elem : _items)
    {
        if(elem.elem.name == item->text())
        {
            QList<Material> meshEditorElem;
            for(Material m : elem.elem.materials)
            {
                meshEditorElem.push_back(m);
            }
            _meshEditor->setMesh(elem.elem.name, meshEditorElem);
            return;
        }
    }
}

void AssetViewWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;

    if (currentItem() == NULL)
        return;

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;

    QList<QUrl> list;
    list.append(QUrl(currentItem()->text()));
    mimeData->setUrls(list);
    mimeData->setObjectName("AssetViewWidget");
    drag->setMimeData(mimeData);

    drag->exec(Qt::CopyAction | Qt::MoveAction);
}

