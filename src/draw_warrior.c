// draw_warrior.c
#include "draw_warrior.h"
#include "raylib.h"
#include <string.h>

// Структура для хранения анимаций
typedef struct {
    Texture2D texture;
    int frame_count;
    int frame_width;
    int frame_height;
} WarriorAnimation;

static WarriorAnimation anims[ANIM_IDLE_FUNNY + 1] = {0};
static bool textures_loaded = false;

// Вспомогательная функция: загрузка анимации
static WarriorAnimation load_animation(const char* filename, int frame_count) {
    WarriorAnimation anim = {0};
    anim.texture = LoadTexture(filename);
    anim.frame_count = frame_count;
    if (anim.texture.id != 0) {
        anim.frame_width = anim.texture.width / frame_count;
        anim.frame_height = anim.texture.height;
    } else {
        TraceLog(LOG_WARNING, "Failed to load warrior animation: %s", filename);
    }
    return anim;
}

// Инициализация
void draw_warrior_init(void) {
    anims[ANIM_IDLE]        = load_animation("data/textures/warrior/idle.png", 4);
    anims[ANIM_WALK]        = load_animation("data/textures/warrior/walk.png", 6);
    anims[ANIM_JUMP]        = load_animation("data/textures/warrior/jump.png", 2);
    anims[ANIM_ATTACK]      = load_animation("data/textures/warrior/attack.png", 4);
    anims[ANIM_SHIELD]      = load_animation("data/textures/warrior/shield.png", 1);
    anims[ANIM_HURT]        = load_animation("data/textures/warrior/hurt.png", 1);
    anims[ANIM_IDLE_FUNNY]  = load_animation("data/textures/warrior/idle_funny.png", 3);

    textures_loaded = true;
}

// Получение прямоугольника кадра
static Rectangle get_frame_rect(const WarriorAnimation* anim, int frame_index) {
    frame_index = frame_index % anim->frame_count;
    return (Rectangle){
        .x = frame_index * anim->frame_width,
        .y = 0,
        .width = anim->frame_width,
        .height = anim->frame_height
    };
}

// Отрисовка одного воина
void draw_warrior_single(const Character* warrior, int frame_counter, bool is_local_player) {
    // Пропуск мёртвых
    if (warrior->hp <= 0) return;

    // Мерцание при неуязвимости
    bool should_draw = true;
    if (warrior->is_invulnerable) {
        should_draw = (frame_counter % 20 < 10);
    }
    if (!should_draw) return;

    AnimationState state = warrior->anim_state;
    if (state < 0 || state > ANIM_IDLE_FUNNY) state = ANIM_IDLE;

    const WarriorAnimation* anim = &anims[state];
    if (anim->texture.id == 0) return;

    // Выбор кадра
    int anim_speed = 10; // кадров на смену
    if (state == ANIM_WALK) anim_speed = 6;
    if (state == ANIM_ATTACK) anim_speed = 4;

    int frame_index = (warrior->frame_counter / anim_speed) % anim->frame_count;

    Rectangle src = get_frame_rect(anim, frame_index);
    Rectangle dst = {
        .x = warrior->x - anim->frame_width / 2,
        .y = warrior->y - anim->frame_height,
        .width = anim->frame_width,
        .height = anim->frame_height
    };

    // Цвет: команда
    Color tint = (warrior->team == TEAM_BLUE) ? ColorAlpha(BLUE, 0.7f) : ColorAlpha(RED, 0.7f);
    if (is_local_player) {
        tint = WHITE; // локальный игрок — без подкраски
    }

    // Отрисовка спрайта
    DrawTexturePro(anim->texture, src, dst, (Vector2){0, 0}, 0, tint);

    // === Визуальные элементы поверх спрайта ===

    // Щит (если активен)
    if (warrior->is_shield_active) {
        // Рисуем щит как небольшой круг слева
        DrawCircle(warrior->x - 6, warrior->y - 10, 4, ColorAlpha(GRAY, 0.8f));
    }

    // Флаг (если несёт)
    if (warrior->is_holding_flag) {
        // Маленький флаг справа
        Color flag_color = (warrior->team == TEAM_BLUE) ? RED : BLUE; // вражеский флаг
        DrawRectangle(warrior->x + 6, warrior->y - 20, 2, 8, flag_color);
        DrawTriangle(
            (Vector2){warrior->x + 6, warrior->y - 20},
            (Vector2){warrior->x + 12, warrior->y - 18},
            (Vector2){warrior->x + 6, warrior->y - 16},
            flag_color
        );
    }

    // Отладка: HP (в debug-режиме)
    #ifdef DEBUG_MODE
    char hp_str[16];
    snprintf(hp_str, sizeof(hp_str), "HP: %d", warrior->hp);
    DrawText(hp_str, warrior->x - 10, warrior->y - anim->frame_height - 15, 10, BLACK);
    #endif
}

// Отрисовка всех воинов
void draw_warrior_all(const WorldState* world, int frame_counter, int local_player_id) {
    for (int i = 0; i < world->char_count; i++) {
        const Character* ch = &world->characters[i];
        if (ch->type != CHAR_WARRIOR) continue;
        bool is_local = (ch->player_id == local_player_id);
        draw_warrior_single(ch, frame_counter, is_local);
    }
}

// Освобождение
void draw_warrior_unload(void) {
    for (int i = 0; i <= ANIM_IDLE_FUNNY; i++) {
        if (anims[i].texture.id != 0) {
            UnloadTexture(anims[i].texture);
        }
    }
}
