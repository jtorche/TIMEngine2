#ifndef DEVICEFUN_H_INCLUDED
#define DEVICEFUN_H_INCLUDED

#include <thread>

namespace tim
{
namespace renderer
{
    using ThreadID = std::thread::id;

    inline ThreadID getThreadId()
    {
        return std::this_thread::get_id();
    }
}
}

#endif //DEVICEFUN_H_INCLUDED
