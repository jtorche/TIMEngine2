#include "Level1.h"
#include "interface/ShaderPool.h"
#include "SimpleSpecProbeImportExport.h"

Level1::Level1(int index, LevelSystem* system) : LevelInterface(index, system)
{
    _indexPortal = 0;
}

void Level1::init()
{
#if 0
    LevelSystem::Level& l = level();
    for(size_t i=0 ; i<l.objects.size() ; ++i)
    {
        if(l.physObjects[i])
            registerPortableTraversable(-1, l.objects[i].meshInstance, l.physObjects[i], {});
    }

    int index = indexObject("soundTest");
    if(index >= 0 && l.physObjects[index])
    {
        BulletObject* bo = l.physObjects[index];
        bo->body()->setCollisionFlags(bo->body()->getCollisionFlags() |
                                      btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

        bo->body()->setUserIndex(102);
    }

    vector<int> indexs = indexObjects("soundTest2");
    for(int i : indexs)
    {
        BulletObject* bo = l.physObjects[i];
        bo->body()->setCollisionFlags(bo->body()->getCollisionFlags() |
                                      btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

        bo->body()->setUserIndex(103);
    }
    indexs = indexObjects("soundTest3");
    for(int i : indexs)
    {
        BulletObject* bo = l.physObjects[i];
        bo->body()->setCollisionFlags(bo->body()->getCollisionFlags() |
                                      btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

        bo->body()->setUserIndex(104);
    }
#endif
}

void Level1::update(float)
{

}

void Level1::prepareEnter()
{

}

