#pragma once

#define VBEAT_CONFIG_ORG "ssc"
#define VBEAT_CONFIG_APP "varibeat"

#ifdef VBEAT_WINDOWS
#	define VBEAT_EXPORT __declspec(dllexport)
#else
#	define VBEAT_EXPORT
#endif

namespace bx {
	struct AllocatorI;
}

namespace vbeat {
	// fixed 60hz timestep
	const double target_framerate = 60.0;
	const double timestep  = 1.0 / target_framerate;

	bx::AllocatorI *get_allocator();
}
