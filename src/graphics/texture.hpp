#pragma once

#include <bgfx/bgfx.h>
#include <string>

namespace vbeat {
namespace graphics {

// TODO: automatic refcounting.
struct texture_t {
	texture_t(bgfx::TextureHandle _tex, unsigned _w, unsigned _h) :
		tex(_tex),
		w(_w),
		h(_h),
		refs(0)
	{}
	virtual ~texture_t() {
		puts("deleting texture");
		bgfx::destroyTexture(this->tex);
	}
	bgfx::TextureHandle tex;
	unsigned w, h;
	int refs;
};

texture_t *get_texture(const std::string &filename);
void unload_textures();

} // graphics
} // vbeat
