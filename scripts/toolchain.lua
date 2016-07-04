local BX_DIR = path.getabsolute("../extern/bx")

function toolchain(_buildDir, _objDir, _libDir)
	newoption {
		trigger = "gcc",
		value = "GCC",
		description = "Choose GCC flavor",
		allowed = {
			{ "android-arm",     "Android - ARM"              },
			{ "android-mips",    "Android - MIPS"             },
			{ "android-x86",     "Android - x86"              },
			{ "linux-gcc",       "Linux (GCC compiler)"       },
			{ "linux-clang",     "Linux (Clang compiler)"     },
			{ "gcw0",            "GCW0" },
			{ "linux-arm-gcc",   "Linux (ARM, GCC compiler)"  },
			{ "mingw-gcc",       "MinGW"                      },
			{ "mingw-clang",     "MinGW (Clang compiler)"     },
			{ "rpi",             "RaspberryPi"                },
		},
	}

	newoption {
		trigger = "vs",
		value = "toolset",
		description = "Choose VS toolset",
		allowed = {
			{ "vs2013-clang",  "Clang 3.6"                       },
			{ "vs2015-clang",  "Clang 3.9"                       },
			{ "vs2013-xp",     "Visual Studio 2013 targeting XP" },
			{ "vs2015-xp",     "Visual Studio 2015 targeting XP" },
		},
	}

	newoption {
		trigger = "with-android",
		value   = "#",
		description = "Set Android platform version (default: android-14).",
	}

	newoption {
		trigger = "with-dynamic-runtime",
		description = "Dynamically link with the runtime rather than statically",
	}

	-- Avoid error when invoking genie --help.
	if (_ACTION == nil) then return false end

	location (path.join(_buildDir, "projects", _ACTION))

	if _ACTION == "clean" then
		os.rmdir(_buildDir)
		os.mkdir(_buildDir)
		os.rmdir(_objDir)
		os.mkdir(_objDir)
		os.exit(1)
	end

	local androidPlatform = "android-14"
	if _OPTIONS["with-android"] then
		androidPlatform = "android-" .. _OPTIONS["with-android"]
	end

	if _ACTION == "gmake" then
		if nil == _OPTIONS["gcc"] then
			print("GCC flavor must be specified!")
			os.exit(1)
		end

		flags {
			"ExtraWarnings",
		}

		if "android-arm" == _OPTIONS["gcc"] then
			if not os.getenv("ANDROID_NDK_ARM") or not os.getenv("ANDROID_NDK_ROOT") then
				print("Set ANDROID_NDK_ARM and ANDROID_NDK_ROOT envrionment variables.")
			end

			premake.gcc.cc  = "$(ANDROID_NDK_ARM)/bin/arm-linux-androideabi-gcc"
			premake.gcc.cxx = "$(ANDROID_NDK_ARM)/bin/arm-linux-androideabi-g++"
			premake.gcc.ar  = "$(ANDROID_NDK_ARM)/bin/arm-linux-androideabi-ar"
			location (path.join(_buildDir, "projects", _ACTION .. "-android-arm"))
		elseif "android-mips" == _OPTIONS["gcc"] then
			if not os.getenv("ANDROID_NDK_MIPS") or not os.getenv("ANDROID_NDK_ROOT") then
				print("Set ANDROID_NDK_MIPS and ANDROID_NDK_ROOT envrionment variables.")
			end

			premake.gcc.cc  = "$(ANDROID_NDK_MIPS)/bin/mipsel-linux-android-gcc"
			premake.gcc.cxx = "$(ANDROID_NDK_MIPS)/bin/mipsel-linux-android-g++"
			premake.gcc.ar  = "$(ANDROID_NDK_MIPS)/bin/mipsel-linux-android-ar"
			location (path.join(_buildDir, "projects", _ACTION .. "-android-mips"))
		elseif "android-x86" == _OPTIONS["gcc"] then
			if not os.getenv("ANDROID_NDK_X86") or not os.getenv("ANDROID_NDK_ROOT") then
				print("Set ANDROID_NDK_X86 and ANDROID_NDK_ROOT envrionment variables.")
			end

			premake.gcc.cc  = "$(ANDROID_NDK_X86)/bin/i686-linux-android-gcc"
			premake.gcc.cxx = "$(ANDROID_NDK_X86)/bin/i686-linux-android-g++"
			premake.gcc.ar  = "$(ANDROID_NDK_X86)/bin/i686-linux-android-ar"
			location (path.join(_buildDir, "projects", _ACTION .. "-android-x86"))
		elseif "linux-gcc" == _OPTIONS["gcc"] then
			location (path.join(_buildDir, "projects", _ACTION .. "-linux"))
		elseif "linux-gcc-5" == _OPTIONS["gcc"] then
			premake.gcc.cc  = "gcc-5"
			premake.gcc.cxx = "g++-5"
			premake.gcc.ar  = "ar"
			location (path.join(_buildDir, "projects", _ACTION .. "-linux"))
		elseif "linux-clang" == _OPTIONS["gcc"] then
			premake.gcc.cc  = "clang"
			premake.gcc.cxx = "clang++"
			premake.gcc.ar  = "ar"
			location (path.join(_buildDir, "projects", _ACTION .. "-linux-clang"))
		elseif "linux-mips-gcc" == _OPTIONS["gcc"] then
			location (path.join(_buildDir, "projects", _ACTION .. "-linux-mips-gcc"))
		elseif "linux-arm-gcc" == _OPTIONS["gcc"] then
			location (path.join(_buildDir, "projects", _ACTION .. "-linux-arm-gcc"))
		elseif "mingw-gcc" == _OPTIONS["gcc"] then
			premake.gcc.cc  = "$(MINGW)/bin/x86_64-w64-mingw32-gcc"
			premake.gcc.cxx = "$(MINGW)/bin/x86_64-w64-mingw32-g++"
			premake.gcc.ar  = "$(MINGW)/bin/ar"
			location (path.join(_buildDir, "projects", _ACTION .. "-mingw-gcc"))
		elseif "mingw-clang" == _OPTIONS["gcc"] then
			premake.gcc.cc   = "$(CLANG)/bin/clang"
			premake.gcc.cxx  = "$(CLANG)/bin/clang++"
			premake.gcc.ar   = "$(MINGW)/bin/ar"
