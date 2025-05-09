#include "ForestLevel.h"
#include "CollisionMask.h"
#include "resource/AssetManager.h"
#include "bullet/BulletObject.h"
#include "openAL/Source.hpp"
#include "PortalGame.h"
#include "Rand.h"

#include "MemoryLoggerOn.h"


ForestLevelBase::ForestLevelBase(int index, LevelSystem* system, BulletEngine& phys) : LevelInterface(index, system), _physEngine(phys)
{
    _birds = resource::AssetManager<resource::SoundAsset>::instance().load<false>("soundBank/birds1.wav", false, Sampler::NONE).value();
    _warp = resource::AssetManager<resource::SoundAsset>::instance().load<false>("soundBank/warp.wav", false, Sampler::NONE).value();
}

void ForestLevelBase::init()
{
    for(size_t i=0 ; i<level().objects.size() ; ++i)
    {
        if(level().physObjects[i] && level().objects[i].model == "caisse")
        {
            bindSound(level().physObjects[i], PortalGame::SoundEffects::WOOD3);
        }

        else if(level().physObjects[i] && (level().objects[i].model == "kapla1" || level().objects[i].model == "kapla2"))
        {
            bindSound(level().physObjects[i], PortalGame::SoundEffects::WOOD1);
        }
    }

    interface::XmlMeshAssetLoader assets;
    if (assets.load("meshBank/sunStone.xml")) {
        _sunStoneMesh[0] = assets.getMesh("sunStone_dark", interface::Texture::sDefaultConfig);
        _sunStoneMesh[1] = assets.getMesh("sunStone_light", interface::Texture::sDefaultConfig);
        _sunStoneRockMesh[0] = assets.getMesh("sunStoneRock_dark", interface::Texture::sDefaultConfig);
        _sunStoneRockMesh[1] = assets.getMesh("sunStoneRock_light", interface::Texture::sDefaultConfig);
    }

}

void ForestLevelBase::update(float time)
{
    static float birdAccumulator = 0;
    birdAccumulator += time;
    if(birdAccumulator > 5)
    {
        birdAccumulator = 0;
        if(Rand::rand() % 4 == 0)
        {
            Source* src = levelSystem().listener().addSource(_birds);

            vec3 p(Rand::frand() * 30 - 15, Rand::frand() * 30 - 15, 2 + Rand::frand()*8);
            if(p.to<2>().length2() < 40)
                p.resize(15);

            p += levelSystem().listener().transform().translation();

            src->setPosition(p);
            src->play();
            src->release();
        }
    }
}

void ForestLevelBase::emitSounddPortal(const vec3& p)
{
    emitSound(p, _warp);
}


ForestLevel1::ForestLevel1(int index, LevelSystem* system, BulletEngine& phys, std::string namePortal)
    : ForestLevelBase(index, system, phys), _namePortal(namePortal)
{
}

void ForestLevel1::init()
{
    ForestLevelBase::init();
    _indexPortal = indexObject(_namePortal);
    if(_indexPortal < 0)
        LOG(_namePortal, " not found");
    else
        setEnablePortal(false, level().objects[_indexPortal].meshInstance);

    int index = indexObject("sunStone");
    if(index >= 0)
        _instSunStone = level().objects[index].meshInstance;
    else
    {
        _instSunStone = nullptr;
        LOG("In forest1, no sunStone found");
    }
}

void ForestLevel1::update(float time)
{
    ForestLevelBase::update(time);

    BulletObject::CollisionPoint p;
    vec3 stp = _instSunStone->matrix().translation() + vec3(0,0,0.05);
    bool allOk = !BulletObject::rayCastFirst(stp, stp - level().levelScene->globalLight.dirLights[0].direction * 30,
                                             p, *_physEngine.dynamicsWorld[index()]);

    if(allOk && _first)
    {
        setEnablePortal(true, level().objects[_indexPortal].meshInstance);

        int index = indexObject("sunStone");

        if(index >= 0)
        {
            interface::MeshInstance* inst = level().objects[index].meshInstance;
            inst->setMesh(_sunStoneMesh[1]);
        }

        _first = false;
        emitSounddPortal(stp - vec3(0,0,0.1));
    }
    else if(!_first)
    {
        setEnablePortal(true, level().objects[_indexPortal].meshInstance);
    }

#ifdef AUTO_SOLVE
    setEnablePortal(true, level().objects[_indexPortal].meshInstance);
#endif
}

ForestLevel2::ForestLevel2(int index, LevelSystem* system, BulletEngine& phys) : ForestLevelBase(index, system, phys)
{

}

