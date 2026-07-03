# --- Compilers ---
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Darwin)
    CXX := clang++
else
    CXX := g++
endif

# --- Paths ---
SRC_DIR   := src
INC_DIR   := include
IMGUI_DIR := external/imgui-1.92.6
JSON_DIR  := external/json/single_include
WS_DIR    := external/easywsclient
BUILD_DIR := build
OUT       := $(BUILD_DIR)/program

# --- Files ---
SRCS := $(SRC_DIR)/quaph.cpp \
        $(SRC_DIR)/setup.cpp \
        $(SRC_DIR)/util.cpp \
        $(SRC_DIR)/state.cpp \
        $(SRC_DIR)/draw.cpp \
        $(SRC_DIR)/net.cpp \
        $(SRC_DIR)/remote.cpp

IMGUI_SRCS := $(IMGUI_DIR)/imgui.cpp \
              $(IMGUI_DIR)/imgui_draw.cpp \
              $(IMGUI_DIR)/imgui_widgets.cpp \
              $(IMGUI_DIR)/imgui_tables.cpp \
              $(IMGUI_DIR)/imgui_demo.cpp \
              $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp \
              $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o) \
        $(IMGUI_SRCS:$(IMGUI_DIR)/%.cpp=$(BUILD_DIR)/imgui/%.o) \
        $(BUILD_DIR)/easywsclient.o

# --- Flags ---
# -I$(INC_DIR) is the most important part here
CXXFLAGS := -std=c++17 -Wall -Wextra -I$(INC_DIR) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(JSON_DIR) -I$(WS_DIR)
LDFLAGS  := -lSDL2

ifeq ($(UNAME_S), Darwin)
    CXXFLAGS += -I/opt/homebrew/include -I/opt/homebrew/include/SDL2
    LDFLAGS  += -L/opt/homebrew/lib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
else
    # Linux Mint / Ubuntu pathing
    CXXFLAGS += $(shell pkg-config --cflags sdl2 2>/dev/null || echo "-I/usr/include/SDL2")
    LDFLAGS  += -lGL -lX11 -lpthread -ldl
endif

# --- Rules ---

.PHONY: all clean run

all: $(OUT)

# Build the final binary
$(OUT): $(OBJS) $(SRC_DIR)/main.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/main.cpp $(OBJS) $(LDFLAGS) -o $(OUT)

# Compile project objects
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile ImGui objects (kept separate to avoid clutter)
$(BUILD_DIR)/imgui/%.o: $(IMGUI_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Native-only WebSocket client (not used in the wasm build, which talks to
# the browser's own WebSocket via emscripten/websocket.h instead)
$(BUILD_DIR)/easywsclient.o: $(WS_DIR)/easywsclient.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

run: all
	./$(OUT)

# --- Emscripten / WASM build ---
# Requires emsdk activated (see https://emscripten.org/docs/getting_started/downloads.html
# and `source /path/to/emsdk/emsdk_env.sh`) so `em++` is on PATH.
EMCC     := em++
WEB_DIR  := $(BUILD_DIR)/web
WASM_OUT := $(WEB_DIR)/index.html

WASM_OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(WEB_DIR)/obj/%.o) \
             $(IMGUI_SRCS:$(IMGUI_DIR)/%.cpp=$(WEB_DIR)/obj/imgui/%.o)

# No easywsclient here: the wasm build talks to the browser's own WebSocket
# via emscripten/websocket.h instead (see net.cpp's __EMSCRIPTEN__ branch).
WASM_CXXFLAGS := -std=c++17 -Wall -Wformat -Os -I$(INC_DIR) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(JSON_DIR) \
                  -s USE_SDL=2 -DIMGUI_DISABLE_FILE_FUNCTIONS
# GROWABLE_ARRAYBUFFERS=0: works around a browser TextDecoder incompatibility
# (decode() rejects a "resizable" ArrayBuffer) that otherwise crashes early,
# e.g. inside SDL_SetWindowTitle, whenever ALLOW_MEMORY_GROWTH is on and the
# browser supports the newer resizable-ArrayBuffer growth path.
WASM_LDFLAGS := -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=0 -s ASSERTIONS=1 -s NO_FILESYSTEM=1 \
                -s GROWABLE_ARRAYBUFFERS=0 \
                -s USE_SDL=2 -lwebsocket.js \
                --shell-file $(IMGUI_DIR)/examples/libs/emscripten/shell_minimal.html

.PHONY: wasm wasm-serve

wasm: $(WASM_OUT)

$(WASM_OUT): $(WASM_OBJS) $(SRC_DIR)/main.cpp
	@mkdir -p $(WEB_DIR)
	$(EMCC) $(WASM_CXXFLAGS) $(SRC_DIR)/main.cpp $(WASM_OBJS) $(WASM_LDFLAGS) -o $(WASM_OUT)

$(WEB_DIR)/obj/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(EMCC) $(WASM_CXXFLAGS) -c $< -o $@

$(WEB_DIR)/obj/imgui/%.o: $(IMGUI_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(EMCC) $(WASM_CXXFLAGS) -c $< -o $@

wasm-serve: wasm
	python3 -m http.server -d $(WEB_DIR)
