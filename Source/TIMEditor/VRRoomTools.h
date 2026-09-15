#ifndef VRROOMTOOLS_H
#define VRROOMTOOLS_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include "core/Matrix.h"
#include "interface/XmlMeshAssetLoader.h"

using namespace tim;

/* Derives the placement of the VR room in every scene of the game.
 *
 * The VR room is a NxN meter ground centered at the origin of the root scene (first line of configScene.txt).
 * Portals are the edges of a graph whose nodes are the VR rooms: crossing "portalA_B" applies the offset
 * M_B * M_A^-1 to the player (MultipleSceneHelper::update), so the VR room reached through it is the room
 * it was crossed from, transformed by that offset. The walk is done algorithmically, no scene is opened. */
class VRRoomGraph
{
public:
    struct Parameters
    {
        QString sceneFolder;            // folder containing configScene.txt and the scene xml files
        float roomSize = 3;             // NxN meters (the game scales the physical room to 3 m)
        float margin = 0.5f;            // tolerance around the room to consider a portal reachable
        float zTolerance = 1;           // a portal on another floor is not reachable
        bool ignoreRoomBounds = false;  // cross every portal of a scene regardless of the room bounds
    };

    struct Portal
    {
        QString objectName;   // object name in the scene file, ex: portalStart_Start2In
        QString name;         // Start
        QString dest;         // Start2In (empty for a portal leading nowhere)
        QString scene;
        QString assetName;
        vector<interface::XmlMeshAssetLoader::MeshElementModel> asset;
        mat3 rotation = mat3::IDENTITY();
        vec3 translation = {0,0,0};
        vec3 scale = {1,1,1};
        mat4 matrix = mat4::IDENTITY();
        bool visible = true;
    };

    struct Room
    {
        int id = 0;
        QString scene;
        mat4 transform = mat4::IDENTITY();  // VR room space -> scene space
        int fromRoom = -1;                  // room the portal was crossed from (-1 for the root)
        QString fromPortal;                 // object name of the crossed portal
    };

    struct Edge  // a portal crossed during the walk
    {
        int fromRoom = 0;
        QString portal;       // object name of the crossed portal, in the scene of fromRoom
        int toRoom = 0;
        QString destPortal;   // object name of the portal it leads to, in the scene of toRoom
    };

    struct Placement  // a portal standing inside a VR room, expressed in VR room space
    {
        int roomId = 0;
        QString portalName;   // object name
        mat4 localMatrix = mat4::IDENTITY();
    };

    bool build(const Parameters&, QString& log);

    const Parameters& parameters() const { return _param; }
    const QStringList& scenes() const { return _scenes; }
    QString sceneFile(const QString& scene) const { return _sceneFiles.value(scene); }
    QString rootScene() const { return _scenes.isEmpty() ? QString() : _scenes.first(); }
    const QList<Room>& rooms() const { return _rooms; }
    const QList<Edge>& edges() const { return _edges; }
    const QList<Placement>& placements() const { return _placements; }
    const Portal* portal(const QString& objectName) const;
    QList<Room> roomsOfScene(const QString&) const;

    // Portals leading to a scene later in configScene.txt (the game progression), placed in the room they are crossed from
    QList<Placement> exitPortals() const;
    // The portals those exits lead to, placed in the room they arrive in
    QList<Placement> entrancePortals() const;

    static QString roomObjectName(int id) { return "vr_room_" + QString::number(id); }

    // Scene names listed in configScene.txt, in order (the first one is the root), with the matching scene files.
    // Lines starting with '-' are ignored like in the game.
    static bool readConfigScene(const QString& folder, QStringList& sceneNames, QStringList& sceneFiles, QString& error);

    // Splits M = [R*S | t] into rotation, translation and scale (the columns of R*S are scaled by S)
    static void decompose(const mat4&, mat3& rot, vec3& tr, vec3& scale);

private:
    Parameters _param;
    QStringList _scenes;
    QMap<QString, QString> _sceneFiles;
    QMap<QString, Portal> _portals;             // key = portal name (the part between "portal" and '_')
    QMap<QString, QString> _portalByObjectName; // object name -> portal name
    QList<Room> _rooms;                         // room id == index in the list
    QList<Edge> _edges;
    QList<Placement> _placements;

    bool loadScenePortals(const QString& scene, QString& log);
    int findRoom(const QString& scene, const mat4&) const;
    bool insideRoom(const mat4& invRoomTransform, const vec3& pos) const;
    bool isForward(const Edge&) const;
    Placement placementOf(int roomId, const QString& portalName) const;
};

/* Writes the VR rooms computed by VRRoomGraph into the scene files, in place: the previous vr_room_* objects
 * and vr_room mesh asset are removed and the new ones are added. The rest of the file is left untouched.
 * The objects are invisible and not physical: the game doesn't render them, it only builds the VR room walls
 * (meshBank/vr_room_walls.obj, colliding with the interactive objects) at their position (MultiSceneManager). */
class VRRoomWriter
{
public:
    struct Parameters
    {
        QString assetFile = "shape/vr_room.xml";  // MeshAsset used for the rooms (a unit cube)
        float roomSize = 3;
        float thickness = 0.05f;                  // height of the ground slab, centered on the room ground
    };

    static bool writeRooms(const VRRoomGraph&, const Parameters&, QStringList& writtenFiles, QString& log);
    static bool writeScene(const QString& file, const QList<VRRoomGraph::Room>&, const QString& assetInnerXml,
                           const Parameters&, bool& changed, QString& log);
    static bool loadAssetXml(const QString& assetFile, QString& innerXml, QString& error);
};

#endif // VRROOMTOOLS_H
