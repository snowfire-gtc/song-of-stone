// src/logic/logic_physics.c
#include "common_game.h"
#include <stdbool.h>

bool is_block_fallable(BlockType type) {
    return (type == BLOCK_SAND || type == BLOCK_DIRT || type == BLOCK_GRAVEL);
}

bool is_block_solid(BlockType type) {
    return (type != BLOCK_AIR && type != BLOCK_WATER && type != BLOCK_LEAFS);
}

bool is_in_water(const WorldState* world, int x, int y) {
    // Персонаж 2 блока высотой
    return (world->blocks[y][x].type == BLOCK_WATER) &&
           (world->blocks[y + 1][x].type == BLOCK_WATER);
}

void logic_update_oxygen_system(WorldState* world) {
    for (int i = 0; i < world->char_count; i++) {
        Character* ch = &world->characters[i];
        int x = ch->x / 16; // предположим, блок = 16x16 пикс.
        int y = ch->y / 16;

        if (is_in_water(world, x, y)) {
            ch->oxygen -= 1;
            if (ch->oxygen <= 0) {
                ch->hp = 0; // утонул
            }
        } else {
            if (ch->oxygen < PLAYER_OXYGEN_MAX) {
                ch->oxygen++;
            }
        }
    }
}

void logic_update_falling_blocks(WorldState* world) {
    if (!world->params.enable_falling_blocks) {
        return;
    }
    
    // Проходим сверху вниз (от больших y к меньшим), справа налево для корректного падения
    // В нашей системе координат y=0 это низ, поэтому блоки должны падать к уменьшению y
    for (int y = world->params.height_blocks - 1; y > 0; y--) {
        for (int x = 0; x < world->params.width_blocks; x++) {
            Block* block = &world->blocks[y][x];
            
            // Пропускаем воздух и несыпучие блоки
            if (!is_block_fallable(block->type)) {
                continue;
            }
            
            // Проверяем блок под текущим (y - 1, так как y=0 это низ)
            Block* block_below = &world->blocks[y - 1][x];
            
            // Если внизу воздух - блок падает
            if (block_below->type == BLOCK_AIR || block_below->type == BLOCK_WATER) {
                // Увеличиваем таймер падения
                block->fall_timer += 0.016f; // ~60 FPS
                
                // Если таймер превысил задержку - меняем блоки местами
                if (block->fall_timer >= world->params.block_fall_delay) {
                    BlockType falling_type = block->type;
                    bool has_grass = block->has_grass;
                    uint8_t grass_variant = block->grass_variant;
                    
                    // Очищаем текущий блок
                    block->type = BLOCK_AIR;
                    block->has_grass = false;
                    block->grass_variant = 0;
                    block->fall_timer = 0;
                    
                    // Перемещаем в блок ниже (y - 1)
                    block_below->type = falling_type;
                    block_below->has_grass = has_grass;
                    block_below->grass_variant = grass_variant;
                    block_below->fall_timer = 0;
                }
            } else {
                // Сбрасываем таймер если блок на твёрдой поверхности
                block->fall_timer = 0;
            }
        }
    }
}

void logic_update_sliding_blocks(WorldState* world) {
    if (!world->params.enable_sliding_blocks) {
        return;
    }
    
    // Логика скольжения по наклонным поверхностям
    // Для простоты: если блок сыпучий и под ним диагонально пусто - он может скатиться
    for (int y = world->params.height_blocks - 1; y > 0; y--) {
        for (int x = 0; x < world->params.width_blocks; x++) {
            Block* block = &world->blocks[y][x];
            
            if (!is_block_fallable(block->type)) {
                continue;
            }
            
            Block* block_below = &world->blocks[y - 1][x];
            
            // Если блок стоит на твёрдом, проверяем возможность скольжения
            if (is_block_solid(block_below->type)) {
                // Проверяем соседей слева и справа на предмет возможности скатиться
                if (x > 0) {
                    Block* block_left = &world->blocks[y][x - 1];
                    Block* block_below_left = &world->blocks[y - 1][x - 1];
                    
                    if (block_left->type == BLOCK_AIR && block_below_left->type == BLOCK_AIR) {
                        block->fall_timer += 0.016f;
                        if (block->fall_timer >= world->params.block_fall_delay * 2) {
                            // Скатываемся влево-вниз
                            BlockType sliding_type = block->type;
                            bool has_grass = block->has_grass;
                            uint8_t grass_variant = block->grass_variant;
                            
                            block->type = BLOCK_AIR;
                            block->has_grass = false;
                            block->grass_variant = 0;
                            block->fall_timer = 0;
                            
                            block_below_left->type = sliding_type;
                            block_below_left->has_grass = has_grass;
                            block_below_left->grass_variant = grass_variant;
                            block_below_left->fall_timer = 0;
                        }
                    }
                }
                
                if (x < world->params.width_blocks - 1) {
                    Block* block_right = &world->blocks[y][x + 1];
                    Block* block_below_right = &world->blocks[y - 1][x + 1];
                    
                    if (block_right->type == BLOCK_AIR && block_below_right->type == BLOCK_AIR) {
                        block->fall_timer += 0.016f;
                        if (block->fall_timer >= world->params.block_fall_delay * 2) {
                            // Скатываемся вправо-вниз
                            BlockType sliding_type = block->type;
                            bool has_grass = block->has_grass;
                            uint8_t grass_variant = block->grass_variant;
                            
                            block->type = BLOCK_AIR;
                            block->has_grass = false;
                            block->grass_variant = 0;
                            block->fall_timer = 0;
                            
                            block_below_right->type = sliding_type;
                            block_below_right->has_grass = has_grass;
                            block_below_right->grass_variant = grass_variant;
                            block_below_right->fall_timer = 0;
                        }
                    }
                }
            }
        }
    }
}

void logic_update_block_physics(WorldState* world) {
    logic_update_falling_blocks(world);
    logic_update_sliding_blocks(world);
}
