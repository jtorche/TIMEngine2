#ifndef FORESTLEVEL_H
#define FORESTLEVEL_H

#include "PortalGame/LevelSystem.h"
#include "bullet/BulletEngine.h"

class ForestLevelBase : public LevelInterface
{
public:
    ForestLevelBase(int index, LevelSystem* system, BulletEngine&);
    void init() override;
    void update(float) override;

protected:
    BulletEngine& _physEngine;
    interface::Mesh _sunStoneMesh[2];
    interface::Mesh _sunStoneRockMesh[2];
    resource::SoundAsset _birds;
    resource::SoundAsset _warp;

    void emitSounddPortal(const vec3&);
};

class ForestLevel1 : public ForestLevelBase
{
public:
    ForestLevel1(int index, LevelSystem* system, BulletEngine&, std::string namePortal);

    void init() override;
    void prepareEnter() override {}
    void update(float) override;

private:
    std::string _namePortal;
    int _indexPortal = 0;
    int _indexPortalOut = 0;
    interface::MeshInstance* _instSunStone = nullptr;

    bool _first = true;
};

class ForestLevel2 : public ForestLevelBase
{
public:
    ForestLevel2(int index, LevelSystem* system, BulletEngine&);

    void init() override;
    void prepareEnter() override {}
    void update(float) override;

private:
    int _indexPortal;

    interface::MeshInstance* _sunStone[7] = {nullptr}; // 1 2 2 2 2 3 3

    int _updateRate = 0;

};

class ForestLevel3 : public ForestLevelBase
{
public:
    ForestLevel3(int index, LevelSystem* system, BulletEngine&);

    void init() override;
    void prepareEnter() override {}
    void update(float) override;

private:
    int _indexPortal;
    int _updateRate = 0;

    int _indexArtifact, _indexSlot, _indexArtifactInPlace;
    interface::Mesh _nonActivatedArtifactMesh;

};


#endif // FORESTLEVEL_H