void ForestLevel2::init()
{
    ForestLevelBase::init();

    int index = indexObject("sunStone1");
    if(index >= 0)
        _sunStone[0] = level().objects[index].meshInstance;

    vector<int> sts = indexObjects("sunStone2");
    for(size_t i=0 ; i<std::max((uint)sts.size(), 4u) ; ++i)
        if(sts[i] >= 0)
            _sunStone[i+1] = level().objects[sts[i]].meshInstance;

    sts = indexObjects("sunStone3");
    for(size_t i=0 ; i<std::max((uint)sts.size(), 2u) ; ++i)
        if(sts[i] >= 0)
            _sunStone[i+5] = level().objects[sts[i]].meshInstance;

    _indexPortal = indexObject("portalForest2Out_Forest3In");
}

void ForestLevel2::update(float time)
{
    ForestLevelBase::update(time);

    if(_updateRate++ % 10 != 0)
        return;

    bool state[7] = {true};
    for(int i=0 ; i<7 ; ++i)
    {
        if(!_sunStone[i]) continue;

        BulletObject::CollisionPoint p_tmp;
        vec3 p;
        if(i == 0)
            p = _sunStone[i]->matrix().translation();// + vec3(-0.05,0,0);
        else
            p = _sunStone[i]->matrix().translation() + vec3(0,0,0.85);

        state[i] = BulletObject::rayCastFirst(p, p - level().levelScene->globalLight.dirLights[0].direction * 30,
                                              p_tmp, *_physEngine.dynamicsWorld[index()]);

        if(i == 0)
            _sunStone[i]->setMesh(_sunStoneMesh[state[i] ? 0:1]);
        else
            _sunStone[i]->setMesh(_sunStoneRockMesh[state[i] ? 0:1]);
    }

    if(state[0] && !state[1] && !state[2] && !state[3] && !state[4] && state[5] && state[6])
    {
        if(_indexPortal >= 0)
            setEnablePortal(true, level().objects[_indexPortal].meshInstance);
    }
    else
    {
        if(_indexPortal >= 0)
            setEnablePortal(false, level().objects[_indexPortal].meshInstance);
    }

#ifdef AUTO_SOLVE
    setEnablePortal(true, level().objects[_indexPortal].meshInstance);
#endif
}

ForestLevel3::ForestLevel3(int index, LevelSystem* system, BulletEngine& phys) : ForestLevelBase(index, system, phys)
{

}

void ForestLevel3::init()
{
    ForestLevelBase::init();

    _indexArtifact = indexObject("artifact");
    _indexSlot = indexObject("artifactSlot");
    _indexPortal = indexObject("portalForest3Out_GroveIn");
    _indexArtifactInPlace = indexObject("artifactInPlace");

    if(_indexArtifact >= 0)
    {
        registerPortableTraversable(_indexArtifact, level().objects[_indexArtifact].meshInstance, level().physObjects[_indexArtifact], {});
        level().physObjects[_indexArtifact]->setMask(CollisionTypes::COL_IOBJ, IOBJECT_COLLISION);
        _nonActivatedArtifactMesh = level().objects[_indexArtifact].meshInstance->mesh();

        bindSound(level().physObjects[_indexArtifact], PortalGame::SoundEffects::METAL1);
    }

    if(_indexPortal >= 0)
        setEnablePortal(false, level().objects[_indexPortal].meshInstance);
}

void ForestLevel3::update(float time)
{
    ForestLevelBase::update(time);

    if(_indexArtifact >= 0 && _indexSlot >= 0)
    {
        static float timeOnSlot = 0;
        static bool justInPlace = false;

        btTransform tr = level().physObjects[_indexArtifact]->body()->getWorldTransform();
        vec3 pos(tr.getOrigin().getX(), tr.getOrigin().getY(), tr.getOrigin().getZ());
        vec3 posSlot = level().objects[_indexSlot].meshInstance->matrix().translation();
        float dist = (pos - posSlot - vec3(0,0,1.2)).length();

        if(dist < 0.2)
            timeOnSlot += time;
        else
            timeOnSlot = 0;

        if(level().physObjects[_indexArtifact]->body()->getLinearVelocity().length() > 0.1)
            timeOnSlot = 0;

        if(timeOnSlot > 2 && _indexArtifactInPlace >= 0)
        {
            level().objects[_indexArtifact].meshInstance->setMesh(level().objects[_indexArtifactInPlace].meshInstance->mesh());

            if(!justInPlace)
            {
                justInPlace = true;
                emitSounddPortal({14.069, 15.393, 1.5});

                if(_indexPortal >= 0)
                    setEnablePortal(true, level().objects[_indexPortal].meshInstance);
            }
        }
        else
        {
            justInPlace = false;
            level().objects[_indexArtifact].meshInstance->setMesh(_nonActivatedArtifactMesh);

            //if(_indexPortal >= 0)
            //    setEnablePortal(false, level().objects[_indexPortal].meshInstance);

        #ifdef AUTO_SOLVE
            if(_indexPortal >= 0)
                setEnablePortal(true, level().objects[_indexPortal].meshInstance);
        #endif
        }
    }
}
