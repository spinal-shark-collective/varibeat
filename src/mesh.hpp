#pragma once

#include <bgfx/bgfx.h>
#include <cstdint>
#include <string>

namespace vbeat {
namespace video {

// Note: Implemented in iqm.cpp
struct mesh {
	bgfx::VertexBufferHandle vbo;
	bgfx::IndexBufferHandle  ibo;
	uint8_t  *vertices;
	uint16_t *indices;
	bgfx::VertexDecl format;

	mesh();
	virtual ~mesh();
};

bool read_iqm(mesh &mesh, const std::string &filename, bool read_anims=false);

} // video
} // myon
