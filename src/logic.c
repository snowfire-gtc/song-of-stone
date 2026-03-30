// src/logic.c
#include "logic.h"
#include "particles.h"
#include "sound.h"
#include <string.h>

extern ParticleSystem g_particles;

void logic_update(WorldState* world, int frame_counter) {
    (void)frame_counter;
    
    // Обновление физики блоков (обрушение и скольжение)
    logic_update_block_physics(world);
    
    // Обновление персонажей
    for (int i = 0; i < world->char_count; i++) {
        Character* ch = &world->characters[i];
        
        // Гравитация
        if (ch->y > 0) {
            int block_x = ch->x / 16;
            int block_y = ch->y / 16;
            
            Block* block_below = logic_get_block(world, block_x, block_y - 1);
            if (block_below && block_below->type == BLOCK_AIR) {
                ch->vy -= world->params.gravity * 0.016f;
            } else {
                // Проверка на шаг — частицы при движении
                if ((ch->vx > 0.5f || ch->vx < -0.5f) && (frame_counter % 10 == 0)) {
                    BlockType ground = block_below ? block_below->type : BLOCK_DIRT;
                    particles_spawn_step(&g_particles, ch->x, ch->y + 16, ground);
                }
            }
        }
        
        // Применение скорости
        ch->x += (int)(ch->vx * 0.016f);
        ch->y += (int)(ch->vy * 0.016f);
        
        // Ограничение по карте
        if (ch->x < 0) ch->x = 0;
        if (ch->x >= world->params.width_blocks * 16) ch->x = world->params.width_blocks * 16 - 1;
        
        // Смерть при падении ниже нуля
        if (ch->y < 0) {
            ch->hp = 0;
        }
        
        // Таймер неуязвимости
        if (ch->is_invulnerable) {
            ch->invuln_timer -= 0.016f;
            if (ch->invuln_timer <= 0) {
                ch->is_invulnerable = false;
            }
        }
        
        // Кислород в воде
        int head_block_y = (ch->y + 32) / 16;
        int feet_block_y = ch->y / 16;
        int block_x = ch->x / 16;
        
        Block* head_block = logic_get_block(world, block_x, head_block_y);
        Block* feet_block = logic_get_block(world, block_x, feet_block_y);
        
        if (head_block && feet_block && 
            head_block->type == BLOCK_WATER && feet_block->type == BLOCK_WATER) {
            ch->oxygen--;
            if (ch->oxygen <= 0) {
                ch->hp--;
                ch->oxygen = 0;
            }
        } else {
            if (ch->oxygen < PLAYER_OXYGEN_MAX) {
                ch->oxygen++;
            }
        }
    }
    
    // Обновление предметов
    for (int i = 0; i < world->item_count; i++) {
        DroppedItem* item = &world->items[i];
        if (item->is_picked_up) continue;
        
        // Гравитация для предметов
        item->y += (int)(item->vy * 0.016f);
        item->vy -= world->params.gravity * 0.016f;
        
        if (item->y < 0) {
            item->y = 0;
            item->vy = 0;
        }
    }
}

bool logic_check_collision(int x, int y, BlockType block) {
    return (block != BLOCK_AIR && block != BLOCK_WATER && block != BLOCK_LEAFS);
}

Block* logic_get_block(WorldState* world, int x, int y) {
    if (x < 0 || x >= WORLD_MAX_WIDTH || y < 0 || y >= WORLD_MAX_HEIGHT) {
        return NULL;
    }
    return &world->blocks[y][x];
}

void logic_set_block(WorldState* world, int x, int y, BlockType type) {
    if (x >= 0 && x < world->params.width_blocks && 
        y >= 0 && y < world->params.height_blocks) {
        world->blocks[y][x].type = type;
        world->blocks[y][x].has_grass = false;
    }
}
