// logic_bomb.c
#include "logic_bomb.h"
#include "sound.h"
#include <math.h>

#define BOMB_RADIUS_PIXELS 32   // 2 блока = 32 пикс (если блок = 16x16)
#define BOMB_GRAVITY 400.0f
#define BOMB_FRICTION 0.98f
#define BOMB_EXPLOSION_DAMAGE 100 // мгновенная смерть без щита

// Создание бомбы
Bomb logic_bomb_create(int x, int y, float vx, float vy, int owner_id, Team owner_team, float fuse_time) {
    Bomb bomb = {0};
    bomb.x = x;
    bomb.y = y;
    bomb.vx = vx;
    bomb.vy = vy;
    bomb.state = BOMB_STATE_FLYING;
    bomb.timer = fuse_time;
    bomb.owner_id = owner_id;
    bomb.owner_team = owner_team;
    bomb.exploded = false;
    return bomb;
}

// Обновление всех бомб
void logic_bomb_update_all(WorldState* world, float delta_time) {
    for (int i = 0; i < world->bomb_count; i++) {
        Bomb* bomb = &world->bombs[i];
        if (bomb->exploded) continue;

        // Уменьшение таймера
        bomb->timer -= delta_time;
        if (bomb->timer <= 0) {
            logic_bomb_explode(world, i);
            continue;
        }

        // Физика полёта (если летит)
        if (bomb->state == BOMB_STATE_FLYING) {
            // Гравитация
            bomb->vy += BOMB_GRAVITY * delta_time;

            // Движение
            bomb->x += (int)(bomb->vx * delta_time);
            bomb->y += (int)(bomb->vy * delta_time);

            // Проверка выхода за пределы мира
            if (bomb->y > WORLD_MAX_HEIGHT * 16 || bomb->x < 0 || bomb->x > WORLD_MAX_WIDTH * 16) {
                bomb->exploded = true;
                continue;
            }

            // Проверка коллизии с блоками
            int block_x = bomb->x / 16;
            int block_y = bomb->y / 16;

            if (block_y >= 0 && block_y < WORLD_MAX_HEIGHT &&
                block_x >= 0 && block_x < WORLD_MAX_WIDTH) {

                BlockType block = world->blocks[block_y][block_x].type;
                if (block != BLOCK_AIR && block != BLOCK_WATER && block != BLOCK_LEAFS) {
                    // Приземлилась
                    bomb->state = BOMB_STATE_PLANTED;
                    bomb->vx = 0;
                    bomb->vy = 0;
                    // Таймер уже идёт
                }
            }
        }

        // Визуальный эффект: мигание при окончании таймера
        if (bomb->timer < 1.0f) {
            // В draw_bomb — мигать
        }
    }
}

// Взрыв
void logic_bomb_explode(WorldState* world, int bomb_index) {
    Bomb* bomb = &world->bombs[bomb_index];
    if (bomb->exploded) return;

    bomb->exploded = true;
    sound_play(SOUND_BOMB_EXPLODE);

    int center_x = bomb->x;
    int center_y = bomb->y;

    // Разрушение блоков
    logic_bomb_destroy_blocks(world, center_x, center_y, BOMB_EXPLOSION_RADIUS);

    // Урон персонажам
    logic_bomb_damage_characters(world, center_x, center_y, BOMB_EXPLOSION_RADIUS, bomb->owner_id);

    // Rocket jump для воинов с щитом рядом
    for (int i = 0; i < world->char_count; i++) {
        Character* ch = &world->characters[i];
        if (ch->type == CHAR_WARRIOR && ch->is_shield_active && ch->hp > 0) {
            float dx = ch->x - center_x;
            float dy = ch->y - center_y;
            float dist_blocks = sqrtf(dx*dx + dy*dy) / 16.0f;
            if (dist_blocks <= 2.0f && dist_blocks > 0.1f) {
                // Вызов в логике воина
                extern void logic_warrior_rocketjump_from_bomb(Character* warrior, Vector2 bomb_pos);
                logic_warrior_rocketjump_from_bomb(ch, (Vector2){center_x, center_y});
            }
        }
    }
}

// Разрушение блоков
void logic_bomb_destroy_blocks(WorldState* world, int center_x, int center_y, int radius) {
    int cx = center_x / 16;
    int cy = center_y / 16;

    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int x = cx + dx;
            int y = cy + dy;
            if (x < 0 || x >= WORLD_MAX_WIDTH || y < 0 || y >= WORLD_MAX_HEIGHT) continue;

            float dist = sqrtf(dx*dx + dy*dy);
            if (dist > radius) continue;

            BlockType type = world->blocks[y][x].type;
            if (type == BLOCK_STONE || type == BLOCK_WOOD || type == BLOCK_GOLD ||
                type == BLOCK_DOOR || type == BLOCK_BRIDGE || type == BLOCK_LADDER) {
                // Выпадение ресурсов
                if (type == BLOCK_STONE) {
                    drop_item(world, ITEM_STONE, 10, TEAM_NONE, x * 16, y * 16);
                } else if (type == BLOCK_WOOD || type == BLOCK_DOOR || type == BLOCK_BRIDGE || type == BLOCK_LADDER) {
                    drop_item(world, ITEM_WOOD, 10, TEAM_NONE, x * 16, y * 16);
                }
                world->blocks[y][x].type = BLOCK_AIR;
                world->blocks[y][x].has_grass = false;
            }
        }
    }
}

// Урон персонажам
void logic_bomb_damage_characters(WorldState* world, int center_x, int center_y, int radius, int owner_id) {
    for (int i = 0; i < world->char_count; i++) {
        Character* ch = &world->characters[i];
        if (ch->hp <= 0) continue;

        float dx = ch->x - center_x;
        float dy = ch->y - center_y;
        float dist_blocks = sqrtf(dx*dx + dy*dy) / 16.0f;

        if (dist_blocks <= radius) {
            // Если воин с активным щитом — выживает
            if (ch->type == CHAR_WARRIOR && ch->is_shield_active) {
                // Уже обработано в rocketjump
                continue;
            }

            // Иначе — смерть
            ch->hp = 0;

            // Выпадение монет
            int drop_coins = (int)(ch->coins * world->params.dropped_coin_ratio);
            if (drop_coins > 0) {
                drop_item(world, ITEM_COINS, drop_coins, TEAM_NONE, ch->x, ch->y);
            }

            // Очки убийцу
            if (ch->player_id != owner_id && owner_id >= 0) {
                Character* killer = NULL;
                for (int j = 0; j < world->char_count; j++) {
                    if (world->characters[j].player_id == owner_id) {
                        killer = &world->characters[j];
                        break;
                    }
                }
                if (killer && killer->team == TEAM_BLUE) world->params.blue_score += 1;
                else if (killer && killer->team == TEAM_RED) world->params.red_score += 1;
            }
        }
    }
}


