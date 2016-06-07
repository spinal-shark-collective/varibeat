#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <bgfx/bgfxplatform.h>

#include "video.hpp"

using namespace vbeat;

namespace {
	SDL_Window *wnd = nullptr;
}

bool video::open(int w, int h, std::string title) {
	printf("Video: Initializing...\n");

	SDL_InitSubSystem(SDL_INIT_VIDEO);

	wnd = SDL_CreateWindow(
		title.c_str(),
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		w, h,
		SDL_WINDOW_OPENGL
	);
	bgfx::sdlSetWindow(wnd);

	/* Undocumented: calling renderFrame() before init() locks BGFX to the
	 * calling thread (forcing it to be single-threaded, reducing latency). */
	bgfx::renderFrame();
	if (!bgfx::init(bgfx::RendererType::OpenGL))
	{
		printf("Video: Unable to obtain rendering context.\n");
		return false;
	}

	printf("Video: Ready.\n");

	return true;
}

void video::close() {
	bgfx::shutdown();
	SDL_DestroyWindow(wnd);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}
