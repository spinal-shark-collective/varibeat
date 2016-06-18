#pragma once

#include <bx/fpumath.h>

#include "widgets/widget.hpp"
#include "bitmap_font.hpp"
#include "fs.hpp"

using namespace vbeat;

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
