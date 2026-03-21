# ╔════════════════════════════════════════════════╗
# ║           Song of Stone — Makefile             ║
# ║     A medieval CTF game in blocks and valor    ║
# ╚════════════════════════════════════════════════╝

# === Настройки ===
BUILD_DIR   := ../build
LIB_DIR     := ../libs

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -I../src -I../include
LDFLAGS := -L$(LIB_DIR) -lraylib -lm -ldl -lpthread

# Оптимизации
CFLAGS_DEBUG   := -g -O0 -DDEBUG
CFLAGS_RELEASE := -O2 -DNDEBUG -flto

# Исходники
SRCS := main.c logic.c gen.c draw.c sound.c
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
TARGET := $(BUILD_DIR)/song-of-stone

GREEN  := \033[0;32m
BLUE   := \033[0;34m
CYAN   := \033[0;36m
NC     := \033[0m

# === Цели ===

.PHONY: all debug release clean run help

all: release

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@printf "$(CYAN)  📝 Compiling $<$(NC)\n"
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += $(CFLAGS_DEBUG)
debug: $(TARGET)
	@printf "$(BLUE)🔧 Debug build done.$(NC)\n"

release: CFLAGS += $(CFLAGS_RELEASE)
release: $(TARGET)
	@printf "$(GREEN)🎉 Release: $(TARGET)$(NC)\n"
	cd $(BUILD_DIR) && ./song-of-stone

$(TARGET): $(OBJS) | $(BUILD_DIR)
	@printf "$(BLUE)🔗 Linking...$(NC)\n"
	$(CC) $(OBJS) $(LDFLAGS) -o $@

run: $(TARGET)
	cd $(BUILD_DIR) && ./song-of-stone

clean:
	rm -rf $(BUILD_DIR)
	@printf "$(BLUE)🧹 Cleaned.$(NC)\n"

help:
	@echo "🎵 Song of Stone — Build System"
	@echo "  make release — сборка (по умолчанию)"
	@echo "  make run     — запуск из build/"
