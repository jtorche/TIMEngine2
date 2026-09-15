#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include <cstring>

#include "VRRoomTools.h"
#include "interface/XmlSceneLoader.h"

using namespace tim;
using namespace tim::core;
using namespace tim::interface;

namespace
{
    QString num(float f) { return QString::number(f); }
    QString vecStr(const vec3& v) { return num(v.x()) + "," + num(v.y()) + "," + num(v.z()); }

    QString matStr(const mat3& m)
    {
        QString s;
        for(int i=0 ; i<9 ; ++i)
            s += num(m.get(i)) + (i != 8 ? "," : "");
        return s;
    }

    QString transformStr(const mat4& m)
    {
        float yaw = atan2f(m[1][0], m[0][0]) * 57.29578f;
        return "pos=(" + num(m[0][3]) + ", " + num(m[1][3]) + ", " + num(m[2][3]) + ") yaw=" + QString::number(yaw, 'f', 1);
    }

    // Value of an attribute in the attribute list of a tag, quoted or not (the editor writes index=0 unquoted)
    QString attributeValue(const QString& attributes, const QString& name)
    {
        QRegularExpression re("(?:^|\\s)" + QRegularExpression::escape(name) + "\\s*=\\s*(?:\"([^\"]*)\"|([^\\s>\"]+))");
        QRegularExpressionMatch m = re.match(attributes);
        if(!m.hasMatch())
            return QString();
        return m.capturedStart(1) >= 0 ? m.captured(1) : m.captured(2);
    }
}

/** VRRoomGraph **/

bool VRRoomGraph::readConfigScene(const QString& folder, QStringList& sceneNames, QStringList& sceneFiles, QString& error)
{
    sceneNames.clear();
    sceneFiles.clear();

    QDir dir(folder);
    QFile file(dir.filePath("configScene.txt"));
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        error = "Can't open " + file.fileName() + " (the scene folder must contain the configScene.txt scene list)";
        return false;
    }

    QTextStream in(&file);
    QString line;
    while(in.readLineInto(&line))
    {
        line = line.trimmed();
        if(line.isEmpty() || line[0] == '-')   // disabled scene, like in the game
            continue;

        QString name = QFileInfo(line).completeBaseName();
        QString path = dir.filePath(line);      // configScene.txt paths are relative to the Data folder ("scene/start.xml")
        if(!QFileInfo::exists(path))
            path = dir.filePath(name + ".xml"); // or the scene folder itself
        if(!QFileInfo::exists(path))
        {
            error = "Scene file not found for \"" + line + "\" in " + dir.absolutePath();
            return false;
        }

        if(sceneNames.contains(name))
            continue;

        sceneNames += name;
        sceneFiles += path;
    }
    return true;
}

