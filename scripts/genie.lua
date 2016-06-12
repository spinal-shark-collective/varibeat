BASE_DIR   = path.getabsolute("..")
EXTERN_DIR = path.join(BASE_DIR, "extern")

solution "Varibeat" do
	configurations {
		"Debug",
		"Release"
	}
	platforms { "Native", "x32", "x64" }
	startproject "Varibeat"
	targetdir(path.join(BASE_DIR, "bin"))
	objdir(path.join(BASE_DIR, "obj"))

	configuration { "Debug" }
	flags { "Symbols" }

	configuration {}
end

dofile("physfs.lua")
dofile("bgfx.lua")

if os.get() == "windows" then
	dofile("sdl2.lua")
end

-- accumulate all the include dirs for code completion.
local includes = {}
local function collect()
	local conf = configuration()
	local dirs = conf.includedirs
	for _, dir in pairs(dirs) do
		table.insert(includes, dir)
	end
end

project "Varibeat" do
	kind "WindowedApp"
	language "C++"
	if os.get() ~= "Windows" then
		-- We only want caps on Windows.
		targetname "varibeat"
	end
	local VBEAT_DIR = path.join(BASE_DIR, "src")

	links {
		"SDL2",
		"BGFX",
		"PhysFS",
	}

	defines {
		"LODEPNG_NO_COMPILE_ENCODER",
		"LODEPNG_NO_COMPILE_DISK",
		"LODEPNG_NO_COMPILE_ALLOCATORS"
	}

	configuration {"Debug"}
	defines {
		"VBEAT_DEBUG"
	}

	configuration {"linux"}
	links {
		"GL",
		"pthread",
		"X11"
	}
	configuration {"linux", "Debug"}
	links {
		"dl"
	}
	configuration {"gmake"}
	buildoptions {
		"-std=c++11",
		"-fno-strict-aliasing",
		"-Wall",
		"-Wextra"
	}

	configuration {}
	vpaths {
		["Headers"] = { "**.h", "**.hpp" },
		["Sources"] = { "**.c", "**.cpp" }
	}
	files {
		path.join(EXTERN_DIR, "lodepng/lodepng.cpp"),
		path.join(VBEAT_DIR, "*.cpp"),
		path.join(VBEAT_DIR, "*.hpp")
	}
	includedirs {
		VBEAT_DIR,
		EXTERN_DIR,
		path.join(EXTERN_DIR, "lodepng"),
		path.join(EXTERN_DIR, "bgfx/include"),
		path.join(BASE_DIR, "scripts/include"),
		PHYSFS_DIR,
		BX_DIR
	}
	if os.get() == "windows" then
		links {
			"SDL2main",
			"psapi"
		}
	end
	collect()
	configuration {"windows", "vs*"}
	defines {
		"_CRT_SECURE_NO_WARNINGS",
		"VBEAT_WINDOWS",
	}
	includedirs {
		path.join(BX_DIR, "compat/msvc")
	}
	collect()
end

-- now that we've got everything, spit out a .clang_complete file.
local f = io.open(path.join(BASE_DIR, ".clang_complete"), "w")
for _, v in ipairs(includes) do
	f:write(string.format("-I%s\n", v))
end
f:close()
