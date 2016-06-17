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

// c-style allocs... everything that allocates calls these.
void *vbeat::v_malloc(size_t bytes) {
	// puts("allocating some shit");
	return bx::alloc(vbeat::get_allocator(), bytes);
}

void *vbeat::v_realloc(void *ptr, size_t new_size) {
	return bx::realloc(vbeat::get_allocator(), ptr, new_size);
}

void vbeat::v_free(void *ptr) {
	// puts("deallocating some shit");
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

void operator delete(void* ptr)
{
	vbeat::v_free(ptr);
}

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

void add_sprite(video::sprite_batch_t *batch, float x, float y, float *rect = nullptr) {
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

static uint32_t good = 200;
static uint32_t great = 50;

struct notefield_t : widget_t {
	video::sprite_batch_t *receptors;
	video::sprite_batch_t *notes;
	bgfx::UniformHandle sampler;
	bgfx::ProgramHandle program;

	float xform[16];

	double time;

	struct note_row_t {
		uint32_t ms;
		uint8_t  columns;
	};

	struct judge_row_t {
		note_row_t *row;
		int16_t  offset;
	};

	std::vector<note_row_t> note_data;
	std::vector<judge_row_t> judge_data;
	std::deque<judge_row_t> judging;

	void init() {
		sampler = bgfx::createUniform("s_tex_color", bgfx::UniformType::Int1);
		program = bgfx::createProgram(
			bgfx::createShader(fs::read_mem("shaders/sprite.vs.bin")),
			bgfx::createShader(fs::read_mem("shaders/sprite.fs.bin")),
			true
		);
		receptors = new video::sprite_batch_t(video::get_texture("buttons_oxygen.png"));
		notes     = new video::sprite_batch_t(video::get_texture("notes_oxygen.png"));

		static float mbutton[] = { 22.f, 2.f, 38.f, 22.f };
		// static float mbutton[] = { 2.f, 2.f, 22.f, 22.f };
		static float hbutton[] = { 42.f, 2.f, 74.f, 22.f };

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

		/*
		 * columns is a bitfield.
		 * 76543210
		 * ||||||||
		 * |||||||+-- reserved (6key)
		 * |||++++--- notes
		 * ||+------- reserved (6key)
		 * ++-------- holds
		 */
		enum {
			NOTE4_MASK = 0x1E,
			NOTE6_MASK = 0x3F,
			HOLD_MASK  = 0xC0
		};
		#define NOTE(x) 1<<x
		note_data = std::vector<note_row_t> {
			{ 250*2, NOTE(1) | NOTE(4) },
			{ 400*2, NOTE(3) },
			{ 550*2, NOTE(2) },
			{ 700*2, NOTE(3) | NOTE(2) },
			{ 950*2, NOTE(4) }
		};
		#undef NOTE

		time = -1;
	}

	virtual ~notefield_t() {
		bgfx::destroyProgram(program);
		bgfx::destroyUniform(sampler);
		delete receptors;
		delete notes;
	}

	void input(const input_event_t &e) {
		// cowbell simulator
		if (e.key == SDLK_SPACE) {
			int64_t now = uint64_t(this->time * 1000.0);
			/* anything in this list is guaranteed to be a valid hit, so we just
			 * need to compute the offset.
			 *
			 * TODO: we actually only want to use this input for the closest row */
			for (auto &jr : this->judging) {
				if (jr.offset != INT16_MIN) {
					continue;
				}
				int64_t offset = jr.row->ms - now;
				jr.offset = int16_t(offset);
				printf("hit! %ldms\n", offset);
			}
		}
	}

	void update(double dt) {
		this->time += dt;

		float speed = 4;
		static float note_rect[] = { 2.f, 2.f, 22.f, 13.f };

		bx::mtxTranslate(xform, 50, 650, 0);
		notes->clear();

		float width    = note_rect[2] - note_rect[0];
		float spacing  = 6.f;
		float x_offset = -26.0f;
		uint32_t now = uint32_t(this->time * 1000.0);

		for (auto &row : this->note_data) {
			uint32_t earliest_row = now - good;
			uint32_t latest_row   = now + good;

			// Send anything out of range into our final judge data.
			while (!this->judging.empty()
				&& this->judging.front().row->ms < earliest_row
			) {
				judge_row_t &judge_row = this->judging.front();
				if (judge_row.offset == INT16_MIN) {
					puts("miss");
				}
				this->judge_data.push_back(this->judging.front());
				this->judging.pop_front();
			}

			// Add anything ahead in range to our judge list
			if (row.ms > earliest_row && row.ms < latest_row) {
				if (this->judging.empty()
					|| this->judging.back().row->ms < row.ms
				) {
					judge_row_t judge_row = { &row, INT16_MIN };
					this->judging.push_back(judge_row);
				}
			}

			double note_spacing = 32.0;
			double y = note_spacing * speed * -(double(row.ms) / 1000.0 - this->time);
			if (y > 0.0) {
				continue;
			}

			for (uint8_t i = 1; i < 5; ++i) {
				uint8_t note = (row.columns >> i) & 0x1;
				if (!note) {
					continue;
				}
				float x = (width + spacing) * i + x_offset;
				add_sprite(notes, x, float(y), note_rect);
			}
		}

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
		bgfx::setIndexBuffer(notes->ibo, 0, notes->indices.size());
		bgfx::setState(state);
		bgfx::submit(0, program);

		bgfx::setTexture(0, sampler, receptors->texture->tex);
		bgfx::setTransform(xform);
		bgfx::setVertexBuffer(receptors->vbo);
		bgfx::setIndexBuffer(receptors->ibo, 0, receptors->indices.size());
		bgfx::setState(state);
		bgfx::submit(0, program);
	}
};

struct font_test_t : widget_t {
	bgfx::UniformHandle sampler;
	bgfx::ProgramHandle dfield;
	bitmap_font_t *fnt;
	float text[16];

	virtual ~font_test_t() {
		delete fnt;
		bgfx::destroyProgram(dfield);
		bgfx::destroyUniform(sampler);
	}

	void init() {
		sampler = bgfx::createUniform("s_tex_color", bgfx::UniformType::Int1);
		dfield = bgfx::createProgram(
			bgfx::createShader(fs::read_mem("shaders/sprite.vs.bin")),
			bgfx::createShader(fs::read_mem("shaders/distance-field.fs.bin")),
			true
		);

		fnt = new bitmap_font_t();
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

	video::unload_textures();

	video::close();

	SDL_Quit();

	return EXIT_SUCCESS;
}
