#pragma once

#include <deque>
#include <bx/fpumath.h>

#include "widgets/widget.hpp"
#include "sprite_batch.hpp"
#include "fs.hpp"

using namespace vbeat;

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
