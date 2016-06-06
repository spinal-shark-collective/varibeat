#include <stdint.h>

namespace vbeat {
namespace math {
	void seed(uint64_t hi, uint64_t low);
	double random();
	void perspective(float *_result, float aspect, float fovy = 60.0f, float zNear = 0.1f, bool infinite = true, float zFar = 1000.0f);
} // math
} // vbeat
