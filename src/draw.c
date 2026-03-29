// src/draw.c
#include "draw.h"
#include <stdio.h>

Font ui_font = {0};  // глобальный, extern в draw_ui.c
static bool font_loaded = false;

// Глобальный счётчик кадров для замены GetFrameCounter()
static int frame_counter = 0;
int get_frame_counter(void) {
    return frame_counter;
}
void increment_frame_counter(void) {
    frame_counter++;
}

void init_ui_font_internal(void) {
    if (!font_loaded) {
        // Используем встроенный шрифт raylib
        ui_font = GetFontDefault();
        font_loaded = true;
    }
}

void draw_background(WorldState* world) {
    (void)world;
    // Простой градиентный фон (небо)
    DrawRectangleGradientV(0, 0, 1280, 720, SKYBLUE, LIGHTGRAY);
}

void draw_blocks(WorldState* world) {
    int block_size = 16;
    int start_x = 0;
    int start_y = 0;
    int end_x = world->params.width_blocks;
    int end_y = world->params.height_blocks;
    
    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            Block* block = &world->blocks[y][x];
            
            if (block->type == BLOCK_AIR) continue;
            
            Rectangle rect = {(float)x * block_size, (float)(end_y - y - 1) * block_size, 
                              (float)block_size, (float)block_size};
            
            switch (block->type) {
                case BLOCK_DIRT:
                    DrawRectangleRec(rect, BROWN);
                    if (block->has_grass) {
                        Color grass_color = (Color){0, 150 + block->grass_variant * 10, 0, 255};
                        DrawRectangle(x * block_size, (end_y - y) * block_size - 4, block_size, 4, grass_color);
                    }
                    break;
                case BLOCK_STONE:
                    DrawRectangleRec(rect, GRAY);
                    break;
                case BLOCK_WOOD:
                    DrawRectangleRec(rect, DARKBROWN);
                    break;
                case BLOCK_LEAFS:
                    DrawRectangleRec(rect, GREEN);
                    break;
                case BLOCK_GOLD:
                    DrawRectangleRec(rect, GOLD);
                    break;
                case BLOCK_WATER:
                    DrawRectangleRec(rect, (Color){0, 100, 255, 150});
                    break;
                case BLOCK_LAVA:
                    DrawRectangleRec(rect, ORANGE);
                    break;
                case BLOCK_SPIKES:
                    DrawTriangle(
                        (Vector2){rect.x, rect.y + rect.height},
                        (Vector2){rect.x + rect.width / 2, rect.y},
                        (Vector2){rect.x + rect.width, rect.y + rect.height},
                        GRAY
                    );
                    break;
                case BLOCK_BRIDGE:
                    DrawRectangleRec(rect, DARKBROWN);
                    DrawLine(rect.x, rect.y, rect.x + rect.width, rect.y, BLACK);
                    break;
                case BLOCK_LADDER:
                    DrawRectangleRec(rect, BLANK);
                    DrawLine(rect.x + 4, rect.y, rect.x + 4, rect.y + rect.height, DARKBROWN);
                    DrawLine(rect.x + 12, rect.y, rect.x + 12, rect.y + rect.height, DARKBROWN);
                    for (int i = 0; i < 4; i++) {
                        DrawLine(rect.x, rect.y + i * 4, rect.x + rect.width, rect.y + i * 4, DARKBROWN);
                    }
                    break;
                case BLOCK_DOOR:
                    DrawRectangleRec(rect, DARKBROWN);
                    DrawCircle(rect.x + rect.width/2 - 3, rect.y + rect.height/2 + 2, 2, GOLD);
                    break;
                case BLOCK_GRASS:
                    DrawRectangleRec(rect, GREEN);
                    break;
                case BLOCK_BOMB:
                    DrawCircle(rect.x + rect.width/2, rect.y + rect.height/2, rect.width/2 - 1, BLACK);
                    break;
                default:
                    DrawRectangleRec(rect, MAGENTA); // Ошибка - неизвестный блок
                    break;
            }
        }
    }
}

void draw_dropped_items(WorldState* world) {
    for (int i = 0; i < world->item_count; i++) {
        DroppedItem* item = &world->items[i];
        if (item->is_picked_up) continue;
        
        int x = item->x / 16;
        int y = item->y / 16;
        float px = x * 16.0f;
        float py = (world->params.height_blocks - y - 1) * 16.0f;
        
        switch (item->type) {
            case ITEM_COINS:
                DrawCircle(px + 8, py + 8, 6, GOLD);
                break;
            case ITEM_WOOD:
                DrawRectangle(px + 4, py + 4, 8, 8, BROWN);
                break;
            case ITEM_STONE:
                DrawRectangle(px + 4, py + 4, 8, 8, GRAY);
                break;
            case ITEM_ARROWS:
                DrawLine(px + 4, py + 8, px + 12, py + 8, WHITE);
                DrawTriangle(
                    (Vector2){px + 12, py + 8},
                    (Vector2){px + 8, py + 6},
                    (Vector2){px + 8, py + 10},
                    GRAY
                );
                break;
            case ITEM_BOMBS:
                DrawCircle(px + 8, py + 8, 7, BLACK);
                break;
            case ITEM_FLAG:
                draw_flag((Vector2){px, py}, item->team, false);
                break;
            default:
                break;
        }
    }
}

void draw_flag(Vector2 pos, Team team, bool carried) {
    float px = pos.x;
    float py = pos.y;
    
    // Флагшток
    DrawLine(px + 8, py, px + 8, py + 32, DARKGRAY);
    
    // Полотнище флага
    Color flag_color = (team == TEAM_BLUE) ? BLUE : RED;
    if (carried) {
        py -= 10; // Поднят выше при переноске
    }
    
    DrawTriangle(
        (Vector2){px + 8, py + 4},
        (Vector2){px + 24, py + 12},
        (Vector2){px + 8, py + 20},
        flag_color
    );
}
