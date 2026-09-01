TARGET   := stack_overflow

SRC_DIR  := src
ASSET_DIR:= assets
OBJ_DIR  := obj
BIN_DIR  := bin
DIST_DIR := dist

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

BUILD ?= debug
ifeq ($(BUILD),release)
    CFLAGS     += -O2 -DNDEBUG
    WIN_CFLAGS += -O2 -DNDEBUG
else
    CFLAGS     += -O0 -g
    WIN_CFLAGS += -O0 -g
endif

SRCS     := $(wildcard $(SRC_DIR)/*.c)
OBJS     := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS     := $(OBJS:.o=.d)

WIN_OBJS := $(patsubst $(SRC_DIR)/%.c,$(WIN_OBJ_DIR)/%.o,$(SRCS))
WIN_DEPS := $(WIN_OBJS:.o=.d)

.PHONY: all windows run clean re dirs dist

all: dirs $(BIN_DIR)/$(TARGET)

windows: dirs $(BIN_DIR)/$(WIN_TARGET)

dirs:
	@mkdir -p $(OBJ_DIR) $(WIN_OBJ_DIR) $(BIN_DIR) $(DIST_DIR)

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

-include $(DEPS) $(WIN_DEPS)

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

clean:
	rm -rf $(OBJ_DIR) $(WIN_OBJ_DIR) $(BIN_DIR) $(DIST_DIR)

re: clean all