#ifndef OCEANLEVEL_H
#define OCEANLEVEL_H

#include "PortalGame/LevelSystem.h"
#include "bullet/BulletEngine.h"
#include "PortalGame/BetweenSceneStruct.h"

class OceanLevel : public LevelInterface
{
public:
    OceanLevel(int index, LevelSystem* system, BulletEngine&, Sync_Ocean_FlyingIsland_PTR);
    virtual ~OceanLevel();
    void init() override;
    void update(float) override;
    void prepareEnter() override;
    void beforeLeave() override;

    void callDebug() override;

protected:
    BulletEngine& _physEngine;
    int _artifactIndex;
    float _timeOnSlot = 0;

    struct SlotArtifact
    {
        std::string name;
        vec3 offset = vec3(0,0,0);
        interface::MeshInstance* inst;
        int resetButtonIndex;
        interface::MeshInstance* portal;

        float timeOn = 0;
        bool resetActive = false;
        bool onReset = false;
    };
    vector<SlotArtifact> _slots;

    resource::SoundAsset _warpSound;
    resource::SoundAsset _buttonSound;
    Source* _ambientOceanSource = nullptr;

    float _timerButtonSound = 0;

    SlotArtifact createSlotArtifact(std::string nameSlot, std::string resetButton, std::string portal, bool alreadyActive = false);

    int _levelState = 0;
    float _timeOnBoat = 0;
    float _distanceBoat = 0;
    const float DIST_BOAT = 1.6;

    Sync_Ocean_FlyingIsland_PTR _syncBoat;

    void manageBoat(float time);
    void moveBoat(float time, int startBoatId, int arrivalBoatId, bool);
};


#endif // OCEANLEVEL_H
