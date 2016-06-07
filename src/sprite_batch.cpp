#include "sprite_batch.hpp"

using namespace vbeat;
using namespace video;

sprite_batch_t::sprite_batch_t(texture_t *_texture):
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

sprite_batch_t::~sprite_batch_t() {
	this->texture->refs--;
	bgfx::destroyDynamicVertexBuffer(this->vbo);
	bgfx::destroyDynamicIndexBuffer(this->ibo);
}

void sprite_batch_t::add(const std::vector<vertex_t> &_vertices, const std::vector<uint16_t> &_indices) {
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

void sprite_batch_t::buffer() {
	if (!this->dirty) {
		return;
	}

	bgfx::updateDynamicVertexBuffer(
		this->vbo, 0,
		bgfx::makeRef(
			this->vertices.data(), this->vertices.size() * sizeof(vertex_t)
		)
	);

	// FIXME: updating isn't throwing out the old data?
	bgfx::updateDynamicIndexBuffer(
		this->ibo, 0,
		bgfx::makeRef(
			this->indices.data(), this->indices.size() * sizeof(uint16_t)
		)
	);

	this->dirty = false;
}

void sprite_batch_t::clear() {
	this->dirty = true;
	vertices.clear();
	indices.clear();
}
