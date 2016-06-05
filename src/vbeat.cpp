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

double get_time() {
	return double(SDL_GetPerformanceCounter()) / SDL_GetPerformanceFrequency();
}

#include "lodepng.h"

void* lodepng_malloc(size_t size) {
	return bx::alloc(vbeat::get_allocator(), size);
}

void* lodepng_realloc(void* ptr, size_t new_size) {
	return bx::realloc(vbeat::get_allocator(), ptr, new_size);
}

void lodepng_free(void* ptr) {
	bx::free(vbeat::get_allocator(), ptr);
}

struct entity_t {
	entity_t() :
		loaded(false),
		additive(false)
	{
		bx::mtxIdentity(this->xform);
	}
	~entity_t() {
		if (!this->loaded) {
			return;
		}
		printf("deleted\n");
		bgfx::destroyVertexBuffer(this->vbo);
		bgfx::destroyIndexBuffer(this->ibo);
		bgfx::destroyTexture(this->tex);
		this->loaded = false;
	}
	bool loaded;
	bool additive;
	bgfx::VertexBufferHandle vbo;
	bgfx::IndexBufferHandle  ibo;
	bgfx::TextureHandle      tex;
	float xform[16];
};

#include "bitmap_font.hpp"

entity_t *new_sprite(const std::string &filename, float x, float y, bool additive = false) {
	entity_t *entity = new entity_t();

	unsigned w, h;
	std::vector<unsigned char> pixels;
	std::vector<unsigned char> file_data;
	fs::read_vector(file_data, filename);
	unsigned err = lodepng::decode(pixels, w, h, file_data);
	if (err) {
		printf("fuck\n");
		return nullptr;
	}

	entity->tex = bgfx::createTexture2D(w, h, 0, bgfx::TextureFormat::RGBA8, 0, bgfx::copy(pixels.data(), w*h*4));

	struct vertex_t {
		float x, y;
		float u, v;
	};
	vertex_t verts[] = {
		// top left
		{ 0.f, 0.f,           0.f, 0.f },
		// top right
		{ (float)w, 0.f,      1.f, 0.f },
		// bottom right
		{ (float)w, (float)h, 1.f, 1.f },
		// bottom left
		{ 0.f, (float)h,      0.f, 1.f }
	};


	bgfx::VertexDecl decl;
	decl.begin()
		.add(bgfx::Attrib::Position,  2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		.end();
	entity->vbo = bgfx::createVertexBuffer(bgfx::copy(verts, sizeof(verts)), decl);

	/*  [0] -----> [1]
	 *   ^  -\   A  |
	 *   |    -\    |
	 *   | B    -\  v
	 *  [3]<-------[2] */
	uint16_t indices[] = {
		0, 1, 2,
		0, 2, 3
	};

	entity->ibo = bgfx::createIndexBuffer(bgfx::copy(indices, sizeof(indices)));

	bx::mtxTranslate(entity->xform, bx::fabsolute(x), bx::fabsolute(y), 0);
	entity->additive = additive;

	entity->loaded = true;

	return entity;
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

	auto sampler = bgfx::createUniform("s_tex_color", bgfx::UniformType::Int1);
	auto program = bgfx::createProgram(
		bgfx::createShader(fs::read_mem("shaders/sprite.vs.bin")),
		bgfx::createShader(fs::read_mem("shaders/sprite.fs.bin")),
		true
	);
	auto dfield = bgfx::createProgram(
		bgfx::createShader(fs::read_mem("shaders/sprite.vs.bin")),
		bgfx::createShader(fs::read_mem("shaders/distance-field.fs.bin")),
		true
	);

	double last = get_time();

	std::vector<entity_t*> entities;
	entities.push_back(new_sprite("buttons_oxygen.png", 50, 100));
	entities.push_back(new_sprite("notes_oxygen.png", 50, 40));
	entities.push_back(new_sprite("laneglow-small_oxygen.png", 50, 75, true));

	bitmap_font fnt;
	fnt.load("fonts/helvetica-neue-55.fnt");
	fnt.set_text(
		"test text look at me it's\n"
		"multi-line text"
	);

	float view[16], proj[16], text[16];
	bx::mtxIdentity(view);
	bx::mtxTranslate(text, 200, 200, 0);
	bx::mtxOrtho(proj, 0.f, gs.width, gs.height, 0.f, -10.f, 10.f);
	bgfx::setViewTransform(0, view, proj);
	bgfx::setViewRect(0, 0, 0, gs.width, gs.height);
	bgfx::setViewScissor(0, 0, 0, gs.width, gs.height);

	while (!gs.finished) {
		handle_events(gs);
		update(gs);

		double now = get_time();
		// double delta = now - last;
		last = now;

		bgfx::touch(0);
		bgfx::setViewClear(0,
			BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
			0x080808ff,
			1.0f
		);

		uint64_t default_state = 0
			| BGFX_STATE_RGB_WRITE
			| BGFX_STATE_CULL_CCW
			| BGFX_STATE_DEPTH_TEST_LEQUAL;
		for (auto &m : entities) {
			uint64_t state = default_state
				| BGFX_STATE_DEPTH_WRITE
				| (m->additive ? BGFX_STATE_BLEND_ADD : BGFX_STATE_BLEND_ALPHA);

			bgfx::setTexture(0, sampler, m->tex);
			bgfx::setTransform(m->xform);
			bgfx::setVertexBuffer(m->vbo);
			bgfx::setIndexBuffer(m->ibo);
			bgfx::setState(state);
			bgfx::submit(0, program);
		}

		bgfx::setTexture(0, sampler, fnt.tex);
		bgfx::setTransform(text);
		bgfx::setVertexBuffer(fnt.vbo);
		bgfx::setIndexBuffer(fnt.ibo);
		bgfx::setState(default_state | BGFX_STATE_BLEND_ALPHA);
		bgfx::submit(0, dfield);

		bgfx::frame();
		bgfx::dbgTextClear();
	}

	for (auto &e : entities) {
		delete e;
	}

	bgfx::destroyProgram(program);

	video::close();

	SDL_Quit();

	return EXIT_SUCCESS;
}
