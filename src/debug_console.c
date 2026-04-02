// debug_console.c
#include "debug_console.h"
#include "raylib.h"
#include "draw.h"  // для get_frame_counter()
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CONSOLE_HEIGHT 200
#define MAX_INPUT_LEN 256
#define MAX_HISTORY 20

static bool console_open = false;
bool debug_mode_enabled = false; // глобальный флаг для draw_debug_vectors

static char input_buffer[MAX_INPUT_LEN] = {0};
static int input_cursor = 0;
static char history[MAX_HISTORY][MAX_INPUT_LEN] = {0};
static int history_count = 0;
static int history_index = 0;

// Вспомогательные функции
static void console_execute_command(WorldState* world, const char* cmd);
static void console_add_to_history(const char* cmd);
static void console_backspace(void);
static void console_add_char(char c);

// === ОСНОВНЫЕ ФУНКЦИИ ===

void debug_console_init(void) {
    // Ничего не нужно, кроме инициализации переменных (уже статические)
}

void debug_console_update(WorldState* world) {
    // Переключение консоли
    if (IsKeyPressed(KEY_GRAVE)) {
        console_open = !console_open;
        debug_mode_enabled = console_open; // или отдельный режим
    }

    if (!console_open) return;

    // Ввод текста
    for (int key = 32; key <= 126; key++) { // печатаемые символы
        if (IsKeyPressed(key) && input_cursor < MAX_INPUT_LEN - 1) {
            console_add_char((char)key);
        }
    }

    // Специальные клавиши
    if (IsKeyPressed(KEY_BACKSPACE)) {
        console_backspace();
    }
    if (IsKeyPressed(KEY_ENTER)) {
        if (input_buffer[0] != '\0') {
            console_execute_command(world, input_buffer);
            console_add_to_history(input_buffer);
        }
        input_buffer[0] = '\0';
        input_cursor = 0;
    }

    // История (стрелки вверх/вниз)
    if (IsKeyPressed(KEY_UP)) {
        if (history_count > 0) {
            history_index = (history_index - 1 + history_count) % history_count;
            strncpy(input_buffer, history[history_index], MAX_INPUT_LEN);
            input_cursor = strlen(input_buffer);
        }
    }
    if (IsKeyPressed(KEY_DOWN)) {
        if (history_count > 0) {
            history_index = (history_index + 1) % history_count;
            strncpy(input_buffer, history[history_index], MAX_INPUT_LEN);
            input_cursor = strlen(input_buffer);
        }
    }
}

// === ОТРИСОВКА КОНСОЛИ ===

void draw_debug_console(void) {
    if (!console_open) return;

    // Фон консоли
    DrawRectangle(0, 0, GetScreenWidth(), CONSOLE_HEIGHT, ColorAlpha(BLACK, 0.8f));
    DrawLine(0, CONSOLE_HEIGHT, GetScreenWidth(), CONSOLE_HEIGHT, GREEN);

    // Текст: "> "
    char prompt[300];
    snprintf(prompt, sizeof(prompt), "> %s", input_buffer);
    DrawText(prompt, 10, 10, 20, GREEN);

    // Курсор
    int cursor_x = 10 + MeasureText("> ", 20) + MeasureText(input_buffer, 20);
    if ((get_frame_counter() / 15) % 2 == 0) {
        DrawText("|", cursor_x, 10, 20, GREEN);
    }
}

// === ОТРИСОВКА ВЕКТОРОВ НА ИГРОВОМ ПОЛЕ ===

void draw_debug_vectors(const WorldState* world) {
    if (!debug_mode_enabled) return;

    // Векторы скорости персонажей
    for (int i = 0; i < world->char_count; i++) {
        const Character* ch = &world->characters[i];
        if (ch->hp <= 0) continue;
        Vector2 start = {ch->x, ch->y - 16};
        Vector2 end = {start.x + ch->vx * 0.1f, start.y + ch->vy * 0.1f};
        Color col = (ch->type == CHAR_WARRIOR) ? RED :
                    (ch->type == CHAR_ARCHER) ? BLUE : BROWN;
        DrawLineEx(start, end, 2, col);
    }

    // Сетка блоков (опционально)
    /*
    for (int y = 0; y < world->params.height_blocks; y++) {
        for (int x = 0; x < world->params.width_blocks; x++) {
            DrawRectangleLines(x * 16, y * 16, 16, 16, ColorAlpha(GRAY, 0.3f));
        }
    }
    */
}

// === ОБРАБОТКА КОМАНД ===

