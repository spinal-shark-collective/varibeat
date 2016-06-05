/* In this file:
 * - Resolving config directories for different platforms
 * - Mounting any relevant archives (including ones attached to the binary)
 * - Wrapping PhysFS functions to be more useful for us */

#include "vbeat.hpp"
#include "fs.hpp"
#include <physfs.h>
#include <bgfx/bgfx.h>
#include <SDL2/SDL_assert.h>
#include <bx/readerwriter.h>
#include <bx/platform.h>
#include <vector>
#include <sys/stat.h>

#ifdef VBEAT_WINDOWS
typedef int mode_t;
#endif

using namespace vbeat;

namespace {
	bool fused = false;
	int files_open = 0;

	int64_t seek(PHYSFS_File *file, int64_t _offset = 0, bx::Whence::Enum _whence = bx::Whence::Current) {
		int64_t limit = (int64_t)PHYSFS_fileLength(file);
		int64_t position = 0;
		switch (_whence) {
			case bx::Whence::Begin: break;
			case bx::Whence::Current:
				position = (int64_t)PHYSFS_tell(file);
				break;
			case bx::Whence::End:
				position = limit;
				break;
			default: break;
		}

		position += _offset;

		BX_CHECK(position > limit || position < 0, "Invalid seek extents.");

		PHYSFS_seek(file, (PHYSFS_uint64)position);
		return (int64_t)PHYSFS_tell(file);
	}
}

// Fuck your fucking multiple inherited undocumented piece of shit I just want
// to read some goddamn files.
struct FileReader_PhysFS : public bx::FileReaderI {
	FileReader_PhysFS() :
	file(NULL)
	{}

	~FileReader_PhysFS() {
	}

	int32_t read(void *_data, int32_t _size, bx::Error *_err) {
		PHYSFS_sint64 bytes = PHYSFS_readBytes(file, _data, _size);
		if (bytes == -1) {
			BX_ERROR_SCOPE(_err);
		}

		return (int32_t)bytes;
	}

	int64_t seek(int64_t _offset = 0, bx::Whence::Enum _whence = bx::Whence::Current) {
		return ::seek(file, _offset, _whence);
	}

	bool open(const char *filename, bx::Error *_err) {
		file = PHYSFS_openRead(filename);
		if (!file) {
			BX_ERROR_SET(_err, BX_ERROR_READERWRITER_OPEN, "Failed to open file for reading.");
			return false;
		}

		::files_open += 1;
		return true;
	}

	void close() {
		if (file != NULL) {
			::files_open -= 1;
			PHYSFS_close(file);
		}
	}

private:
	// Disallow access to this so that you can't fuck up the refcount.
	PHYSFS_File *file;
};

struct FileWriter_PhysFS : public bx::FileWriterI {
	FileWriter_PhysFS() :
		file(NULL)
	{}

	~FileWriter_PhysFS() {}
	int32_t write(const void *_data, int32_t _size, bx::Error *_err) {
		int32_t size = (int32_t)PHYSFS_writeBytes(file, _data, _size);
		if (size != _size) {
			BX_ERROR_SET(_err, BX_ERROR_READERWRITER_WRITE, "Failed to write file.");
			return size >= 0 ? size : 0;
		}

		return size;
	}

	int64_t seek(int64_t _offset = 0, bx::Whence::Enum _whence = bx::Whence::Current) {
		return ::seek(file, _offset, _whence);
	}

	bool open(const char *filename, bool append, bx::Error *_err) {
		file = append ? PHYSFS_openAppend(filename) : PHYSFS_openWrite(filename);

		if (!file) {
			BX_ERROR_SET(_err, BX_ERROR_READERWRITER_OPEN, "Failed to open file for writing.");
			return false;
		}

		::files_open += 1;
		return true;
	}

	void close() {
		if (file != NULL) {
			::files_open -= 1;
			PHYSFS_close(file);
		}
	}

private:
	PHYSFS_File *file;
};

#ifdef VBEAT_WINDOWS
// For SHGetKnownFolderPath
#include <ShlObj.h>
#endif

#include <locale>

