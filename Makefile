TARGET   := stack_overflow

SRC_DIR  := src
ASSET_DIR:= assets
OBJ_DIR  := obj
BIN_DIR  := bin
DIST_DIR := dist
WEB_DIR  := web

CC       := gcc
STD      := -std=c11
WARN     := -Wall -Wextra -Wpedantic
INCLUDES := -I$(SRC_DIR)

# --- Configuration Linux (Statique) ---
LIN_RAYLIB   := lib/raylib-linux
CFLAGS       := $(STD) $(WARN) $(INCLUDES) -I$(LIN_RAYLIB)/include
LDFLAGS      := $(LIN_RAYLIB)/lib/libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11

# --- Configuration Windows (Cross-compilation Statique) ---
WIN_CC       := x86_64-w64-mingw32-gcc
WIN_OBJ_DIR  := obj_win
WIN_TARGET   := $(TARGET).exe
WIN_RAYLIB   := lib/raylib-win64
WIN_INCLUDES := $(INCLUDES) -I$(WIN_RAYLIB)/include
WIN_CFLAGS   := $(STD) $(WARN) $(WIN_INCLUDES)
WIN_LDFLAGS  := -L$(WIN_RAYLIB)/lib -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32 -static

# --- Configuration Web (Emscripten) ---
WEB_CC       := emcc
WEB_OBJ_DIR  := obj_web
WEB_TARGET   := index.html
WEB_RAYLIB   := lib/raylib-web
WEB_SHELL    := platform/web/shell.html
WEB_INCLUDES := $(INCLUDES) -I$(WEB_RAYLIB)/include
WEB_CFLAGS   := -std=gnu11 $(WARN) -Wno-variadic-macro-arguments-omitted \
                $(WEB_INCLUDES) -DPLATFORM_WEB
WEB_LDFLAGS  := $(WEB_RAYLIB)/lib/libraylib.a -sUSE_GLFW=3 -sFORCE_FILESYSTEM=1 \
                -sALLOW_MEMORY_GROWTH=1 -sEXPORTED_FUNCTIONS=_main,_AppReady \
                -sEXPORTED_RUNTIME_METHODS=HEAPF32,HEAP8,HEAPU8,HEAP16,HEAPU16,HEAP32,HEAPU32,HEAPF64,requestFullscreen \
                -lidbfs.js --shell-file $(WEB_SHELL) \
                --preload-file $(ASSET_DIR)

BUILD ?= debug
ifeq ($(BUILD),release)
    CFLAGS     += -O2 -DNDEBUG
    WIN_CFLAGS += -O2 -DNDEBUG
    WEB_CFLAGS += -O2 -DNDEBUG
else
    CFLAGS     += -O0 -g
    WIN_CFLAGS += -O0 -g
    WEB_CFLAGS += -O0 -g
    WEB_LDFLAGS += -sASSERTIONS=1
endif

ifeq ($(DEBUG_MENU),1)
    CFLAGS     += -DENABLE_DEBUG_MENU
    WIN_CFLAGS += -DENABLE_DEBUG_MENU
    WEB_CFLAGS += -DENABLE_DEBUG_MENU
endif

SRCS     := $(wildcard $(SRC_DIR)/*.c)
OBJS     := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS     := $(OBJS:.o=.d)

WIN_OBJS := $(patsubst $(SRC_DIR)/%.c,$(WIN_OBJ_DIR)/%.o,$(SRCS))
WIN_DEPS := $(WIN_OBJS:.o=.d)

WEB_OBJS := $(patsubst $(SRC_DIR)/%.c,$(WEB_OBJ_DIR)/%.o,$(SRCS))
WEB_DEPS := $(WEB_OBJS:.o=.d)

.PHONY: all windows web run clean re dirs dist dist-web DEBUG

all: dirs $(BIN_DIR)/$(TARGET)

windows: dirs $(BIN_DIR)/$(WIN_TARGET)

web: dirs $(WEB_DIR)/$(WEB_TARGET)

DEBUG:
	$(MAKE) DEBUG_MENU=1 all

dirs:
	@mkdir -p $(OBJ_DIR) $(WIN_OBJ_DIR) $(WEB_OBJ_DIR) $(BIN_DIR) $(DIST_DIR) $(WEB_DIR)

# --- Compilation Linux ---
$(BIN_DIR)/$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# --- Compilation Windows ---
$(BIN_DIR)/$(WIN_TARGET): $(WIN_OBJS)
	$(WIN_CC) $(WIN_OBJS) -o $@ $(WIN_LDFLAGS)

$(WIN_OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	$(WIN_CC) $(WIN_CFLAGS) -MMD -MP -c $< -o $@

# --- Compilation Web ---
$(WEB_DIR)/$(WEB_TARGET): $(WEB_OBJS)
	$(WEB_CC) $(WEB_OBJS) -o $(WEB_DIR)/$(WEB_TARGET) $(WEB_LDFLAGS)

$(WEB_OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	$(WEB_CC) $(WEB_CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS) $(WIN_DEPS) $(WEB_DEPS)

run: all
	@./$(BIN_DIR)/$(TARGET)

dist: BUILD=release
dist: re windows
	@rm -rf $(DIST_DIR)/*
	@echo "Création de la distribution Linux..."
	@mkdir -p $(DIST_DIR)/$(TARGET)_linux
	@cp $(BIN_DIR)/$(TARGET) $(DIST_DIR)/$(TARGET)_linux/
	@cp -r $(ASSET_DIR) $(DIST_DIR)/$(TARGET)_linux/
	@cd $(DIST_DIR) && zip -r $(TARGET)_linux.zip $(TARGET)_linux > /dev/null
	@echo "Création de la distribution Windows..."
	@mkdir -p $(DIST_DIR)/$(TARGET)_windows
	@cp $(BIN_DIR)/$(WIN_TARGET) $(DIST_DIR)/$(TARGET)_windows/
	@cp -r $(ASSET_DIR) $(DIST_DIR)/$(TARGET)_windows/
	@cd $(DIST_DIR) && zip -r $(TARGET)_windows.zip $(TARGET)_windows > /dev/null
	@echo "Fichiers ZIP générés dans le dossier $(DIST_DIR)/"

dist-web:
	@rm -rf $(WEB_OBJ_DIR) $(WEB_DIR)
	$(MAKE) BUILD=release web
	@mkdir -p $(DIST_DIR)
	@rm -f $(DIST_DIR)/StackOverflow-Web.zip
	@cd $(WEB_DIR) && zip -r ../$(DIST_DIR)/StackOverflow-Web.zip . > /dev/null
	@echo "Web zip créé : $(DIST_DIR)/StackOverflow-Web.zip"

clean:
	rm -rf $(OBJ_DIR) $(WIN_OBJ_DIR) $(WEB_OBJ_DIR) $(BIN_DIR) $(DIST_DIR) $(WEB_DIR)

re: clean all