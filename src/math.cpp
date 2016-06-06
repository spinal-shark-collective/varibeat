#include <cstring>
#include <cmath>

#include "math.hpp"

namespace vbeat {
namespace math {

/* The state must be seeded so that it is not everywhere zero. */
uint64_t s[2] = {
	0xCBBF7A44,
	0x0139408D,
};

void seed(uint64_t hi, uint64_t low) {
	s[0] = hi;
	s[1] = low;
}

double random() {
	uint64_t x = s[0];
	uint64_t const y = s[1];
	s[0] = y;
	x ^= x << 23; // a
	s[1] = x ^ y ^ (x >> 17) ^ (y >> 26); // b, c
	return double(s[1] + y) / double(UINT64_MAX);
}

void perspective(
	float *_result, float aspect, float fovy,
	float zNear, bool infinite, float zFar
) {
	memset(_result, 0, sizeof(float)*16);

	float range  = tan(fovy / 2.0f);
	_result[11] = -1.0f;

	if (infinite) {
		range *= zNear;

		const float ep     = 0.0f;
		const float left   = -range * aspect;
		const float right  =  range * aspect;
		const float bottom = -range;
		const float top    =  range;

		_result[0] = (2.0f * zNear) / (right - left);
		_result[5] = (2.0f * zNear) / (top - bottom);
		_result[9] = ep - 1.0f;
		_result[14] = (ep - 2.0f) * zNear;
	}
	else {
		_result[0] = 1.0f / (aspect * range);
		_result[5] = 1.0f / (range);
		_result[9] = -(zFar + zNear) / (zFar - zNear);
		_result[14] = -(2.0f * zFar * zNear) / (zFar - zNear);
	}

}

} // math
} // vbeat
