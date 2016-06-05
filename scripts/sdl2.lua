SDL_DIR    = path.join(EXTERN_DIR, "sdl")

project "SDL2" do
	targetname "SDL2"
	kind "SharedLib"
	language "C++"
	uuid "3ABE8B7C-26F5-8C0D-CFE1-7210BBF7080F"

	local headers = os.matchfiles(path.join(SDL_DIR, "include") .. "/*.h")
	os.mkdir("include")
	os.mkdir("include/SDL2")
	for _, header in pairs(headers) do
		local file = path.getname(header)
		local path = path.join("include/SDL2", file)
		os.copyfile(header, path)
	end

	local SDL_SRC = path.join(SDL_DIR, "src")

	includedirs {
		path.join(SDL_DIR, "include")
	}

	-- common files
	files {
		path.join(SDL_SRC, "*.c"),
		path.join(SDL_SRC, "atomic/*.c"),
		path.join(SDL_SRC, "audio/*.c"),
		path.join(SDL_SRC, "audio/dummy/*.c"),
		path.join(SDL_SRC, "audio/disk/*.c"),
		path.join(SDL_SRC, "core/*.c"),
		path.join(SDL_SRC, "cpuinfo/*.c"),
		path.join(SDL_SRC, "dynapi/*.c"),
		path.join(SDL_SRC, "events/*.c"),
		path.join(SDL_SRC, "file/*.c"),
		path.join(SDL_SRC, "filesystem/dummy/*.c"),
		path.join(SDL_SRC, "haptic/*.c"),
		path.join(SDL_SRC, "haptic/dummy/*.c"),
		path.join(SDL_SRC, "joystick/*.c"),
		path.join(SDL_SRC, "joystick/dummy/*.c"),
		path.join(SDL_SRC, "libm/*.c"),
		path.join(SDL_SRC, "loadso/*.c"),
		path.join(SDL_SRC, "main/*.c"),
		path.join(SDL_SRC, "power/*.c"),
		path.join(SDL_SRC, "render/*.c"),
		path.join(SDL_SRC, "stdlib/*.c"),
		path.join(SDL_SRC, "thread/*.c"),
		path.join(SDL_SRC, "timer/*.c"),
		path.join(SDL_SRC, "timer/dummy/*.c"),
		path.join(SDL_SRC, "video/*.c"),
		path.join(SDL_SRC, "video/dummy/*.c")
	}
	configuration { "windows", "vs*" }
	files {
		path.join(SDL_SRC, "audio/directsound/*.c"),
		-- this is, apparently, possible.
		path.join(SDL_SRC, "audio/pulseaudio/*.c"),
		path.join(SDL_SRC, "audio/xaudio2/*.c"),
		path.join(SDL_SRC, "audio/winmm/*.c"),
		path.join(SDL_SRC, "core/windows/*.c"),
		path.join(SDL_SRC, "filesystem/windows/*.c"),
		path.join(SDL_SRC, "haptic/windows/*.c"),
		path.join(SDL_SRC, "joystick/windows/*.c"),
		path.join(SDL_SRC, "loadso/windows/*.c"),
		path.join(SDL_SRC, "power/windows/*.c"),
		path.join(SDL_SRC, "render/direct3d/*.c"),
		path.join(SDL_SRC, "render/direct3d11/*.c"),
		path.join(SDL_SRC, "render/opengl/*.c"),
		path.join(SDL_SRC, "render/opengles/*.c"),
		path.join(SDL_SRC, "render/opengles2/*.c"),
		path.join(SDL_SRC, "render/software/*.c"),
		path.join(SDL_SRC, "thread/generic/SDL_syscond.c"),
		path.join(SDL_SRC, "thread/windows/*.c"),
		path.join(SDL_SRC, "timer/windows/*.c"),
		path.join(SDL_SRC, "video/windows/*.c")
	}
	links {
		"version",
		"imm32",
		"dxguid",
		"xinput",
		"winmm"
	}
end

project "SDL2main" do
	targetname "SDL2main"
	kind "StaticLib"
	language "C++"
	uuid "1FBB913C-0B88-EC47-34A1-DAEF20CD21D6"

	local SDL_SRC = path.join(SDL_DIR, "src")
	includedirs { path.join(SDL_DIR, "include") }
	configuration { "windows", "vs*" }
	files {
		path.join(SDL_SRC, "main/windows/*.c")
	}
end