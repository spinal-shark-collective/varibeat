#include <cstring>
#include <cmath>

#include "math.hpp"

namespace vbeat {
namespace math {

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
