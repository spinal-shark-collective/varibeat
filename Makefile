SHADER_DIR      ?= assets/shaders
SHADER_PLATFORM ?= linux

all: shaders
	@+ make -C scripts all

clean:
	make -C scripts clean

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
	@exec ./bin/varibeat

.PHONY: all clean shaders run