--			premake.gcc.ar   = "$(CLANG)/bin/llvm-ar"
--			premake.gcc.llvm = true
			location (path.join(_buildDir, "projects", _ACTION .. "-mingw-clang"))
		elseif "rpi" == _OPTIONS["gcc"] then
			location (path.join(_buildDir, "projects", _ACTION .. "-rpi"))
		end
	elseif _ACTION == "vs2013" or _ACTION == "vs2015" then
		if (_ACTION .. "-clang") == _OPTIONS["vs"] then
			if "vs2015-clang" == _OPTIONS["vs"] then
				premake.vstudio.toolset = ("LLVM-vs2014")
			else
				premake.vstudio.toolset = ("LLVM-" .. _ACTION)
			end
			location (path.join(_buildDir, "projects", _ACTION .. "-clang"))
		elseif "vs2013-xp" == _OPTIONS["vs"] then
			premake.vstudio.toolset = ("v120_xp")
			location (path.join(_buildDir, "projects", _ACTION .. "-xp"))
		elseif "vs2015-xp" == _OPTIONS["vs"] then
			premake.vstudio.toolset = ("v140_xp")
			location (path.join(_buildDir, "projects", _ACTION .. "-xp"))
		end
	end

	if not _OPTIONS["with-dynamic-runtime"] then
		flags { "StaticRuntime" }
	end

	flags {
		"NoPCH",
		"NativeWChar",
		"NoRTTI",
		"NoExceptions",
		"NoEditAndContinue",
		"NoFramePointer",
		"Symbols",
	}

	defines {
		"__STDC_LIMIT_MACROS",
		"__STDC_FORMAT_MACROS",
		"__STDC_CONSTANT_MACROS",
	}

	configuration { "Debug" }
		targetsuffix "_d"

	configuration { "Release" }
		flags {
			"NoBufferSecurityCheck",
			"OptimizeSpeed",
		}

	configuration { "vs*", "x32" }
		flags {
			"EnableSSE2",
		}

	configuration { "vs*" }
		includedirs { path.join(BX_DIR, "include/compat/msvc") }
		defines {
			"WIN32",
			"_WIN32",
			"_HAS_EXCEPTIONS=0",
			"_HAS_ITERATOR_DEBUGGING=0",
			"_SCL_SECURE=0",
			"_SECURE_SCL=0",
			"_SCL_SECURE_NO_WARNINGS",
			"_CRT_SECURE_NO_WARNINGS",
			"_CRT_SECURE_NO_DEPRECATE",
		}
		buildoptions {
			"/wd4201", -- warning C4201: nonstandard extension used: nameless struct/union
			"/wd4324", -- warning C4324: '': structure was padded due to alignment specifier
			"/Ob2",    -- The Inline Function Expansion
		}
		linkoptions {
			"/ignore:4221", -- LNK4221: This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library
		}

	configuration { "x32", "vs*" }
		targetdir (path.join(_buildDir, "win32_" .. _ACTION, "bin"))
		objdir (path.join(_objDir, "win32_" .. _ACTION, "obj"))
		libdirs {
			path.join(_libDir, "lib/win32_" .. _ACTION),
		}

	configuration { "x64", "vs*" }
		defines { "_WIN64" }
		targetdir (path.join(_buildDir, "win64_" .. _ACTION, "bin"))
		objdir (path.join(_objDir, "win64_" .. _ACTION, "obj"))
		libdirs {
			path.join(_libDir, "lib/win64_" .. _ACTION),
		}

	configuration { "ARM", "vs*" }
		targetdir (path.join(_buildDir, "arm_" .. _ACTION, "bin"))
		objdir (path.join(_objDir, "arm_" .. _ACTION, "obj"))

	configuration { "vs*-clang" }
		buildoptions {
			"-Qunused-arguments",
		}

	configuration { "x32", "vs*-clang" }
		targetdir (path.join(_buildDir, "win32_" .. _ACTION .. "-clang/bin"))
		objdir (path.join(_objDir, "win32_" .. _ACTION .. "-clang/obj"))

	configuration { "x64", "vs*-clang" }
		targetdir (path.join(_buildDir, "win64_" .. _ACTION .. "-clang/bin"))
		objdir (path.join(_objDir, "win64_" .. _ACTION .. "-clang/obj"))

	configuration { "winphone8* or winstore8*" }
		removeflags {
			"StaticRuntime",
			"NoExceptions",
		}

	configuration { "*-gcc* or osx" }
		buildoptions {
			"-Wshadow",
		}

	configuration { "mingw-*" }
		defines { "WIN32" }
		includedirs { path.join(BX_DIR, "include/compat/mingw") }
		buildoptions {
			"-Wunused-value",
			"-fdata-sections",
			"-ffunction-sections",
			"-msse2",
			"-Wunused-value",
			"-Wundef",
		}
		buildoptions_cpp {
			"-std=c++0x",
		}
		linkoptions {
			"-Wl,--gc-sections",
			"-static",
			"-static-libgcc",
			"-static-libstdc++",
		}

	configuration { "x32", "mingw-gcc" }
		targetdir (path.join(_buildDir, "win32_mingw-gcc/bin"))
		objdir (path.join(_objDir, "win32_mingw-gcc/obj"))
		libdirs {
			path.join(_libDir, "lib/win32_mingw-gcc"),
		}
		buildoptions {
			"-m32",
			"-mstackrealign",
		}

	configuration { "x64", "mingw-gcc" }
		targetdir (path.join(_buildDir, "win64_mingw-gcc/bin"))
		objdir (path.join(_objDir, "win64_mingw-gcc/obj"))
		libdirs {
			path.join(_libDir, "lib/win64_mingw-gcc"),
		}
		buildoptions { "-m64" }

	configuration { "mingw-clang" }
		buildoptions {
			"-isystem$(MINGW)/lib/gcc/x86_64-w64-mingw32/4.8.1/include/c++",
			"-isystem$(MINGW)/lib/gcc/x86_64-w64-mingw32/4.8.1/include/c++/x86_64-w64-mingw32",
			"-isystem$(MINGW)/x86_64-w64-mingw32/include",
		}
		linkoptions {
			"-Qunused-arguments",
			"-Wno-error=unused-command-line-argument-hard-error-in-future",
		}

	configuration { "x32", "mingw-clang" }
		targetdir (path.join(_buildDir, "win32_mingw-clang/bin"))
		objdir (path.join(_objDir, "win32_mingw-clang/obj"))
		libdirs {
			path.join(_libDir, "lib/win32_mingw-clang"),
		}
		buildoptions { "-m32" }

	configuration { "x64", "mingw-clang" }
		targetdir (path.join(_buildDir, "win64_mingw-clang/bin"))
		objdir (path.join(_objDir, "win64_mingw-clang/obj"))
		libdirs {
			path.join(_libDir, "lib/win64_mingw-clang"),
		}
		buildoptions { "-m64" }

	configuration { "linux-clang" }

	configuration { "linux-gcc-5" }
		buildoptions {
--			"-fno-omit-frame-pointer",
--			"-fsanitize=address",
--			"-fsanitize=undefined",
--			"-fsanitize=float-divide-by-zero",
--			"-fsanitize=float-cast-overflow",
		}
		links {
--			"asan",
--			"ubsan",
		}

	configuration { "linux-gcc" }
		buildoptions {
			"-mfpmath=sse",
		}

	configuration { "linux-gcc* or linux-clang*" }
		buildoptions {
			"-msse2",
			"-Wunused-value",
			"-Wundef",
		}
		buildoptions_cpp {
			"-std=c++0x",
		}
		links {
			"rt",
			"dl",
		}
		linkoptions {
			"-Wl,--gc-sections",
		}

	configuration { "linux-gcc*", "x32" }
		targetdir (path.join(_buildDir, "linux32_gcc/bin"))
		objdir (path.join(_objDir, "linux32_gcc/obj"))
		libdirs { path.join(_libDir, "lib/linux32_gcc") }
		buildoptions {
			"-m32",
		}

	configuration { "linux-gcc*", "x64" }
		targetdir (path.join(_buildDir, "linux64_gcc/bin"))
		objdir (path.join(_objDir, "linux64_gcc/obj"))
		libdirs { path.join(_libDir, "lib/linux64_gcc") }
		buildoptions {
			"-m64",
		}

	configuration { "linux-clang*", "x32" }
		targetdir (path.join(_buildDir, "linux32_clang/bin"))
		objdir (path.join(_objDir, "linux32_clang/obj"))
		libdirs { path.join(_libDir, "lib/linux32_clang") }
		buildoptions {
			"-m32",
		}

	configuration { "linux-clang*", "x64" }
		targetdir (path.join(_buildDir, "linux64_clang/bin"))
		objdir (path.join(_objDir, "linux64_clang/obj"))
		libdirs { path.join(_libDir, "lib/linux64_clang") }
		buildoptions {
			"-m64",
		}

	configuration { "linux-mips-gcc" }
		targetdir (path.join(_buildDir, "linux32_mips_gcc/bin"))
		objdir (path.join(_objDir, "linux32_mips_gcc/obj"))
		libdirs { path.join(_libDir, "lib/linux32_mips_gcc") }
		buildoptions {
			"-Wunused-value",
			"-Wundef",
		}
		buildoptions_cpp {
			"-std=c++0x",
		}
		links {
			"rt",
			"dl",
		}
		linkoptions {
			"-Wl,--gc-sections",
		}

	configuration { "linux-arm-gcc" }
		targetdir (path.join(_buildDir, "linux32_arm_gcc/bin"))
		objdir (path.join(_objDir, "linux32_arm_gcc/obj"))
		libdirs { path.join(_libDir, "lib/linux32_arm_gcc") }
		buildoptions {
			"-Wunused-value",
			"-Wundef",
		}
		buildoptions_cpp {
			"-std=c++0x",
		}
		links {
			"rt",
			"dl",
		}
		linkoptions {
			"-Wl,--gc-sections",
		}

	configuration { "android-*" }
		targetprefix ("lib")
		flags {
			"NoImportLib",
		}
		includedirs {
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.8/include",
			"$(ANDROID_NDK_ROOT)/sources/android/native_app_glue",
		}
		linkoptions {
			"-nostdlib",
			"-static-libgcc",
		}
		links {
			"c",
			"dl",
			"m",
			"android",
			"log",
			"gnustl_static",
			"gcc",
		}
		buildoptions {
			"-fPIC",
			"-no-canonical-prefixes",
			"-Wa,--noexecstack",
			"-fstack-protector",
			"-ffunction-sections",
			"-Wno-psabi", -- note: the mangling of 'va_list' has changed in GCC 4.4.0
			"-Wunused-value",
			"-Wundef",
		}
		buildoptions_cpp {
			"-std=c++0x",
		}
		linkoptions {
			"-no-canonical-prefixes",
			"-Wl,--no-undefined",
			"-Wl,-z,noexecstack",
			"-Wl,-z,relro",
			"-Wl,-z,now",
		}

	configuration { "android-arm" }
		targetdir (path.join(_buildDir, "android-arm/bin"))
		objdir (path.join(_objDir, "android-arm/obj"))
		libdirs {
			path.join(_libDir, "lib/android-arm"),
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi-v7a",
		}
		includedirs {
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi-v7a/include",
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.9/include",
		}
		buildoptions {
			"--sysroot=" .. path.join("$(ANDROID_NDK_ROOT)/platforms", androidPlatform, "arch-arm"),
			"-mthumb",
			"-march=armv7-a",
			"-mfloat-abi=softfp",
			"-mfpu=neon",
			"-Wunused-value",
			"-Wundef",
		}
		linkoptions {
			"--sysroot=" .. path.join("$(ANDROID_NDK_ROOT)/platforms", androidPlatform, "arch-arm"),
			path.join("$(ANDROID_NDK_ROOT)/platforms", androidPlatform, "arch-arm/usr/lib/crtbegin_so.o"),
			path.join("$(ANDROID_NDK_ROOT)/platforms", androidPlatform, "arch-arm/usr/lib/crtend_so.o"),
			"-march=armv7-a",
			"-Wl,--fix-cortex-a8",
		}

	configuration { "android-mips" }
		targetdir (path.join(_buildDir, "android-mips/bin"))
		objdir (path.join(_objDir, "android-mips/obj"))
		libdirs {
			path.join(_libDir, "lib/android-mips"),
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.8/libs/mips",
		}
		includedirs {
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.8/libs/mips/include",
		}
		buildoptions {
			"--sysroot=" .. path.join("$(ANDROID_NDK_ROOT)/platforms", androidPlatform, "arch-mips"),
			"-Wunused-value",
			"-Wundef",
		}
		linkoptions {
			"--sysroot=" .. path.join("$(ANDROID_NDK_ROOT)/platforms", androidPlatform, "arch-mips"),
			path.join("$(ANDROID_NDK_ROOT)/platforms", androidPlatform, "arch-mips/usr/lib/crtbegin_so.o"),
			path.join("$(ANDROID_NDK_ROOT)/platforms", androidPlatform, "arch-mips/usr/lib/crtend_so.o"),
		}

	configuration { "android-x86" }
		targetdir (path.join(_buildDir, "android-x86/bin"))
		objdir (path.join(_objDir, "android-x86/obj"))
		libdirs {
			path.join(_libDir, "lib/android-x86"),
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.8/libs/x86",
		}
		includedirs {
			"$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/4.8/libs/x86/include",
		}
		buildoptions {
			"--sysroot=" .. path.join("$(ANDROID_NDK_ROOT)/platforms", androidPlatform, "arch-x86"),
			"-march=i686",
			"-mtune=atom",
			"-mstackrealign",
			"-msse3",
			"-mfpmath=sse",
			"-Wunused-value",
			"-Wundef",
		}
		linkoptions {
			"--sysroot=" .. path.join("$(ANDROID_NDK_ROOT)/platforms", androidPlatform, "arch-x86"),
			path.join("$(ANDROID_NDK_ROOT)/platforms", androidPlatform, "arch-x86/usr/lib/crtbegin_so.o"),
			path.join("$(ANDROID_NDK_ROOT)/platforms", androidPlatform, "arch-x86/usr/lib/crtend_so.o"),
		}

	configuration { "rpi" }
		targetdir (path.join(_buildDir, "rpi/bin"))
		objdir (path.join(_objDir, "rpi/obj"))
		libdirs {
			path.join(_libDir, "lib/rpi"),
			"/opt/vc/lib",
		}
		defines {
			"__VCCOREVER__=0x04000000", -- There is no special prefedined compiler symbol to detect RaspberryPi, faking it.
			"__STDC_VERSION__=199901L",
		}
		buildoptions {
			"-Wunused-value",
			"-Wundef",
		}
		buildoptions_cpp {
			"-std=c++0x",
		}
		includedirs {
			"/opt/vc/include",
			"/opt/vc/include/interface/vcos/pthreads",
			"/opt/vc/include/interface/vmcs_host/linux",
		}
		links {
			"rt",
		}
		linkoptions {
			"-Wl,--gc-sections",
		}

	configuration {} -- reset configuration

	return true
end

function strip()
	configuration { "android-arm", "Release" }
		postbuildcommands {
			"$(SILENT) echo Stripping symbols.",
			"$(SILENT) $(ANDROID_NDK_ARM)/bin/arm-linux-androideabi-strip -s \"$(TARGET)\""
		}

	configuration { "android-mips", "Release" }
		postbuildcommands {
			"$(SILENT) echo Stripping symbols.",
			"$(SILENT) $(ANDROID_NDK_MIPS)/bin/mipsel-linux-android-strip -s \"$(TARGET)\""
		}

	configuration { "android-x86", "Release" }
		postbuildcommands {
			"$(SILENT) echo Stripping symbols.",
			"$(SILENT) $(ANDROID_NDK_X86)/bin/i686-linux-android-strip -s \"$(TARGET)\""
		}

	configuration { "linux-* or rpi", "Release" }
		postbuildcommands {
			"$(SILENT) echo Stripping symbols.",
			"$(SILENT) strip -s \"$(TARGET)\""
		}

	configuration { "mingw*", "Release" }
		postbuildcommands {
			"$(SILENT) echo Stripping symbols.",
			"$(SILENT) $(MINGW)/bin/strip -s \"$(TARGET)\""
		}

	configuration {} -- reset configuration
end
