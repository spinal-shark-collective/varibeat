#include <SDL2/SDL.h>
#include <bx/bx.h>
#include <bx/crtimpl.h>
#include <bx/fpumath.h>

#include <stack>

#include "vbeat.hpp"
#include "fs.hpp"
#include "window.hpp"
#include "math.hpp"
#include "graphics/bitmap_font.hpp"
#include "graphics/texture.hpp"
#include "graphics/sprite_batch.hpp"

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

bx::AllocatorI* vbeat::get_allocator()
{
	static bx::CrtAllocator s_allocator;
	return &s_allocator;
}

// c-style allocs... everything that allocates calls these.
void *vbeat::v_malloc(size_t bytes) {
	return bx::alloc(vbeat::get_allocator(), bytes);
}

void *vbeat::v_realloc(void *ptr, size_t new_size) {
	return bx::realloc(vbeat::get_allocator(), ptr, new_size);
}

void vbeat::v_free(void *ptr) {
	bx::free(vbeat::get_allocator(), ptr);
}

// c++-style allocs. redirects to v_*
void* operator new(size_t sz) {
	void *ptr = vbeat::v_malloc(sz);
	if (!ptr) {
		throw std::bad_alloc();
	}
	return ptr;
}

void operator delete(void* ptr) VBEAT_NOEXCEPT
{
	vbeat::v_free(ptr);
}

#include "widgets/widget.hpp"

struct screen_t : widget_t {
	widget_t *focused;

	std::vector<widget_t*> widgets;

	void input(const input_event_t &e) {
		this->focused->input(e);
	}

	void update(double delta) {
		for (auto &w : this->widgets) {
			w->update(delta);
		}
	}

	void draw() {
		for (auto &w : this->widgets) {
			w->draw();
		}
	}
};

struct game_state_t {
	game_state_t():
		queue_quit(false),
		finished(false),
		width(0),
		height(0),
		tick(0)
	{}
	game_state_t(const game_state_t &other):
		queue_quit(other.queue_quit),
		finished(other.finished),
		width(other.width),
		height(other.height),
		tick(other.tick)
	{}
	std::stack<screen_t*> screens;
	std::stack<input_event_t> events;
	bool queue_quit, finished;
	int width, height;
	uint64_t tick;
};

uint32_t debug_flags = 0
#ifdef VBEAT_DEBUG
	| BGFX_DEBUG_TEXT
#endif
	;
uint32_t reset_flags = 0
	| BGFX_RESET_VSYNC
	| BGFX_RESET_DEPTH_CLAMP
	| BGFX_RESET_SRGB_BACKBUFFER
	| BGFX_RESET_FLIP_AFTER_RENDER
	| BGFX_RESET_FLUSH_AFTER_RENDER
	;

const auto handle_events = [](game_state_t &gs) {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
			case SDL_QUIT: {
				gs.queue_quit = true;
				break;
			}
			case SDL_KEYDOWN: {
				input_event_t ie;
				ie.key = e.key.keysym.sym;
				gs.events.push(ie);
			}
			case SDL_KEYUP: {
				if (e.key.keysym.sym == SDLK_ESCAPE) {
					gs.queue_quit = true;
				}
				if (e.key.keysym.sym == SDLK_1) {
					debug_flags ^= BGFX_DEBUG_STATS;
					bgfx::setDebug(debug_flags);
				}
				if (e.key.keysym.sym == SDLK_2) {
					debug_flags ^= BGFX_DEBUG_WIREFRAME;
					bgfx::setDebug(debug_flags);
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

	while (!gs.events.empty()) {
		gs.screens.top()->input(gs.events.top());
		gs.events.pop();
	}
};

#include "widgets/font_test.hpp"
#include "widgets/notefield.hpp"

double get_time() {
	return double(SDL_GetPerformanceCounter()) / SDL_GetPerformanceFrequency();
}

int main(int, char **argv) {
	fs::state vfs(argv[0]);

#ifdef VBEAT_DEBUG
	setvbuf(stdout, NULL, _IONBF, 0);
#endif

	SDL_InitSubSystem(SDL_INIT_EVENTS);
	SDL_InitSubSystem(SDL_INIT_TIMER);

	game_state_t gs;

	std::string title = "Varibeat";

	gs.width  = 1280;
	gs.height = 720;

	if (!video::open(gs.width, gs.height, title)) {
		return EXIT_FAILURE;
	}

	bgfx::reset(gs.width, gs.height, reset_flags);
	bgfx::setDebug(debug_flags);

	screen_t *_s = new screen_t();
	// XXX: why isn't the widget_t constructor working?
	notefield_t *w = new notefield_t();
	w->parent = _s;
	w->init();
	_s->widgets.push_back(w);
	_s->focused = w;

	font_test_t *f = new font_test_t();
	f->parent = _s;
	f->init();
	_s->widgets.push_back(f);

	gs.screens.push(_s);

	float view[16], proj[16];
	bx::mtxIdentity(view);
	bx::mtxOrtho(proj, 0.f, float(gs.width), float(gs.height), 0.f, -10.f, 10.f);
	bgfx::setViewTransform(0, view, proj);
	bgfx::setViewRect(0, 0, 0, gs.width, gs.height);
	bgfx::setViewScissor(0, 0, 0, gs.width, gs.height);

	double last = get_time();
	while (!gs.finished) {
		handle_events(gs);

		if (gs.queue_quit) {
			gs.finished = true;
		}

		double now = get_time();
		double delta = now - last;
		last = now;

		bgfx::touch(0);
		bgfx::setViewClear(0,
			BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
			0x080808ff,
			1.0f
		);

		auto &s = gs.screens.top();
		s->update(delta);
		s->draw();

		bgfx::frame();
		bgfx::dbgTextClear();
	}

	while (!gs.screens.empty()) {
		screen_t *top = gs.screens.top();
		for (auto &w : top->widgets) {
			delete w;
		}
		delete top;
		gs.screens.pop();
	}

	graphics::unload_textures();

	video::close();

	SDL_Quit();

	return EXIT_SUCCESS;
}
