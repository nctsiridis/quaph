CC= clang++
WASM_CC= emcc
MAIN= src/main.cpp
EXAMPLE = src/example.cpp
SRC= external/imgui-1.92.6/backends/imgui_impl_sdl2.cpp \
	external/imgui-1.92.6/imgui_widgets.cpp \
	external/imgui-1.92.6/imgui.cpp \
	external/imgui-1.92.6/imgui_draw.cpp \
	external/imgui-1.92.6/imgui_tables.cpp \
	external/imgui-1.92.6/backends/imgui_impl_opengl3.cpp \
	external/imgui-1.92.6/imgui_demo.cpp \
	src/quaph.cpp \
	src/setup.cpp \
	src/util.cpp \
	src/graphics.cpp

OUT= build/program
WASM_OUT= build/program.html

DFLAGS= -Wall -Wextra
CFLAGS= -I/opt/homebrew/include -I/opt/homebrew/include/SDL2
LDFLAGS = -L/opt/homebrew/lib \
	-lsfml-graphics \
	-lsfml-window \
	-lsfml-system \
	-lSDL2 \
	-lglfw \
	-framework OpenGL \
	-framework Cocoa \
	-framework IOKit \
	-framework CoreVideo
WASM_FLAGS = \
	-s USE_SDL=2 \
	-s FULL_ES3=1 \
	-s WASM=1 \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s ASSERTIONS=1 \
	-s NO_EXIT_RUNTIME=1 \
	-s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'

OTHER= -std=c++17

setup:
	mkdir -p build

build: $(MAIN) $(SRC) setup
	$(CC) $(OTHER) $(DFlAGS) $(CFLAGS) $(LDFLAGS) $(MAIN) $(SRC) -o $(OUT)

build-wasm: $(MAIN) $(SRC) setup
	$(WASM_CC) $(OTHER) $(DFlAGS) $(WASM_FLAGS) $(MAIN) $(SRC) -o $(WASM_OUT)

build-example: $(EXAMPLE) $(SRC) setup
	$(CC) $(OTHER) $(DFlAGS) $(CFLAGS) $(LDFLAGS) $(EXAMPLE) $(SRC) -o $(OUT)

build-wasm-example: $(EXAMPLE) $(SRC) setup
	$(WASM_CC) $(OTHER) $(DFlAGS) $(WASM_FLAGS) $(EXAMPLE) $(SRC) -o $(WASM_OUT)

run: build $(OUT)
	./$(OUT)

run-wasm: build-wasm $(WASM_OUT)
	@python3 -m http.server 8000 >/dev/null 2>&1 & \
	sleep 0.5; \
	open http://localhost:8000/$(WASM_OUT)

run-example: build-example $(OUT)
	./$(OUT)

run-wasm-example: build-wasm-example $(WASM_OUT)
	@python3 -m http.server 8000 >/dev/null 2>&1 & \
	sleep 0.5; \
	open http://localhost:8000/$(WASM_OUT)
