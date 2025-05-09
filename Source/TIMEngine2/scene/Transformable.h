#ifndef TRANSFORMABLE_H_INCLUDED
#define TRANSFORMABLE_H_INCLUDED

#include "core.h"
#include "Sphere.h"
#include "BasicScene.h"

#include "MemoryLoggerOn.h"
namespace tim
{
    using namespace core;
namespace scene
{
    class Transformable : NonCopyable
    {
        friend class BasicScene<Transformable>;

    public:
        const Sphere& volume() const { return _volume; }

        void setEnable(bool b) { _enable=b; }
        bool enabled() const { return _enable; }

    protected:
        void setVolume(const Sphere& s) { _volume = s; }

        Transformable() = default;
        virtual ~Transformable() = 0;

    private:
        Sphere _volume;
        bool _enable = true;

        mutable BasicScene<Transformable>::TransformableInfo _containerInfo;
    };

    inline Transformable::~Transformable() {}
}
}
#include "MemoryLoggerOff.h"


#endif // TRANSFORMABLE_H_INCLUDED
