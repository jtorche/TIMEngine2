#include "MainHelper.h"
#include "renderer/GLState.h"
#include "interface/ShaderPool.h"
#include <SDL_image.h>

#undef interface
using namespace tim;

SDL_Window *g_pWindow;
SDL_GLContext g_contexteOpenGL;

void initContextSDL(uint x, uint y)
{
	/* Initialisation simple */
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		LOG("Echec de l'initialisation de la SDL\n"); //out(SDL_GetError()); //out("\n");
		system("pause");
		return;
	}

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	g_pWindow = SDL_CreateWindow("SDL2", SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		x, y,
		SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL /* | SDL_WINDOW_FULLSCREEN*/);
	g_contexteOpenGL = SDL_GL_CreateContext(g_pWindow);

	//SDL_ShowCursor(SDL_DISABLE);
    SDL_SetWindowGrab(g_pWindow, SDL_TRUE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

	if (g_contexteOpenGL == 0)
	{
		LOG(SDL_GetError(), "\n");
		system("pause");
		return;
	}
    renderer::openGL.setViewPort({ 0,0 }, { x, y });

	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
}

void swapBuffer()
{
    SDL_GL_SwapWindow(g_pWindow);
}

void delContextSDL()
{
	IMG_Quit();
	SDL_GL_DeleteContext(g_contexteOpenGL);
	SDL_DestroyWindow(g_pWindow);
	SDL_Quit();
}
