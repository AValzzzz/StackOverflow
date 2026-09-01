TARGET   := stack_overflow

SRC_DIR  := src
ASSET_DIR:= assets
OBJ_DIR  := obj
BIN_DIR  := bin

CC       := gcc
STD      := -std=c11
WARN     := -Wall -Wextra -Wpedantic
INCLUDES := -I$(SRC_DIR)

RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS   := $(shell pkg-config --libs raylib 2>/dev/null)

ifeq ($(strip $(RAYLIB_LIBS)),)
    RAYLIB_LIBS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif

CFLAGS   := $(STD) $(WARN) $(INCLUDES) $(RAYLIB_CFLAGS)
LDFLAGS  := $(RAYLIB_LIBS) -lm

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

.PHONY: all windows run clean re dirs

all: dirs $(BIN_DIR)/$(TARGET)

windows: dirs $(BIN_DIR)/$(WIN_TARGET)

dirs:
	@mkdir -p $(OBJ_DIR) $(WIN_OBJ_DIR) $(BIN_DIR)

$(BIN_DIR)/$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BIN_DIR)/$(WIN_TARGET): $(WIN_OBJS)
	$(WIN_CC) $(WIN_OBJS) -o $@ $(WIN_LDFLAGS)

$(WIN_OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	$(WIN_CC) $(WIN_CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS) $(WIN_DEPS)

run: all
	@./$(BIN_DIR)/$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(WIN_OBJ_DIR) $(BIN_DIR)

re: clean all