static void console_execute_command(WorldState* world, const char* cmd) {
    char command[64] = {0};
    int x = 0, y = 0, value = 0, amount = 0;
    Team team = TEAM_NONE;

    if (sscanf(cmd, "pos %d", &x) == 1) {
        // Позиция персонажа
        Character* p = &world->characters[world->local_player_id];
        TraceLog(LOG_INFO, "Player pos: (%d, %d)", p->x, p->y);
    }
    else if (strcmp(cmd, "inv") == 0) {
        Character* p = &world->characters[world->local_player_id];
        TraceLog(LOG_INFO, "Inv: $%d W:%d S:%d A:%d B:%d",
                 p->coins, p->wood, p->stone, p->arrows, p->bombs);
    }
    else if (sscanf(cmd, "block %d %d", &x, &y) == 2) {
        if (x >= 0 && x < WORLD_MAX_WIDTH && y >= 0 && y < WORLD_MAX_HEIGHT) {
            BlockType type = world->blocks[y][x].type;
            const char* names[] = {
                "AIR", "WATER", "DIRT", "LAVA", "STONE", "WOOD",
                "LEAFS", "GOLD", "SPIKES", "BRIDGE", "LADDER", "DOOR", "GRASS"
            };
            TraceLog(LOG_INFO, "Block[%d][%d] = %s", x, y, names[type]);
        }
    }
    else if (sscanf(cmd, "gravity %f", (float*)&value) == 1) {
        world->params.gravity = value;
        TraceLog(LOG_INFO, "Gravity set to %.2f", world->params.gravity);
    }
    else if (sscanf(cmd, "genitem %d %d", &value, &amount) == 2) {
        ItemType type = (ItemType)value;
        Character* p = &world->characters[world->local_player_id];
        switch (type) {
            case ITEM_COINS: p->coins += amount; break;
            case ITEM_WOOD: p->wood += amount; break;
            case ITEM_STONE: p->stone += amount; break;
            case ITEM_ARROWS: p->arrows += amount; break;
            case ITEM_BOMBS: p->bombs += amount; break;
            default: break;
        }
        TraceLog(LOG_INFO, "Added %d of item %d", amount, type);
    }
    else if (sscanf(cmd, "destroy %d %d", &x, &y) == 2) {
        if (x >= 0 && x < WORLD_MAX_WIDTH && y >= 0 && y < WORLD_MAX_HEIGHT) {
            world->blocks[y][x].type = BLOCK_AIR;
            world->blocks[y][x].has_grass = false;
            TraceLog(LOG_INFO, "Block destroyed at (%d, %d)", x, y);
        }
    }
    else if (strcmp(cmd, "debug on") == 0) {
        debug_mode_enabled = true;
        TraceLog(LOG_INFO, "Debug mode ON");
    }
    else if (strcmp(cmd, "debug off") == 0) {
        debug_mode_enabled = false;
        TraceLog(LOG_INFO, "Debug mode OFF");
    }
    else if (strcmp(cmd, "help") == 0) {
        TraceLog(LOG_INFO, "=== Available Commands ===");
        TraceLog(LOG_INFO, "pos                  - Show player position");
        TraceLog(LOG_INFO, "inv                  - Show player inventory");
        TraceLog(LOG_INFO, "block x y            - Show block type at coordinates");
        TraceLog(LOG_INFO, "gravity f            - Set gravity value");
        TraceLog(LOG_INFO, "genitem type amt     - Generate items (type, amount)");
        TraceLog(LOG_INFO, "destroy x y          - Destroy block at coordinates");
        TraceLog(LOG_INFO, "debug on/off         - Toggle debug mode");
        TraceLog(LOG_INFO, "help                 - Show this help message");
        TraceLog(LOG_INFO, "========================");
    }
    else {
        TraceLog(LOG_WARNING, "Unknown command: %s", cmd);
    }
}

// === ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ===

static void console_add_to_history(const char* cmd) {
    if (history_count < MAX_HISTORY) {
        strcpy(history[history_count], cmd);
        history_count++;
    } else {
        // Сдвигаем историю
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            strcpy(history[i], history[i + 1]);
        }
        strcpy(history[MAX_HISTORY - 1], cmd);
    }
    history_index = history_count; // после добавления — в конец
}

static void console_add_char(char c) {
    if (input_cursor < MAX_INPUT_LEN - 1) {
        input_buffer[input_cursor] = c;
        input_cursor++;
        input_buffer[input_cursor] = '\0';
    }
}

static void console_backspace(void) {
    if (input_cursor > 0) {
        input_cursor--;
        input_buffer[input_cursor] = '\0';
    }
}