void fs::init(const char *argv0) {
	printf("VFS: Initializing...\n");
	SDL_assert(PHYSFS_init(argv0));
	int sc = PHYSFS_mount(argv0, "/", 0);
	if (sc) {
		printf("VFS: Fused executable detected. Mounted onto root.\n");
		fused = true;
	}
	// if it didn't work, throw the error out.
	PHYSFS_getLastError();

	// resolve the assets folder path, mount it.
	std::string path = std::string(argv0);
	size_t pos = path.find_last_of(PHYSFS_getDirSeparator());        // get bindir
	pos        = path.find_last_of(PHYSFS_getDirSeparator(), pos-1); // go up one
	path       = path.substr(0, pos);
	path       = path + PHYSFS_getDirSeparator() + "assets";
	sc = PHYSFS_mount(path.c_str(), "/", 1);
	if (!sc) {
		fprintf(stderr, "/!\\ Couldn't locate the assets folder! /!\\\n");
		fprintf(stderr, "Tried path: %s\n", path.c_str());
	}
	printf("VFS: Mounted assets dir.\n");

	PHYSFS_setSaneConfig(VBEAT_CONFIG_ORG, VBEAT_CONFIG_APP, "zip", 0, 0);

	std::string write_dir = PHYSFS_getWriteDir();
	if (!write_dir.empty()) {
		printf("VFS: Resolved save dir to \"%s\".\n", write_dir.c_str());
	}
	else {
		printf("VFS: Couldn't figure out save dir; dragons lie ahead.\n");
	}

	while (const char *err = PHYSFS_getLastError()) {
		fprintf(stderr, "PhysFS Error: %s\n", err);
	}

	printf("VFS: Ready.\n");
}

void fs::deinit() {
	PHYSFS_deinit();
	printf("VFS: Shutting down (%d remaining file handles).\n", ::files_open);
}

bool fs::is_fused() {
	return fused;
}

#include <bx/string.h>

std::string fs::get_real_path(const std::string &filename) {
	if (!fs::is_file(filename)) {
		return std::string();
	}
	const char *fname = filename.c_str();
	const char *real = PHYSFS_getRealDir(fname);
	return std::string(real) + "/" + bx::baseName(fname);
}

bool fs::is_file(const std::string &filename) {
	const char *fn = filename.c_str();
	if (PHYSFS_exists(fn)) {
		PHYSFS_Stat stat;
		PHYSFS_stat(fn, &stat);
		return stat.filetype == PHYSFS_FILETYPE_REGULAR;
	}
	return false;
}

void fs::get_directory_items(std::vector<std::string> &items, const std::string &path, bool check_read) {
	char **files = PHYSFS_enumerateFiles(path.c_str());
	PHYSFS_File *f = NULL;
	while (*files) {
		const char *fname = *files;
		files += 1;
		if (check_read) {
			f = PHYSFS_openRead(fname);
		}
		if (!check_read) {
			items.push_back(fname);
			continue;
		}
		if (f) {
			items.push_back(fname);
			PHYSFS_close(f);
			f = NULL;
		}
	}
}

bool fs::read_string(std::string &data, const std::string &filename, int bytes) {
	auto file = FileReader_PhysFS();
	if (bx::open(&file, filename.c_str())) {
		int32_t _size = (int32_t)bx::getSize(&file);
		int32_t _read = bytes > 0 ? bytes : _size;
		data.resize(_read+1);
		bx::read(&file, &data[0], _read);
		data[_read] = '\0';
		bx::close(&file);
		return true;
	}

	return false;
}

bool fs::read_vector(std::vector<uint8_t> &data, const std::string &filename, int bytes) {
	auto file = FileReader_PhysFS();
	if (bx::open(&file, filename.c_str())) {
		int32_t _size = (int32_t)bx::getSize(&file);
		int32_t _read = bytes > 0 ? bytes : _size;
		data.resize(_read);
		bx::read(&file, &data[0], _read);
		bx::close(&file);
		return true;
	}
	return false;
}

const bgfx::Memory *fs::read_mem(const std::string &filename, int bytes) {
	auto file = FileReader_PhysFS();
	if (bx::open(&file, filename.c_str())) {
		int32_t _size = (int32_t)bx::getSize(&file);
		int32_t _read = bytes > 0 ? bytes : _size;
		void *buf = bx::alloc(vbeat::get_allocator(), _size);
		bx::read(&file, buf, _read);
		bx::close(&file);

		return bgfx::makeRef(buf, _size);
	}
	return bgfx::makeRef(NULL, 0);
}

bool fs::write(const std::string &, const std::string &, int) {
	return false;
}
