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
LDFLAGS  := $(RAYLIB_LIBS)

BUILD ?= debug
ifeq ($(BUILD),release)
    CFLAGS += -O2 -DNDEBUG
else
    CFLAGS += -O0 -g
endif

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all run clean re dirs

all: dirs $(BIN_DIR)/$(TARGET)

dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(BIN_DIR)/$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

run: all
	@cd $(BIN_DIR) && ASSET_DIR=../$(ASSET_DIR) ./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

re: clean all
