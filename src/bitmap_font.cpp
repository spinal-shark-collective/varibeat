#include <string>
#ifndef VBEAT_WINDOWS
#include <unistd.h>
#endif
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cassert>
#include <bgfx/bgfx.h>

#include "bitmap_font.hpp"
#include "lodepng.h"
#include "fs.hpp"

using namespace vbeat;

namespace {
	struct vertex_t {
		float x, y;
		float u, v;
	};
}

bitmap_font::bitmap_font() :
	empty(true),
	KernCount(0),
	current_text("")
{
	set_text("");

	bgfx::VertexDecl decl;
	decl.begin()
		.add(bgfx::Attrib::Position,  2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		.end();
	this->vbo = bgfx::createDynamicVertexBuffer(1, decl, BGFX_BUFFER_ALLOW_RESIZE);
	this->ibo = bgfx::createDynamicIndexBuffer(1, BGFX_BUFFER_ALLOW_RESIZE);
}

bitmap_font::~bitmap_font() {
	bgfx::destroyTexture(this->tex);
	bgfx::destroyDynamicIndexBuffer(this->ibo);
	bgfx::destroyDynamicVertexBuffer(this->vbo);

	Chars.clear();
	Kern.clear();
}

bool bitmap_font::parse_font(std::string fontfile) {
	std::string data;
	fs::read_string(data, fontfile);

	std::istringstream Stream(data);
	std::string Line;
	std::string Read, Key, Value;
	size_t i;

	kerning_info K;
	char_descriptor C;

	while (!Stream.eof()) {
		std::stringstream LineStream;
		std::getline(Stream, Line);
		LineStream << Line;

		LineStream >> Read;
		if (Read == "common") {
			while (!LineStream.eof()) {
				std::stringstream Converter;
				LineStream >> Read;
				i = Read.find('=');
				Key = Read.substr(0, i);
				Value = Read.substr(i + 1);

				Converter << Value;
				if (Key == "lineHeight")	Converter >> LineHeight;
				else if (Key == "base")		Converter >> Base;
				else if (Key == "scaleW")	Converter >> Width;
				else if (Key == "scaleH")	Converter >> Height;
				else if (Key == "pages")	Converter >> Pages;
				else if (Key == "outline")	Converter >> Outline;
			}
		}
		else if (Read == "page") {
			int page = 0;
			std::string path = "";
			while (!LineStream.eof()) {
				std::stringstream Converter;
				LineStream >> Read;
				i = Read.find('=');
				Key = Read.substr(0, i);
				Value = Read.substr(i + 1);

				Converter << Value;
				if (Key == "id")			Converter >> page;
				else if (Key == "file")		Converter >> path;
			}
			path = path.substr(path.find_first_of("\"")+1, path.find_last_of("\"")-1);
			if (!path.empty())
				texture_path.push_back(path);
		}
		else if (Read == "char") {
			int CharID = 0;

			while (!LineStream.eof()) {
				std::stringstream Converter;
				LineStream >> Read;
				i = Read.find('=');
				Key = Read.substr(0, i);
				Value = Read.substr(i + 1);

				Converter << Value;
				if (Key == "id")			Converter >> CharID;
				else if (Key == "x")		Converter >> C.x;
				else if (Key == "y")		Converter >> C.y;
				else if (Key == "width")	Converter >> C.Width;
				else if (Key == "height")	Converter >> C.Height;
				else if (Key == "xoffset")	Converter >> C.XOffset;
				else if (Key == "yoffset")	Converter >> C.YOffset;
				else if (Key == "xadvance")	Converter >> C.XAdvance;
				else if (Key == "page")		Converter >> C.Page;
			}

			Chars.insert(std::map<int, char_descriptor>::value_type(CharID,C));

		}
		else if (Read == "kernings") {
			while (!LineStream.eof()) {
				std::stringstream Converter;
				LineStream >> Read;
				i = Read.find('=');
				Key = Read.substr(0, i);
				Value = Read.substr(i + 1);

				Converter << Value;
				if (Key == "count")	Converter >> KernCount;
			}
		}
		else if (Read == "kerning") {
			while (!LineStream.eof()) {
				std::stringstream Converter;
				LineStream >> Read;
				i = Read.find('=');
				Key = Read.substr(0, i);
				Value = Read.substr(i + 1);

				Converter << Value;
				if (Key == "first")			Converter >> K.First;
				else if (Key == "second")	Converter >> K.Second;
				else if (Key == "amount")	Converter >> K.Amount;
			}
			Kern.push_back(K);
		}
	}

	return true;
}

int bitmap_font::get_kerning_pair(int first, int second) {
	if (KernCount) {
		for (int j = 0; j < KernCount; j++) {
			if (Kern[j].First == first && Kern[j].Second == second)
				return Kern[j].Amount;
		}
	}
	return 0;
}

float bitmap_font::get_string_width(std::string string) {
	if (string.empty()) string = current_text;

	float total = 0;
	size_t len = string.length();

	for (size_t i = 0; i != len; i++) {
		if (len > 1 && i < len)
			total += get_kerning_pair(string[i], string[i+1]);
		total += Chars[string[i]].XAdvance;
	}

	return total;
}

bool bitmap_font::load(std::string fontfile) {
	if (!fs::is_file(fontfile)) {
		printf("Couldn't find font: %s\n", fontfile.c_str());
		return false;
	}

	parse_font(fontfile);

	KernCount = (int)Kern.size();

	for (size_t i = 0; i < texture_path.size(); ++i)
	{
		std::string path = fontfile.substr(0, fontfile.find_last_of("/"));
		texture_path[i] = path + "/" + texture_path[i];

		unsigned w, h;
		std::vector<unsigned char> pixels;
		std::vector<unsigned char> file_data;
		fs::read_vector(file_data, texture_path[i]);
		unsigned err = lodepng::decode(pixels, w, h, file_data);
		if (err) {
			printf("fuck\n");
			return false;
		}

		this->tex = bgfx::createTexture2D(w, h, 0, bgfx::TextureFormat::RGBA8, 0, bgfx::copy(pixels.data(), w*h*4));

		// TODO: support n>1 texture maps.
		break;
	}

	return true;
}

void bitmap_font::set_text(std::string text) {
	int Flen = text.length();
	current_text = text;

	this->empty = Flen == 0;
	if (this->empty) {
		return;
	}

	// Font texture atlas spacing.
	float advx = (float) 1.0 / Width;
	float advy = (float) 1.0 / Height;
	char_descriptor *f;

	float x = 0;
	float y = LineHeight;
	int line = -1;

	std::vector<vertex_t> texlst(Flen*4);
	std::vector<uint16_t> indices(Flen*6);

	for (int i = 0; i < Flen; ++i) {
		f=&Chars[text[i]];
		if (text[i] == '\n') {
			x = 0;
			line++;
		}

		y = LineHeight * line;

		float CurX = x + f->XOffset;
		float CurY = y + f->YOffset;
		float DstX = CurX + f->Width;
		float DstY = CurY + f->Height;

		// 0,1 Texture Coord
		texlst[i*4].u = advx * f->x;
		texlst[i*4].v = advy * (f->y + f->Height);
		texlst[i*4].x = CurX;
		texlst[i*4].y = DstY;

		// 1,1 Texture Coord
		texlst[(i*4)+1].u = advx * (f->x + f->Width);
		texlst[(i*4)+1].v = advy * (f->y + f->Height);
		texlst[(i*4)+1].x = DstX;
		texlst[(i*4)+1].y = DstY;

		// 0,0 Texture Coord
		texlst[(i*4)+2].u = advx * f->x;
		texlst[(i*4)+2].v = advy * f->y;
		texlst[(i*4)+2].x = CurX;
		texlst[(i*4)+2].y = CurY;

		// 1,0 Texture Coord
		texlst[(i*4)+3].u = advx * (f->x + f->Width);
		texlst[(i*4)+3].v = advy * f->y;
		texlst[(i*4)+3].x = DstX;
		texlst[(i*4)+3].y = CurY;

		int base = i*4;
		indices.push_back(base+0);
		indices.push_back(base+2);
		indices.push_back(base+1);
		indices.push_back(base+1);
		indices.push_back(base+2);
		indices.push_back(base+3);

		// Only check kerning if there is greater then 1 character and
		// if the check character is 1 less then the end of the string.
		if (Flen > 1 && i < Flen)
			x += get_kerning_pair(text[i], text[i+1]);

		x += f->XAdvance;
	}

	bgfx::updateDynamicVertexBuffer(this->vbo, 0, bgfx::copy(texlst.data(), texlst.size() * sizeof(vertex_t)));
	bgfx::updateDynamicIndexBuffer(this->ibo, 0, bgfx::copy(indices.data(), indices.size() * sizeof(uint16_t)));
}
