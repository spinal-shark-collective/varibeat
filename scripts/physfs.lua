PHYSFS_DIR = path.join(EXTERN_DIR, "physfs/src")

project "PhysFS" do
	targetname "physfs"
	kind "StaticLib"
	language "C++"
	uuid "E2F719CA-CE99-44A0-B754-58F4A395CBF0"
	includedirs { PHYSFS_DIR }
	files {
		path.join(PHYSFS_DIR, "archiver_dir.c"),
		path.join(PHYSFS_DIR, "archiver_iso9660.c"),
		path.join(PHYSFS_DIR, "archiver_unpacked.c"),
		path.join(PHYSFS_DIR, "archiver_zip.c"),
		path.join(PHYSFS_DIR, "physfs_byteorder.c"),
		path.join(PHYSFS_DIR, "physfs.c"),
		path.join(PHYSFS_DIR, "physfs_casefolding.h"),
		path.join(PHYSFS_DIR, "physfs.h"),
		path.join(PHYSFS_DIR, "physfs_internal.h"),
		path.join(PHYSFS_DIR, "physfs_miniz.h"),
		path.join(PHYSFS_DIR, "physfs_platforms.h"),
		path.join(PHYSFS_DIR, "physfs_unicode.c"),
		path.join(PHYSFS_DIR, "platform_macosx.c"),
		path.join(PHYSFS_DIR, "platform_posix.c"),
		path.join(PHYSFS_DIR, "platform_unix.c"),
		path.join(PHYSFS_DIR, "platform_windows.c"),
		path.join(PHYSFS_DIR, "platform_winrt.cpp")
	}
	defines {
		"__EMX__=0"
	}
	configuration {"linux"}
	defines {"PHYSFS_PLATFORM_POSIX"}
	configuration {"windows"}
	defines {"PHYSFS_PLATFORM_WINDOWS"}
	configuration {"vs*"}
	defines {
		"_CRT_SECURE_NO_WARNINGS"
	}
end
