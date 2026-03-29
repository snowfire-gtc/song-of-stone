// logic_warrior.c
#include "logic_warrior.h"
#include "sound.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define WARRIOR_NORMAL_SPEED 120.0f      // пикс/сек
#define WARRIOR_SHIELD_SPEED 60.0f       // в 2 раза медленнее
#define WARRIOR_JUMP_POWER 300.0f
#define WARRIOR_SWORD_DAMAGE 2
#define WARRIOR_SWORD_CHARGED_DAMAGE 4
#define WARRIOR_BOMB_COST 20
#define WARRIOR_SHIELD_DURATION 0.3f     // чтобы не включать/выключать слишком часто

// Обновление воина
void logic_warrior_update(Character* warrior, WorldState* world, int frame_counter) {
    // Обновление таймера неуязвимости
    if (warrior->is_invulnerable) {
        warrior->invuln_timer -= GetFrameTime();
        if (warrior->invuln_timer <= 0) {
            warrior->is_invulnerable = false;
        }
    }

    // Мерцание при неуязвимости
    if (warrior->is_invulnerable && (frame_counter % 20 < 10)) {
        // В draw_character — не рисовать
        // Здесь — просто маркер
    }

    // Обновление заряда атаки
    if (warrior->is_charging) {
        warrior->charge_time += GetFrameTime();
        if (warrior->charge_time > 1.0f) {
            warrior->charge_time = 1.0f; // максимум 1 сек
        }
    }
}

// Атака мечом
void logic_warrior_perform_sword_attack(Character* warrior, WorldState* world) {
    int damage = warrior->charge_time >= 0.8f ? WARRIOR_SWORD_CHARGED_DAMAGE : WARRIOR_SWORD_DAMAGE;

    // Найти ближайших врагов в радиусе 1 блока (16 пикс)
    for (int i = 0; i < world->char_count; i++) {
        Character* target = &world->characters[i];
        if (target->player_id == warrior->player_id) continue;
        if (target->team == warrior->team) continue;
        if (target->hp <= 0) continue;

        float dx = fabsf(warrior->x - target->x);
        float dy = fabsf(warrior->y - target->y);
        if (dx < 16 && dy < 32) {
            if (target_block == BLOCK_WOOD) {
                sound_play(SOUND_SWORD_HIT_WOOD);
            } else if (target_block == BLOCK_STONE) {
                sound_play(SOUND_SWORD_HIT_STONE);
            } else
            // Урон без брони (у воина бронь)
            target->hp -= damage;
            if (target->hp < 0) target->hp = 0;
            sound_play(SOUND_SWORD_HIT);

            // Начисление очков
            if (target->hp <= 0) {
                if (warrior->team == TEAM_BLUE) world->params.blue_score += 1;
                else world->params.red_score += 1;
            }
            break;
        }
    }

    // Сброс заряда
    warrior->is_charging = false;
    warrior->charge_time = 0.0f;

    // Анимация
    warrior->anim_state = ANIM_ATTACK;
    warrior->frame_counter = 0;
}

// Бросок бомбы
void logic_warrior_throw_bomb(Character* warrior, WorldState* world, float throw_power) {
    if (warrior->bombs <= 0) return;
    if (throw_power < 0.1f) return;

    warrior->bombs--;
    sound_play(SOUND_BOMB_THROW);

    // Направление: предположим, бросок вправо (для простоты)
    // В реальности — по направлению взгляда или курсора
    float angle = 0.0f; // радианы (0 = вправо)
    if (warrior->team == TEAM_BLUE) angle = 0.0f;
    else angle = M_PI; // влево

    // Сила: от 200 до 600 пикс/сек
    float speed = 200.0f + throw_power * 400.0f;

    DroppedItem bomb = {0};
    bomb.x = warrior->x;
    bomb.y = warrior->y - 8; // из руки
    bomb.type = ITEM_BOMBS;
    bomb.amount = 1;
    bomb.vx = cosf(angle) * speed;
    bomb.vy = sinf(angle) * -200.0f; // немного вверх
    bomb.is_picked_up = false;

    // Добавить в мир как временный объект с таймером
    // (в реальности — отдельная структура Bomb)
    // Здесь — упрощённо: добавим в items с таймером
    if (world->item_count < MAX_PLAYERS * 4) {
        world->items[world->item_count] = bomb;
        world->item_count++;
    }
}

// Переключение щита
void logic_warrior_toggle_shield(Character* warrior) {
    warrior->is_shield_active = !warrior->is_shield_active;
    if (warrior->is_shield_active) {
        warrior->anim_state = ANIM_SHIELD;
    } else {
        warrior->anim_state = ANIM_IDLE;
    }
}

// Может ли воин пройти?
bool logic_warrior_can_pass_block(BlockType block_type) {
    return block_type == BLOCK_AIR || block_type == BLOCK_WATER || block_type == BLOCK_LEAFS;
}

// Получение урона
void logic_warrior_take_damage(Character* warrior, int damage, bool is_melee) {
    if (warrior->is_invulnerable) return;

    // Если щит активен и урон от ближнего боя — броня снижает урон в 2 раза
    if (warrior->is_shield_active && is_melee) {
        damage = (damage + 1) / 2; // ceil(damage/2)
    }

    warrior->hp -= damage;
    if (warrior->hp < 0) warrior->hp = 0;

    sound_play(SOUND_PLAYER_HURT);

    // Анимация получения урона (можно изменить anim_state)
}

// Rocket jump от бомбы
void logic_warrior_rocketjump_from_bomb(Character* warrior, Vector2 bomb_pos) {
    if (!warrior->is_shield_active) return;

    float dx = warrior->x - bomb_pos.x;
    float dy = warrior->y - bomb_pos.y;
    float dist = sqrtf(dx*dx + dy*dy) / 16.0f; // в блоках

    if (dist <= 2.0f && dist > 0.01f) {
        // Отталкивание от бомбы
        float force = 400.0f / (dist + 0.1f);
        warrior->vx = (dx / dist) * force;
        warrior->vy = -300.0f; // вверх
        sound_play(SOUND_BOMB_EXPLODE);
    }
}
