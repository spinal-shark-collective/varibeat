#include <bx/bx.h>
#include <bx/crtimpl.h>
#include "vbeat.hpp"

bx::AllocatorI* vbeat::get_allocator()
{
	static bx::CrtAllocator s_allocator;
	return &s_allocator;
}

#include <cstdint>
#include <cmath>
#include <map>

#include <SDL2/SDL.h>
#include <bx/fpumath.h>

#include "vbeat.hpp"
#include "fs.hpp"
#include "video.hpp"
#include "mesh.hpp"
#include "math.hpp"

using namespace vbeat;

#ifdef VBEAT_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
extern "C" {
	/* Use high-performance modes for NVidia Optimus and AMD's thing.
	* http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
	* https://community.amd.com/thread/169965 */
	VBEAT_EXPORT DWORD NvOptimusEnablement = 1;
	VBEAT_EXPORT DWORD AmdPowerXpressRequestHighPerformance = 1;
}
#endif // VBEAT_WINDOWS

struct game_state {
	game_state():
		queue_quit(false),
		finished(false),
		width(0),
		height(0),
		tick(0)
	{}
	game_state(const game_state &other):
		queue_quit(other.queue_quit),
		finished(other.finished),
		width(other.width),
		height(other.height),
		tick(other.tick)
	{}
	bool queue_quit, finished;
	int width, height;
	uint64_t tick;
};

uint32_t debug_flags = 0
#ifdef VBEAT_DEBUG
	| BGFX_DEBUG_TEXT
	//| BGFX_DEBUG_STATS
#endif
	;
uint32_t reset_flags = 0
	| BGFX_RESET_VSYNC
	| BGFX_RESET_DEPTH_CLAMP
	| BGFX_RESET_SRGB_BACKBUFFER
	| BGFX_RESET_FLIP_AFTER_RENDER
	| BGFX_RESET_FLUSH_AFTER_RENDER
	//| BGFX_RESET_MSAA_X16
	;

const auto handle_events = [](game_state &gs) {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
			case SDL_QUIT: {
				gs.queue_quit = true;
				break;
			}
			case SDL_KEYUP: {
				if (e.key.keysym.sym == SDLK_ESCAPE) {
					gs.queue_quit = true;
				}
				break;
			}
			case SDL_JOYDEVICEADDED: {
				int dev = e.jdevice.which;
				printf("Added device %i (gc: %i)", dev, SDL_IsGameController(dev));
				break;
			}
			case SDL_JOYDEVICEREMOVED: {
				int dev = e.jdevice.which;
				printf("Removed device %i", dev);
				break;
			}
			default: {
				break;
			}
		}
	}
};

const auto interpolate = [](game_state&, game_state &current, double) {
	game_state interp(current);

	return interp;
};

const auto update = [](game_state &gs) {
	if (gs.queue_quit) {
		gs.finished = true;
		return;
	}
};

std::vector<video::mesh*> meshes;
bgfx::ProgramHandle program;

double get_time() {
	return double(SDL_GetPerformanceCounter()) / SDL_GetPerformanceFrequency();
}

const auto render = [](game_state &gs) {
	float x90 = bx::toRad(-90.0f);
	float fov = bx::toRad(60.0f);
	float aspect = float(gs.width) / float(gs.height);

	float view[16], rot[16], tmp[16];
	bx::mtxTranslate(tmp, 0.0f, 2.0f, -1.6f);
	bx::mtxRotateX(rot, -x90);
	bx::mtxMul(view, tmp, rot);// rot, tmp);

	math::perspective(tmp, aspect, fov);//, 0.01f, true, 1000.0f);
	bgfx::setViewTransform(0, view, tmp);
	bgfx::setViewRect(0, 0, 0, gs.width, gs.height);
	bgfx::setViewScissor(0, 0, 0, gs.width, gs.height);

	bgfx::touch(0);

	float xform[16];
	float spin = float(get_time() * 0.5);
	bx::mtxRotateZ(xform, spin);

	bgfx::setViewClear(0,
		BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
		0x101010ff,
		1.0f
	);

	for (auto &m : meshes) {
		bgfx::setTransform(xform);
		bgfx::setVertexBuffer(m->vbo);
		bgfx::setIndexBuffer(m->ibo);
		bgfx::setState(0
			| BGFX_STATE_MSAA
			| BGFX_STATE_ALPHA_WRITE
			| BGFX_STATE_RGB_WRITE
			| BGFX_STATE_CULL_CCW
			| BGFX_STATE_DEPTH_WRITE
			| BGFX_STATE_DEPTH_TEST_LEQUAL
		);
		bgfx::submit(0, program);
	}
	bgfx::frame();
	bgfx::dbgTextClear();
};

int main(int, char **argv) {
	fs::state vfs(argv[0]);

#ifdef VBEAT_DEBUG
	setvbuf(stdout, NULL, _IONBF, 0);
#endif

	SDL_InitSubSystem(SDL_INIT_EVENTS);
	SDL_InitSubSystem(SDL_INIT_TIMER);

	game_state gs;

	std::string title = "Varibeat";

	gs.width  = 1280;
	gs.height = 720;

	if (!video::open(gs.width, gs.height, title)) {
		return EXIT_FAILURE;
	}

	bgfx::reset(gs.width, gs.height, reset_flags);
	bgfx::setDebug(debug_flags);

	program = bgfx::createProgram(
		bgfx::createShader(fs::read_mem("shaders/test.vs.bin")),
		bgfx::createShader(fs::read_mem("shaders/test.fs.bin")),
		true
	);

	video::mesh *tmp = new video::mesh();
	video::read_iqm(*tmp, "models/chair.iqm");
	meshes.push_back(tmp);

	double lag = 0.0;
	double peak = 0.0;
	double last = get_time();

	const double min_framerate = target_framerate / 3.0;
	const double max_delta = 1.0 / min_framerate;

	while (!gs.finished) {
		handle_events(gs);

		double now = get_time();
		double delta = now - last;
		last = now;

		update(gs);

		// present what we've got.
		render(gs);
	}

	for (auto &m : meshes) {
		delete m;
	}
	bgfx::destroyProgram(program);

	video::close();

	SDL_Quit();

	return EXIT_SUCCESS;
}
