#ifndef DEVICEFUN_H_INCLUDED
#define DEVICEFUN_H_INCLUDED

#ifdef USE_SDL
#include <SDL.h>
#endif

namespace tim
{
namespace renderer
{

#ifdef USE_SDL
    using ThreadID = SDL_threadID;
#else
    using ThreadID = int;
#endif

    inline ThreadID getThreadId()
    {
    #ifdef USE_SDL
        return SDL_ThreadID();
    #else
        TIM_ASSERT(false);
        return 0;
    #endif
    }
}
}

#endif //DEVICEFUN_H_INCLUDED