bool VRRoomGraph::build(const Parameters& param, QString& log)
{
    _param = param;
    _scenes.clear();
    _sceneFiles.clear();
    _portals.clear();
    _portalByObjectName.clear();
    _rooms.clear();
    _edges.clear();
    _placements.clear();

    QString error;
    QStringList files;
    if(!readConfigScene(param.sceneFolder, _scenes, files, error))
    {
        log += error + "\n";
        return false;
    }
    if(_scenes.isEmpty())
    {
        log += "No scene listed in configScene.txt\n";
        return false;
    }
    for(int i=0 ; i<_scenes.size() ; ++i)
        _sceneFiles[_scenes[i]] = files[i];

    log += "Scene folder: " + QDir(param.sceneFolder).absolutePath() + "\n";
    log += "Root scene (first line of configScene.txt): " + rootScene() + "\n";
    log += "Room size: " + num(param.roomSize) + " m, portal reach margin: " + num(param.margin) + " m, vertical tolerance: " + num(param.zTolerance) + " m"
           + (param.ignoreRoomBounds ? " (room bounds ignored)" : "") + "\n";
    log += "\nScenes (" + QString::number(_scenes.size()) + "):\n";

    for(const QString& scene : _scenes)
    {
        if(!loadScenePortals(scene, log))
            return false;
    }

    // Breadth first walk of the portal graph, from the VR room at the origin of the root scene
    Room root;
    root.id = 0;
    root.scene = rootScene();
    _rooms += root;

    QStringList dangling;
    for(int r=0 ; r<_rooms.size() ; ++r)  // _rooms grows while walking
    {
        const Room room = _rooms[r];
        const mat4 invRoom = room.transform.inverted();

        for(auto it = _portals.cbegin() ; it != _portals.cend() ; ++it)
        {
            const Portal& p = it.value();
            if(p.scene != room.scene || !p.visible)
                continue;

            if(!param.ignoreRoomBounds && !insideRoom(invRoom, p.translation))
                continue;

            Placement placement;
            placement.roomId = room.id;
            placement.portalName = p.objectName;
            placement.localMatrix = invRoom * p.matrix;
            _placements += placement;

            if(p.dest.isEmpty())
                continue;

            auto destIt = _portals.constFind(p.dest);
            if(destIt == _portals.cend())
            {
                dangling += "  " + p.objectName + " (" + p.scene + "): no portal named " + p.dest;
                continue;
            }
            const Portal& d = destIt.value();
            if(!d.visible)
            {
                dangling += "  " + p.objectName + " -> " + d.objectName + " (" + d.scene + ") is invisible, no edge in game";
                continue;
            }

            // Crossing the portal applies offset = M_dest * M_portal^-1 to the player (MultipleSceneHelper::update)
            mat4 offset = d.matrix * p.matrix.inverted();
            mat4 transform = offset * room.transform;

            int destRoom = findRoom(d.scene, transform);
            if(destRoom < 0)
            {
                Room newRoom;
                newRoom.id = _rooms.size();
                newRoom.scene = d.scene;
                newRoom.transform = transform;
                newRoom.fromRoom = room.id;
                newRoom.fromPortal = p.objectName;
                _rooms += newRoom;
                destRoom = newRoom.id;
            }
            _edges += Edge{room.id, p.objectName, destRoom, d.objectName};
        }
    }

    log += "\nVR rooms (" + QString::number(_rooms.size()) + "):\n";
    for(const QString& scene : _scenes)
    {
        QList<Room> rooms = roomsOfScene(scene);
        log += "  " + scene + ": " + QString::number(rooms.size()) + " room(s)\n";
        for(const Room& room : rooms)
        {
            log += "    " + roomObjectName(room.id) + " " + transformStr(room.transform);
            if(room.fromRoom < 0)
                log += " [root]";
            else
                log += " <- " + roomObjectName(room.fromRoom) + " via " + room.fromPortal;
            log += "\n";
        }
    }

    log += "\nPortals crossed (" + QString::number(_edges.size()) + "):\n";
    for(const Edge& e : _edges)
        log += "  " + roomObjectName(e.fromRoom) + " (" + _rooms[e.fromRoom].scene + ") --" + e.portal + "--> " + roomObjectName(e.toRoom) + " (" + _rooms[e.toRoom].scene + ")\n";

    const QList<Placement> exits = exitPortals(), entrances = entrancePortals();
    log += "\nExit portals, leading to a later scene of configScene.txt (" + QString::number(exits.size()) + "), VR Room > Aggregate exit portals:\n";
    for(const Placement& placement : exits)
        log += "  " + placement.portalName + " in " + roomObjectName(placement.roomId) + " (" + _rooms[placement.roomId].scene + ") at room position (" + vecStr(placement.localMatrix.translation()) + ")\n";
    log += "\nEntrance portals, reached from an earlier scene (" + QString::number(entrances.size()) + "), VR Room > Aggregate entrance portals:\n";
    for(const Placement& placement : entrances)
        log += "  " + placement.portalName + " in " + roomObjectName(placement.roomId) + " (" + _rooms[placement.roomId].scene + ") at room position (" + vecStr(placement.localMatrix.translation()) + ")\n";

    QStringList unreachable;
    for(auto it = _portals.cbegin() ; it != _portals.cend() ; ++it)
    {
        bool placed = false;
        for(const Placement& placement : _placements)
            placed = placed || placement.portalName == it.value().objectName;
        if(!placed)
            unreachable += "  " + it.value().objectName + " (" + it.value().scene + ")" + (it.value().visible ? "" : " invisible");
    }
    if(!unreachable.isEmpty())
        log += "\nPortals standing in no VR room (" + QString::number(unreachable.size()) + "):\n" + unreachable.join("\n") + "\n";
    if(!dangling.isEmpty())
        log += "\nPortals leading nowhere (" + QString::number(dangling.size()) + "):\n" + dangling.join("\n") + "\n";

    return true;
}

