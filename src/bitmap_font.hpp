#pragma once

#include <vector>
#include <map>
#include <string>
#include <bgfx/bgfx.h>

namespace vbeat
{

class kerning_info
{
public:
	int First, Second, Amount;

	kerning_info() :
		First(0), Second(0), Amount(0)
	{ }
};

class char_descriptor
{
public:
	int x, y, Width, Height;
	int XOffset, YOffset, XAdvance;
	int Page;

	char_descriptor() :
		x(0),
		y(0),
		Width(0),
		Height(0),
		XOffset(0),
		YOffset(0),
		XAdvance(0),
		Page(0)
	{}
};

class bitmap_font
{
public:
	bool load(std::string filename);
	float get_height() { return LineHeight; }
	std::string get_texture_path(int page = 0) { return texture_path[page]; }
	size_t get_texture_pages() { return texture_path.size(); }
	float get_string_width(std::string = "");

	void set_text(std::string str);

	bitmap_font();
	~bitmap_font();

	bgfx::DynamicVertexBufferHandle vbo;
	bgfx::DynamicIndexBufferHandle  ibo;
	bgfx::TextureHandle tex;
	bool empty;

private:
	int LineHeight, Base, Width, Height;
	int Pages, Outline, KernCount;
	std::map<int, char_descriptor> Chars;
	std::vector<kerning_info> Kern;
	std::vector<std::string> texture_path;
	std::string current_text;

	bool parse_font(std::string);
	int get_kerning_pair(int, int);
};

}
