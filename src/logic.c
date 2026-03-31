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
                ch->vy += world->params.gravity * 0.016f;
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
        item->vy += world->params.gravity * 0.016f;
        
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

// Серверная логика обновления (только для dedicated server)
#ifdef DEDICATED_SERVER
void logic_update_server(GameServer* server, WorldState* world, double dt) {
    if (!server || !world) return;
    
    // Обновление времени
    server->game_time += dt;
    
    // Обработка ввода от клиентов и обновление персонажей
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!server->clients[i].connected) continue;
        
        Character* ch = &server->characters[i];
        if (!ch->active) continue;
        
        // Применение ввода к персонажу
        if (ch->input_jump && ch->on_ground) {
            ch->vy = CHARACTER_JUMP_STRENGTH;
            ch->on_ground = false;
        }
        
        if (ch->input_left) {
            ch->vx -= CHARACTER_ACCELERATION;
            ch->facing_right = false;
        }
        if (ch->input_right) {
            ch->vx += CHARACTER_ACCELERATION;
            ch->facing_right = true;
        }
        
        // Трение
        ch->vx *= CHARACTER_FRICTION;
        
        // Гравитация
        ch->vy += world->params.gravity * dt;
        
        // Ограничение скорости
        float max_speed = CHARACTER_MAX_SPEED;
        if (ch->vx > max_speed) ch->vx = max_speed;
        if (ch->vx < -max_speed) ch->vx = -max_speed;
        if (ch->vy > CHARACTER_MAX_FALL_SPEED) ch->vy = CHARACTER_MAX_FALL_SPEED;
        
        // Предварительное перемещение по X
        int new_x = ch->x + (int)(ch->vx * dt * 60);
        
        // Проверка коллизий по X
        int left = new_x / BLOCK_SIZE;
        int right = (new_x + CHARACTER_WIDTH - 1) / BLOCK_SIZE;
        int top = ch->y / BLOCK_SIZE;
        int bottom = (ch->y + CHARACTER_HEIGHT - 1) / BLOCK_SIZE;
        
        bool collision_x = false;
        for (int by = top; by <= bottom && !collision_x; by++) {
            for (int bx = left; bx <= right && !collision_x; bx++) {
                if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
                    BlockType block = world->blocks[by][bx].type;
                    if (is_block_solid(block)) {
                        collision_x = true;
                    }
                }
            }
        }
        
        if (!collision_x) {
            ch->x = new_x;
        } else {
            ch->vx = 0;
        }
        
        // Предварительное перемещение по Y
        int new_y = ch->y + (int)(ch->vy * dt * 60);
        ch->on_ground = false;
        
        // Проверка коллизий по Y
        left = ch->x / BLOCK_SIZE;
        right = (ch->x + CHARACTER_WIDTH - 1) / BLOCK_SIZE;
        top = new_y / BLOCK_SIZE;
        bottom = (new_y + CHARACTER_HEIGHT - 1) / BLOCK_SIZE;
        
        bool collision_y = false;
        for (int by = top; by <= bottom && !collision_y; by++) {
            for (int bx = left; bx <= right && !collision_y; bx++) {
                if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
                    BlockType block = world->blocks[by][bx].type;
                    if (is_block_solid(block)) {
                        collision_y = true;
                        
                        // Если падали вниз и столкнулись - на земле
                        if (ch->vy < 0 && new_y < ch->y) {
                            ch->on_ground = true;
                        }
                    }
                }
            }
        }
        
        if (!collision_y) {
            ch->y = new_y;
        } else {
            if (ch->vy < 0) {
                ch->vy = 0;
            }
        }
        
        // Ограничение границами мира
        if (ch->x < 0) ch->x = 0;
        if (ch->x > WORLD_WIDTH * BLOCK_SIZE - CHARACTER_WIDTH) {
            ch->x = WORLD_WIDTH * BLOCK_SIZE - CHARACTER_WIDTH;
        }
        if (ch->y < 0) {
            ch->y = 0;
            ch->vy = 0;
        }
        if (ch->y > WORLD_HEIGHT * BLOCK_SIZE - CHARACTER_HEIGHT) {
            ch->y = WORLD_HEIGHT * BLOCK_SIZE - CHARACTER_HEIGHT;
            ch->vy = 0;
            ch->on_ground = true;
        }
        
        // Проверка смерти (падение, удушье)
        logic_check_character_death((WorldState*)world, ch);
    }
    
    server->total_ticks++;
}
#endif // DEDICATED_SERVER
