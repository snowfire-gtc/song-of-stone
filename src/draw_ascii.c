// draw_ascii.c - ASCII рендерер для Song of Stone
#include "draw_ascii.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifndef DEDICATED_SERVER
#include "raylib.h"
#endif

#define ASCII_WIDTH 80
#define ASCII_HEIGHT 40

static bool g_ascii_enabled = false;
static char g_ascii_buffer[ASCII_HEIGHT][ASCII_WIDTH + 1];

// Символы для отрисовки блоков
static const char* get_block_char(BlockType type) {
    switch (type) {
        case BLOCK_AIR:   return " ";
        case BLOCK_DIRT:  return "#";
        case BLOCK_STONE: return "O";
        case BLOCK_WOOD:  return "=";
        case BLOCK_LEAFS: return "%";
        case BLOCK_GOLD:  return "$";
        case BLOCK_WATER: return "~";
        case BLOCK_LAVA:  return "^";
        case BLOCK_SPIKES:return "/\\";
        case BLOCK_BRIDGE:return "-";
        case BLOCK_LADDER:return "H";
        case BLOCK_DOOR:  return "+";
        case BLOCK_GRASS: return ",";
        case BLOCK_BOMB:  return "*";
        case BLOCK_SAND:  return ".";
        case BLOCK_GRAVEL:return ":";
        default:          return "?";
    }
}

// Символы для персонажей
static const char* get_char_symbol(CharacterType type, Team team, AnimationState anim, bool facing_right) {
    static char symbol[3];
    
    // Базовые символы для разных классов
    char base;
    switch (type) {
        case CHAR_WORKER: base = 'w'; break;
        case CHAR_WARRIOR: base = 'W'; break;
        case CHAR_ARCHER: base = 'a'; break;
        default: base = '?'; break;
    }
    
    // Цвет/маркер команды
    if (team == TEAM_BLUE) {
        snprintf(symbol, sizeof(symbol), "%c", base);
    } else if (team == TEAM_RED) {
        snprintf(symbol, sizeof(symbol), "%c", base + 'A' - 'a'); // Заглавная для красных
    } else {
        snprintf(symbol, sizeof(symbol), "%c", base);
    }
    
    return symbol;
}

void ascii_init(void) {
    g_ascii_enabled = false;
    memset(g_ascii_buffer, ' ', sizeof(g_ascii_buffer));
    for (int y = 0; y < ASCII_HEIGHT; y++) {
        g_ascii_buffer[y][ASCII_WIDTH] = '\0';
    }
}

void ascii_toggle_mode(void) {
    g_ascii_enabled = !g_ascii_enabled;
}

bool ascii_is_enabled(void) {
    return g_ascii_enabled;
}

// Преобразование координат мира в ASCII координаты
static void world_to_ascii(int world_x, int world_y, WorldState* world, int* out_x, int* out_y) {
    // Масштабирование: мир может быть больше экрана ASCII
    int block_size = 16;
    
    // Центрируем камеру на игроке или флаге
    int view_center_x = world->throne_blue.x;
    int view_center_y = world->throne_blue.y;
    
    // Если есть игроки, центрируем на первом игроке
    if (world->char_count > 0) {
        Character* player = &world->characters[0];
        view_center_x = player->x + 8;
        view_center_y = player->y + 16;
    }
    
    // Вычисляем область видимости
    int view_width = ASCII_WIDTH * block_size;
    int view_height = ASCII_HEIGHT * block_size;
    
    int start_x = view_center_x - view_width / 2;
    int start_y = view_center_y - view_height / 2;
    
    // Преобразуем координаты
    *out_x = (world_x - start_x) / block_size;
    *out_y = ASCII_HEIGHT - 1 - ((world_y - start_y) / block_size);
}

