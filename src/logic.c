// src/logic.c
#include "logic.h"
#include "particles.h"
#include "sound.h"
#include <string.h>

extern ParticleSystem g_particles;

// Проверка смерти персонажа (кислород, HP ≤ 0, падение)
void logic_check_character_death(WorldState* world, Character* ch) {
    // Если уже мёртв — пропускаем
    if (ch->hp <= 0) {
        return;
    }
    
    // Смерть от недостатка кислорода уже обработана в logic_update
    
    // Проверка на смерть от падения
    // (реализуется отдельно при приземлении)
    
    // Если HP упал до 0 или ниже — смерть
    if (ch->hp <= 0) {
        // Спавн частиц смерти
        particles_spawn_death(&g_particles, ch->x, ch->y, ch->team);
        
        // Звук смерти
        sound_play(SOUND_PLAYER_DEATH);
        
        // Возрождение
        logic_respawn_character(world, ch);
    }
}

// Возрождение персонажа у своего трона
void logic_respawn_character(WorldState* world, Character* ch) {
    // Определение позиции возрождения по команде
    Vector2 spawn_pos;
    if (ch->team == TEAM_BLUE) {
        spawn_pos = world->throne_blue;
    } else if (ch->team == TEAM_RED) {
        spawn_pos = world->throne_red;
    } else {
        // Если нет команды — спавн в центре
        spawn_pos.x = (world->params.width_blocks * 16) / 2;
        spawn_pos.y = (world->params.height_blocks * 16) / 2;
    }
    
    // Расчёт времени возрождения (зависит от расстояния до трона)
    int dist_x = abs(ch->x - (int)spawn_pos.x) / 16;
    int dist_y = abs(ch->y - (int)spawn_pos.y) / 16;
    int max_dist = (dist_x > dist_y) ? dist_x : dist_y;
    float respawn_time = world->params.respawn_time_per_block * max_dist;
    
    // Ограничение: не больше 120 секунд
    if (respawn_time > 120.0f) respawn_time = 120.0f;
    if (respawn_time < 2.0f) respawn_time = 2.0f; // минимум 2 секунды
    
    // Сброс состояния персонажа
    ch->x = (int)spawn_pos.x;
    ch->y = (int)spawn_pos.y + 32; // чуть выше трона
    ch->vx = 0;
    ch->vy = 0;
    ch->hp = PLAYER_MAX_HP;
    ch->oxygen = PLAYER_OXYGEN_MAX;
    ch->is_shield_active = false;
    ch->is_charging = false;
    ch->charge_time = 0;
    ch->anim_state = ANIM_IDLE;
    ch->frame_counter = 0;
    ch->is_aiming = false;
    ch->aim_time = 0;
    ch->is_climbing = false;
    
    // Если держал флаг — сбросить
    if (ch->is_holding_flag) {
        ch->is_holding_flag = false;
        // Флаг остаётся там, где умер (будет подобран позже)
    }
    
    // Включение неуязвимости после возрождения
    ch->is_invulnerable = true;
    ch->invuln_timer = respawn_time; // время неуязвимости = время возрождения
    
    // Логирование (для отладки)
    // printf("Player %s (%s) resurrected at (%d, %d). Invulnerable for %.1f sec\n",
    //        ch->name, ch->team == TEAM_BLUE ? "BLUE" : "RED",
    //        ch->x, ch->y, respawn_time);
}

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
                
                // Частицы удушья
                if (ch->hp > 0 && frame_counter % 30 == 0) {
                    particles_spawn_drowning(&g_particles, ch->x, ch->y + 8);
                }
            }
        } else {
            if (ch->oxygen < PLAYER_OXYGEN_MAX) {
                ch->oxygen++;
            }
        }
        
        // Проверка смерти после всех обновлений
        logic_check_character_death(world, ch);
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
