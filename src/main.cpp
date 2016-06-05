#include <cstdint>
#include <cmath>
#include <map>

#include <SDL2/SDL.h>
#include <bx/fpumath.h>

#include "vbeat.hpp"
#include "fs.hpp"
#include "video.hpp"
#include "mesh.hpp"

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

namespace {
	void perspective(
		float *_result,
		float aspect,
		float fovy     = 60.0f,
		float zNear    = 0.1f,
		bool  infinite = true,
		float zFar     = 1000.0f
	) {
		memset(_result, 0, sizeof(float)*16);

		float range  = tan(fovy / 2.0f);
		_result[11] = -1.0f;

		if (infinite) {
			range *= zNear;

			const float ep     = 0.0f;
			const float left   = -range * aspect;
			const float right  =  range * aspect;
			const float bottom = -range;
			const float top    =  range;

			_result[0] = (2.0f * zNear) / (right - left);
			_result[5] = (2.0f * zNear) / (top - bottom);
			_result[9] = ep - 1.0f;
			_result[14] = (ep - 2.0f) * zNear;
		}
		else {
			_result[0] = 1.0f / (aspect * range);
			_result[5] = 1.0f / (range);
			_result[9] = -(zFar + zNear) / (zFar - zNear);
			_result[14] = -(2.0f * zFar * zNear) / (zFar - zNear);
		}

	}
}

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
#ifndef VBEAT_DEBUG
	// Don't start into HMD mode in debug builds.
	| BGFX_RESET_HMD
#endif
	;

const auto handle_events = [](game_state &gs) {
#ifdef VBEAT_USE_SIXENSE
	sixenseAllControllerData acd;
	for (int base = 0; base < sixenseGetMaxBases(); base++) {
		sixenseSetActiveBase(base);
		sixenseGetAllNewestData(&acd);
		for (int con = 0; con < sixenseGetMaxControllers(); con++) {
			if (!sixenseIsControllerEnabled(con)) {
				break;
			}
			// HANDLE EVERYTHING
		}
	}
#endif

	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
			case SDL_QUIT: {
				gs.queue_quit = true;
				break;
			}
			case SDL_KEYDOWN: {
				if (e.key.keysym.sym == SDLK_F5) {
					if (!bgfx::getHMD()) {
						break;
					}
					if (reset_flags & BGFX_RESET_HMD) {
						reset_flags ^= BGFX_RESET_HMD;
					}
					else {
						reset_flags |= BGFX_RESET_HMD;
					}
					bgfx::reset(gs.width, gs.height, reset_flags);
				}
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
	float at[3]  = { 0.0f, 0.0f, 0.5f };
	float eye[3] = { 0.0f, -5.0f, 0.0f };
	float up[3]  = { 0.0f, 0.0f, 1.0f };

	float x90 = bx::toRad(-90.0f);
	float fov = bx::toRad(60.0f);
	float aspect = float(gs.width) / float(gs.height);

	float view[16], rot[16], tmp[16];
	bx::mtxTranslate(tmp, 0.0f, 2.0f, -1.6f);
	bx::mtxRotateX(rot, -x90);
	bx::mtxMul(view, tmp, rot);// rot, tmp);

	::perspective(tmp, aspect, fov);//, 0.01f, true, 1000.0f);
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

namespace vbeat {
	template<typename T>
	class world {
		std::vector<T>   entities;
		std::map<T, size_t> entity_lookup;
		T add_entity(T e) {
			entities.push_back(e);
			entity_lookup[e] = entities.size();
			return e;
		}
		T remove_entity(T e) {
			auto i = entity_lookup[e];
			entity_lookup.erase(e);
			
		}
	};
}

int main(int, char **argv) {
	fs::state vfs(argv[0]);

#ifdef VBEAT_DEBUG
	setvbuf(stdout, NULL, _IONBF, 0);
#endif

	vbeat::world<int> world;
	// world.add_entity(e);

	SDL_InitSubSystem(SDL_INIT_EVENTS);
	SDL_InitSubSystem(SDL_INIT_TIMER);

	struct {
		game_state previous;
		game_state current;
	} gs;

	std::string title = "Varibeat";

	gs.current.width  = 1280;
	gs.current.height = 720;

	if (!video::open(gs.current.width, gs.current.height, title)) {
		return EXIT_FAILURE;
	}

	bgfx::reset(gs.current.width, gs.current.height, reset_flags);
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

	while (!gs.current.finished) {
		handle_events(gs.current);

		double now = get_time();
		double delta = now - last;
		last = now;

		uint8_t color = 0x2f;
		if (lag > timestep / 1.5) {
			color = 0x9f;
		}
		bgfx::dbgTextPrintf(1, 1, color, "Lag: %0.05f", lag);

		// allow the game to slowmo if delta is too big.
		// TODO: allow frame skipping to prevent desync instead.
		lag += fmin(delta, max_delta);

		// update game logic as lag permits
		while (lag >= timestep) {
			lag -= timestep;
			peak = fmax(peak, lag);

			gs.current.tick++;
			gs.previous = gs.current;
			update(gs.current); // update at a fixed rate each time
		}

		bgfx::dbgTextPrintf(14, 1, 0x8f, "Peak: %0.05f", peak);

		// how far between frames is this?
		auto alpha = lag / timestep;
		auto state = interpolate(gs.previous, gs.current, alpha);
		bgfx::dbgTextPrintf(28, 1, 0x8f,  "Mix: %3.02f%%", alpha * 100.0);

		// present what we've got.
		render(state);
	}

	for (auto &m : meshes) {
		delete m;
	}
	bgfx::destroyProgram(program);

	video::close();

	SDL_Quit();

	return EXIT_SUCCESS;
}
