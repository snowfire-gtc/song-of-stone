// src/gen.c
#include "gen.h"
#include "perlin.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

// Статическая структура мира для избежания переполнения стека
static WorldState g_world;

void gen_dirt_with_grass_internal(Block* block, int x, int y);

// Инициализация генератора шума
void gen_init_noise(uint32_t seed) {
    perlin_init(seed);
}

// Генерация процедурного мира
WorldState* gen_world_procedural(uint32_t seed, int width, int height) {
    memset(&g_world, 0, sizeof(WorldState));
    
    // Инициализация шума
    gen_init_noise(seed);
    
    // Параметры мира
    g_world.params.width_blocks = width;
    g_world.params.height_blocks = height;
    g_world.params.gravity = 9.8f;
    g_world.params.max_speed = 200.0f;
    g_world.params.friction = 0.85f;
    g_world.params.bomb_fuse_time_seconds = 3;
    g_world.params.respawn_time_per_block = 0.5f;
    g_world.params.dropped_coin_ratio = 0.5f;
    g_world.params.flag_capture_share = 0.2f;
    
    // Стоимость построек
    g_world.params.build_costs.spikes_wood = 20;
    g_world.params.build_costs.bridge_wood = 20;
    g_world.params.build_costs.ladder_wood = 10;
    g_world.params.build_costs.door_wood = 20;
    g_world.params.build_costs.arrows_per_10_coins = 10;
    g_world.params.build_costs.bombs_per_20_coins = 1;
    
    g_world.params.blue_score = 0;
    g_world.params.red_score = 0;
    
    // Физика блоков
    g_world.params.enable_falling_blocks = true;
    g_world.params.enable_sliding_blocks = true;
    g_world.params.block_fall_delay = 0.1f;
    
    // Определение уровня поверхности с помощью шума Перлина
    int base_height = height / 2;
    float terrain_scale = 0.02f;  // Масштаб ландшафта
    float detail_scale = 0.08f;   // Масштаб деталей
    
    for (int x = 0; x < width; x++) {
        // Основной рельеф (низкие частоты)
        float main_noise = perlin_normalized_2d(x * terrain_scale, 0.0f, 4, 0.5f, 2.0f);
        // Детали (высокие частоты)
        float detail_noise = perlin_normalized_2d(x * detail_scale, 0.0f, 2, 0.3f, 2.0f);
        
        // Комбинирование шумов
        float combined = main_noise * 0.7f + detail_noise * 0.3f;
        
        // Высота поверхности для этой колонки
        int surface_y = base_height - (int)(combined * 15.0f);
        if (surface_y < 10) surface_y = 10;
        if (surface_y > height - 5) surface_y = height - 5;
        
        // Генерация колонки блоков
        for (int y = 0; y < height; y++) {
            Block* block = &g_world.blocks[y][x];
            block->x = x;
            block->y = y;
            
            if (y == 0) {
                // Дно - всегда камень
                block->type = BLOCK_STONE;
                block->has_grass = false;
            } else if (y < surface_y - 8) {
                // Глубокие слои - камень с жилами золота и пещерами
                float cave_noise = perlin_noise_2d(x * 0.05f, y * 0.05f);
                if (cave_noise > 0.6f) {
                    // Пещера
                    block->type = BLOCK_AIR;
                    block->has_grass = false;
                } else if (rand() % 25 == 0) {
                    // Золотая жила
                    block->type = BLOCK_GOLD;
                    block->has_grass = false;
                } else {
                    block->type = BLOCK_STONE;
                    block->has_grass = false;
                }
            } else if (y < surface_y - 2) {
                // Средние слои - камень с землёй
                if (rand() % 3 == 0) {
                    block->type = BLOCK_STONE;
                } else {
                    gen_dirt_with_grass_internal(block, x, y);
                }
            } else if (y < surface_y) {
                // Верхние слои - земля
                gen_dirt_with_grass_internal(block, x, y);
            } else if (y == surface_y) {
                // Поверхность - трава
                block->type = BLOCK_DIRT;
                block->has_grass = true;
                block->grass_variant = rand() % 4;
                
                // Генерация деревьев на поверхности
                if (rand() % 8 == 0 && x > 15 && x < width - 15) {
                    int tree_height = 4 + rand() % 6;
                    gen_tree(&g_world, x, y, tree_height);
                }
            } else {
                // Воздух выше поверхности
                block->type = BLOCK_AIR;
                block->has_grass = false;
            }
        }
    }
    
    // Генерация воды в низинах
    int water_level = base_height - 10;
    for (int x = 0; x < width; x++) {
        // Проверяем высоту поверхности в этой точке
        float main_noise = perlin_normalized_2d(x * terrain_scale, 0.0f, 4, 0.5f, 2.0f);
        int surface_y = base_height - (int)(main_noise * 15.0f);
        
        // Если поверхность ниже уровня воды, заполняем водой
        if (surface_y > water_level) {
            for (int y = water_level; y < surface_y && y < height; y++) {
                if (g_world.blocks[y][x].type == BLOCK_AIR) {
                    g_world.blocks[y][x].type = BLOCK_WATER;
                }
            }
        }
    }
    
    // Генерация песка на пляжах
    for (int x = 0; x < width; x++) {
        float main_noise = perlin_normalized_2d(x * terrain_scale, 0.0f, 4, 0.5f, 2.0f);
        int surface_y = base_height - (int)(main_noise * 15.0f);
        
        // Если рядом с водой
        if (abs(surface_y - water_level) <= 2) {
            // Заменяем несколько блоков земли на песок
            for (int dy = 0; dy < 3 && surface_y - dy >= 0; dy++) {
                int y = surface_y - dy;
                if (g_world.blocks[y][x].type == BLOCK_DIRT) {
                    g_world.blocks[y][x].type = BLOCK_SAND;
                    g_world.blocks[y][x].has_grass = false;
                }
            }
        }
    }
    
    // Троны и флаги
    gen_thrones_and_flags(&g_world);
    
    g_world.char_count = 0;
    g_world.item_count = 0;
    g_world.bomb_count = 0;
    g_world.game_over = false;
    g_world.winner = TEAM_NONE;
    g_world.is_multiplayer = false;
    g_world.local_player_id = 0;
    
    return &g_world;
}