bool VRRoomGraph::loadScenePortals(const QString& scene, QString& log)
{
    const QString file = _sceneFiles.value(scene);
    TiXmlDocument doc(file.toStdString());
    if(!doc.LoadFile())
    {
        log += "Fail to load scene " + file + "\n";
        return false;
    }

    std::map<int, vector<XmlMeshAssetLoader::MeshElementModel>> assets;
    std::map<int, QString> assetNames;
    for(TiXmlElement* elem = doc.FirstChildElement() ; elem ; elem = elem->NextSiblingElement())
    {
        if(elem->ValueStr() != std::string("MeshAsset"))
            continue;

        int index = -1;
        elem->QueryIntAttribute("index", &index);
        std::string name;
        assets[index] = XmlMeshAssetLoader::parseMeshAssetElement(elem, name);
        assetNames[index] = QString::fromStdString(name);
    }

    int nbPortals = 0;
    const std::string prefix = "portal";
    for(TiXmlElement* elem = doc.FirstChildElement() ; elem ; elem = elem->NextSiblingElement())
    {
        if(elem->ValueStr() != std::string("Object"))
            continue;

        std::string name;
        elem->QueryStringAttribute("name", &name);
        if(name.compare(0, prefix.size(), prefix) != 0)
            continue;

        // Same parsing as MultiSceneManager: portal<Name>_<DestName>
        vector<std::string> w = StringUtils(name.substr(prefix.size())).splitWord('_');
        if(w.empty())
            continue;

        Portal p;
        p.objectName = QString::fromStdString(name);
        p.name = QString::fromStdString(w[0]);
        p.dest = w.size() > 1 ? QString::fromStdString(w[1]) : QString();
        p.scene = scene;

        int model = -1;
        elem->QueryIntAttribute("model", &model);
        auto assetIt = assets.find(model);
        if(assetIt != assets.end())
        {
            p.asset = assetIt->second;
            p.assetName = assetNames[model];
        }

        bool visible = true;
        elem->QueryBoolAttribute("isVisible", &visible);
        p.visible = visible;

        XmlSceneLoader::parseTransformation(elem, p.translation, p.scale, p.rotation, nullptr);
        p.matrix = mat4::constructTransformation(p.rotation, p.translation, p.scale);

        if(_portals.contains(p.name))
            log += "  WARNING: duplicate portal name \"" + p.name + "\" (" + _portals[p.name].scene + " and " + scene + "), the last one wins like in the game\n";

        _portals[p.name] = p;
        _portalByObjectName[p.objectName] = p.name;
        ++nbPortals;
    }

    log += "  " + scene + ": " + QString::number(nbPortals) + " portal(s)\n";
    return true;
}

int VRRoomGraph::findRoom(const QString& scene, const mat4& transform) const
{
    const float eps = 0.01f;
    for(const Room& room : _rooms)
    {
        if(room.scene != scene)
            continue;

        bool same = true;
        for(int i=0 ; i<3 && same ; ++i)
            for(int j=0 ; j<4 && same ; ++j)
                same = fabsf(room.transform[i][j] - transform[i][j]) <= eps;

        if(same)
            return room.id;
    }
    return -1;
}

bool VRRoomGraph::insideRoom(const mat4& invRoomTransform, const vec3& pos) const
{
    vec3 local = invRoomTransform * pos;
    float half = _param.roomSize * 0.5f + _param.margin;
    return fabsf(local.x()) <= half && fabsf(local.y()) <= half && fabsf(local.z()) <= _param.zTolerance;
}

const VRRoomGraph::Portal* VRRoomGraph::portal(const QString& objectName) const
{
    auto nameIt = _portalByObjectName.constFind(objectName);
    if(nameIt == _portalByObjectName.cend())
        return nullptr;

    auto it = _portals.constFind(nameIt.value());
    return it == _portals.cend() ? nullptr : &it.value();
}

bool VRRoomGraph::isForward(const Edge& e) const
{
    return _scenes.indexOf(_rooms[e.toRoom].scene) > _scenes.indexOf(_rooms[e.fromRoom].scene);
}

