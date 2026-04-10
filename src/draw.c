// src/draw.c
#include "draw.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

// Объявления статических функций для генерации текстур
static Texture2D GenTextureProceduralNear(int width, int height);
static Texture2D GenTextureProceduralMiddle(int width, int height);
static Texture2D GenTextureProceduralFar(int width, int height);
static Texture2D GenTextureProceduralClouds(int width, int height);
static Texture2D GenTextureProceduralSky(int width, int height);

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

// ========= ПАРАЛЛАКС ФОН =========

// Структура слоя параллакс-фона
typedef struct {
    Texture2D texture;
    float parallax_factor;  // коэффициент параллакса (0.0 - дальний, 1.0 - ближний)
    const char* name;
    bool loaded;
} ParallaxLayer;

// Слои фона в порядке увеличения глубины (от ближнего к дальнему)
// background_near, background_middle, background_far, clouds, sky
#define PARALLAX_LAYER_COUNT 5
static ParallaxLayer parallax_layers[PARALLAX_LAYER_COUNT] = {0};

// Инициализация текстур параллакс-фона
void init_parallax_background(void) {
    const char* asset_paths[] = {
        "data/textures/background/background_near.png",
        "data/textures/background/background_middle.png",
        "data/textures/background/background_far.png",
        "data/textures/background/clouds.png",
        "data/textures/background/sky.png"
    };
    
    const float parallax_factors[] = {0.8f, 0.5f, 0.3f, 0.15f, 0.0f};
    const char* layer_names[] = {"background_near", "background_middle", "background_far", "clouds", "sky"};
    
    for (int i = 0; i < PARALLAX_LAYER_COUNT; i++) {
        parallax_layers[i].name = layer_names[i];
        parallax_layers[i].parallax_factor = parallax_factors[i];
        parallax_layers[i].texture = LoadTexture(asset_paths[i]);
        parallax_layers[i].loaded = parallax_layers[i].texture.id != 0;
        
        if (!parallax_layers[i].loaded) {
            TraceLog(LOG_WARNING, "Failed to load background layer: %s", asset_paths[i]);
        }
    }
}