void ascii_render(WorldState* world) {
    // Очистка буфера
    for (int y = 0; y < ASCII_HEIGHT; y++) {
        for (int x = 0; x < ASCII_WIDTH; x++) {
            g_ascii_buffer[y][x] = ' ';
        }
    }
    
    // Определяем область видимости
    int block_size = 16;
    int view_center_x = world->throne_blue.x;
    int view_center_y = world->throne_blue.y;
    
    if (world->char_count > 0) {
        Character* player = &world->characters[world->local_player_id];
        view_center_x = player->x + 8;
        view_center_y = player->y + 16;
    }
    
    int view_width = ASCII_WIDTH * block_size;
    int view_height = ASCII_HEIGHT * block_size;
    
    int start_x_px = view_center_x - view_width / 2;
    int start_y_px = view_center_y - view_height / 2;
    
    // Отрисовка блоков
    for (int by = 0; by < world->params.height_blocks; by++) {
        for (int bx = 0; bx < world->params.width_blocks; bx++) {
            Block* block = &world->blocks[by][bx];
            
            if (block->type == BLOCK_AIR) continue;
            
            int px = bx * block_size;
            int py = by * block_size;
            
            // Проверяем, попадает ли блок в область видимости
            if (px < start_x_px || px >= start_x_px + view_width ||
                py < start_y_px || py >= start_y_px + view_height) {
                continue;
            }
            
            int ax = (px - start_x_px) / block_size;
            int ay = ASCII_HEIGHT - 1 - ((py - start_y_px) / block_size);
            
            if (ax >= 0 && ax < ASCII_WIDTH && ay >= 0 && ay < ASCII_HEIGHT) {
                const char* ch = get_block_char(block->type);
                g_ascii_buffer[ay][ax] = ch[0];
            }
        }
    }
    
    // Отрисовка тронов
    int throne_blue_ax = (world->throne_blue.x - start_x_px) / block_size;
    int throne_blue_ay = ASCII_HEIGHT - 1 - ((world->throne_blue.y - start_y_px) / block_size);
    if (throne_blue_ax >= 0 && throne_blue_ax < ASCII_WIDTH && 
        throne_blue_ay >= 0 && throne_blue_ay < ASCII_HEIGHT) {
        g_ascii_buffer[throne_blue_ay][throne_blue_ax] = 'B';
    }
    
    int throne_red_ax = (world->throne_red.x - start_x_px) / block_size;
    int throne_red_ay = ASCII_HEIGHT - 1 - ((world->throne_red.y - start_y_px) / block_size);
    if (throne_red_ax >= 0 && throne_red_ax < ASCII_WIDTH && 
        throne_red_ay >= 0 && throne_red_ay < ASCII_HEIGHT) {
        g_ascii_buffer[throne_red_ay][throne_red_ax] = 'R';
    }
    
    // Отрисовка персонажей
    for (int i = 0; i < world->char_count; i++) {
        Character* chr = &world->characters[i];
        
        int px = chr->x;
        int py = chr->y;
        
        if (px < start_x_px || px >= start_x_px + view_width ||
            py < start_y_px || py >= start_y_px + view_height) {
            continue;
        }
        
        int ax = (px - start_x_px) / block_size;
        int ay = ASCII_HEIGHT - 1 - ((py - start_y_px) / block_size);
        
        if (ax >= 0 && ax < ASCII_WIDTH && ay >= 0 && ay < ASCII_HEIGHT) {
            const char* sym = get_char_symbol(chr->type, chr->team, chr->anim_state, chr->facing_right);
            g_ascii_buffer[ay][ax] = sym[0];
            
            // Индикатор флага
            if (chr->is_holding_flag) {
                g_ascii_buffer[ay][ax] = 'F';
            }
        }
    }
    
    // Отрисовка бомб
    for (int i = 0; i < world->bomb_count; i++) {
        Bomb* bomb = &world->bombs[i];
        
        int px = bomb->x;
        int py = bomb->y;
        
        if (px < start_x_px || px >= start_x_px + view_width ||
            py < start_y_px || py >= start_y_px + view_height) {
            continue;
        }
        
        int ax = (px - start_x_px) / block_size;
        int ay = ASCII_HEIGHT - 1 - ((py - start_y_px) / block_size);
        
        if (ax >= 0 && ax < ASCII_WIDTH && ay >= 0 && ay < ASCII_HEIGHT) {
            g_ascii_buffer[ay][ax] = '@'; // Бомба
        }
    }
    
    // Отрисовка стрел
    for (int i = 0; i < world->arrow_count; i++) {
        Arrow* arrow = &world->arrows[i];
        
        if (arrow->hit) continue;
        
        int px = arrow->x;
        int py = arrow->y;
        
        if (px < start_x_px || px >= start_x_px + view_width ||
            py < start_y_px || py >= start_y_px + view_height) {
            continue;
        }
        
        int ax = (px - start_x_px) / block_size;
        int ay = ASCII_HEIGHT - 1 - ((py - start_y_px) / block_size);
        
        if (ax >= 0 && ax < ASCII_WIDTH && ay >= 0 && ay < ASCII_HEIGHT) {
            g_ascii_buffer[ay][ax] = arrow->owner_team == TEAM_BLUE ? '>' : '<';
        }
    }
    
    // Отрисовка выпавших предметов
    for (int i = 0; i < world->item_count; i++) {
        DroppedItem* item = &world->items[i];
        
        if (item->is_picked_up) continue;
        
        int px = item->x;
        int py = item->y;
        
        if (px < start_x_px || px >= start_x_px + view_width ||
            py < start_y_px || py >= start_y_px + view_height) {
            continue;
        }
        
        int ax = (px - start_x_px) / block_size;
        int ay = ASCII_HEIGHT - 1 - ((py - start_y_px) / block_size);
        
        if (ax >= 0 && ax < ASCII_WIDTH && ay >= 0 && ay < ASCII_HEIGHT) {
            char item_char;
            switch (item->type) {
                case ITEM_COINS:  item_char = 'o'; break;
                case ITEM_WOOD:   item_char = '|'; break;
                case ITEM_STONE:  item_char = '['; break;
                case ITEM_ARROWS: item_char = '^'; break;
                case ITEM_BOMBS:  item_char = '@'; break;
                case ITEM_FLAG:   item_char = 'F'; break;
                default:          item_char = '?'; break;
            }
            g_ascii_buffer[ay][ax] = item_char;
        }
    }
    
    // Вывод буфера на экран
    printf("\033[2J\033[H"); // Очистка экрана и перемещение курсора
    
    // Заголовок с информацией
    printf("=== SONG OF STONE - ASCII MODE ===\n");
    printf("Blue Score: %d | Red Score: %d\n", world->params.blue_score, world->params.red_score);
    printf("Players: %d | Bombs: %d | Arrows: %d\n\n", world->char_count, world->bomb_count, world->arrow_count);
    
    // Рамка сверху
    printf("+");
    for (int x = 0; x < ASCII_WIDTH; x++) printf("-");
    printf("+\n");
    
    // Основной контент
    for (int y = 0; y < ASCII_HEIGHT; y++) {
        printf("|%s|\n", g_ascii_buffer[y]);
    }
    
    // Рамка снизу
    printf("+");
    for (int x = 0; x < ASCII_WIDTH; x++) printf("-");
    printf("+\n");
    
    // Легенда
    printf("\nLegend: w=Worker W=Warrior a=Archer B=BlueThrone R=RedThrone\n");
    printf("        #=Dirt O=Stone ==Wood %%=Leafs $=Gold ~=Water ^=Lava\n");
    printf("        @=Bomb =>Arrow <=Arrow(Flying) o=Coin |=Wood []=Stone\n");
    printf("Press 'M' to toggle ASCII/Graphics mode\n");
    fflush(stdout);
}