VRRoomGraph::Placement VRRoomGraph::placementOf(int roomId, const QString& portalName) const
{
    Placement placement;
    placement.roomId = roomId;
    placement.portalName = portalName;
    if(const Portal* p = portal(portalName))
        placement.localMatrix = _rooms[roomId].transform.inverted() * p->matrix;
    return placement;
}

QList<VRRoomGraph::Placement> VRRoomGraph::exitPortals() const
{
    QList<Placement> res;
    for(const Edge& e : _edges)
    {
        if(isForward(e))
            res += placementOf(e.fromRoom, e.portal);
    }
    return res;
}

QList<VRRoomGraph::Placement> VRRoomGraph::entrancePortals() const
{
    QList<Placement> res;
    for(const Edge& e : _edges)
    {
        if(isForward(e))
            res += placementOf(e.toRoom, e.destPortal);
    }
    return res;
}

QList<VRRoomGraph::Room> VRRoomGraph::roomsOfScene(const QString& scene) const
{
    QList<Room> res;
    for(const Room& room : _rooms)
    {
        if(room.scene == scene)
            res += room;
    }
    return res;
}

void VRRoomGraph::decompose(const mat4& m, mat3& rot, vec3& tr, vec3& scale)
{
    tr = m.translation();
    for(int c=0 ; c<3 ; ++c)
    {
        vec3 column = {m[0][c], m[1][c], m[2][c]};
        float length = column.length();
        scale[c] = length;
        for(int r=0 ; r<3 ; ++r)
            rot[r][c] = length > 0 ? m[r][c] / length : (r == c ? 1.f : 0.f);
    }
}

/** VRRoomWriter **/

