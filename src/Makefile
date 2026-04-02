# Song of Stone - Makefile

BUILD_DIR   := ../build
LIB_DIR     := ../libs

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -I../src -I../include
LDFLAGS := -L$(LIB_DIR) -lraylib -lm -ldl -lpthread -lX11

CFLAGS_DEBUG   := -g -O0 -DDEBUG
CFLAGS_RELEASE := -O2 -DNDEBUG -flto

# Основная игра (клиент с сетевой поддержкой)
SRCS := main.c logic.c gen.c draw.c sound.c \
        draw_warrior.c draw_archer.c draw_worker.c draw_bomb.c draw_arrow.c draw_ui.c draw_ascii.c \
        debug_console.c \
        logic_worker.c logic_warrior.c logic_archer.c logic_bomb.c logic_arrow.c \
        logic_physics.c logic_flags.c logic_items.c \
        particles.c net_protocol.c menu.c settings.c perlin.c \
        local_server.c net_server.c client.c
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
TARGET := $(BUILD_DIR)/song-of-stone

# Сервер (старый - для тестов)
SERVER_SRCS := server.c gen.c logic.c logic_physics.c logic_flags.c logic_items.c net_protocol.c perlin.c
SERVER_OBJS := $(SERVER_SRCS:%.c=$(BUILD_DIR)/server_%.o)
SERVER_TARGET := $(BUILD_DIR)/server

# Dedicated Server (новый, полноценный)
DEDICATED_SRCS := server_main.c gen.c logic.c logic_physics.c logic_flags.c logic_items.c \
                  net_protocol.c net_server.c sound.c particles.c perlin.c
DEDICATED_OBJS := $(DEDICATED_SRCS:%.c=$(BUILD_DIR)/dedicated_%.o)
DEDICATED_TARGET := $(BUILD_DIR)/dedicated_server

.PHONY: all debug release clean run help server

all: release

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/server_%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += $(CFLAGS_DEBUG)
debug: $(TARGET)

release: CFLAGS += $(CFLAGS_RELEASE)
release: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

# Сборка сервера (старый)
server: $(SERVER_OBJS) | $(BUILD_DIR)
	$(CC) $(SERVER_OBJS) -o $(SERVER_TARGET) -lpthread

# Сборка Dedicated Server (новый)
dedicated: $(DEDICATED_OBJS) | $(BUILD_DIR)
	$(CC) $(DEDICATED_OBJS) -o $(DEDICATED_TARGET) -lpthread

run: $(TARGET)
	cd $(BUILD_DIR) && ./song-of-stone

run-server: server
	cd $(BUILD_DIR) && ./server

run-dedicated: dedicated
	cd $(BUILD_DIR) && ./dedicated_server

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Song of Stone Build System"
	@echo "  make release       - build game (default)"
	@echo "  make run           - run game from build/"
	@echo "  make server        - build old test server"
	@echo "  make run-server    - run old test server"
	@echo "  make dedicated     - build dedicated server"
	@echo "  make run-dedicated - run dedicated server"
	@echo "  make clean         - clean build files"
