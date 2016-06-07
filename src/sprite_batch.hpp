#pragma once

#include <vector>
#include "texture.hpp"

namespace vbeat {
namespace video {

struct vertex_t {
	float x, y;
	float u, v;
};

struct sprite_batch_t {
	bool dirty;
	video::texture_t *texture;

	std::vector<vertex_t> vertices;
	std::vector<uint16_t> indices;

	bgfx::DynamicVertexBufferHandle vbo;
	bgfx::DynamicIndexBufferHandle  ibo;

	sprite_batch_t(video::texture_t *_texture);
	virtual ~sprite_batch_t();

	void add(const std::vector<vertex_t> &_vertices, const std::vector<uint16_t> &_indices);

	void buffer();
	void clear();
};

} // video
} // vbeat