// Генерация текстуры ближнего фона (горы/холмы)
static Texture2D GenTextureProceduralNear(int width, int height) {
    Image img = GenImageColor(width, height, BLANK);
    
    // Рисуем силуэты гор/холмов тёмно-зелёного цвета
    Color mountain_color = (Color){30, 50, 30, 255};
    
    for (int x = 0; x < width; x++) {
        // Генерируем высоту горы с помощью синусоид
        float base_height = height * 0.6f;
        float h1 = sinf(x * 0.02f) * 50.0f;
        float h2 = sinf(x * 0.05f + 1.0f) * 30.0f;
        float h3 = sinf(x * 0.1f + 2.0f) * 15.0f;
        int mountain_height = (int)(base_height + h1 + h2 + h3);
        
        // Рисуем вертикальную линию от низа до вершины горы
        for (int y = mountain_height; y < height; y++) {
            ImageDrawPixel(&img, x, y, mountain_color);
        }
    }
    
    // Добавляем деревья на переднем плане
    Color tree_color = (Color){20, 40, 20, 255};
    for (int i = 0; i < 20; i++) {
        int tree_x = (i * 73) % width;
        int tree_base = height - 20 - (i % 5) * 10;
        // Ствол
        for (int ty = tree_base - 30; ty < tree_base; ty++) {
            ImageDrawPixel(&img, tree_x, ty, tree_color);
            ImageDrawPixel(&img, tree_x + 1, ty, tree_color);
            ImageDrawPixel(&img, tree_x + 2, ty, tree_color);
        }
        // Крона
        for (int ty = tree_base - 50; ty < tree_base - 30; ty++) {
            int crown_width = 8 - abs(ty - (tree_base - 40)) / 3;
            for (int tx = tree_x - crown_width; tx <= tree_x + crown_width; tx++) {
                ImageDrawPixel(&img, tx, ty, tree_color);
            }
        }
    }
    
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

// Генерация текстуры среднего фона (холмы)
static Texture2D GenTextureProceduralMiddle(int width, int height) {
    Image img = GenImageColor(width, height, BLANK);
    
    Color hill_color = (Color){50, 70, 50, 255};
    
    for (int x = 0; x < width; x++) {
        float base_height = height * 0.5f;
        float h1 = sinf(x * 0.015f) * 40.0f;
        float h2 = sinf(x * 0.03f + 0.5f) * 25.0f;
        float h3 = sinf(x * 0.08f + 1.5f) * 12.0f;
        int hill_height = (int)(base_height + h1 + h2 + h3);
        
        for (int y = hill_height; y < height; y++) {
            ImageDrawPixel(&img, x, y, hill_color);
        }
    }
    
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

// Генерация текстуры дальнего фона (далёкие горы)
static Texture2D GenTextureProceduralFar(int width, int height) {
    Image img = GenImageColor(width, height, BLANK);
    
    Color far_color = (Color){70, 80, 90, 255};
    
    for (int x = 0; x < width; x++) {
        float base_height = height * 0.4f;
        float h1 = sinf(x * 0.01f) * 30.0f;
        float h2 = sinf(x * 0.02f + 2.0f) * 20.0f;
        int far_height = (int)(base_height + h1 + h2);
        
        for (int y = far_height; y < height; y++) {
            ImageDrawPixel(&img, x, y, far_color);
        }
    }
    
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

// Генерация текстуры облаков
static Texture2D GenTextureProceduralClouds(int width, int height) {
    Image img = GenImageColor(width, height, BLANK);
    
    Color cloud_color = (Color){200, 200, 220, 180};
    
    // Генерируем несколько облаков
    for (int c = 0; c < 8; c++) {
        int cloud_x = (c * 137) % width;
        int cloud_y = 30 + (c * 43) % (height / 3);
        int cloud_size = 20 + (c % 4) * 10;
        
        // Рисуем пушистое облако из кругов
        for (int i = 0; i < 5; i++) {
            int px = cloud_x + (i - 2) * 15;
            int py = cloud_y + (i % 2) * 5;
            int radius = cloud_size - i * 2;
            
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    if (dx * dx + dy * dy <= radius * radius) {
                        int px_final = px + dx;
                        int py_final = py + dy;
                        if (px_final >= 0 && px_final < width && py_final >= 0 && py_final < height) {
                            ImageDrawPixel(&img, px_final, py_final, cloud_color);
                        }
                    }
                }
            }
        }
    }
    
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

// Генерация текстуры неба (градиент)
static Texture2D GenTextureProceduralSky(int width, int height) {
    Image img = GenImageColor(width, height, BLANK);
    
    Color sky_top = (Color){20, 30, 60, 255};
    Color sky_bottom = (Color){100, 150, 200, 255};
    
    for (int y = 0; y < height; y++) {
        float t = (float)y / height;
        Color current_color = (Color){
            (unsigned char)(sky_top.r + (sky_bottom.r - sky_top.r) * t),
            (unsigned char)(sky_top.g + (sky_bottom.g - sky_top.g) * t),
            (unsigned char)(sky_top.b + (sky_bottom.b - sky_top.b) * t),
            255
        };
        
        for (int x = 0; x < width; x++) {
            ImageDrawPixel(&img, x, y, current_color);
        }
    }
    
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

// Освобождение ресурсов параллакс-фона
void unload_parallax_background(void) {
    for (int i = 0; i < PARALLAX_LAYER_COUNT; i++) {
        if (parallax_layers[i].loaded && parallax_layers[i].texture.id != 0) {
            UnloadTexture(parallax_layers[i].texture);
            parallax_layers[i].texture = (Texture2D){0};
            parallax_layers[i].loaded = false;
        }
    }
}

// Отрисовка параллакс-фона
void draw_background(WorldState* world, Camera2D* camera) {
    (void)world;
    
    int screen_width = GetScreenWidth();
    
    // Получаем смещение камеры
    float camera_x = camera ? camera->target.x : 0.0f;
    float camera_y = camera ? camera->target.y : 0.0f;
    
    // Рисуем слои от дальнего к ближнему (painter's algorithm)
    for (int i = PARALLAX_LAYER_COUNT - 1; i >= 0; i--) {
        if (!parallax_layers[i].loaded || parallax_layers[i].texture.id == 0) {
            continue;
        }
        
        ParallaxLayer* layer = &parallax_layers[i];
        
        // Вычисляем смещение слоя на основе позиции камеры и коэффициента параллакса
        float parallax_offset_x = camera_x * layer->parallax_factor;
        float parallax_offset_y = camera_y * layer->parallax_factor * 0.5f; // Вертикальное движение слабее
        
        // Для создания эффекта бесконечной прокрутки используем модуль
        float texture_width = (float)layer->texture.width;
        float offset_mod = fmodf(parallax_offset_x, texture_width);
        
        // Рисуем текстуру дважды для бесшовной прокрутки
        Vector2 pos1 = {-offset_mod, -parallax_offset_y};
        Vector2 pos2 = {texture_width - offset_mod, -parallax_offset_y};
        
        // Рисуем слой
        DrawTexture(layer->texture, (int)pos1.x, (int)pos1.y, WHITE);
        DrawTexture(layer->texture, (int)pos2.x, (int)pos2.y, WHITE);
        
        // Если нужно, рисуем ещё раз для полного покрытия экрана
        if (pos1.x + texture_width < screen_width) {
            DrawTexture(layer->texture, (int)(pos2.x + texture_width), (int)pos2.y, WHITE);
        }
    }
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
    
    // Полотнище флага с анимацией
    Color flag_color = (team == TEAM_BLUE) ? BLUE : RED;
    if (carried) {
        py -= 10; // Поднят выше при переноске
    }
    
    // Анимация развевающегося флага
    int frame = get_frame_counter();
    float time = frame * 0.05f;
    
    // Многослойная волна для более реалистичного развевания
    float wave1 = sinf(time) * 3.0f;
    float wave2 = sinf(time * 1.5f + 1.0f) * 1.5f;
    float wave3 = cosf(time * 2.0f) * 0.8f;
    float total_wave = wave1 + wave2 + wave3;
    
    // Рисуем флаг с волной (основная часть)
    DrawTriangle(
        (Vector2){px + 8, py + 4},
        (Vector2){px + 24 + total_wave, py + 12},
        (Vector2){px + 8, py + 20},
        flag_color
    );
    
    // Добавляем складки для эффекта ткани
    Color shadow = (Color){flag_color.r - 40, flag_color.g - 40, flag_color.b - 40, 255};
    float fold_offset = total_wave * 0.6f;
    DrawTriangle(
        (Vector2){px + 8, py + 4},
        (Vector2){px + 16 + fold_offset, py + 12},
        (Vector2){px + 8, py + 20},
        shadow
    );
    
    // Вторая складка для глубины
    Color highlight = (Color){flag_color.r + 20, flag_color.g + 20, flag_color.b + 20, 255};
    float fold_offset2 = total_wave * 0.3f;
    DrawTriangle(
        (Vector2){px + 8, py + 4},
        (Vector2){px + 20 + fold_offset2, py + 12},
        (Vector2){px + 8, py + 20},
        highlight
    );
}
