SHADER_DIR      ?= assets/shaders
SHADER_PLATFORM ?= linux

all: shaders
	@+ make -C build all

clean:
	make -C build clean

shaders:
	@# Generic sprite VS+FS
	shaderc -f $(SHADER_DIR)/sprite.vs.sc \
		-o $(SHADER_DIR)/sprite.vs.bin \
		-i $(SHADER_DIR) \
		--type v \
		--platform $(SHADER_PLATFORM)
	shaderc -f $(SHADER_DIR)/sprite.fs.sc \
		-o $(SHADER_DIR)/sprite.fs.bin \
		-i $(SHADER_DIR) \
		--type f \
		--platform $(SHADER_PLATFORM)
	@# Distance field FS
	shaderc -f $(SHADER_DIR)/distance-field.fs.sc \
		-o $(SHADER_DIR)/distance-field.fs.bin \
		-i $(SHADER_DIR) \
		--type f \
		--platform $(SHADER_PLATFORM)

run: all
	@if [ -f ./bin/varibeat ];    then exec ./bin/varibeat; \
	elif [ -f ./bin/varibeat_d ]; then exec ./bin/varibeat_d; fi

.PHONY: all clean shaders run
