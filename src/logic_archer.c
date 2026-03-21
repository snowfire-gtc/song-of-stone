// logic_archer.c
#include "logic_archer.h"
#include "../sound/sound.h"
#include <math.h>

#define ARCHER_NORMAL_SPEED 130.0f
#define ARCHER_CLIMB_SPEED 80.0f
#define ARCHER_MAX_ARROWS 200
#define ARCHER_MIN_SHOOT_POWER 0.2f
#define FALL_DAMAGE_THRESHOLD 10
#define FALL_DAMAGE_PER_STEP 4

// Обновление лучника
void logic_archer_update(Character* archer, WorldState* world, int frame_counter) {
    // Неуязвимость после возрождения
    if (archer->is_invulnerable) {
        archer->invuln_timer -= GetFrameTime();
        if (archer->invuln_timer <= 0) {
            archer->is_invulnerable = false;
        }
    }

    // Обновление натяжки лука
    if (archer->is_aiming) {
        archer->aim_time += GetFrameTime();
    }

    // Лазание
    if (archer->is_climbing) {
        logic_archer_update_climbing(archer, world);
    }

    // Смешная анимация (если долго стоит)
    if (!archer->is_aiming && !archer->is_climbing && archer->vx == 0 && archer->vy == 0) {
        if (archer->frame_counter > 300) { // 5 секунд
            archer->anim_state = ANIM_IDLE_FUNNY;
        }
    }
}

// Стрельба
// В logic_archer_shoot_arrow()
void logic_archer_shoot_arrow(Character* archer, WorldState* world, float power) {
    if (archer->arrows <= 0 || power < ARCHER_MIN_SHOOT_POWER) return;

    archer->arrows--;
    archer->is_aiming = false;
    archer->aim_time = 0.0f;
    sound_play_at(SOUND_ARROW_FLY, archer->x, archer->y);

    // Создание стрелы
    if (world->arrow_count < MAX_PLAYERS * 10) {
        Arrow* arrow = &world->arrows[world->arrow_count];
        arrow->x = archer->x + (archer->team == TEAM_BLUE ? 10 : -10);
        arrow->y = archer->y - 8;
        arrow->vx = (archer->team == TEAM_BLUE ? 1 : -1) * (200.0f + power * 400.0f);
        arrow->vy = -50.0f * power; // выше при большей силе
        arrow->owner_team = archer->team;
        arrow->hit = false;
        arrow->rotation = 0;
        world->arrow_count++;
    }
}

// Добыча стрел
void logic_archer_harvest_arrows(Character* archer, WorldState* world) {
    int x = archer->x / 16;
    int y = archer->y / 16;

    // Проверка дерева (BLOCK_WOOD)
    if (y >= 0 && y < WORLD_MAX_HEIGHT && x >= 0 && x < WORLD_MAX_WIDTH) {
        if (world->blocks[y][x].type == BLOCK_WOOD) {
            // Удаление блока дерева
            world->blocks[y][x].type = BLOCK_AIR;
            // Сдвиг верхних блоков вниз
            for (int i = y + 1; i < WORLD_MAX_HEIGHT; i++) {
                if (world->blocks[i][x].type == BLOCK_WOOD) {
                    world->blocks[i - 1][x] = world->blocks[i][x];
                    world->blocks[i][x].type = BLOCK_AIR;
                } else {
                    break;
                }
            }
            archer->arrows += 2;
            play_sound(SOUND_HARVEST_WOOD);
            return;
        }

        // Проверка листвы (BLOCK_LEAFS)
        if (world->blocks[y][x].type == BLOCK_LEAFS) {
            world->blocks[y][x].type = BLOCK_AIR;
            archer->arrows += 1;
            play_sound(SOUND_HARVEST_LEAFS);
        }
    }
}

// Может ли лезть?
bool logic_archer_can_climb_block(BlockType block_type) {
    return block_type == BLOCK_WOOD || block_type == BLOCK_STONE;
}

// Обновление лазания
void logic_archer_update_climbing(Character* archer, WorldState* world) {
    // Простая реализация: удержание позиции и медленное движение вверх/вниз
    if (IsKeyDown(KEY_W)) {
        archer->y -= ARCHER_CLIMB_SPEED * GetFrameTime();
    }
    if (IsKeyDown(KEY_S)) {
        archer->y += ARCHER_CLIMB_SPEED * GetFrameTime();
    }

    // Проверка: всё ещё рядом с блоком?
    int block_x = archer->climbing_block_x;
    int block_y = archer->y / 16;
    if (block_y < 0 || block_y >= WORLD_MAX_HEIGHT ||
        world->blocks[block_y][block_x].type != BLOCK_WOOD &&
        world->blocks[block_y][block_x].type != BLOCK_STONE) {
        archer->is_climbing = false;
    }
}

// Получение урона
void logic_archer_take_damage(Character* archer, int damage) {
    if (archer->is_invulnerable) return;
    archer->hp -= damage;
    if (archer->hp < 0) archer->hp = 0;
    play_sound(SOUND_PLAYER_HURT);
}

// Падение на листву
bool logic_archer_survive_fall_on_leafs(Character* archer, WorldState* world, int fall_height) {
    if (fall_height <= 10) {
        int x = archer->x / 16;
        int y = archer->y / 16;
        if (y >= 0 && y < WORLD_MAX_HEIGHT && x >= 0 && x < WORLD_MAX_WIDTH) {
            if (world->blocks[y][x].type == BLOCK_LEAFS) {
                // Зацепился за листву — урона нет
                archer->y = y * 16; // прилип к блоку
                archer->vy = 0;
                return true;
            }
        }
    }
    return false;
}
