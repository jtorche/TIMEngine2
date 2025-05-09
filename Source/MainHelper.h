#ifndef MAINHELPER_H_INCLUDED
#define MAINHELPER_H_INCLUDED

#include "TIM_SDL/SDLInputManager.h"
#include "TIM_SDL/SDLTimer.h"
#include "TIM_SDL/SDLTextureLoader.h"

#include "MemoryLoggerOn.h"

void initContextSDL(uint x, uint y);
void delContextSDL();
void swapBuffer();
extern SDL_Window* g_pWindow;
extern SDL_GLContext g_contexteOpenGL;

template <int I, int N = 100>
float countTime(float time)
{
    static uint counter = 0;
    static float totalTime = 0;
    static float fps = 0;

    counter++;

    totalTime += time;
    if (counter % N == 0)
    {
        fps = totalTime / N;
        counter = 0;
        totalTime = 0;
        return fps;
    }
    return -fps;
}

#include "MemoryLoggerOff.h"

#endif // MAINHELPER_H_INCLUDED
