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
#include "bitmap_font.hpp"
#include "texture.hpp"
#include "sprite_batch.hpp"

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

#include <stack>

struct input_event_t {
	SDL_Keycode key;
};

struct widget_t {
	widget_t *parent;
	widget_t(): parent(nullptr) {}
	widget_t(widget_t *_parent): parent(_parent) {}
	virtual ~widget_t() {}

	virtual void init() {}
	virtual void input(const input_event_t &) {};
	virtual void update(double dt) = 0;
	virtual void draw() = 0;
};

struct screen_t : public widget_t {
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
	std::stack<screen_t*> screens;
	std::stack<input_event_t> events;
	bool queue_quit, finished;
	int width, height;
	uint64_t tick;
};

uint32_t debug_flags = 0
#ifdef VBEAT_DEBUG
	| BGFX_DEBUG_TEXT
	// | BGFX_DEBUG_STATS
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

const auto update = [](game_state &gs) {
	if (gs.queue_quit) {
		gs.finished = true;
		return;
	}
};

void add_sprite(video::sprite_batch *batch, float x, float y, float *rect = nullptr) {
	float umin = 0.f;
	float vmin = 0.f;
	float umax = 1.f;
	float vmax = 1.f;

	video::texture_t *tex = batch->texture;

	float w = (float)tex->w;
	float h = (float)tex->h;

	if (rect) {
		umin = rect[0]/w;
		vmin = rect[1]/h;
		umax = rect[2]/w;
		vmax = rect[3]/h;

		w = rect[2] - rect[0];
		h = rect[3] - rect[1];
	}

	/*  [0] -----> [1]
	*   ^  -\   A  |
	*   |    -\    |
	*   | B    -\  v
	*  [3]<-------[2] */
	std::vector<video::vertex_t> verts = {
		{ 0.f + x, 0.f + y,           umin, vmin }, // top left
		{ (float)w + x, 0.f + y,      umax, vmin }, // top right
		{ (float)w + x, (float)h + y, umax, vmax }, // bottom right
		{ 0.f + x, (float)h + y,      umin, vmax }  // bottom left
	};
	std::vector<uint16_t> indices = {
		0, 1, 2,
		0, 2, 3
	};
	batch->add(verts, indices);
};

struct notefield_t : public widget_t {
	video::sprite_batch *receptors;
	video::sprite_batch *notes;
	bgfx::UniformHandle sampler;
	bgfx::ProgramHandle program;

	float xform[16];

	void init() {
		sampler = bgfx::createUniform("s_tex_color", bgfx::UniformType::Int1);
		program = bgfx::createProgram(
			bgfx::createShader(fs::read_mem("shaders/sprite.vs.bin")),
			bgfx::createShader(fs::read_mem("shaders/sprite.fs.bin")),
			true
		);
		receptors = new video::sprite_batch(video::get_texture("buttons_oxygen.png"));
		notes     = new video::sprite_batch(video::get_texture("notes_oxygen.png"));

		float mbutton[] = { 22.f, 2.f, 38.f, 22.f };
		// float mbutton[] = { 2.f, 2.f, 22.f, 22.f };
		float hbutton[] = { 42.f, 2.f, 74.f, 22.f };

		// upper buttons
		// float max     = 0;
		float width   = mbutton[2] - mbutton[0];
		float spacing = 10.f;
		for (int i = 0; i < 4; ++i) {
			float x = (width + spacing) * i;
			float y = 0.f;
			// max = x + width;
			add_sprite(receptors, x, y, mbutton);
		}

		// lower buttons
		width   = hbutton[2] - hbutton[0];
		spacing = 14.f;
		float offset = 8.0f;
		for (int i = 0; i < 2; ++i) {
			float x = (width + spacing) * i + offset;
			float y = 20.f;
			add_sprite(receptors, x, y, hbutton);
		}

		receptors->buffer();
	}

	virtual ~notefield_t() {
		bgfx::destroyProgram(program);
		bgfx::destroyUniform(sampler);
		delete receptors;
		delete notes;
	}

	void input(const input_event_t &e) {
		printf("%d\n", e.key);
	}

	void update(double) {
		bx::mtxIdentity(xform);
		notes->clear();
		// add new notes...
		notes->buffer();
	}

	void draw() {
		uint64_t state = 0
			| BGFX_STATE_RGB_WRITE
			| BGFX_STATE_CULL_CCW
			| BGFX_STATE_DEPTH_TEST_LEQUAL
			| BGFX_STATE_DEPTH_WRITE
			| BGFX_STATE_BLEND_ALPHA;

		bgfx::setTexture(0, sampler, notes->texture->tex);
		bgfx::setTransform(xform);
		bgfx::setVertexBuffer(notes->vbo);
		bgfx::setIndexBuffer(notes->ibo);
		bgfx::setState(state);
		bgfx::submit(0, program);

		bgfx::setTexture(0, sampler, receptors->texture->tex);
		bgfx::setTransform(xform);
		bgfx::setVertexBuffer(receptors->vbo);
		bgfx::setIndexBuffer(receptors->ibo);
		bgfx::setState(state);
		bgfx::submit(0, program);
	}
};

struct font_test_t : public widget_t {
	bgfx::UniformHandle sampler;
	bgfx::ProgramHandle dfield;
	bitmap_font *fnt;
	float text[16];

	virtual ~font_test_t() {
		delete fnt;
	}

	void init() {
		sampler = bgfx::createUniform("s_tex_color", bgfx::UniformType::Int1);
		dfield = bgfx::createProgram(
			bgfx::createShader(fs::read_mem("shaders/sprite.vs.bin")),
			bgfx::createShader(fs::read_mem("shaders/distance-field.fs.bin")),
			true
		);

		fnt = new bitmap_font();
		fnt->load("fonts/helvetica-neue-55.fnt");
		fnt->set_text(
			"test text look at me it's\n"
			"multi-line text"
		);
	}

	void update(double) {
		bx::mtxTranslate(text, 200, 200, 0);
	}

	void draw() {
		bgfx::setTexture(0, sampler, fnt->texture->tex);
		bgfx::setTransform(text);
		bgfx::setVertexBuffer(fnt->vbo);
		bgfx::setIndexBuffer(fnt->ibo);
		bgfx::setState(0
			| BGFX_STATE_RGB_WRITE
			| BGFX_STATE_CULL_CCW
			| BGFX_STATE_DEPTH_TEST_LEQUAL
			| BGFX_STATE_BLEND_ALPHA
		);
		bgfx::submit(0, dfield);
	}
};

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

	game_state gs;

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
	bx::mtxOrtho(proj, 0.f, gs.width, gs.height, 0.f, -10.f, 10.f);
	bgfx::setViewTransform(0, view, proj);
	bgfx::setViewRect(0, 0, 0, gs.width, gs.height);
	bgfx::setViewScissor(0, 0, 0, gs.width, gs.height);

	double last = get_time();
	while (!gs.finished) {
		handle_events(gs);
		update(gs);

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

		// s->input(e);
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

	video::unload_textures();

	video::close();

	SDL_Quit();

	return EXIT_SUCCESS;
}
