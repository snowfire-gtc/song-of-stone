# Song of Stone - Makefile

BUILD_DIR   := ../build
LIB_DIR     := ../libs

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -I../src -I../include
LDFLAGS := -L$(LIB_DIR) -lraylib -lm -ldl -lpthread -lX11

CFLAGS_DEBUG   := -g -O0 -DDEBUG
CFLAGS_RELEASE := -O2 -DNDEBUG -flto

SRCS := main.c logic.c gen.c draw.c sound.c \
        draw_warrior.c draw_archer.c draw_worker.c draw_bomb.c draw_arrow.c draw_ui.c \
        debug_console.c \
        logic_worker.c logic_warrior.c logic_archer.c logic_bomb.c logic_arrow.c \
        logic_physics.c logic_flags.c logic_items.c
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
TARGET := $(BUILD_DIR)/song-of-stone

.PHONY: all debug release clean run help

all: release

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += $(CFLAGS_DEBUG)
debug: $(TARGET)

release: CFLAGS += $(CFLAGS_RELEASE)
release: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

run: $(TARGET)
	cd $(BUILD_DIR) && ./song-of-stone

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Song of Stone Build System"
	@echo "  make release - build (default)"
	@echo "  make run     - run from build/"
