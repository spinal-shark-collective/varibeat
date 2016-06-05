SHADER_DIR      ?= assets/shaders
SHADER_PLATFORM ?= linux

all: shaders
	@+ make -C scripts all

clean:
	make -C scripts clean

shaders:
	shaderc -f $(SHADER_DIR)/test.vs.sc \
		-o $(SHADER_DIR)/test.vs.bin \
		-i $(SHADER_DIR) \
		--type v \
		--platform $(SHADER_PLATFORM)
	shaderc -f $(SHADER_DIR)/test.fs.sc \
		-o $(SHADER_DIR)/test.fs.bin \
		-i $(SHADER_DIR) \
		--type f \
		--platform $(SHADER_PLATFORM)

run: all
	@exec ./bin/myon

.PHONY: all clean shaders run