void gen_tree(WorldState* world, int base_x, int base_y, int height) {
    if (height < 3 || height > 10) height = 5;
    
    // Ствол дерева
    for (int i = 0; i < height; i++) {
        int by = base_y + i;
        if (by >= world->params.height_blocks) break;
        world->blocks[by][base_x].type = BLOCK_WOOD;
        world->blocks[by][base_x].has_grass = false;
    }
    
    // Листва (конус сверху)
    int leaf_start = base_y + height - 2;
    for (int ly = leaf_start; ly < base_y + height + 2 && ly < world->params.height_blocks; ly++) {
        int radius = (ly == leaf_start) ? 1 : (ly == leaf_start + 1) ? 2 : 1;
        for (int lx = base_x - radius; lx <= base_x + radius; lx++) {
            if (lx >= 0 && lx < world->params.width_blocks) {
                if (world->blocks[ly][lx].type == BLOCK_AIR) {
                    world->blocks[ly][lx].type = BLOCK_LEAFS;
                }
            }
        }
    }
}

void gen_gold_vein(WorldState* world, int center_x, int center_y, int radius) {
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int x = center_x + dx;
            int y = center_y + dy;
            if (x >= 0 && x < world->params.width_blocks && 
                y >= 0 && y < world->params.height_blocks) {
                if (dx*dx + dy*dy <= radius*radius && rand() % 3 == 0) {
                    world->blocks[y][x].type = BLOCK_GOLD;
                    world->blocks[y][x].has_grass = false;
                }
            }
        }
    }
}

void gen_water_fill(WorldState* world, int water_level) {
    for (int y = 0; y <= water_level && y < world->params.height_blocks; y++) {
        for (int x = 0; x < world->params.width_blocks; x++) {
            if (world->blocks[y][x].type == BLOCK_AIR) {
                world->blocks[y][x].type = BLOCK_WATER;
            }
        }
    }
}

