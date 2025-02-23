.PHONY: spikes build

CC:=clang
SED:=sed
BASE64:=base64 -i
# Linux
# BASE64=base64 -w 0
STACK_SIZE:=$$(( 8 * 1024 * 1024 ))

# Compiler and linker flags
CFLAGS:=-ggdb -O2 \
	-Wall -Wextra -Wpedantic \
	-Wformat=2 -Wno-unused-parameter -Wshadow \
	-Wwrite-strings -Wstrict-prototypes -Wold-style-definition \
	-Wredundant-decls -Wnested-externs -Wmissing-include-dirs \
	-Wno-unused
WASMFLAGS:=--target=wasm32 -std=c99 \
	-nostdlib \
	-Os -flto \
	-Wl,--allow-undefined \
	-Wl,--export-dynamic \
	-Wl,--no-entry \
	-Wl,--lto-O3 \
	-Wl,--export-all \
	-Wl,-z,stack-size=$(STACK_SIZE) \
	-mbulk-memory
LDFLAGS:=-lm

# Source and target files
PLAIN_JS_MIN=release/pl_synth.min.js
WASM_SRC=wasm/pl_synth_wasm_module.c
WASM_TARGET=wasm/pl_synth.wasm

WASM_TEMPLATE=wasm/pl_synth_wasm_template.js
WASM_JS_MIN=release/pl_synth_wasm.min.js

NATIVE_SRC=src/example.c src/pl_synth.h
NATIVE_BIN=build/example

.PHONY: all clean

all: $(WASM_JS_MIN) $(NATIVE_BIN)

# Embed WASM module into JS
$(WASM_JS_MIN): $(WASM_TEMPLATE) $(WASM_TARGET)
	@ $(SED) "s|{{WASM_MODULE_EMBEDDED_HERE_AS_BASE64}}|$(shell $(BASE64) $(WASM_TARGET))|" $(WASM_TEMPLATE) > $@
	cp tracker.html release/index.html

# Compile the WASM module
$(WASM_TARGET): $(WASM_SRC)
	$(CC) $(CFLAGS) $(WASMFLAGS) -o $@ $<

# Target: Compiled native example
$(NATIVE_BIN): $(NATIVE_SRC)
	mkdir -p build
	$(CC) $< $(CFLAGS) -o $@ $(LDFLAGS)

# Clean up generated files
clean:
	rm -f $(PLAIN_JS_MIN) $(WASM_TARGET) $(WASM_JS_MIN) $(NATIVE_BIN)
	rm -f release/index.html
	rm -rf build

serve:
	busboy --secure --root=release --api-host=127.0.0.1:8080

spikes:
	busboy --secure --root=spikes