bool VRRoomWriter::loadAssetXml(const QString& assetFile, QString& innerXml, QString& error)
{
    QFile file(assetFile);
    if(!file.open(QIODevice::ReadOnly))
    {
        error = "Can't open the vr_room asset file " + assetFile;
        return false;
    }

    QString text = QString::fromUtf8(file.readAll());
    text.replace("\r\n", "\n");

    QRegularExpression re("<MeshAsset\\b[^>]*>(.*?)</MeshAsset>", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch m = re.match(text);
    if(!m.hasMatch())
    {
        error = "No <MeshAsset> element in " + assetFile;
        return false;
    }

    innerXml = m.captured(1);
    while(innerXml.startsWith('\n'))
        innerXml.remove(0, 1);
    while(!innerXml.isEmpty() && innerXml.back().isSpace())
        innerXml.chop(1);

    return true;
}

bool VRRoomWriter::writeRooms(const VRRoomGraph& graph, const Parameters& param, QStringList& writtenFiles, QString& log)
{
    QString inner, error;
    if(!loadAssetXml(param.assetFile, inner, error))
    {
        log += "\n" + error + "\n";
        return false;
    }

    log += "\nScene files:\n";
    bool ok = true;
    for(const QString& scene : graph.scenes())
    {
        bool changed = false;
        if(!writeScene(graph.sceneFile(scene), graph.roomsOfScene(scene), inner, param, changed, log))
            ok = false;
        else if(changed)
            writtenFiles += graph.sceneFile(scene);
    }
    return ok;
}

bool VRRoomWriter::writeScene(const QString& file, const QList<VRRoomGraph::Room>& rooms, const QString& assetInnerXml,
                              const Parameters& param, bool& changed, QString& log)
{
    changed = false;
    const QString sceneName = QFileInfo(file).completeBaseName();

    QFile in(file);
    if(!in.open(QIODevice::ReadOnly))
    {
        log += "  " + sceneName + ": can't read " + file + "\n";
        return false;
    }
    // Edited with '\n' line breaks, the CRLF of a git working copy is restored when writing
    QString original = QString::fromUtf8(in.readAll());
    in.close();
    const bool crlf = original.contains("\r\n");
    original.replace("\r\n", "\n");
    QString text = original;

    // Remove the vr_room_* objects of a previous run
    QRegularExpression objectRe("[ \\t]*<Object\\s+name=\"vr_room_[^\"]*\"[^>]*>.*?</Object>[ \\t]*\\n?", QRegularExpression::DotMatchesEverythingOption);
    int nbRemoved = 0;
    for(QRegularExpressionMatchIterator it = objectRe.globalMatch(text) ; it.hasNext() ; it.next())
        ++nbRemoved;
    text.remove(objectRe);

    // Mesh assets: index of the previous vr_room asset (kept) and the highest index in use
    int maxIndex = -1, vrIndex = -1, vrStart = -1;
    QRegularExpression assetTagRe("<MeshAsset\\s+([^>]*)>");
    for(QRegularExpressionMatchIterator it = assetTagRe.globalMatch(text) ; it.hasNext() ; )
    {
        QRegularExpressionMatch m = it.next();
        bool ok = false;
        int index = attributeValue(m.captured(1), "index").toInt(&ok);
        if(ok)
            maxIndex = std::max(maxIndex, index);

        if(vrStart < 0 && attributeValue(m.captured(1), "name") == "vr_room")
        {
            vrIndex = ok ? index : -1;
            vrStart = m.capturedStart(0);
        }
    }

    // Remove the previous vr_room asset block, it is rewritten from the asset file
    if(vrStart >= 0)
    {
        int end = text.indexOf("</MeshAsset>", vrStart);
        if(end < 0)
        {
            log += "  " + sceneName + ": malformed vr_room MeshAsset, file left untouched\n";
            return false;
        }
        end += int(strlen("</MeshAsset>"));
        while(end < text.size() && (text[end] == ' ' || text[end] == '\t'))
            ++end;
        if(end < text.size() && text[end] == '\n')
            ++end;
        text.remove(vrStart, end - vrStart);
    }

    // An object which still uses the vr_room asset index (not named vr_room_*) keeps the asset alive
    bool indexStillUsed = false;
    if(vrIndex >= 0)
    {
        QRegularExpression modelRe("<Object\\s[^>]*\\bmodel=\"?" + QString::number(vrIndex) + "\"?(?=[\\s>/\"])");
        indexStillUsed = modelRe.match(text).hasMatch();
    }

    if(!rooms.isEmpty() || indexStillUsed)
    {
        const int index = vrIndex >= 0 ? vrIndex : maxIndex + 1;
        QString assetBlock = "<MeshAsset name=\"vr_room\" index=" + QString::number(index) + " >\n" + assetInnerXml + "\n</MeshAsset>\n";

        // After the last mesh asset (the editor writes the assets first, then the objects)
        int assetPos = text.lastIndexOf("</MeshAsset>");
        if(assetPos >= 0)
        {
            assetPos += int(strlen("</MeshAsset>"));
            if(assetPos < text.size() && text[assetPos] == '\n')
                ++assetPos;
        }
        else
        {
            assetPos = text.indexOf("<Object");
            if(assetPos < 0)
                assetPos = text.size();
        }
        text.insert(assetPos, assetBlock);

        if(!rooms.isEmpty())
        {
            if(!text.endsWith('\n'))
                text += '\n';

            for(const VRRoomGraph::Room& room : rooms)
            {
                mat3 rot;
                vec3 tr, scale;
                VRRoomGraph::decompose(room.transform, rot, tr, scale);
                scale = {scale.x() * param.roomSize, scale.y() * param.roomSize, scale.z() * param.thickness};

                text += "<Object name=\"" + VRRoomGraph::roomObjectName(room.id) + "\" model=" + QString::number(index) + " isStatic=1 isPhysic=0 isVisible=0 >\n";
                text += "   <translate>" + vecStr(tr) + "</translate>\n";
                text += "   <scale>" + vecStr(scale) + "</scale>\n";
                text += "   <rotate>" + matStr(rot) + "</rotate>\n";
                text += "   <collider type=0 mass=1 restitution=0.8 friction=0.75 rollingFriction=0.1/>\n";
                text += "</Object>\n";
            }
        }
    }

    if(text == original)
    {
        log += "  " + sceneName + ": unchanged (" + QString::number(rooms.size()) + " room(s))\n";
        return true;
    }

    QFile out(file);
    if(!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        log += "  " + sceneName + ": can't write " + file + "\n";
        return false;
    }
    if(crlf)
        text.replace("\n", "\r\n");
    out.write(text.toUtf8());
    out.close();
    changed = true;

    log += "  " + sceneName + ": " + QString::number(rooms.size()) + " room(s) written, " + QString::number(nbRemoved) + " previous removed\n";
    return true;
}
