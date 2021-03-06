#include <map>
#include "vbeat.hpp"
#include "lodepng.h"
#include "texture.hpp"
#include "fs.hpp"

void* lodepng_malloc(size_t size) {
	return vbeat::v_malloc(size);
}

void* lodepng_realloc(void* ptr, size_t new_size) {
	return vbeat::v_realloc(ptr, new_size);
}

void lodepng_free(void* ptr) {
	return vbeat::v_free(ptr);
}

using namespace vbeat;

namespace {
	std::map<std::string, graphics::texture_t*> loaded_textures;
}

graphics::texture_t *graphics::get_texture(const std::string &filename) {
	texture_t *tex = loaded_textures[filename];
	if (tex == nullptr) {
		unsigned w, h;
		std::vector<unsigned char> pixels;
		std::vector<unsigned char> file_data;
		fs::read_vector(file_data, filename);
		unsigned err = lodepng::decode(pixels, w, h, file_data);
		if (err) {
			puts("fuck");
			return nullptr;
		}

		tex = new texture_t(bgfx::createTexture2D(w, h, 0, bgfx::TextureFormat::RGBA8, 0, bgfx::copy(pixels.data(), w*h*4)), w, h);
		loaded_textures[filename] = tex;
	}
	return tex;
}

void graphics::unload_textures() {
	for (auto &t : loaded_textures) {
		if (t.second != nullptr) {
			if (t.second->refs > 0) {
				puts("/!\\ texture has non-zero refcount! /!\\");
			}
			delete t.second;
			loaded_textures.erase(t.first);
		}
	}
}
