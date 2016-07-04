SHADERC         ?= ./bin/shaderc
SHADER_DIR      ?= assets/shaders
SHADER_PLATFORM ?= linux

all: shaders
	@+ make -C build all

clean:
	make -C build clean

# only needed to build tools
tools:
	@# Bug? This file breaks shaderc build.
	@mv extern/bgfx/3rdparty/glsl-optimizer/src/glsl/glcpp/glcpp-parse.y \
		extern/bgfx/3rdparty/glsl-optimizer/src/glsl/glcpp/glcpp-parse._y

	@+make -C extern/bgfx shaderc

	@# Let's be nice and put it back.
	@mv extern/bgfx/3rdparty/glsl-optimizer/src/glsl/glcpp/glcpp-parse._y \
		extern/bgfx/3rdparty/glsl-optimizer/src/glsl/glcpp/glcpp-parse.y

	@+make -C extern/bgfx texturec
	@+make -C extern/bgfx texturev

	@# Bad assumption: linux64_gcc target.
	cp extern/bgfx/.build/linux64_gcc/bin/shadercRelease bin/shaderc
	cp extern/bgfx/.build/linux64_gcc/bin/texturecRelease bin/texturec
	cp extern/bgfx/.build/linux64_gcc/bin/texturevRelease bin/texturev

shaders:
	@# Generic sprite VS+FS
	$(SHADERC) -f $(SHADER_DIR)/sprite.vs.sc \
		-o $(SHADER_DIR)/sprite.vs.bin \
		-i $(SHADER_DIR) \
		--type v \
		--platform $(SHADER_PLATFORM)
	$(SHADERC) -f $(SHADER_DIR)/sprite.fs.sc \
		-o $(SHADER_DIR)/sprite.fs.bin \
		-i $(SHADER_DIR) \
		--type f \
		--platform $(SHADER_PLATFORM)
	@# Distance field FS
	$(SHADERC) -f $(SHADER_DIR)/distance-field.fs.sc \
		-o $(SHADER_DIR)/distance-field.fs.bin \
		-i $(SHADER_DIR) \
		--type f \
		--platform $(SHADER_PLATFORM)

run: all
	@if [ -f ./bin/varibeat ];    then exec ./bin/varibeat; \
	elif [ -f ./bin/varibeat_d ]; then exec ./bin/varibeat_d; fi

.PHONY: all clean shaders run tools
