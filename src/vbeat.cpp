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

#include "bitmap_font.hpp"
#include "texture.hpp"

struct vertex_t {
	float x, y;
	float u, v;
};

struct sprite_batch {
	bool dirty;
	video::texture_t *texture;

	std::vector<vertex_t> vertices;
	std::vector<uint16_t> indices;

	bgfx::DynamicVertexBufferHandle vbo;
	bgfx::DynamicIndexBufferHandle  ibo;

	sprite_batch(video::texture_t *_texture):
		dirty(true),
		texture(_texture)
	{
		this->texture->refs++;
		const int start_vertices = 16;
		bgfx::VertexDecl decl;
		decl
			.begin()
			.add(bgfx::Attrib::Position,  2, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
		this->vbo = bgfx::createDynamicVertexBuffer(start_vertices, decl, BGFX_BUFFER_ALLOW_RESIZE);
		this->ibo = bgfx::createDynamicIndexBuffer(start_vertices*3, BGFX_BUFFER_ALLOW_RESIZE);
	}

	virtual ~sprite_batch() {
		this->texture->refs--;
		bgfx::destroyDynamicVertexBuffer(this->vbo);
		bgfx::destroyDynamicIndexBuffer(this->ibo);
	}

	void buffer() {
		if (!this->dirty) {
			return;
		}

		bgfx::updateDynamicVertexBuffer(
			this->vbo, 0,
			bgfx::makeRef(
				this->vertices.data(), this->vertices.size() * sizeof(vertex_t)
			)
		);

		bgfx::updateDynamicIndexBuffer(
			this->ibo, 0,
			bgfx::makeRef(
				this->indices.data(), this->indices.size() * sizeof(uint16_t)
			)
		);

		this->dirty = false;
	}

	void add(const std::vector<vertex_t> &_vertices, const std::vector<uint16_t> &_indices) {
		size_t base = this->vertices.size();
		vertices.reserve(vertices.size() + _vertices.size());
		indices.reserve(indices.size() + _indices.size());
		// This, apparently, is the right way to append two vectors.
		// You may note that it's much more verbose than a simple for...
		this->vertices.insert(
			std::end(this->vertices),
			std::begin(_vertices),
			std::end(_vertices)
		);
		for (auto &i : _indices) {
			this->indices.push_back(base + i);
		}
		this->dirty = true;
	}

	void clear() {
		this->dirty = true;
		vertices.clear();
		indices.clear();
	}
};

void add_sprite(sprite_batch &batch, float x, float y, float *rect = nullptr) {
	float umin = 0.f;
	float vmin = 0.f;
	float umax = 1.f;
	float vmax = 1.f;

	video::texture_t *tex = batch.texture;

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
	std::vector<vertex_t> verts = {
		{ 0.f + x, 0.f + y,           umin, vmin }, // top left
		{ (float)w + x, 0.f + y,      umax, vmin }, // top right
		{ (float)w + x, (float)h + y, umax, vmax }, // bottom right
		{ 0.f + x, (float)h + y,      umin, vmax }  // bottom left
	};
	std::vector<uint16_t> indices = {
		0, 1, 2,
		0, 2, 3
	};
	batch.add(verts, indices);
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

	sprite_batch *b = new sprite_batch(video::get_texture("buttons_oxygen.png"));
	auto receptor = [&](int x, int y, bool alt) {
		if (math::random() < 0.5) {
			float btn[]  = { alt ? 20.f : 0.f, 0.f, alt ? 40.f : 20.f, 22.f };
			add_sprite(*b, x, y, btn);
		} else {
			float btn[] = { alt ? 20.f : 0.f, 22.f, alt ? 40.f : 20.f, 44.f };
			add_sprite(*b, x, y, btn);
		}
	};

	int limit = 7;
	int spacing = 20;
	for (int i = 0; i < limit; i++) {
		receptor(gs.width/2 - (limit/2)*spacing + i * spacing, gs.height - 200 + ((i % 2) ? 0 : 5) , i % 2 == 0);
	}
	b->buffer();

	bitmap_font *fnt = new bitmap_font;
	fnt->load("fonts/helvetica-neue-55.fnt");
	fnt->set_text(
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

		uint64_t default_state = 0
			| BGFX_STATE_RGB_WRITE
			| BGFX_STATE_CULL_CCW
			| BGFX_STATE_DEPTH_TEST_LEQUAL;

		bgfx::setTexture(0, sampler, b->texture->tex);
		bgfx::setTransform(view);
		bgfx::setVertexBuffer(b->vbo);
		bgfx::setIndexBuffer(b->ibo);
		bgfx::setState(default_state
			| BGFX_STATE_DEPTH_WRITE
			| BGFX_STATE_BLEND_ALPHA
		);
		bgfx::submit(0, program);

		bgfx::setTexture(0, sampler, fnt->texture->tex);
		bgfx::setTransform(text);
		bgfx::setVertexBuffer(fnt->vbo);
		bgfx::setIndexBuffer(fnt->ibo);
		bgfx::setState(default_state
			| BGFX_STATE_BLEND_ALPHA
		);
		bgfx::submit(0, dfield);

		bgfx::frame();
		bgfx::dbgTextClear();
	}

	delete fnt;
	delete b;

	video::unload_textures();

	bgfx::destroyProgram(program);

	video::close();

	SDL_Quit();

	return EXIT_SUCCESS;
}
