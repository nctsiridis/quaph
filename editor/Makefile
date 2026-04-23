# --- Compilers ---
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Darwin)
    CXX := clang++
else
    CXX := g++
endif

WASM_CC := emcc

# --- Paths ---
SRC_DIR   := src
INC_DIR   := include
IMGUI_DIR := external/imgui-1.92.6
BUILD_DIR := build
OUT       := $(BUILD_DIR)/program

# --- Files ---
SRCS := $(SRC_DIR)/quaph.cpp \
        $(SRC_DIR)/setup.cpp \
        $(SRC_DIR)/util.cpp \
        $(SRC_DIR)/state.cpp \
        $(SRC_DIR)/draw.cpp

IMGUI_SRCS := $(IMGUI_DIR)/imgui.cpp \
              $(IMGUI_DIR)/imgui_draw.cpp \
              $(IMGUI_DIR)/imgui_widgets.cpp \
              $(IMGUI_DIR)/imgui_tables.cpp \
              $(IMGUI_DIR)/imgui_demo.cpp \
              $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp \
              $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o) \
        $(IMGUI_SRCS:$(IMGUI_DIR)/%.cpp=$(BUILD_DIR)/imgui/%.o)

# --- Flags ---
# -I$(INC_DIR) is the most important part here
CXXFLAGS := -std=c++17 -Wall -Wextra -I$(INC_DIR) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
LDFLAGS  := -lsfml-graphics -lsfml-window -lsfml-system -lSDL2 -lglfw

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

clean:
	rm -rf $(BUILD_DIR)

run: all
	./$(OUT)