void gen_thrones_and_flags(WorldState* world) {
    // Синий трон (слева)
    world->throne_blue.x = 10 * 16;
    world->throne_blue.y = world->params.height_blocks * 16 / 2;
    
    // Красный трон (справа)
    world->throne_red.x = (world->params.width_blocks - 10) * 16;
    world->throne_red.y = world->params.height_blocks * 16 / 2;
    
    // Флаги на тронах
    world->flag_blue_pos = world->throne_blue;
    world->flag_red_pos = world->throne_red;
    world->flag_blue_carried = false;
    world->flag_red_carried = false;
    world->flag_carrier_id = -1;
}

WorldState* gen_world_default(void) {
    memset(&g_world, 0, sizeof(WorldState));
    
    // Параметры мира по умолчанию
    g_world.params.width_blocks = 256;
    g_world.params.height_blocks = 64;
    g_world.params.gravity = 9.8f;
    g_world.params.max_speed = 200.0f;
    g_world.params.friction = 0.85f;
    g_world.params.bomb_fuse_time_seconds = 3;
    g_world.params.respawn_time_per_block = 0.5f;
    g_world.params.dropped_coin_ratio = 0.5f;
    g_world.params.flag_capture_share = 0.2f;
    
    // Стоимость построек
    g_world.params.build_costs.spikes_wood = 20;
    g_world.params.build_costs.bridge_wood = 20;
    g_world.params.build_costs.ladder_wood = 10;
    g_world.params.build_costs.door_wood = 20;
    g_world.params.build_costs.arrows_per_10_coins = 10;
    g_world.params.build_costs.bombs_per_20_coins = 1;
    
    g_world.params.blue_score = 0;
    g_world.params.red_score = 0;
    
    // Физика блоков
    g_world.params.enable_falling_blocks = true;
    g_world.params.enable_sliding_blocks = true;
    g_world.params.block_fall_delay = 0.1f; // 100 мс задержка перед падением
    
    srand((unsigned int)time(NULL));
    
    // Генерация блоков
    for (int y = 0; y < g_world.params.height_blocks; y++) {
        for (int x = 0; x < g_world.params.width_blocks; x++) {
            Block* block = &g_world.blocks[y][x];
            block->x = x;
            block->y = y;
            
            // Дно карты - камень
            if (y == 0) {
                block->type = BLOCK_STONE;
                block->has_grass = false;
            }
            // Нижние слои - камень с жилами золота
            else if (y < 10) {
                if (rand() % 20 == 0) {
                    block->type = BLOCK_GOLD;
                } else {
                    block->type = BLOCK_STONE;
                }
                block->has_grass = false;
            }
            // Средние слои - почва
            else if (y < 30) {
                gen_dirt_with_grass_internal(block, x, y);
                
                // Редкие деревья
                if (y == 30 && rand() % 50 == 0) {
                    int tree_height = 3 + rand() % 8;
                    gen_tree(&g_world, x, y, tree_height);
                }
            }
            // Верхние слои - воздух
            else {
                block->type = BLOCK_AIR;
                block->has_grass = false;
            }
        }
    }
    
    // Заполнение водой нижних уровней
    gen_water_fill(&g_world, 5);
    
    // Троны и флаги
    gen_thrones_and_flags(&g_world);
    
    g_world.char_count = 0;
    g_world.item_count = 0;
    g_world.bomb_count = 0;
    g_world.game_over = false;
    g_world.winner = TEAM_NONE;
    g_world.is_multiplayer = false;
    g_world.local_player_id = 0;
    
    return &g_world;
}

void gen_dirt_with_grass_internal(Block* block, int x, int y) {
    block->x = x;
    block->y = y;
    block->type = BLOCK_DIRT;
    block->has_grass = (rand() % 3 == 0); // 33% шанс травы
    block->grass_variant = rand() % 4;    // 4 варианта травы
}
