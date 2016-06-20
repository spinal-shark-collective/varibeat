#include <map>
#include <cstring>
#include <SDL2/SDL_assert.h>

#include "vbeat.hpp"
#include "fs.hpp"

#include "graphics/iqm.h"
#include "graphics/mesh.hpp"

using namespace vbeat;

graphics::mesh::mesh() :
	vertices(nullptr),
	indices(nullptr)
{}

graphics::mesh::~mesh() {
	if (this->vertices) {
		bgfx::destroyVertexBuffer(this->vbo);
		v_free(this->vertices);
	}
	if (this->indices) {
		bgfx::destroyIndexBuffer(this->ibo);
		v_free(this->indices);
	}
}

#define CHECK_SANITY(field, count, type) SDL_assert_release(header->field + (sizeof(type) * header->count) <= data.size())

bgfx::AttribType::Enum iqm2bgfxfmt(unsigned int format) {
	switch (format) {
		case IQM_UBYTE: return bgfx::AttribType::Uint8;
		case IQM_FLOAT: return bgfx::AttribType::Float;
		default: {
			SDL_assert_always(false);
			// Note: Unreachable, unless someone continues past the assertion.
			return bgfx::AttribType::Float;
		}
	}
}

bgfx::Attrib::Enum iqm2bgfxtype(unsigned int type) {
	switch (type) {
		case IQM_POSITION:     return bgfx::Attrib::Position;
		case IQM_TEXCOORD:     return bgfx::Attrib::TexCoord0;
		case IQM_NORMAL:       return bgfx::Attrib::Normal;
		case IQM_TANGENT:      return bgfx::Attrib::Tangent;
		case IQM_BLENDINDEXES: return bgfx::Attrib::Indices;
		case IQM_BLENDWEIGHTS: return bgfx::Attrib::Weight;
		case IQM_COLOR:        return bgfx::Attrib::Color0;
		default:
			SDL_assert_release(false);
			return bgfx::Attrib::TexCoord7;
	}
}

void read_anims(std::vector<uint8_t> &data, iqmheader *header) {
	// nothing to see here
	if (header->num_anims == 0) {
		return;
	}
}

bool graphics::read_iqm(graphics::mesh &mesh, const std::string &filename, bool read_anims) {
	std::vector<uint8_t> data;
	fs::read_vector(data, filename);

	iqmheader *header = (iqmheader*)&data[0];

	if (strncmp(header->magic, IQM_MAGIC, 16) != 0) {
		printf("IQM: Bad magic.\n");
		return false;
	}

	if (header->version != IQM_VERSION) {
		printf("IQM: Unsupported version.\n");
		return false;
	}

	CHECK_SANITY(ofs_vertexarrays, num_vertexarrays, iqmvertexarray);

	/* Read the vertex arrays, so we know what kind of vertex format to expect.
	* Most of the work here is just some switches. */
	std::map<bgfx::Attrib::Enum, iqmvertexarray> va_map;
	bgfx::VertexDecl vertex_format;
	vertex_format.begin();

	iqmvertexarray *vas = (iqmvertexarray*)&data[header->ofs_vertexarrays];
	for (unsigned int i = 0; i < header->num_vertexarrays; i++) {
		iqmvertexarray va = vas[i];
		vertex_format.add(iqm2bgfxtype(va.type), va.size, iqm2bgfxfmt(va.format));
		va_map[iqm2bgfxtype(va.type)] = va;
	}

	vertex_format.end();

	/* Read the vertex list into a vertex buffer. It's a bit of a pain in the
	* ass to get right due to the flexible vertex format... oh well.
	*
	* Almost all the trouble here is just from needing to interleave the
	* attributes, because we can't really guarantee anything exists first. */
	size_t stride = vertex_format.getStride();
	size_t size = vertex_format.getSize(header->num_vertexes);
	uint8_t *vertices = (uint8_t*)v_malloc(size);
	for (auto &p : va_map) {
		bgfx::Attrib::Enum attr = p.first;
		iqmvertexarray va = p.second;
		for (unsigned int i = 0; i < header->num_vertexes; i++) {
			auto offset = vertex_format.getOffset(attr);
			size_t bytes = 0;
			switch (va.format) {
				case IQM_FLOAT: bytes = sizeof(float); break;
				case IQM_UBYTE: bytes = sizeof(uint8_t); break;
				default: return false;
			}
			size_t vtx_offset = (stride * i) + offset;
			size_t va_offset = i * (va.size * bytes) + va.offset;
			memcpy(&vertices[vtx_offset], &data[va_offset], va.size * bytes);
		}
	}

	/* Read the triangle list into an index buffer (we can't simply memcpy it
	* since it involves converting from uint to ushort). */
	iqmtriangle *triangles = (iqmtriangle*)&data[header->ofs_triangles];
	uint16_t indices_size = header->num_triangles * sizeof(uint16_t) * 3;
	uint16_t *indices = (uint16_t*)v_malloc(indices_size);

	for (unsigned int i = 0; i < header->num_triangles; i++) {
		indices[i * 3 + 0] = triangles[i].vertex[0];
		indices[i * 3 + 1] = triangles[i].vertex[1];
		indices[i * 3 + 2] = triangles[i].vertex[2];
	}

	// We've got everything now, just need to make handles for bgfx.
	bgfx::VertexBufferHandle vbo = bgfx::createVertexBuffer(
		bgfx::makeRef(vertices, size),
		vertex_format
	);

	bgfx::IndexBufferHandle ibo = bgfx::createIndexBuffer(
		bgfx::makeRef(indices, indices_size)
	);

	mesh.vbo = vbo;
	mesh.ibo = ibo;
	mesh.vertices = vertices;
	mesh.indices = indices;
	mesh.format = vertex_format;

	return true;
}

#undef CHECK_SANITY
