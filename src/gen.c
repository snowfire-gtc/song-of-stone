// src/gen.c
#include "gen.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

void gen_dirt_with_grass_internal(Block* block, int x, int y);

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

WorldState gen_world_default(void) {
    WorldState world;
    memset(&world, 0, sizeof(WorldState));
    
    // Параметры мира по умолчанию
    world.params.width_blocks = 256;
    world.params.height_blocks = 64;
    world.params.gravity = 9.8f;
    world.params.max_speed = 200.0f;
    world.params.friction = 0.85f;
    world.params.bomb_fuse_time_seconds = 3;
    world.params.respawn_time_per_block = 0.5f;
    world.params.dropped_coin_ratio = 0.5f;
    world.params.flag_capture_share = 0.2f;
    
    // Стоимость построек
    world.params.build_costs.spikes_wood = 20;
    world.params.build_costs.bridge_wood = 20;
    world.params.build_costs.ladder_wood = 10;
    world.params.build_costs.door_wood = 20;
    world.params.build_costs.arrows_per_10_coins = 10;
    world.params.build_costs.bombs_per_20_coins = 1;
    
    world.params.blue_score = 0;
    world.params.red_score = 0;
    
    srand((unsigned int)time(NULL));
    
    // Генерация блоков
    for (int y = 0; y < world.params.height_blocks; y++) {
        for (int x = 0; x < world.params.width_blocks; x++) {
            Block* block = &world.blocks[y][x];
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
                    gen_tree(&world, x, y, tree_height);
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
    gen_water_fill(&world, 5);
    
    // Троны и флаги
    gen_thrones_and_flags(&world);
    
    world.char_count = 0;
    world.item_count = 0;
    world.bomb_count = 0;
    world.game_over = false;
    world.winner = TEAM_NONE;
    world.is_multiplayer = false;
    world.local_player_id = 0;
    
    return world;
}

void gen_dirt_with_grass_internal(Block* block, int x, int y) {
    block->x = x;
    block->y = y;
    block->type = BLOCK_DIRT;
    block->has_grass = (rand() % 3 == 0); // 33% шанс травы
    block->grass_variant = rand() % 4;    // 4 варианта травы
}
