#pragma once

#include <string>

namespace vbeat {
namespace video {
	bool open(int w, int h, std::string title);
	void close();
} // video
} // myon
