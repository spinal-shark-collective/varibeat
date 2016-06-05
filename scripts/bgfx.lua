BX_DIR     = path.join(EXTERN_DIR, "bx/include")

project "BGFX" do
	targetname "bgfx"
	uuid "EC77827C-D8AE-830D-819B-69106DB1FF0E"
	if _OPTIONS["force-gl3"] then
		defines { "BGFX_CONFIG_RENDERER_OPENGL=33" }
	end
	kind "StaticLib"
	language "C++"
	local BGFX_DIR = path.join(EXTERN_DIR, "bgfx")
	local BGFX_SRC_DIR = path.join(BGFX_DIR, "src")
	includedirs {
		path.join(BGFX_DIR, "include"),
		path.join(BGFX_DIR, "3rdparty/khronos"),
		path.join(BGFX_DIR, "3rdparty"),
		BGFX_SRC_DIR,
		BX_DIR
	}
	files {
		path.join(BGFX_SRC_DIR, "bgfx.cpp"),
		path.join(BGFX_SRC_DIR, "glcontext_egl.cpp"),
		path.join(BGFX_SRC_DIR, "glcontext_glx.cpp"),
		path.join(BGFX_SRC_DIR, "glcontext_ppapi.cpp"),
		path.join(BGFX_SRC_DIR, "glcontext_wgl.cpp"),
		path.join(BGFX_SRC_DIR, "image.cpp"),
		path.join(BGFX_SRC_DIR, "shader.cpp"),
		path.join(BGFX_SRC_DIR, "topology.cpp"),
		path.join(BGFX_SRC_DIR, "hmd_ovr.cpp"),
		path.join(BGFX_SRC_DIR, "hmd_openvr.cpp"),
		path.join(BGFX_SRC_DIR, "debug_renderdoc.cpp"),
		path.join(BGFX_SRC_DIR, "renderer_d3d9.cpp"),
		path.join(BGFX_SRC_DIR, "renderer_d3d11.cpp"),
		path.join(BGFX_SRC_DIR, "renderer_d3d12.cpp"),
		path.join(BGFX_SRC_DIR, "renderer_null.cpp"),
		path.join(BGFX_SRC_DIR, "renderer_gl.cpp"),
		path.join(BGFX_SRC_DIR, "renderer_vk.cpp"),
		path.join(BGFX_SRC_DIR, "shader_dxbc.cpp"),
		path.join(BGFX_SRC_DIR, "shader_dx9bc.cpp"),
		path.join(BGFX_SRC_DIR, "shader_spirv.cpp"),
		path.join(BGFX_SRC_DIR, "vertexdecl.cpp")
	}
	configuration {"vs*"}
	-- defines {
	-- 	"BGFX_CONFIG_MULTITHREADED=0"
	-- }
	configuration {"vs*"}
	defines { "_CRT_SECURE_NO_WARNINGS" }
	links {
		"psapi"
	}
	includedirs {
		path.join(BGFX_DIR, "3rdparty/dxsdk/include"),
		path.join(BX_DIR, "compat/msvc")
	}
end
