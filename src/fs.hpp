#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace bgfx { struct Memory; }

struct lua_State;

namespace vbeat {
namespace fs {
	// Set up us the PhysFS
	void init(const char *argv0);
	void deinit();

	// Check that the executable has an archive fused to it.
	bool is_fused();

	// Check that a given file is a file (i.e. not a directory).
	bool is_file(const std::string &filename);

	// Get the physical path of a file in the VFS.
	std::string get_real_path(const std::string &filename);

	// List files in a given path in the VFS.
	void get_directory_items(std::vector<std::string> &items, const std::string &path, bool check_read = false);

	// Read file contents into string.
	bool read_string(std::string &data, const std::string &filename, int bytes = -1);

	// Read file contents into vector.
	bool read_vector(std::vector<uint8_t> &data, const std::string &filename, int bytes = -1);

	// Read file into a buffer for BGFX, or nullptr.
	const bgfx::Memory *read_mem(const std::string &filename, int bytes = -1);

	// Write string contents to file.
	bool write(const std::string &filename, const std::string &data, int bytes = -1);

	struct state {
		state(const char *argv0) { fs::init(argv0); }
		virtual ~state() { fs::deinit(); }
	};
} // fs
} // myon
